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
#include <sys/socket.h>

class NetworkDevice;
class Socket;

class SocketDescriptor : public IODescriptor {
public:
  SocketDescriptor(int pid, int fd, SOCKET_TYPE type);
  ~SocketDescriptor() override;

  int read(void* buffer, int len) override;
  bool canRead() override { return true; }

  int write(const void* buffer, int len) override;
  bool canWrite() override { return true; }

  void seek(int seekType, int offset) override { }
  uint32_t getOffset() const override { return 0; }

private:
  Socket* _socket;
};
