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
#include <Global.h>
#include <MemConstants.h>
#include <XHCIController.h>
#include <XHCIManager.h>
#include <IrqManager.h>
#include <KeyboardHandler.h>
#include <InterruptHandlers.h>

static const IRQ* XHCI_IRQ = nullptr;

void XHCIManager::Handler() {
  //printf("\n XHCI IRQ");
  if(XHCIManager::Instance().Initialized() && XHCIManager::Instance().GetEventMode() == XHCIManager::Interrupt)
  {
    for(auto c : XHCIManager::Instance().Controllers())
      c->NotifyEvent();
  }

  IrqManager::Instance().SendEOI(*XHCI_IRQ);
}

extern "C" {
  void _xhci_interrupt_handler();
}

XHCIManager::XHCIManager() : _initialized(false)
{
  XHCI_IRQ = IrqManager::Instance().RegisterIRQ(XHCI_IRQ_NO, (uintptr_t)_xhci_interrupt_handler);
  if(XHCI_IRQ) {
    _eventMode = EventMode::Interrupt;
    IrqManager::Instance().DisableIRQ(*XHCI_IRQ);
  } else {
    _eventMode = EventMode::Poll;
    printf("Failed to register XHCI IRQ: %d", XHCI_IRQ_NO);
  }
}

void XHCIManager::Initialize()
{
  if(!XHCI_IRQ)
    return;

  for(auto c : _controllers) {
    delete c;
  }
  _controllers.clear();
  
  for(auto pPCIEntry : PCIBusHandler::Instance().PCIEntries())
  {
    if(pPCIEntry->bHeaderType & PCI_HEADER_BRIDGE)
      continue;

    if(pPCIEntry->bInterface == 48 && 
      pPCIEntry->bClassCode == PCI_SERIAL_BUS && 
      pPCIEntry->bSubClass == PCI_USB)
    {
      printf("\n Interface = %u, Class = %u, SubClass = %u", pPCIEntry->bInterface, pPCIEntry->bClassCode, pPCIEntry->bSubClass);
      try
      {
        _controllers.push_back(new XHCIController(pPCIEntry));
      }
      catch(const upan::exception& ex)
      {
        printf("\n Failed to add XHCI controller - Reason: %s", ex.ErrorMsg().c_str());
      }
    }
  }

	if(_controllers.size())
    KC::MConsole().LoadMessage("USB XHCI Controller Found", Success);
	else
    KC::MConsole().LoadMessage("No USB XHCI Controller Found", Failure);

  if(_eventMode == EventMode::Interrupt)
    IrqManager::Instance().EnableIRQ(*XHCI_IRQ);
  _initialized = true;
}

void XHCIManager::ProbeDevice()
{
  if(!_initialized)
    throw upan::exception(XLOC, "XHCI is not initialized yet!");

  for(auto c : _controllers)
  {
    try
    {
      c->Probe();
    }
    catch(const upan::exception& ex)
    {
      printf("\n Error probing XHCI controller ports - %s", ex.ErrorMsg().c_str());
    }
  }
}
