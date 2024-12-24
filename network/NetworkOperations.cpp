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

#include <NetworkOperations.h>
#include <exception.h>
#include <ProcessManager.h>
#include <SocketDescriptorStream.h>
#include <SocketDescriptorDataGram.h>

NetworkOperations::NetworkOperations() {
}

NetworkOperations& NetworkOperations::Instance() {
  static NetworkOperations instance;
  return instance;
}

int NetworkOperations::createSocket(SA_FAMILY_TYPE family, SOCKET_TYPE socketType, IPPROTO_TYPE protocol) {
  if (family != AF_INET) {
    throw upan::exception(XLOC, "only AF_INET socket family type is supported");
  }

  switch(socketType) {
    case SOCK_STREAM:
    {
      if (protocol == IPPROTO_IP) {
        protocol = IPPROTO_TCP;
      }

      if (protocol != IPPROTO_TCP) {
        throw upan::exception(XLOC, "invalid IPPROTO_TYPE %d for stream-socket", protocol);
      }
    }
    break;

    case SOCK_DGRAM:
    {
      if (protocol == IPPROTO_IP) {
        protocol = IPPROTO_UDP;
      }

      if (protocol != IPPROTO_UDP) {
        throw upan::exception(XLOC, "invalid IPPROTO_TYPE %d for dgram-socket", protocol);
      }
    }
    break;

    default:
      throw upan::exception(XLOC, "unsupported socket type: %d", socketType);
  }

  Process& process = ProcessManager::Instance().GetCurrentPAS();
  auto& sd = process.iodTable().allocate([&](int fd) -> SocketDescriptor* {
    switch (socketType) {
      case SOCK_STREAM:
        return new SocketDescriptorStream(process.processID(), fd, protocol);
        break;
      case SOCK_DGRAM:
        return new SocketDescriptorDataGram(process.processID(), fd, protocol);
        break;
      default:
        throw upan::exception(XLOC, "unsupport socket-type: %d", socketType);
    }
  });

  return sd.id();
}

void NetworkOperations::bind(sock_t fd, const struct sockaddr& address, socklen_t len) {
  auto& descriptor = ProcessManager::Instance().GetCurrentPAS().iodTable().getRealNonDupped(fd);
  dynamic_cast<SocketDescriptor&>(descriptor).bind(address, len);
}

void NetworkOperations::setSockOpt(sock_t fd, int level, SOCKET_OPTION option, const void* optval, socklen_t len) {
  if (level != SOL_SOCKET) {
    throw upan::exception(XLOC, "unsupported socket option level: %d", level);
  }

  auto& descriptor = dynamic_cast<SocketDescriptor&>(ProcessManager::Instance().GetCurrentPAS().iodTable().getRealNonDupped(fd));
  switch(option) {
    case SO_BROADCAST:
    {
      if (len < 1) {
        throw upan::exception(XLOC, "invalid len(%d) for SO_BROADCAST option", len);
      }

      const bool allowBroadcast = *((const uint8_t*)optval) == 0 ? false : true;
      descriptor.setAllowBroadcast(allowBroadcast);
    }
    break;

    default:
      throw upan::exception(XLOC, "unsupported socket option: %d", option);
  }
}

void NetworkOperations::sendTo(int fd, const void *buf, size_t n, int flags, const struct sockaddr* addr, socklen_t len) {
  auto& descriptor = dynamic_cast<SocketDescriptor&>(ProcessManager::Instance().GetCurrentPAS().iodTable().getRealNonDupped(fd));
  descriptor.sendTo(buf, n, flags, addr, len);
}

void NetworkOperations::recvFrom(int fd, const void *buf, size_t n, int flags, struct sockaddr* addr, socklen_t* len) {
}