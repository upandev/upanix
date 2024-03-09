/*
 *	Upanix - An x86 based Operating System
 *  Copyright (C) 2011 'Prajwala Prabhakar' 'srinivasa.prajwal@gmail.com'
 *
 *  I am making my contributions/submissions to this project solely in
 *  my personal capacity and am not conveying any rights to any
 *  intellectual property of any third parties.
 *                                                                          
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *                                                                          
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *                                                                          
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/
 */

#include <TRB.h>
#include <newalloc.h>
#include <DMM.h>
#include <mutex.h>

#define INTERRUPT_ON_COMPLETE (1 << 5)

TransferRing::TransferRing(unsigned size) : _size(size), _cycleState(true), _nextTRBIndex(0), _freeSlots(size - 1), _dqIndex(0)
{
  _trbs = new ((void*)KernelDMM::Instance().allocate(sizeof(TRB) * _size, 16))TRB[_size];
  auto& link = *new (&_trbs[_size - 1])LinkTRB();
  link.SetLinkAddr(KERNEL_REAL_ADDRESS(_trbs));
  link.SetToggleBit(true);
}

TransferRing::~TransferRing()
{
  KernelDMM::Instance().free((uintptr_t)_trbs);
}

void TransferRing::UpdateDeEnQPtr(uint64_t dnqPtr)
{
  const auto curdqIndex = (dnqPtr - (uintptr_t)&_trbs[0]) / sizeof(TRB);
  uint32_t freeSlots = 0;

  if(curdqIndex < _dqIndex)
    // _size - 2/* -2 = -1 + (-1) -> Link TRB */ - _dqIndex + 1 + curdqIndex + 1 --> _size - _dqIndex + curdqIndex
    freeSlots = (_size - _dqIndex + curdqIndex);
  else
    freeSlots = (curdqIndex - _dqIndex + 1);

  _freeSlots.add(freeSlots);
  _dqIndex = (curdqIndex + 1) % (_size - 1);
}

TRB& TransferRing::NextTRB()
{
  if(_nextTRBIndex == (_size - 1))
  {
    _trbs[_nextTRBIndex].SetCycleBit(_cycleState);
    _nextTRBIndex = 0;
    _cycleState = !_cycleState;
  }
  _freeSlots.dec();
  return _trbs[_nextTRBIndex++];
}

void TransferRing::AddSetupStageTRB(uint32_t bmRequestType, uint32_t bmRequest,
                                    uint32_t wValue, uint32_t wIndex, uint32_t wLength,
                                    TransferType trt)
{
  TRB& trb = NextTRB();
  trb._b1 =  wValue << 16 | bmRequest << 8 | bmRequestType;
  trb._b2 =  wLength << 16 | wIndex;
  trb._b3 = 8;
  trb._b4 = (trt << 16) | (1 << 6);
  trb.Type(2);
  trb.SetCycleBit(_cycleState);
}

void TransferRing::AddDataStageTRB(uintptr_t dataBufferAddr, uint32_t len, DataDirection dir, int32_t maxPacketSize)
{
  dataBufferAddr = KERNEL_REAL_ADDRESS(dataBufferAddr);
  int32_t transferLen = len;
  int32_t remainingPackets = ((len + (maxPacketSize - 1)) / maxPacketSize) - 1;
  if(remainingPackets < 0)
    remainingPackets = 0;

  uint32_t trbType = 3; //Data
  while(transferLen > 0)
  {
    TRB& trb = NextTRB();
    //TODO: Write to 64bit field (i.e. _b1 + _b2 together) in a single assigment
    trb._b1 = dataBufferAddr;
    trb._b2 = 0;
    trb._b3 = (remainingPackets << 17) | (transferLen < maxPacketSize ? transferLen : maxPacketSize);
    trb._b4 = (dir << 16) | ((remainingPackets != 0) << 4);
//    trb._b4 = (dir << 16) | (1 << 4) | (remainingPackets == 0) << 1;
    trb.Type(trbType);
    trb.SetCycleBit(_cycleState);

    dataBufferAddr += maxPacketSize;
    transferLen -= maxPacketSize;
    --remainingPackets;
    
    //for DATA stage, after the first trb, the remaining are NORMAL TRBs and direction is not used
    trbType = 1; //Normal
    dir = DataDirection::OUT;
  }
//  auto statusAddr = KernelDMM::Instance().allocate(4, 16);
//  AddEventDataTRB(statusAddr, true);
}

uintptr_t TransferRing::AddStatusStageTRB(uint32_t dir)
{
  TRB& trb = NextTRB();
  trb._b1 = 0;
  trb._b2 = 0;
  trb._b3 = 0;
  trb._b4 = dir << 16 | INTERRUPT_ON_COMPLETE;
  trb.Type(4);
  trb.SetCycleBit(_cycleState);
  return (uintptr_t)&trb;
}

void TransferRing::AddEventDataTRB(uintptr_t statusAddr, bool ioc)
{
  TRB& trb = NextTRB();
  trb._b1 = statusAddr;
  trb._b2 = 0;
  trb._b3 = 0;
  trb._b4 = ioc ? INTERRUPT_ON_COMPLETE : 0;
  trb.Type(7);
  trb.SetCycleBit(_cycleState);
}

TRB::Result TransferRing::AddDataTRB(uintptr_t dataBufferAddr, uint32_t len, DataDirection dir, int32_t maxPacketSize)
{
  dataBufferAddr = KERNEL_REAL_ADDRESS(dataBufferAddr);
  int32_t remainingBytesToTransfer = len;
  TRB* lastTRB = nullptr;

  uint32_t requiredSlots = (len / PAGE_SIZE);
  const bool startAlign = (dataBufferAddr % PAGE_SIZE) == 0;
  const bool endAlign = ((dataBufferAddr + len) % PAGE_SIZE) == 0;
  if(!startAlign)
  {
    ++requiredSlots;
    if(!endAlign && (dataBufferAddr / PAGE_SIZE) != ((dataBufferAddr + len) / PAGE_SIZE) )
      ++requiredSlots;
  }
  else if(!endAlign)
    ++requiredSlots;

  if(requiredSlots > _freeSlots.get())
    return TRB::Result::bad("Transfer Ring is Full!");

  while(remainingBytesToTransfer > 0)
  {
    int32_t bytesToTransfer = PAGE_SIZE - (dataBufferAddr % PAGE_SIZE);
    if(remainingBytesToTransfer < bytesToTransfer)
      bytesToTransfer = remainingBytesToTransfer;
    remainingBytesToTransfer -= bytesToTransfer;

    int32_t remainingPackets = (remainingBytesToTransfer + (maxPacketSize - 1)) / maxPacketSize;

    const bool ioc = remainingPackets <= 0;

    TRB& trb = NextTRB();
    //TODO: Write to 64bit field (i.e. _b1 + _b2 together) in a single assigment
    trb._b1 = dataBufferAddr;
    trb._b2 = 0;
    trb._b3 = (remainingPackets << 17) | bytesToTransfer;
    trb._b4 = (dir << 16) | ((remainingPackets != 0) << 4) | (ioc ? INTERRUPT_ON_COMPLETE : 0);
    trb.Type(1);
    trb.SetCycleBit(_cycleState);

    dataBufferAddr += bytesToTransfer;
    
    if(ioc)
      lastTRB = &trb;
  }
  return TRB::Result((uint64_t)lastTRB);
}
