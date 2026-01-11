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

#include <FSTerminalDevice.h>
#include <ProcessConstants.h>
#include <KernelComponents.h>
#include <ProcessManager.h>
#include <PCSound.h>

FSTerminalDevice::FSTerminalDevice(Process& owner, const upan::string& path, int inBufSize, int outBufSize)
  : FSDevice(path), _owner(owner),
    _inBuffer(inBufSize), _outBuffer(outBufSize),
    _directKernelConsole(false) {
  _termios.c_lflag = ICANON | ECHO |  ISIG;
}

bool FSTerminalDevice::isReady(TERMINAL_IO_TYPES ioType) const {
  switch(ioType) {
    case TERMINAL_IO_TYPES::TERMINAL_IN_READ:
      return canReadInStream();
    case TERMINAL_IO_TYPES::TERMINAL_IN_WRITE:
      return canWriteInStream();
    case TERMINAL_IO_TYPES::TERMINAL_OUT_READ:
      return canReadOutStream();
    case TERMINAL_IO_TYPES::TERMINAL_OUT_WRITE:
      return canWriteOutStream();
    default:
      return false;
  }
}

bool FSTerminalDevice::canReadInStream() const {
  return _inBuffer.canRead();
}

bool FSTerminalDevice::canWriteInStream() const {
  return _inBuffer.canWrite();
}

int FSTerminalDevice::readInStream(void* buffer, int len) {
  return _inBuffer.read(buffer, len, true, [&]() {
    ProcessManager::Instance().WaitOnTerminalIO(path(), TERMINAL_IO_TYPES::TERMINAL_IN_READ, 0);
  });
}

int FSTerminalDevice::writeInStream(const void* buffer, int len) {
  const int n = _inBuffer.write(buffer, len, false, [&]() {
    ProcessManager::Instance().WaitOnTerminalIO(path(), TERMINAL_IO_TYPES::TERMINAL_IN_WRITE, 0);
  });
  if ((_termios.c_lflag & ECHO) && n > 0) {
    for (int i = 0; i < n; ++i) {
      auto ch = ((uint8_t*)buffer)[i];
      switch (ch) {
        case Keyboard_F1:
        case Keyboard_F2:
        case Keyboard_F3:
        case Keyboard_F4:
        case Keyboard_F5:
        case Keyboard_F6:
        case Keyboard_F7:
        case Keyboard_F8:
        case Keyboard_BACKSPACE:
          break;
        default:
          writeOutStream(&ch, 1);
      }
    }
  }
  return n;
}

bool FSTerminalDevice::canReadOutStream() const {
  return _outBuffer.canRead();
}

bool FSTerminalDevice::canWriteOutStream() const {
    return _outBuffer.canWrite();
}

int FSTerminalDevice::readOutStream(void* buffer, int len) {
  return _outBuffer.read(buffer, len, true, [&]() {
    ProcessManager::Instance().WaitOnTerminalIO(path(), TERMINAL_IO_TYPES::TERMINAL_OUT_READ, 0);
  });
}

int FSTerminalDevice::writeOutStream(const void* buffer, int len) {
  if (_owner.processID() == NO_PROCESS_ID && _directKernelConsole) {
    KC::MConsole().nMessage((char*) buffer, len, upanui::CharStyle::WHITE_ON_BLACK());
    return len;
  }
  return _outBuffer.write(buffer, len, true, [&]() {
    ProcessManager::Instance().WaitOnTerminalIO(path(), TERMINAL_IO_TYPES::TERMINAL_OUT_WRITE, 0);
  });
}