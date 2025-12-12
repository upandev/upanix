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
#include <SocketDescriptorPacket.h>
#include <fs.h>
#include <NetworkManager.h>
#include <ProcessManager.h>

SocketDescriptorPacket::SocketDescriptorPacket(int pid, int fd, SA_FAMILY_TYPE family, int protocol)
  : SocketDescriptor(pid, fd, family, protocol),
    _srcAddr({(sa_family_t)family, 0, {INADDR_ANY }, {0 } }),
    _packetQueue(1024) {
}

int SocketDescriptorPacket::_read(void* buffer, int len) {
  throw upan::exception(XLOC, "read not supported for packet sockets");
}

bool SocketDescriptorPacket::_canRead_1() {
  upan::mutex_guard g(_ioSync);
  return !_packetQueue.empty();
}

int SocketDescriptorPacket::_write(const void* buffer, int len) {
  throw upan::exception(XLOC, "write not supported for packet sockets");
}

upan::shared_ptr<RawNetPacket> SocketDescriptorPacket::recvPacket() {
  while(true) {
    {
      upan::mutex_guard g(_ioSync);
      if (!_packetQueue.empty()) {
        const auto& packet = _packetQueue.front();
        _packetQueue.pop_front();
        if (filterPacket(packet)) {
          return packet;
        }
        continue;
      }
    }
    if (getMode() & O_NONBLOCK) {
      return {};
    }
    ProcessManager::Instance().WaitOnIODescriptor(id(), IODescriptorTable::IO_OP_TYPES::IO_Read, getRecvTimeout());
    const auto r = ProcessManager::Instance().GetCurrentPAS().stateInfo().getError();
    if (r == ProcessStateInfo::TIMEOUT) {
      throw upan::exception(XLOC, "socket receive timed-out");
    } else if (r == ProcessStateInfo::INTERRUPTED) {
      throw upan::exception(XLOC, "socket receive interrupted");
    }
  }
}

void SocketDescriptorPacket::_shutdown(SOCKET_SHUTDOWN_TYPE type) {
  upan::mutex_guard g(_ioSync);
  if (type == SHUT_RDWR || type == SHUT_RD) {
    _packetQueue.clear();
  }
}

void SocketDescriptorPacket::recvNotify(const upan::shared_ptr<RawNetPacket>& packet) {
  upan::mutex_guard g(_ioSync);

  if (shutdownStatus() == SHUT_RDWR || shutdownStatus() == SHUT_RD) {
    //ignore the packets
    return;
  }

  if (_packetQueue.full()) {
    printf("\nsocket (%d) queue is full - dropping packet", id());
    _packetQueue.pop_front();
  }

  _packetQueue.push_back(packet);
}

const struct sockaddr_in& SocketDescriptorPacket::extractDestAddr(const struct sockaddr& addr, socklen_t len, bool portRequired) {
  if (len != sizeof(struct sockaddr_in)) {
    throw upan::exception(XLOC, "invalid socket len: %d", len);
  }

  const auto& destAddr = reinterpret_cast<const struct sockaddr_in&>(addr);
  if (destAddr.sin_addr.s_addr == INADDR_BROADCAST && !canBroadcast()) {
    throw upan::exception(XLOC, "connect failed - broadcast socket-option is not enabled on socket: %d", id());
  }

  if (destAddr.sin_addr.s_addr == INADDR_ANY) {
    throw upan::exception(XLOC, "connect failed - socket %d doesn't have a destination address set", id());
  }

  if (portRequired && destAddr.sin_port == 0) {
    throw upan::exception(XLOC, "connect failed - socket %d doesn't have a port set", id());
  }

  return destAddr;
}

const struct sockaddr_in& SocketDescriptorPacket::resolveDestAddr(const struct sockaddr_in& connectedAddr,
                                                                  const struct sockaddr* sendAddr, socklen_t len, bool portRequired) {
  if (!sendAddr) {
    if (connectedAddr.sin_port == 0) {
      throw upan::exception(XLOC, "sendPacket failed - socket %d doesn't have a destination address set", id());
    }
    return connectedAddr;
  } else {
    return extractDestAddr(*sendAddr, len, portRequired);
  }
}