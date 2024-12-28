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

#include <StreamBufferDescriptor.h>
#include <fs.h>
#include <ProcessConstants.h>
#include <KernelComponents.h>
#include <ProcessManager.h>

StreamBufferDescriptor::StreamBufferDescriptor(int pid, int id, uint32_t bufSize, uint32_t mode)
  : IODescriptor(pid, id, O_APPEND | mode), _queue(bufSize) {
}

int StreamBufferDescriptor::read(void* buffer, int len) {
  while(true) {
    {
      upan::mutex_guard g(_ioSync);
      if (!_queue.empty()) {
        return _queue.read((uint8_t*)buffer, len);
      }
    }
    if (getMode() & O_RD_NONBLOCK) {
      return 0;
    }
    ProcessManager::Instance().WaitOnIODescriptor(id(), IO_OP_TYPES::IO_Read, 0);
  }
}

bool StreamBufferDescriptor::canRead() {
  upan::mutex_guard g(_ioSync);
  return !_queue.empty();
}

int StreamBufferDescriptor::write(const void* buffer, int len) {
  if (getPid() == NO_PROCESS_ID && id() == IODescriptorTable::STDOUT) {
    KC::MConsole().nMessage((char*)buffer, len, upanui::CharStyle::WHITE_ON_BLACK());
    return len;
  } else {
    while(true) {
      {
        upan::mutex_guard g(_ioSync);
        if (!_queue.full()) {
          return _queue.write((uint8_t*)buffer, len);
        }
      }
      if (getMode() & O_WR_NONBLOCK) {
        return 0;
      }
      ProcessManager::Instance().WaitOnIODescriptor(id(), IO_OP_TYPES::IO_Write, 0);
    }
  }
}

bool StreamBufferDescriptor::canWrite() {
  upan::mutex_guard g(_ioSync);
  return !_queue.full();
}