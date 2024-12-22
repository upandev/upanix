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
#include <SocketDescriptor.h>

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
  auto& sd = process.iodTable().allocate([&](int fd) {
    return new SocketDescriptor(process.processID(), fd, socketType, protocol);
  });

  return sd.id();
}

void NetworkOperations::bind(sock_t fd, const struct sockaddr& address, socklen_t len) {
  auto& descriptor = ProcessManager::Instance().GetCurrentPAS().iodTable().getRealNonDupped(fd);
  dynamic_cast<SocketDescriptor&>(descriptor).bind(address, len);
}