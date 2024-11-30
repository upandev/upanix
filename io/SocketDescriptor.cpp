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

#include <exception.h>
#include <SocketDescriptor.h>
#include <fs.h>
#include <StreamSocket.h>
#include <DataGramSocket.h>

SocketDescriptor::SocketDescriptor(int pid, int fd, SOCKET_TYPE type) : IODescriptor(pid, fd, O_RDWR), _socket(nullptr) {
  switch (type) {
    case SOCK_STREAM:
      _socket = new StreamSocket();
      break;
    case SOCK_DGRAM:
      _socket = new DataGramSocket();
      break;
    default:
      throw upan::exception(XLOC, "unsupport socket-type: %d", type);
  }
}

SocketDescriptor::~SocketDescriptor() {
  delete _socket;
}

int SocketDescriptor::read(void* buffer, int len) {
  return len;
}

int SocketDescriptor::write(const void* buffer, int len) {
  return len;
}