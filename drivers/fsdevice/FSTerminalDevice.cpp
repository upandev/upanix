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
    _inBuffer(inBufSize, SB_IN, path), _outBuffer(outBufSize, SB_OUT, path),
    _directKernelConsole(false), _mode(CANONICAL), _echo(true) {
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
  return _inBuffer.read(buffer, len, true);
}

int FSTerminalDevice::writeInStream(const void* buffer, int len) {
  return _inBuffer.write(buffer, len, false);
}

bool FSTerminalDevice::canReadOutStream() const {
  return _outBuffer.canRead();
}

bool FSTerminalDevice::canWriteOutStream() const {
    return _outBuffer.canWrite();
}

int FSTerminalDevice::readOutStream(void* buffer, int len) {
  return _outBuffer.read(buffer, len, true);
}

int FSTerminalDevice::writeOutStream(const void* buffer, int len) {
  if (_owner.processID() == NO_PROCESS_ID && _directKernelConsole) {
    KC::MConsole().nMessage((char*) buffer, len, upanui::CharStyle::WHITE_ON_BLACK());
    return len;
  }
  return _outBuffer.write(buffer, len, true);
}

FSTerminalDevice::StreamBuffer::StreamBuffer(int bufSize, STREAM_BUFFER_TYPE type, const upan::string& path) : _queue(bufSize), _type(type), _path(path) {
}

bool FSTerminalDevice::StreamBuffer::canRead() const {
  upan::mutex_guard g(_ioSync);
  return !_queue.empty();
}

int FSTerminalDevice::StreamBuffer::read(void* buffer, int len, bool block) {
  while(true) {
    {
      upan::mutex_guard g(_ioSync);
      if (!_queue.empty()) {
        return _queue.read((uint8_t*)buffer, len);
      }
    }
    if (!block) {
      PCSound::Instance().Beep();
      return 0;
    }

    const auto waitType = _type == SB_IN ? TERMINAL_IO_TYPES::TERMINAL_IN_READ : TERMINAL_IO_TYPES::TERMINAL_OUT_READ;
    ProcessManager::Instance().WaitOnTerminalIO(_path, waitType, 0);

    const auto err = ProcessManager::Instance().GetCurrentPAS().stateInfo().getError();
    if (err == ProcessStateInfo::INTERRUPTED) {
      throw upan::exception(XLOC, "TerminalDevice read interrupted for process %d", 1);
    }
  }
}

bool FSTerminalDevice::StreamBuffer::canWrite() const {
  upan::mutex_guard g(_ioSync);
  return !_queue.full();
}

int FSTerminalDevice::StreamBuffer::write(const void* buffer, int len, bool block) {
  while(true) {
    {
      upan::mutex_guard g(_ioSync);
      if (!_queue.full()) {
        return _queue.write((uint8_t*)buffer, len);
      }
    }
    if (!block) {
      PCSound::Instance().Beep();
      return 0;
    }

    const auto waitType = _type == SB_IN ? TERMINAL_IO_TYPES::TERMINAL_IN_WRITE : TERMINAL_IO_TYPES::TERMINAL_OUT_WRITE;
    ProcessManager::Instance().WaitOnTerminalIO(_path, waitType, 0);

    const auto err = ProcessManager::Instance().GetCurrentPAS().stateInfo().getError();
    if (err == ProcessStateInfo::INTERRUPTED) {
      throw upan::exception(XLOC, "TerminalDevice write interrupted for process %d", 1);
    }
  }
}