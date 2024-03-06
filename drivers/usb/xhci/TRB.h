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
#ifndef _XHCI_TRB_H_
#define _XHCI_TRB_H_

#include <Global.h>
#include <stdio.h>
#include <exception.h>
#include <result.h>
#include <Bit.h>
#include <atomicop.h>

enum TransferType { NO_DATA_STAGE, RESERVED, OUT_DATA_STAGE, IN_DATA_STAGE };
enum DataDirection { OUT, IN };

class TRB
{
  public:
    typedef upan::result<uint64_t> Result;

  public:
    TRB() : _b1(0), _b2(0), _b3(0), _b4(0)
    {
    }

    void Clear()
    {
      _b1 = 0;
      _b2 = 0;
      _b3 = 0;
      _b4 = 0;
    }

    void Type(unsigned type)
    {
      type &= 0x3F;
      _b4 = (_b4 & ~(0x3F << 10)) | (type << 10);
    }

    unsigned Type() const
    {
      return (_b4 >> 10) & 0x3F;
    }

    void SetCycleBit(bool val) { _b4 = Bit::Set(_b4, 0x1, val); }
    bool IsCycleBitSet() const { return Bit::IsSet(_b4, 0x1); }

    void Print()
    {
      printf("\n TRB0: %x, TRB1: %x, TRB2: %x, TRB3: %x", _b1, _b2, _b3, _b4);
    }

  protected:
    unsigned _b1;
    unsigned _b2;
    unsigned _b3;
    unsigned _b4;

    friend class TransferRing;
} PACKED;

class LinkTRB : public TRB
{
  public:
    LinkTRB()
    {
      Type(6);
    }

    void SetLinkAddr(uint64_t la)
    {
      /*
      if(la & 0x3F)
        throw upan::exception(XLOC, "link address is not 64 byte aligned");
      */
      _b1 = la & 0xFFFFFFFF;
      _b2 = (la >> 32);
    }
    uint64_t GetLinkAddr() const
    {
      uint64_t la = _b2;
      return la << 32 | _b1;
    }

    void SetInterrupterTarget(unsigned itarget)
    {
      itarget &= 0x3FF;
      _b3 = (_b3 & ~(0x3FF << 22)) | (itarget << 22);
    }
    unsigned GetInterrupterTarget() const
    {
      return (_b3 >> 22) & 0x3FF;
    }

    void SetToggleBit(bool val) { _b4 = Bit::Set(_b4, 0x2, val); }
    bool IsToggleBitSet() const { return Bit::IsSet(_b4, 0x2); }

    void SetChainBit(bool val) { _b4 = Bit::Set(_b4, 0x10, val); }
    bool IsChainBitSet() const { return Bit::IsSet(_b4, 0x10); }

    void SetIOC(bool val) { _b4 = Bit::Set(_b4, 0x20, val); }
    bool IsIOC() const { return Bit::IsSet(_b4, 0x20); }
} PACKED;

class CommandTRB : public TRB
{
  public:
    void SlotID(unsigned slotID)
    {
      _b4 = _b4 | (slotID << 24);
    }
} PACKED;

class EnableSlotTRB : public CommandTRB
{
  public:
    EnableSlotTRB(unsigned slotType)
    {
      Type(9);
      _b4 = _b4 | (slotType << 16);
    }
} PACKED;

class DisableSlotTRB : public CommandTRB
{
  public:
    DisableSlotTRB(unsigned slotID)
    {
      Type(10);
      SlotID(slotID);
    }
} PACKED;

class AddressDeviceTRB : public CommandTRB
{
  public:
    AddressDeviceTRB(unsigned inputContextPtr, unsigned slotID, bool blockSetAddressReq)
    {
      if(inputContextPtr & 0xF)
        throw upan::exception(XLOC, "InputContext Address must be 8 byte aligned");
      _b1 = inputContextPtr;
      _b4 = _b4 | (blockSetAddressReq << 9);
      Type(11);
      SlotID(slotID);
    }
} PACKED;

class ConfigureEndPointTRB : public CommandTRB
{
  public:
    ConfigureEndPointTRB(unsigned inputContextPtr, unsigned slotID)
    {
      if(inputContextPtr & 0xF)
        throw upan::exception(XLOC, "InputContext Address must be 8 byte aligned");
      _b1 = inputContextPtr;
      Type(12);
      SlotID(slotID);
    }
} PACKED;

class EvaluateContextTRB : public CommandTRB
{
  public:
    EvaluateContextTRB(unsigned inputContextPtr, unsigned slotID)
    {
      if(inputContextPtr & 0xF)
        throw upan::exception(XLOC, "InputContext Address must be 8 byte aligned");
      _b1 = inputContextPtr;
      Type(13);
      SlotID(slotID);
    }
} PACKED;

class EventTRB : public TRB
{
  public:
    uint64_t TRBPointer() const
    {
      uint64_t addr = _b2;
      return (addr << 32 | (uint64_t)_b1);
    }

    unsigned CompletionCode() const
    {
      return _b3 >> 24;
    }

    bool IsCommandSuccess() const
    {
      return CompletionCode() == 1;
    }

    unsigned SlotID() const
    {
      return _b4 >> 24;
    }

    bool IsEventDataTRB() const
    {
      return (_b4 & 0x4) != 0;
    }

    uint32_t TransferLength() const
    {
      return _b3 & 0xFFFFFF;
    }

    void ValidateCommandResult()
    {
      if(Type() != 33)
        throw upan::exception(XLOC, "Got invalid Event TRB: %d", Type());

      if(!IsCommandSuccess())
        throw upan::exception(XLOC, "Command did not complete successfully - Completion Code: %u", CompletionCode());
    }

    void ValidateTransferResult() const
    {
      if(Type() != 32)
        throw upan::exception(XLOC, "Got invalid Event TRB: %d", Type());

      if(!IsCommandSuccess())
        throw upan::exception(XLOC, "Transfer command did not complete successfully - Completion Code: %u", CompletionCode());
    }

} PACKED;

class TransferRing
{
  public:
    TransferRing(unsigned size);
    ~TransferRing();
    void AddSetupStageTRB(uint32_t bmRequestType, uint32_t bmRequest, uint32_t wValue, uint32_t wIndex, uint32_t wLength, TransferType trt);
    void AddDataStageTRB(uintptr_t dataBufferAddr, uint32_t len, DataDirection dir, int32_t maxPacketSize);
    uintptr_t AddStatusStageTRB(uint32_t dir);
    void AddEventDataTRB(uintptr_t statusAddr, bool ioc);
    TRB::Result AddDataTRB(uintptr_t dataBufferAddr, uint32_t len, DataDirection dir, int32_t maxPacketSize);
    TRB* RingBase() { return _trbs; }
    void UpdateDeEnQPtr(uint64_t dnqPtr);

  private:
    TRB& NextTRB();

    uint32_t  _size;
    bool      _cycleState;
    uint32_t  _nextTRBIndex;
    TRB*      _trbs;
    upan::atomic::integral<uint32_t> _freeSlots;
    __volatile__ uint32_t _dqIndex;
};

#endif
