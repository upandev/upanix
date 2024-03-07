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
#ifndef _XHCI_CONTROLLER_H_
#define _XHCI_CONTROLLER_H_

#include <USBController.h>
#include <PCIBusHandler.h>
#include <XHCIStructures.h>
#include <TRB.h>
#include <map.h>
#include <ProcessManager.h>

class CommandManager;
class EventManager;
class SupProtocolXCap;
class IRQ;
class XHCIDevice;
class SlotContext;
class EventResult;
class InputContext;

class XHCIController
{
  public:
    XHCIController(PCIEntry*);
    void Probe();
    void NotifyEvent();
    const XHCICapRegister& CapReg() const { return *_capReg; }

  private:
    void Start();
    void InitInterruptHandler();
    void LoadXCaps(uint64_t base);
    void PerformBiosToOSHandoff();
    EventTRB InitiateCommand();
    EventTRB InitiateTransfer(uint64_t trbId, uint32_t slotID, uint32_t ep);
    void InitiateInterruptTransfer(InputContext& context, uint64_t trbId, uint32_t slotID, uint32_t ep, uint64_t interruptDataAddress);
    void SetDeviceContext(uint32_t slotID, SlotContext& slotContext);
    void RingDoorBell(unsigned index, unsigned value);
    unsigned EnableSlot(unsigned slotType);
    void AddressDevice(unsigned inputContextPtr, unsigned slotID, bool blockSetAddressReq);
    void ConfigureEndPoint(unsigned icptr, unsigned slotID);
    void EvaluateContext(unsigned icptr, unsigned slotID);
    EventTRB WaitForCmdCompletion();
    EventTRB WaitForTransferCompletion(uint64_t trbId);
    EventTRB WaitForEvent(uint64_t trbId);

    SupProtocolXCap* GetSupportedProtocol(unsigned portId) const;
    const char* PortProtocolName(USB_PROTOCOL) const;
    const char* PortSpeedName(DEVICE_SPEED speed) const;

    void RegisterForWaitedEventResult(uint64_t trbId);
    EventResult& ConsumeEventResult(uint64_t trbId);
    void PublishEventResult(const EventTRB& result);

    static uint64_t  _memMapBaseAddress;
    PCIEntry*        _pPCIEntry;
    uint64_t*        _deviceContextAddrArray;
    XHCICapRegister* _capReg;
    XHCIOpRegister*  _opReg;
    CommandManager*  _cmdManager;
    EventManager*    _eventManager;
    LegSupXCap*      _legSupXCap;
    volatile unsigned* _doorBellRegs;
    upan::list<SupProtocolXCap*> _supProtoXCaps;
    upan::map<uint64_t, EventResult*> _eventResults;
		upan::map<uint32_t, XHCIDevice*> _devices;

    friend class XHCIManager;
    friend class EventManager;
		friend class XHCIDevice;
    friend class InputContext;
    friend class InterruptEventResult;
};

class CommandManager
{
  private:
    CommandManager(XHCICapRegister&, XHCIOpRegister&, EventManager& eventManager);
    void Apply();
    void EnableSlot(unsigned slotType);
    void DisableSlot(unsigned slotType);
    void AddressDevice(unsigned inputContextPtr, unsigned slotID, bool blockSetAddressReq);
    void ConfigureEndPoint(unsigned icptr, unsigned slotID);
    void EvaluateContext(unsigned icptr, unsigned slotID);
    void DebugPrint();
    uintptr_t CommandTRBAddress() const
    {
      return (uintptr_t)&_ring->_cmd;
    }

  private:
    struct Ring
    {
      CommandTRB _cmd;
      LinkTRB    _link;
    } PACKED;

    bool             _pcs;
    Ring*            _ring;
    XHCICapRegister& _capReg;
    XHCIOpRegister&  _opReg;
    EventManager&    _eventManager;
  friend class XHCIController;
};

class EventManager
{
  public:
    void DebugPrint() const;
    bool WaitForEvent(uint64_t trbId, EventTRB& result);
    void NotifyEvents();

  private:
    EventManager(XHCIController& controller, XHCICapRegister&, XHCIOpRegister&);

  private:
    static const int ERST_SIZE;
    //Event Ring Segment Table
    struct ERSTEntry
    {
      ERSTEntry();
      uint64_t _ersAddr;
      unsigned _size;
      unsigned _reserved;
    } PACKED;

    class InterrupterRegister
    {
      public:
      InterrupterRegister();

      ERSTEntry& ERST(unsigned index)
      {
        if(index >= _erstSize)
          throw upan::exception(XLOC, "\n Invalid ERST index %d - ERST Size is %d", index, _erstSize);
        auto erst = (ERSTEntry*)KERNEL_VIRTUAL_ADDRESS(_erstBA);
        return erst[index];
      }

      EventTRB* ERSegment(unsigned index)
      {
        return (EventTRB*)KERNEL_VIRTUAL_ADDRESS(ERST(index)._ersAddr);
      }

      bool IsLastDQPtr()
      {
        EventTRB* lastTRB = &ERSegment(0)[ERST_SIZE - 1];
        EventTRB* curTRB = DQEvent();
        return curTRB == lastTRB;
      }

      void IncrementDQPtr()
      {
        if(IsLastDQPtr())
          DQPtr(ERST(0)._ersAddr);
        else
          DQPtr(_erdqPtr + sizeof(EventTRB));
      }

      EventTRB* DQEvent()
      {
        return (EventTRB*)DQPtr();
      }

      bool EventCycleBitSet()
      {
        EventTRB* event = (EventTRB*)DQPtr();
        return event->IsCycleBitSet();
      }
       
      void EnableInterrupt()
      {
        _iman |= 0x2;
      }

      void DisableInterrupt()
      {
        _iman &= ~((uint32_t)(0x2));
      }

      //For manual Interrupt handling/PCI Pin interrupt handling
      void ClearIPBit()
      {
        _iman |= 0x1;
      }

      void DebugPrint();

    private:
      uint64_t DQPtr() { return KERNEL_VIRTUAL_ADDRESS(_erdqPtr) & ~(0xF); }
      void DQPtr(uint64_t addr) { _erdqPtr = (addr & ~(0xF)) | (1 << 3); }

      unsigned _iman;
      unsigned _imod;
      unsigned _erstSize;
      unsigned _reserved;
      uint64_t _erstBA;
      uint64_t _erdqPtr;
    } PACKED;

    bool     _eventCycleBit;
    InterrupterRegister* _iregs;
    XHCICapRegister& _capReg;
    XHCIOpRegister& _opReg;
    XHCIController& _controller;
  friend class XHCIController;
};

class EventResult
{
public:
  EventResult(int pid) : _pid(pid) {}
  virtual ~EventResult() { }

  int Pid() const { return _pid; }
  const EventTRB& Result() const { return _result; }

  virtual void Consume(const EventTRB& r) = 0;
protected:
  int      _pid;
  EventTRB _result;
};

class WaitedEventResult : public EventResult
{
public:
  WaitedEventResult(int pid) : EventResult(pid) { }
  void Consume(const EventTRB& r);
};

class InterruptEventResult : public EventResult
{
public:
  InterruptEventResult(InputContext& context, int pid, uint64_t dataAddress) : EventResult(pid), _context(context), _dataAddress(dataAddress) { }
  void Consume(const EventTRB &r);
  uint64_t InterruptDataAddress() const { return _dataAddress; }

private:
  InputContext& _context;
  uint64_t _dataAddress;
};

#endif
