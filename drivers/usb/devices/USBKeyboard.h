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
#ifndef _USB_KEYBOARD_H_
#define _USB_KEYBOARD_H_

#include <Global.h>
#include <SCSIHandler.h>
#include <USBStructures.h>
#include <USBDataHandler.h>
#include <list.h>
#include <set.h>
#include <KernelUtil.h>

class USBKeyboard final : public USBInterruptDataHandler, public KernelUtil::TimerTask
{
public:
  USBKeyboard(USBDevice& device, int interfaceIndex);
  ~USBKeyboard();
  USBDevice& GetUSBDevice() { return _device; }

private:
  virtual void Handle(uint64_t data);
  virtual bool TimerTrigger();

  const int STD_USB_KB_REPORT_LEN = 8;
  USBDevice&      _device;
  upan::set<byte> _pressedKeys;
  byte**          _reports;
  int             _currentReportIndex;
};

class USBKeyboardDriver final : public USBDriver
{
private:
  static USBKeyboardDriver* _instance;
public:
  USBKeyboardDriver(const upan::string& name);
  static void Register();
  static USBKeyboardDriver& Instance();
  void Process(byte rawKey, bool isKeyReleased);
  void ProcessSpecialKeys(byte report);

private:
  bool AddDevice(USBDevice*);
  void RemoveDevice(USBDevice*);
  int MatchingInterfaceIndex(USBDevice*);

private:
  const int CLASS_CODE = 0x3;
  const int SUB_CLASS_CODE_BOOT = 0x01;
  const int PROTOCOL = 0x1;

  bool _isShiftKey;
  bool _isCapsLock;
  bool _isCtrlKey;

  upan::list<USBKeyboard*> _devices;
};

#endif
