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
#include <NetworkManager.h>
#include <ProcessManager.h>

SocketDescriptor::SocketDescriptor(int pid, int fd, SA_FAMILY_TYPE family, int protocol)
  : IODescriptor(pid, fd, O_RDWR),
    _family(family),
    _protocol(protocol),
    _allowBroadcast(false), _sendTimeoutInMs(0), _recvTimeoutInMs(0) {
}

void SocketDescriptor::validateSockAddrLen(socklen_t len) const {
  if (len != sizeof(struct sockaddr_in) && len != sizeof(struct sockaddr_ll)) {
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
    throw upan::exception(XLOC, "sendPacket/recv buf can't be null");
  }
}

void SocketDescriptor::validateSendToParams(const void* buf, int flags, const struct sockaddr* addr, socklen_t len) {
  validateSockAddrLen(len);
  validateFlags(flags);
  validateBuf(buf);
}

void SocketDescriptor::validateRecvFromParams(const void* buf, int flags, struct sockaddr* addr, socklen_t* len) {
  if (addr && len) {
    validateSockAddrLen(*len);
  }
  validateFlags(flags);
  validateBuf(buf);
}

void SocketDescriptor::bind(const struct sockaddr& address, socklen_t len) {
  closeCheckAndThrow();
  _bind(address, len);
}

void SocketDescriptor::connect(const struct sockaddr& address, socklen_t len) {
  closeCheckAndThrow();
  _connect(address, len);
}

void SocketDescriptor::listen(int backlog) {
  closeCheckAndThrow();
  _listen(backlog);
}

int SocketDescriptor::accept(struct sockaddr* addr, socklen_t* len) {
  closeCheckAndThrow();
  return _accept(addr, len);
}

ssize_t SocketDescriptor::sendTo(const uint8_t* buf, size_t n, int flags, const struct sockaddr* addr, socklen_t len) {
  closeCheckAndThrow();
  return _sendTo(buf, n, flags, addr, len);
}

ssize_t SocketDescriptor::recvFrom(uint8_t* buf, size_t n, int flags, struct sockaddr* addr, socklen_t* len) {
  closeCheckAndThrow();
  return _recvFrom(buf, n, flags, addr, len);
}