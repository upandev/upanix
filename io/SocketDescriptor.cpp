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
    _bindAddress({ (sa_family_t)family, 0, { INADDR_ANY }, { 0 } }),
    _allowBroadcast(false), _recvTimeoutInMs(0),
    _packetQueue(1024) {
}

SocketDescriptor::~SocketDescriptor() {
  NetworkManager::Instance().unbind(*this);
}

bool SocketDescriptor::canRead() {
  upan::mutex_guard g(_ioSync);
  return !_packetQueue.empty();
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

int SocketDescriptor::read(void* buffer, int len) {
  return len;
}

int SocketDescriptor::write(const void* buffer, int len) {
  return len;
}

void SocketDescriptor::bind(const struct sockaddr& address, socklen_t len) {
  upan::mutex_guard g(_ioSync);

  if (_bindAddress.sin_port != 0) {
    throw upan::exception(XLOC, "bind failed - socket %d is already bound to port %d", id(), _bindAddress.sin_port);
  }

  validateSockAddrLen(len);

  memcpy((void*)&_bindAddress, (void*)&address, len);
  NetworkManager::Instance().bind(*this, nullptr, 0);
}

upan::shared_ptr<RawNetPacket> SocketDescriptor::recvPacket() {
  while(true) {
    {
      upan::mutex_guard g(_ioSync);
      if (!_packetQueue.empty()) {
        const auto& packet = _packetQueue.front();
        _packetQueue.pop_front();
        return packet;
      }
    }
    if (getMode() & O_RD_NONBLOCK) {
      return {};
    }
    ProcessManager::Instance().WaitOnIODescriptor(id(), IO_OP_TYPES::IO_Read, _recvTimeoutInMs);
    if (ProcessManager::Instance().GetCurrentPAS().stateInfo().getError() == ProcessStateInfo::TIMEOUT) {
      throw upan::exception(XLOC, "socket receive timed-out");
    }
  }
}

void SocketDescriptor::recvNotify(const upan::shared_ptr<RawNetPacket>& packet) {
  upan::mutex_guard g(_ioSync);

  if (_packetQueue.full()) {
    printf("\nsocket (%d) queue is full - dropping packet", id());
    _packetQueue.pop_front();
  }

  _packetQueue.push_back(packet);
}