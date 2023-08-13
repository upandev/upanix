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
#include <PCIBusHandler.h>
#include <DMM.h>
#include <MemManager.h>
#include <MemUtil.h>
#include <stdio.h>
#include <uniq_ptr.h>
#include <KeyboardHandler.h>
#include <XHCIController.h>
#include <XHCIContext.h>
#include <XHCIManager.h>
#include <USBDataHandler.h>
#include <XHCIDevice.h>
#include <Cpu.h>

unsigned XHCIController::_memMapBaseAddress = XHCI_MMIO_BASE_ADDR;

XHCIController::XHCIController(PCIEntry* pPCIEntry)
  : _pPCIEntry(pPCIEntry), _capReg(nullptr), _opReg(nullptr),
    _legSupXCap(nullptr), _doorBellRegs(nullptr) {
  const uint64_t ioAddress = pPCIEntry->GetIOMapAddress();
  const uint32_t ioSize = pPCIEntry->GetPCIMemSize(0);
	printf(", Raw MMIO BaseAddr: %lx, IOSize: %d, KernelPageTableMmap Address: %lx", ioAddress, ioSize, _memMapBaseAddress);

  const unsigned availableMemMapSize = XHCI_MMIO_BASE_ADDR_END - _memMapBaseAddress;
	if(ioSize > availableMemMapSize)
    throw upan::exception(XLOC, "XHCI IO Size is %x > greater than available size %x!", ioSize, availableMemMapSize);

  MemManager::Instance().MapAddressSpace(MEM_PML4_TABLE, 0x7, _memMapBaseAddress, ioAddress, ioSize);
  Mem_FlushTLB();

	printf("\n Bus: %d, Dev: %d, Func: %d", pPCIEntry->uiBusNumber, pPCIEntry->uiDeviceNumber, pPCIEntry->uiFunction);
	printf("\n Vendor: %x, Device: %x, Rev: %x", pPCIEntry->usVendorID, pPCIEntry->usDeviceID, pPCIEntry->bRevisionID);

  const uint64_t uiMappedIOAddr = _memMapBaseAddress + (ioAddress % PAGE_SIZE);
	_capReg = (XHCICapRegister*)uiMappedIOAddr;
	_opReg = (XHCIOpRegister*)(uiMappedIOAddr + _capReg->CapLength());

  _capReg->Print();
  //Enable Ports for Intel PanthorPoint XHCI
  if(pPCIEntry->usVendorID == INTEL_VENDOR_ID && pPCIEntry->usDeviceID == 0x1E31) {
    unsigned portsAvailable;

    portsAvailable = 0xFFFFFFFF;
    /* Write USB3_PSSEN, the USB 3.0 Port SuperSpeed Enable
     * Register, to turn on SuperSpeed terminations for all
     * available ports.
     */
    pPCIEntry->WritePCIConfig(0xD8, 4, portsAvailable);
    pPCIEntry->ReadPCIConfig(0xD8, 4, &portsAvailable);
    printf("\n USB 3.0 ports that are now enabled: %x", portsAvailable);

    /* Write XUSB2PR, the xHC USB 2.0 Port Routing Register, to
     * switch the USB 2.0 power and data lines over to the xHCI
     * host.
     */
    portsAvailable = 0xFFFFFFFF;
    pPCIEntry->WritePCIConfig(0xD0, 4, portsAvailable);
    pPCIEntry->ReadPCIConfig(0xD0, 4, &portsAvailable);
    printf("\n USB 2.0 ports that are now enabled: %x", portsAvailable);
  }

	/* Enable busmaster */
	unsigned short usCommand;
	pPCIEntry->ReadPCIConfig(PCI_COMMAND, 2, &usCommand);
	printf("\n CurVal of PCI_COMMAND: %x", usCommand);
	pPCIEntry->WritePCIConfig(PCI_COMMAND, 2, usCommand | PCI_COMMAND_MMIO | PCI_COMMAND_MASTER);
	pPCIEntry->ReadPCIConfig(PCI_COMMAND, 2, &usCommand);
	printf(" -> After Bus Master Enable, PCI_COMMAND: %x", usCommand);

  if(!_opReg->IsHCReady())
    throw upan::exception(XLOC, "HC is not ready yet!");

  _opReg->Stop();
  _opReg->HCReset();

  if(!_opReg->IsHCReady())
    throw upan::exception(XLOC, "HC is not ready yet!");

  LoadXCaps(uiMappedIOAddr);
  PerformBiosToOSHandoff();

  //program Max slots
  _opReg->MaxSlotsEnabled(_capReg->MaxSlots());
  _opReg->SetDNCTRL(0x2);

  //program device context base address pointer
  unsigned deviceContextTable = MemManager::Instance().AllocatePageForKernel() * PAGE_SIZE;
  _deviceContextAddrArray = (uint64_t*)KERNEL_VIRTUAL_ADDRESS(deviceContextTable);
  memset((void*)_deviceContextAddrArray, 0, PAGE_SIZE);
  _opReg->SetDCBaap(deviceContextTable);

  //allocate scratchpad buffer
  auto maxScratchpadBuffers = _capReg->MaxScratchpadBufSize();
  if(maxScratchpadBuffers > 0) {
    printf("\n Allocating %u scratchpad buffer entries", maxScratchpadBuffers);
    auto scratchpadBufferArray = (uint64_t*)DMM_AllocateForKernel(sizeof(uint64_t) * maxScratchpadBuffers, 64);
    for(uint32_t i = 0; i < maxScratchpadBuffers; ++i)
      scratchpadBufferArray[i] = MemManager::Instance().AllocatePageForKernel() * PAGE_SIZE;
    _deviceContextAddrArray[0] = (uint64_t)scratchpadBufferArray;
  }

  //Door Bell array
  _doorBellRegs = (unsigned*)(uiMappedIOAddr + _capReg->DoorBellOffset());

  InitInterruptHandler();

  //Setup Event Ring
  _eventManager = new EventManager(*this, *_capReg, *_opReg);

  //Setup Command Ring
  _cmdManager = new CommandManager(*_capReg, *_opReg, *_eventManager);
}

void XHCIController::Start()
{
  if(_opReg->IsHCRunning())
    return;
  _opReg->DisableHCInterrupt();
  _opReg->Run();
  ProcessManager::Instance().Sleep(100);
  if(!_opReg->IsHCRunning() || _opReg->IsHCHalted())
    throw upan::exception(XLOC, "Failed to start XHCI HC");
  _opReg->Print();
  _opReg->EnableHCInterrupt();
}

//Should be called from IRQ handler - so no need for any synchronization constructs
void XHCIController::NotifyEvent()
{
  if(!_opReg->StatusChanged())
    return;

  try
  {
    if(_opReg->IsHCHalted())
    {
      printf("\n XHCI host controller halted!!");
    }
    if(_opReg->IsHSError())
    {
      printf("\n XHCI host system error!!");
    }
    if(_opReg->IsAnyEventPending())
    {
      _eventManager->NotifyEvents();
    }
    if(_opReg->Saving())
    {
      printf("\n Saving XHCI internal state");
    }
    if(_opReg->Restoring())
    {
      printf("\n Restoring XHCI internal state");
    }
    if(_opReg->IsSRError())
    {
      printf("\n XHCI Save/Restore error!!");
    }
    if(_opReg->IsHCNotReady())
    {
      printf("\n XHCI host controller is not ready!!");
    }
    if(_opReg->IsHCError())
    {
      printf("\n XHCI host controller error!!");
    }
  }
  catch(const upan::exception& e)
  {
    printf("\n Exception while handling XHCI event - %s", e.ErrorMsg().c_str());
  }

  _opReg->Clear();
}

void XHCIController::InitInterruptHandler()
{
  //Can only support MSI capable XHCI
  if(!_pPCIEntry->SetupMsiInterrupt(XHCI_IRQ_NO))
    throw upan::exception(XLOC, "XHCI is not capable of MSI interrupts!");
  _pPCIEntry->SwitchToMsi();
}

void XHCIController::LoadXCaps(uint64_t base) {
	unsigned ecpOffset = _capReg->ECPOffset();

	if(!ecpOffset) {
    printf("\nXHCI System does not support Extended Capabilities");
    return;
  }

  _legSupXCap = nullptr;
  _supProtoXCaps.clear();

  base += ecpOffset;
  while(true) {
    unsigned& xCapReg = *(unsigned*)(base);
    unsigned capId = xCapReg & 0xFF;
    if(!capId)
      throw upan::exception(XLOC, "Invalid Xtended CapID 0!");

    if(capId == 1) {
      if(!_legSupXCap)
        _legSupXCap = (LegSupXCap*)&xCapReg;
      else
        printf("\n Found more than 1 LEG SUP Extended capability entries - something wrong!!");
    } else if(capId == 2) {
      _supProtoXCaps.push_back((SupProtocolXCap *) &xCapReg);
    } else {
      printf("\n Unhandled Extended CapID: %d", capId);
    }
    unsigned nextOffset = (((xCapReg >> 8) & 0xFF) << 2) & 0x3FC;
    if(!nextOffset)
      break;
    base += nextOffset;
  }
  for(auto i : _supProtoXCaps)
    i->Print();
}

void XHCIController::PerformBiosToOSHandoff() {
  if(!_legSupXCap) {
    printf("\n LEG SUP Extended capability is not supported - cannot perform BIOS to OS Handoff");
    return;
  }
  printf("\n Trying to perform complete handoff of XHCI Controller from BIOS to OS");
  _legSupXCap->BiosToOSHandoff();
}

SupProtocolXCap* XHCIController::GetSupportedProtocol(unsigned portId) const
{
  for(auto i : _supProtoXCaps)
  {
    if(i->HasPort(portId))
      return i;
  }
  return nullptr;
}

const char* XHCIController::PortProtocolName(USB_PROTOCOL protocol) const
{
  switch(protocol)
  {
    case USB_PROTOCOL::USB3: return "usb3";
    case USB_PROTOCOL::USB2: return "usb2";
    case USB_PROTOCOL::USB1: return "usb1";
    default: return "unknown";
  }
}

void XHCIController::Probe()
{
  Start();
	printf("\n Setup Ports") ;
  unsigned uiNoOfPorts = _capReg->MaxPorts();
	printf("-> NumPorts = %d", uiNoOfPorts) ;
  if(_capReg->IsPPC())
		printf("-> Software PortPowerCntrl") ;
	else
		printf("-> Hardware PortPowerCtrl") ;
	for(unsigned i = 0; i < uiNoOfPorts; i++)
	{
    const uint32_t portId = i + 1;
    auto& port = _opReg->PortRegisters()[i];
    auto supProtocol = GetSupportedProtocol(portId);
    if(!supProtocol)
    {
      printf("\n There is no Extended Supported Protocol Cap for Port: %d !", portId);
      continue;
    }
    auto protocol = supProtocol->Protocol();
    printf("\n Setup Port: %d [%s] -> ", portId, PortProtocolName(protocol));
    port.Print();
    if(_capReg->IsPPC())
      port.PowerOn();

    if(!port.IsPowerOn())
    {
      printf("\n Port %d is not Powered On!", portId);
      continue;
    }

    if(!port.IsDeviceConnected())
    {
      printf("\n No Device connected to Port %d", portId);
      continue;
    }

    if(!port.IsEnabled())
    {
      try 
      {
        if(protocol == USB_PROTOCOL::USB2)
        {
          port.Reset();
          //_eventManager->DebugPrint();
        }
        else
          port.WarmReset();
        if(!port.IsEnabled())
        {
          printf("\n Port still not enabled! ");
          port.Print();
          _opReg->Print();
          continue;
        }
      }
      catch(const upan::exception& ex)
      {
        printf("\n%s", ex.ErrorMsg().c_str());
      }
    }

    port.Print();
    PortSpeed ps = supProtocol->PortSpeedInfo(port.PortSpeedID());
    printf("\n %s [%u %s] device connected to port %d", PortProtocolName(protocol), ps.Mantissa(), ps.BitRateS(), portId);
    _devices[portId] = new XHCIDevice(*this, port, portId, supProtocol->SlotType());

    auto driver = USBController::Instance().FindDriver(_devices[portId]);
    if(driver)
      printf("\n'%s' driver found for the USB Device", driver->Name().c_str());
    else
      printf("\nNo Driver found for this USB device") ;
	}
}

void XHCIController::SetDeviceContext(uint32_t slotID, SlotContext& slotContext)
{
  _deviceContextAddrArray[slotID] = (uint64_t)KERNEL_REAL_ADDRESS(&slotContext);
}

void XHCIController::RingDoorBell(unsigned index, unsigned value)
{
//  __volatile__ uint32_t temp = _doorBellRegs[index];
//  _doorBellRegs[index] = value;
//  temp = _doorBellRegs[index];
  upan::atomic::op::swap(_doorBellRegs[index], value);
}

EventTRB XHCIController::InitiateCommand()
{
  RegisterForWaitedEventResult(_cmdManager->CommandTRBAddress());
  RingDoorBell(0, 0);

  return WaitForCmdCompletion();
}

EventTRB XHCIController::InitiateTransfer(uint32_t trbId, uint32_t slotID, uint32_t ep)
{
  RegisterForWaitedEventResult(trbId);
  RingDoorBell(slotID, ep);

  return WaitForTransferCompletion(trbId);
}

void XHCIController::InitiateInterruptTransfer(InputContext& context, uint32_t trbId, uint32_t slotID, uint32_t ep, uint32_t interruptDataAddress)
{
  {
    //TODO: Find an efficient way w/o disabling interrupts
    IrqGuard g;
    _eventResults[trbId] = new InterruptEventResult(context, ProcessManager::Instance().GetCurProcId(), interruptDataAddress);
  }
  RingDoorBell(slotID, ep);
}

unsigned XHCIController::EnableSlot(unsigned slotType)
{
  _cmdManager->EnableSlot(slotType);
  return InitiateCommand().SlotID();
}

void XHCIController::AddressDevice(unsigned icptr, unsigned slotID, bool blockSetAddressReq)
{
  _cmdManager->AddressDevice(icptr, slotID, blockSetAddressReq);
  InitiateCommand();
}

void XHCIController::ConfigureEndPoint(unsigned icptr, unsigned slotID)
{
  _cmdManager->ConfigureEndPoint(icptr, slotID);
  InitiateCommand();
}

void XHCIController::EvaluateContext(unsigned icptr, unsigned slotID)
{
  _cmdManager->EvaluateContext(icptr, slotID);
  InitiateCommand();
}

EventTRB XHCIController::WaitForCmdCompletion()
{
  auto result = WaitForEvent(_cmdManager->CommandTRBAddress());
  result.ValidateCommandResult();
  return result;
}

EventTRB XHCIController::WaitForTransferCompletion(uint32_t trbId)
{
  auto result = WaitForEvent(trbId);

  result.ValidateTransferResult();
//  if(!result.IsEventDataTRB())
  //  throw upan::exception(XLOC, "Transfer event is not Event Data TRB");
  return result;
}

EventTRB XHCIController::WaitForEvent(uint32_t trbId)
{
  if(XHCIManager::Instance().GetEventMode() == XHCIManager::Poll)
  {
    EventTRB result;
    if(!_eventManager->WaitForEvent(trbId, result))
      throw upan::exception(XLOC, "Timedout while waiting for Command Completion");
    return result;
  }
  else
  {
    ProcessManager::Instance().WaitForEvent();
    EventResult& result = ConsumeEventResult(trbId);
    auto eventTRB = result.Result();
    delete &result;
    return eventTRB;
  }
}

void XHCIController::RegisterForWaitedEventResult(uint32_t trbId)
{
  //TODO: Find an efficient way w/o disabling interrupts
  IrqGuard g;
  //printf("\n Registering for TRB event: %x, Process: %d", trbId, ProcessManager::Instance().GetCurProcId());
  _eventResults[trbId] = new WaitedEventResult(ProcessManager::Instance().GetCurProcId());
}

EventResult& XHCIController::ConsumeEventResult(uint32_t trbId)
{
  //TODO: Find an efficient way w/o disabling interrupts
  IrqGuard g;
  auto it = _eventResults.find(trbId);
  if(it == _eventResults.end())
    throw upan::exception(XLOC, "Can't find TRB ID: %x in EventResults", trbId);
  auto result = it->second;
  _eventResults.erase(it);
  return *result;
}

//This function is called from XHCI IRQ handler, hence doesn't need any
//synchronization construct i.e. IrqGuard/ProcessSwitchLock/Mutex
void XHCIController::PublishEventResult(const EventTRB& result)
{
  auto it = _eventResults.find(result.TRBPointer());
  if(it == _eventResults.end())
  {
    printf("\n No entry found in EventResults for TRB Id: %x, Event Type: %d CC: %d, TLen: %d\n",
           (uint32_t)result.TRBPointer(), result.Type(), result.CompletionCode(), result.TransferLength());
    ((TRB*)KERNEL_VIRTUAL_ADDRESS(result.TRBPointer()))->Print();
    return;
  }
  it->second->Consume(result);
}

CommandManager::CommandManager(XHCICapRegister& creg, 
  XHCIOpRegister& oreg,
  EventManager& eventManager)
  : _pcs(true), _ring(nullptr), _capReg(creg), _opReg(oreg), _eventManager(eventManager)
{
  _ring = new ((void*)DMM_AllocateForKernel(sizeof(Ring), 64))Ring();
  uint64_t ringAddr = KERNEL_REAL_ADDRESS(_ring);

  _ring->_link.SetLinkAddr(ringAddr);
  _ring->_link.SetToggleBit(true);

  _opReg.SetCommandRingPointer(ringAddr);
}

void CommandManager::Apply()
{
  _ring->_cmd.SetCycleBit(_pcs);
  _ring->_link.SetCycleBit(_pcs);
  _pcs = !_pcs;
}

void CommandManager::DebugPrint()
{
  printf("\n Command Ring - ");
  _ring->_cmd.Print();
  _ring->_link.Print();
}

void CommandManager::EnableSlot(unsigned slotType)
{
  _ring->_cmd = EnableSlotTRB(slotType);
  Apply();
}

void CommandManager::DisableSlot(unsigned slotID)
{
  _ring->_cmd = DisableSlotTRB(slotID);
  Apply();
}

void CommandManager::AddressDevice(unsigned icptr, unsigned slotID, bool blockSetAddressReq)
{
  _ring->_cmd = AddressDeviceTRB(icptr, slotID, blockSetAddressReq);
  Apply();
}

void CommandManager::ConfigureEndPoint(unsigned icptr, unsigned slotID)
{
  _ring->_cmd = ConfigureEndPointTRB(icptr, slotID);
  Apply();
}

void CommandManager::EvaluateContext(unsigned icptr, unsigned slotID)
{
  _ring->_cmd = EvaluateContextTRB(icptr, slotID);
  Apply();
}

EventManager::InterrupterRegister::InterrupterRegister() 
{
  _iman = 0; //Interrupter is disabled by default
  _imod = 4000; //15:0 = 4000 --> 1ms, 31:16 = 0 --> initial count
  const int ERST_SIZE = 1;
  _erstSize = (_erstSize & 0xFFFF0000) | ERST_SIZE;

  ERSTEntry* erst = new ((void*)DMM_AllocateForKernel(sizeof(ERSTEntry) * ERST_SIZE, 64))ERSTEntry[ERST_SIZE];
  _erstBA = (uint64_t)KERNEL_REAL_ADDRESS(erst);

  _erdqPtr = erst[0]._ersAddr;
}

void EventManager::InterrupterRegister::DebugPrint()
{
  printf("\n IMAN: %x, IMON: %x, DQPTR: %x", _iman, _imod, (unsigned)DQPtr());
//  _erdqPtr = _erdqPtr | 0x8;
//  _iman = _iman | 0x1;
  for(int i = 0; i < 16; ++i)
    ERSegment(0)[i].Print();
}

const int EventManager::ERST_SIZE = 256;
EventManager::ERSTEntry::ERSTEntry() : _size(ERST_SIZE)
{
  unsigned eventSegmentTable = MemManager::Instance().AllocatePageForKernel() * PAGE_SIZE;
  EventTRB* events = new ((void*)KERNEL_VIRTUAL_ADDRESS(eventSegmentTable))EventTRB[_size];
  _ersAddr = (uint64_t)KERNEL_REAL_ADDRESS(events);
}

EventManager::EventManager(XHCIController& controller, XHCICapRegister& creg, XHCIOpRegister& oreg)
  : _eventCycleBit(true), _capReg(creg), _opReg(oreg), _controller(controller)
{
  _iregs = (InterrupterRegister*)((unsigned)&_capReg + _capReg.RTSOffset() + 0x20);
  for(unsigned i = 0; i < _capReg.MaxIntrs(); ++i)
    new ((void*)&_iregs[i])InterrupterRegister();
  _iregs[0].EnableInterrupt();
}

void EventManager::DebugPrint() const
{
  printf("\n Event Ring - ");
  _iregs[0].DebugPrint();
}

bool EventManager::WaitForEvent(uint32_t trbId, EventTRB& result)
{
  int timeout = 2000;//2 seconds
  while(timeout > 10)
  {
    if(_iregs[0].EventCycleBitSet() == _eventCycleBit)
    {
      if(_iregs[0].IsLastDQPtr())
        _eventCycleBit = !_eventCycleBit;
      result = *_iregs[0].DQEvent();
      _iregs[0].IncrementDQPtr();
      if(result.TRBPointer() == trbId)
        return true;
    }
    ProcessManager::Instance().Sleep(10);
    timeout -= 10;
  }
  return false;
}

void EventManager::NotifyEvents()
{
  unsigned count = 0;
  while(_iregs[0].EventCycleBitSet() == _eventCycleBit)
  {
    if(_iregs[0].IsLastDQPtr())
      _eventCycleBit = !_eventCycleBit;

    _controller.PublishEventResult(*_iregs[0].DQEvent());

    _iregs[0].IncrementDQPtr();

    ++count;
    if(count == ERST_SIZE)
      break;
  }
}

void WaitedEventResult::Consume(const EventTRB &r)
{
  _result = r;
  ProcessManager::Instance().EventCompleted(Pid());
}

void InterruptEventResult::Consume(const EventTRB &r)
{
  _result = r;

  try
  {
    r.ValidateTransferResult();
    EventResult& eventResult = _context.Controller().ConsumeEventResult(r.TRBPointer());
    auto intEventResult = dynamic_cast<InterruptEventResult*>(&eventResult);
    if(!intEventResult)
      throw upan::exception(XLOC, "Non-InterruptEventResult found for interrupt transfer @ TRB ID: %x", r.TRBPointer());
    _context.OnInterrupt(r, intEventResult->InterruptDataAddress());
    delete &eventResult;
  }
  catch(upan::exception& e)
  {
    e.Print();
  }
}
