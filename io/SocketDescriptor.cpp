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
#include "SocketDescriptorStream.h"
#include "SocketDescriptorDataGram.h"
#include "network/NetworkManager.h"

SocketDescriptor::SocketDescriptor(int pid, int fd, IPPROTO_TYPE protocol)
  : IODescriptor(pid, fd, O_RDWR), _protocol(protocol) {
}

SocketDescriptor::~SocketDescriptor() {
  if (isBound()) {
    NetworkManager::Instance().releasePort(ntohs(_bindAddress.sin_port));
  }
}

void SocketDescriptor::validateSockAddrLen(socklen_t len) const {
  if (len != sizeof(struct sockaddr_in)) {
    throw upan::exception(XLOC, "invalid socket len: %d", len);
  }
}

void SocketDescriptor::validateFlags(int flags) const {
  if (flags != 0) {
    throw upan::exception(XLOC, "socket flags are not supported yet");
  }
}

void SocketDescriptor::validateBuf(const void* buf) const {
  if (!buf) {
    throw upan::exception(XLOC, "send/recv buf can't be null");
  }
}

void SocketDescriptor::validateSendToParams(const void* buf, int flags, const struct sockaddr* addr, socklen_t len) {
  validateSockAddrLen(len);
  validateFlags(flags);
  validateBuf(buf);
}

void SocketDescriptor::ensureBind() {
  if (!isBound()) {
    _bindAddress.sin_family = AF_INET;
    _bindAddress.sin_addr.s_addr = INADDR_ANY;
    _bindAddress.sin_port = htons(NetworkManager::Instance().allocatePort());
  }
}

int SocketDescriptor::read(void* buffer, int len) {
  return len;
}

int SocketDescriptor::write(const void* buffer, int len) {
  return len;
}

void SocketDescriptor::bind(const struct sockaddr& address, socklen_t len) {
  upan::mutex_guard g(_mutex);

  if (isBound()) {
    throw upan::exception(XLOC, "bind failed - socket %d is already bound to port %d", id(), _bindAddress.sin_port);
  }

  validateSockAddrLen(len);

  memcpy((void*)&_bindAddress, (void*)&address, len);
  NetworkManager::Instance().bind(_bindAddress.sin_addr.s_addr, _bindAddress.sin_port, *this);
}