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

#include <MemManager.h>
#include <newalloc.h>
#include <XHCIContext.h>
#include <XHCIStructures.h>
#include <TRB.h>
#include <XHCIController.h>
#include <USBDataHandler.h>

InputContext::InputContext(XHCIController& controller, uint32_t slotID, const XHCIPortRegister& port, uint32_t portId, uint32_t routeString)
  : _slotID(slotID), _controller(controller), _interruptDataHandler(nullptr)
{
  uint64_t addr = MemManager::Instance().AllocatePageForKernel() * PAGE_SIZE;
  if(_controller.CapReg().IsContextSize64())
  {
    auto context64 = new ((void*)addr)InputContext64();
    _control = &context64->_controlContext;
    _devContext = new DeviceContext(context64->_deviceContext);
  }
  else
  {
    auto context32 = new ((void*)addr)InputContext32();
    _control = &context32->_controlContext;
    _devContext = new DeviceContext(context32->_deviceContext);
  }
  Slot().Init(portId, routeString, port.PortSpeedID());
  _controlEP = new ControlEndPoint(*this, port.MaxPacketSize());
  //set A0 and A1 -> Slot and EP0 are affected by command
  Control().SetAddContextFlag(0x3);
}

InputContext::~InputContext()
{
  uint64_t addr = (uint64_t)_control;
  MemManager::Instance().DeAllocatePageForKernel(addr / PAGE_SIZE);
  delete _devContext;
  delete _controlEP;
  for(auto ep : _bulkInEPs) delete ep;
  for(auto ep : _bulkOutEPs) delete ep;
  for(auto ep : _interruptInEPs) delete ep;
  for(auto ep : _interruptOutEPs) delete ep;
}

uint32_t InputContext::AddEndPoint(const USBStandardEndPt& endpoint)
{
  EndPoint* endPoint = nullptr;
  switch(endpoint.Direction())
  {
  case USBStandardEndPt::IN:
    switch(endpoint.Type())
    {
    case USBStandardEndPt::BULK:
      endPoint = new BulkInEndPoint(*this, endpoint);
      break;
    case USBStandardEndPt::INTERRUPT:
      endPoint = new InterruptInEndPoint(*this, endpoint);
      break;
    default:
      break;
    }
    break;

  case USBStandardEndPt::OUT:
    switch(endpoint.Type())
    {
    case USBStandardEndPt::BULK:
      endPoint = new BulkOutEndPoint(*this, endpoint);
      break;
    case USBStandardEndPt::INTERRUPT:
      endPoint = new InterruptOutEndPoint(*this, endpoint);
      break;
    default:
      break;
    }
    break;

  default:
    break;
  }
  if(endPoint)
    return endPoint->Id();
  throw upan::exception(XLOC, "EndpointFactory: unsupported endpoint Dir %d and Type %d", (int32_t)endpoint.Direction(), (int32_t)endpoint.Type());
}

void InputContext::SendCommand(uint32_t bmRequestType, uint32_t bmRequest, 
                               uint32_t wValue, uint32_t wIndex, uint32_t wLength, 
                               TransferType trt, void* dataBuffer)
{
  const uint32_t trbId = _controlEP->SetupTransfer(bmRequestType, bmRequest, wValue, wIndex, wLength, trt, dataBuffer);
  _controller.InitiateTransfer(trbId, _slotID, _controlEP->Id());
}

void InputContext::SendData(uint64_t bufferAddress, uint32_t len)
{
  const uint64_t trbId = BulkOutEP().SetupTransfer(bufferAddress, len).goodValueOrThrow(XLOC);
  const auto& result = _controller.InitiateTransfer(trbId, _slotID, BulkOutEP().Id());
  BulkOutEP().UpdateDeEnQPtr(result.TRBPointer());
}

void InputContext::ReceiveData(uint64_t bufferAddress, uint32_t len)
{
  const uint32_t trbId = BulkInEP().SetupTransfer(bufferAddress, len).goodValueOrThrow(XLOC);
  const auto& result = _controller.InitiateTransfer(trbId, _slotID, BulkInEP().Id());
  BulkInEP().UpdateDeEnQPtr(result.TRBPointer());
}

bool InputContext::ReceiveInterruptData(uint64_t bufferAddress, uint32_t len)
{
  const TRB::Result& trbResult = InterruptInEP().SetupTransfer(bufferAddress, len);
  if(trbResult.isBad())
    return false;
  _controller.InitiateInterruptTransfer(*this, trbResult.goodValue(), _slotID, InterruptInEP().Id(), bufferAddress);
  return true;
}

void InputContext::OnInterrupt(const EventTRB& result, uint64_t interruptDataAddress)
{
  InterruptInEP().UpdateDeEnQPtr(result.TRBPointer());
  if(_interruptDataHandler)
    _interruptDataHandler->Handle(interruptDataAddress);
}

DeviceContext::DeviceContext(bool use64) : _allocated(true)
{
  uint64_t addr = MemManager::Instance().AllocatePageForKernel() * PAGE_SIZE;
  if(use64)
    Init64(*new ((void*)addr)DeviceContext64());
  else
    Init32(*new ((void*)addr)DeviceContext32());
}

DeviceContext::DeviceContext(DeviceContext32& dc32) : _allocated(false)
{
  Init32(dc32);
}

DeviceContext::DeviceContext(DeviceContext64& dc64) : _allocated(false)
{
  Init64(dc64);
}

void DeviceContext::Init32(DeviceContext32& dc)
{
  _slot = &dc._slotContext;
  _ep0 = &dc._epContext0;
  _eps32 = dc._epContexts;
  _eps64 = nullptr;
}

void DeviceContext::Init64(DeviceContext64& dc)
{
  _slot = &dc._slotContext;
  _ep0 = &dc._epContext0;
  _eps64 = dc._epContexts;
  _eps32 = nullptr;
}

DeviceContext::~DeviceContext()
{
  if(_allocated)
  {
    uint64_t addr = uint64_t(_slot);
    MemManager::Instance().DeAllocatePageForKernel(addr / PAGE_SIZE);
  }
}

EndPoint::EndPoint(uint32_t maxPacketSize) : _id(0), _ep(nullptr), _tRing(new TransferRing(TRANSFER_RING_SIZE)), _maxPacketSize(maxPacketSize)
{
}

EndPoint::~EndPoint()
{
  delete _tRing;
}

ControlEndPoint::ControlEndPoint(InputContext& inContext, int32_t maxPacketSize) : EndPoint(maxPacketSize)
{
  _ep = &inContext.EP0();
  _ep->Init(uint64_t(_tRing->RingBase()), USBStandardEndPt::BI, USBStandardEndPt::CONTROL, maxPacketSize, 0);
  _id = 1;
}

uint32_t ControlEndPoint::SetupTransfer(uint32_t bmRequestType, uint32_t bmRequest, 
                                        uint32_t wValue, uint32_t wIndex, uint32_t wLength, 
                                        TransferType trt, void* dataBuffer)
{
  //Setup stage
  _tRing->AddSetupStageTRB(bmRequestType, bmRequest, wValue, wIndex, wLength, trt);
  //Data stage
  if(trt != TransferType::NO_DATA_STAGE)
  {
    const auto dir = trt == TransferType::IN_DATA_STAGE ? DataDirection::IN : DataDirection::OUT;
    _tRing->AddDataStageTRB((uint64_t)dataBuffer, wLength, dir, _maxPacketSize);
  }
  //Status Stage
  uint32_t statusDir = DataDirection::IN;
  if(wLength > 0 && trt == TransferType::IN_DATA_STAGE)
    statusDir = DataDirection::OUT;
  return _tRing->AddStatusStageTRB(statusDir);
}

DataEndPoint::DataEndPoint(InputContext& inContext, const USBStandardEndPt& endpoint) : EndPoint(endpoint.wMaxPacketSize)
{
  const uint32_t epOffset = endpoint.Direction() == USBStandardEndPt::IN ? 1 : 2;
  const byte epID = endpoint.Address();
  const uint32_t epIndex = epID * 2 - epOffset;

  _ep = &inContext.EP(epIndex);
  _ep->Init((uint64_t)_tRing->RingBase(), endpoint.Direction(), endpoint.Type(), endpoint.wMaxPacketSize, endpoint.bInterval);
  _id = epIndex + 2;
}

BulkInEndPoint::BulkInEndPoint(InputContext& inContext, const USBStandardEndPt& endpoint) : DataEndPoint(inContext, endpoint)
{
  inContext.AddBulkInEP(this);
}

TRB::Result BulkInEndPoint::SetupTransfer(uint64_t bufferAddress, uint32_t len)
{
  return _tRing->AddDataTRB(bufferAddress, len, DataDirection::IN, _maxPacketSize);
}

BulkOutEndPoint::BulkOutEndPoint(InputContext& inContext, const USBStandardEndPt& endpoint) : DataEndPoint(inContext, endpoint)
{
  inContext.AddBulkOutEP(this);
}

TRB::Result BulkOutEndPoint::SetupTransfer(uint64_t bufferAddress, uint32_t len)
{
  return _tRing->AddDataTRB(bufferAddress, len, DataDirection::OUT, _maxPacketSize);
}

InterruptEndPoint::InterruptEndPoint(InputContext& inContext, const USBStandardEndPt& endpoint) : DataEndPoint(inContext, endpoint)
{
  auto maxBurstSize = (endpoint.wMaxPacketSize & 0x1800) >> 11;
  _ep->SetMaxBurstSize(maxBurstSize);
  //TODO: assuming the device is not SUPER_SPEED
  _ep->SetMaxESITPayload(endpoint.wMaxPacketSize * (maxBurstSize + 1));

  uint32_t interval = endpoint.bInterval;
  switch(inContext.Slot().DevSpeed())
  {
  case LOW_SPEED:
  case FULL_SPEED:
  {
    //valid range 3 to10 --> nearest 2 base multiple of bInterval *8
    uint32_t residue = endpoint.bInterval * 8;
    interval = 0;
    while(residue >= 2)
    {
      residue /= 2;
      ++interval;
    }
    if(interval < 3)
      interval = 3;
    else if(interval > 10)
      interval = 10;
  }
    break;

  case HIGH_SPEED:
  case SUPER_SPEED:
    //valid range 0 to 15
    if(interval > 15)
      interval = 15;
    else if(interval != 0)
      interval -= 1;
    break;
  }

  _ep->SetInterval(interval);
}

InterruptInEndPoint::InterruptInEndPoint(InputContext& inContext, const USBStandardEndPt& endpoint) : InterruptEndPoint(inContext, endpoint), _interval(endpoint.bInterval)
{
  inContext.AddInterruptInEP(this);
}

TRB::Result InterruptInEndPoint::SetupTransfer(uint64_t bufferAddress, uint32_t len)
{
  return _tRing->AddDataTRB(bufferAddress, len, DataDirection::IN, _maxPacketSize);
}

InterruptOutEndPoint::InterruptOutEndPoint(InputContext& inContext, const USBStandardEndPt& endpoint) : InterruptEndPoint(inContext, endpoint)
{
  inContext.AddInterruptOutEP(this);
}

void EndPointContext::Init(uint64_t dqPtr, USBStandardEndPt::DirectionTypes dir, USBStandardEndPt::Types type, int32_t maxPacketSize, byte interval)
{
  uint32_t epType = 4;

  uint32_t avgTRBLen;
  if(dir == USBStandardEndPt::BI)
    avgTRBLen = 8;
  else
  {
   avgTRBLen = maxPacketSize / 2;
   if(avgTRBLen == 0)
      avgTRBLen = 1;
  }

  switch (type)
  {
    case USBStandardEndPt::CONTROL:
      epType = 4;
      avgTRBLen = 8;
      break;
    case USBStandardEndPt::ISOCHRONOUS:
      epType = 1;
      break;
    case USBStandardEndPt::BULK:
      epType = 2;
      break;
    case USBStandardEndPt::INTERRUPT:
      epType = 3;
      break;
  }

  if(dir == USBStandardEndPt::IN)
    epType += 4;

  EPType(epType);

  //Interval, MaxPStreams = 0, Mult = 0
  _context1 = (_context1 & 0xFF008000);

  //SetInterval(interval);

  //Max packet size
  _context2 = (_context2 & ~(0xFFFF << 16)) | (maxPacketSize << 16);
  //Max Burst Size = 0
  SetMaxBurstSize(0);
  //Error count = 3
  _context2 = (_context2 & ~(0x7)) | 0x6;
  //Average TRB Len
  _context3 = avgTRBLen;
  //TR DQ Ptr + DCS = 1
  _trDQPtr = (uint64_t)(dqPtr | 0x1);
}
