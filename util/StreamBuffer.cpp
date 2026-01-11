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

#include <StreamBuffer.h>
#include <ProcessManager.h>
#include <PCSound.h>

StreamBuffer::StreamBuffer(int bufSize) : _queue(bufSize) {
}

bool StreamBuffer::canRead() const {
  upan::mutex_guard g(_ioSync);
  return !_queue.empty();
}

int StreamBuffer::read(void* buffer, int len, bool block, upan::function<void> waitFunc) {
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

    waitFunc();

    const auto err = ProcessManager::Instance().GetCurrentPAS().stateInfo().getError();
    if (err == ProcessStateInfo::INTERRUPTED) {
      throw upan::exception(XLOC, "Stream read interrupted for process %d", 1);
    }
  }
}

bool StreamBuffer::canWrite() const {
  upan::mutex_guard g(_ioSync);
  return !_queue.full();
}

int StreamBuffer::write(const void* buffer, int len, bool block, upan::function<void> waitFunc) {
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

    waitFunc();

    const auto err = ProcessManager::Instance().GetCurrentPAS().stateInfo().getError();
    if (err == ProcessStateInfo::INTERRUPTED) {
      throw upan::exception(XLOC, "Stream write interrupted for process %d", 1);
    }
  }
}