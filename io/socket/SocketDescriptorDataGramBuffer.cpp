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

#include <SocketDescriptorDataGramBuffer.h>
#include <ProcessManager.h>

SocketDescriptorDataGramBuffer::SocketDescriptorDataGramBuffer(int pid, int id, uint32_t bufSize, uint32_t mode)
  : IODescriptor(pid, id, O_APPEND | mode), _maxBufSize(bufSize), _bufSize(0) {
}

int SocketDescriptorDataGramBuffer::_read(void* buffer, int len) {
  while(true) {
    {
      upan::mutex_guard g(_ioSync);
      if (!_queue.empty()) {
        const auto& message = *_queue.begin();
        _queue.pop_front();
        int n = upan::min(message._len, len);
        memcpy(buffer, message._buffer.get(), n);
        _bufSize -= message._len;
        return n;
      }
    }
    if (getMode() & O_RD_NONBLOCK || getMode() & O_NONBLOCK) {
      return 0;
    }
    ProcessManager::Instance().WaitOnIODescriptor(id(), IODescriptorTable::IO_OP_TYPES::IO_Read, 0);
    const auto err = ProcessManager::Instance().GetCurrentPAS().stateInfo().getError();
    if (err == ProcessStateInfo::INTERRUPTED) {
      throw upan::exception(XLOC, "StreamBuffer read interrupted");
    }
  }
}

bool SocketDescriptorDataGramBuffer::_canRead() {
  upan::mutex_guard g(_ioSync);
  return !_queue.empty();
}

int SocketDescriptorDataGramBuffer::_write(const void* buffer, int len) {
  if (_peer.isEmpty()) {
    throw upan::exception(XLOC, "write failed - socket-pair peer is not connected for %d", id());
  }
  return _peer->send(buffer, len);
}

SocketDescriptorDataGramBuffer::Message::Message(const void* buffer, int len) : _buffer(new uint8_t[len]), _len(len) {
};

int SocketDescriptorDataGramBuffer::send(const void* buffer, int len) {
  while (true) {
    {
      upan::mutex_guard g(_ioSync);
      if ((_bufSize + len) <= _maxBufSize) {
        _queue.push_back(Message(buffer, len));
        return len;
      }
    }
    if (getMode() & O_WR_NONBLOCK || getMode() & O_NONBLOCK) {
      return 0;
    }
    ProcessManager::Instance().WaitOnIODescriptor(id(), IODescriptorTable::IO_OP_TYPES::IO_Write, 0);
    const auto err = ProcessManager::Instance().GetCurrentPAS().stateInfo().getError();
    if (err == ProcessStateInfo::INTERRUPTED) {
      throw upan::exception(XLOC, "StreamBuffer write interrupted");
    }
  }
}

bool SocketDescriptorDataGramBuffer::_canWrite() {
  if (_peer.isEmpty()) {
    throw upan::exception(XLOC, "canWrite failed - socket-pair peer is not connected for %d", id());
  }
  return _peer->canSend();
}

bool SocketDescriptorDataGramBuffer::canSend() {
  upan::mutex_guard g(_ioSync);
  return _bufSize < _maxBufSize;
}