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

#pragma once

#include <IODescriptor.h>
#include <shared_ptr.h>
#include <list.h>
#include <mutex.h>

class SocketDescriptorDataGramBuffer : public IODescriptor {
public:
  SocketDescriptorDataGramBuffer(int pid, int id, uint32_t bufSize, uint32_t mode);
  void setPeer(upan::shared_ptr<SocketDescriptorDataGramBuffer> peer) {
    _peer = peer;
  }
  int send(const void* buffer, int len);
  bool canSend();

private:
  int _read(void* buffer, int len) override;
  bool _canRead() override;
  int _write(const void* buffer, int len) override;
  bool _canWrite() override;
  void _seek(int seekType, int offset) override {}
  uint32_t _getOffset() const override { return 0; }
  void _close() override {}

  struct Message {
    Message(const void* buffer, int len);
    upan::shared_ptr<uint8_t[]> _buffer;
    int _len;
  };
private:
  const uint32_t _maxBufSize;
  upan::shared_ptr<SocketDescriptorDataGramBuffer> _peer;
  uint32_t _bufSize;
  upan::list<Message> _queue;
  upan::mutex _ioSync;
};
