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
#include <stdio.h>
#include <XHCIStructures.h>
#include <ProcessManager.h>

void XHCICapRegister::Print() const
{
  printf("\n CapLen: %d, HCSPARAMS1: %x, HCSPARAMS2: %x, HCSPARAMS3: %x", _capLength, _hcsParams1, _hcsParams2, _hcsParams3);
  printf("\n HCCPARAMS1: %x, HCCPARAMS2: %x, DoorBellOffset: %x, RTSOffset: %x", _hccParams1, _hccParams2, DoorBellOffset(), RTSOffset());
}

void XHCIOpRegister::Print() const
{
  printf("\n CMD: %x, STAT: %x, PAGESIZE: %x, DNCTRL: %x", _usbCmd, _usbStatus, _pageSize, _dnCtrl);
  printf("\n CRCR: %llx, DCBAAP: %llx, CONFIG: %x", _crcr, _dcBaap, _config);
}

void XHCIOpRegister::Run()
{
  if(!IsHCRunning())
  {
    if(!IsHCHalted())
    {
      if(!ProcessManager::Instance().ConditionalWait(&_usbStatus, 0, true))
        throw upan::exception(XLOC, "Cannot Run HC while it's not Halted");
    }
    _usbCmd |= 0x1;
  }

  if(!ProcessManager::Instance().ConditionalWait(&_usbStatus, 0, false))
    throw upan::exception(XLOC, "Failed to start HC - timedout");

  ProcessManager::Instance().Sleep(100000);
}

void XHCIOpRegister::Stop()
{
  if(IsHCRunning())
  {
    if(IsHCHalted())
    {
      if(!ProcessManager::Instance().ConditionalWait(&_usbStatus, 0, false))
        throw upan::exception(XLOC, "Cannot stop HC while it's Halted");
    }
    _usbCmd &= ~(0x1);
  }

  if(!ProcessManager::Instance().ConditionalWait(&_usbStatus, 0, true))
    throw upan::exception(XLOC, "Failed to stop HC - timedout");
}

void XHCIOpRegister::HCReset()
{
  if(!IsHCHalted())
    throw upan::exception(XLOC, "Cannot reset while HC is halted");

  if(_usbCmd & 0x2)
    throw upan::exception(XLOC, "Cannot reset while reset in-progress");

  _usbCmd |= 0x2;

  if(!ProcessManager::Instance().ConditionalWait(&_usbCmd, 1, false))
    throw upan::exception(XLOC, "reset didn't complete: timed-out");
}

void XHCIOpRegister::LightHCReset(const XHCICapRegister* cr)
{
  if(!cr->IsLHRC())
    throw upan::exception(XLOC, "Light HC reset is not supported by HC");

  if(_usbCmd & 0x80)
    throw upan::exception(XLOC, "Cannot reset while reset is in-progress");

  _usbCmd |= 0x80;

  if(!ProcessManager::Instance().ConditionalWait(&_usbCmd, 7, false))
    throw upan::exception(XLOC, "light reset didn't complete - timedout");
}

void XHCIOpRegister::HCSave()
{
  if(!IsHCHalted())
    throw upan::exception(XLOC, "Cannot Save while HC is not halted");

  if(Saving())
    throw upan::exception(XLOC, "Cannot initiate save while save is in-progress");

  if(Restoring())
    throw upan::exception(XLOC, "Cannot initiate save while restore is in-progress");

  _usbCmd |= 0x100;

  ProcessManager::Instance().Sleep(20000);
  if(!ProcessManager::Instance().ConditionalWait(&_usbStatus, 8, false))
    throw upan::exception(XLOC, "HC Save didn't complete - timedout");
}

void XHCIOpRegister::HCRestore()
{
  if(!IsHCHalted())
    throw upan::exception(XLOC, "Cannot restore while HC is not halted");

  if(Saving())
    throw upan::exception(XLOC, "Cannot initiate restore while save is in-progress");

  if(Restoring())
    throw upan::exception(XLOC, "Cannot initiate restore while restore is in-progress");

  _usbCmd |= 0x200;

  ProcessManager::Instance().Sleep(20000);
  if(!ProcessManager::Instance().ConditionalWait(&_usbStatus, 9, false))
    throw upan::exception(XLOC, "HC Restore didn't complete - timedout");
}

bool XHCIOpRegister::IsShortPacketEnabled(const XHCICapRegister* cr) const
{
  if(cr->IsSPC())
    return _usbCmd & 0x1000 ? true : false;
  return false;
}

void XHCIOpRegister::EnableShortPacket(const XHCICapRegister* cr)
{
  if(!cr->IsSPC())
    throw upan::exception(XLOC, "Stopped - short packet completion is not supported by HC");

  _usbCmd |= 0x1000;
}

void XHCIOpRegister::DisableShortPacket(const XHCICapRegister* cr)
{
  if(!cr->IsSPC())
    throw upan::exception(XLOC, "Stopped - short packet completion is not supported by HC");

  _usbCmd &= ~(0x1000);
}

unsigned XHCIOpRegister::SupportedPageSize() const
{
  const unsigned pageSize = _pageSize & 0xFFFF;
  unsigned k = 1;
  for(unsigned i = 1; i > 0x8000; i <<= 1)
  {
    if(pageSize & i)
      break;
    ++k;
  }
  return 4 * k;
}

void XHCIOpRegister::MaxSlotsEnabled(unsigned val)
{
  if(IsHCRunning())
    throw upan::exception(XLOC, "Cannot set max-slots-enabled while HC is running");
  _config = (_config & ~(0xFF)) | (val & 0xFF);
}

bool XHCIOpRegister::IsHCReady() const 
{ 
  if(IsHCNotReady())
  {
    ProcessManager::Instance().Sleep(100000);
    return !IsHCNotReady();
  }
  return true;
}

void XHCIPortRegister::Reset()
{
  if(_sc & 0x10)
    throw upan::exception(XLOC, "Cannot reset while reset is in-progress");

  ClearPRC();
  _sc |= 0x10;

  if(!ProcessManager::Instance().ConditionalWait(&_sc, 21, true))
    throw upan::exception(XLOC, "Failed to complete reset - timedout (SC = %x)", _sc);
}

void XHCIPortRegister::WarmReset()
{
  if(_sc & 0x10)
    throw upan::exception(XLOC, "Cannot reset while reset is in-progress");

  //Clear port reset change bit
  _sc |= 0x80000;
  _sc |= 0x80000000;

  if(!ProcessManager::Instance().ConditionalWait(&_sc, 19, true))
    throw upan::exception(XLOC, "Failed to complete reset - timedout");
}

void XHCIPortRegister::PowerOn()
{
  Bit::Set(_sc, 0x200, true);
  ProcessManager::Instance().Sleep(100000);
}

void XHCIPortRegister::PowerOff()
{
  Bit::Set(_sc, 0x200, false);
  ProcessManager::Instance().Sleep(100000);
}

void XHCIPortRegister::Print()
{
  printf("SC: 0x%x, PMSC: 0x%x, LI: 0x%x, HLPMC: 0x%x", _sc, _pmsc, _li, _hlpmc);
}

void LegSupXCap::BiosToOSHandoff()
{
  if(IsOSOwned())
  {
    printf("XHCI Controller is already owned by OS. No need for Handoff");
    return;
  }

  _usbLegSup |= 0x1000000;
  if(!ProcessManager::Instance().ConditionalWait(&_usbLegSup, 24, true))
    throw upan::exception(XLOC, "BIOS to OS Handoff failed - HC Owned bit is stil not set");
  if(!ProcessManager::Instance().ConditionalWait(&_usbLegSup, 16, false))
    throw upan::exception(XLOC, "BIOS to OS Handoff failed - Bios Owned bit is still set");

  printf("\n XHCI Bios to OS Handoff completed - LEGSUP: %x", _usbLegSup);
}
