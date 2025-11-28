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
#include <SocketDescriptorARP.h>
#include <SocketDescriptorICMP.h>
#include <SocketDescriptorLocalDataGram.h>
#include <SocketDescriptorStreamBuffer.h>
#include <SocketDescriptorDataGramBuffer.h>

NetworkOperations::NetworkOperations() {
}

NetworkOperations& NetworkOperations::Instance() {
  static NetworkOperations instance;
  return instance;
}

int NetworkOperations::createSocket(SA_FAMILY_TYPE family, SOCKET_TYPE socketType, int protocol) {
  if (family == AF_LOCAL) {
    if (socketType != SOCK_DGRAM) {
      throw upan::exception(XLOC, "invalid socket type %d for AF_LOCAL socket", socketType);
    }

    Process& process = ProcessManager::Instance().GetCurrentPAS();
    auto sd = process.iodTable().allocate([&](int fd) -> SocketDescriptor* {
      return new SocketDescriptorLocalDataGram(process.processID(), fd, family);
    });

    return sd->id();
  } else {
    switch (socketType) {
      case SOCK_STREAM: {
        if (protocol == IPPROTO_IP) {
          protocol = IPPROTO_TCP;
        }

        if (protocol != IPPROTO_TCP) {
          throw upan::exception(XLOC, "invalid protocol %d for stream-socket", protocol);
        }
      }
        break;

      case SOCK_DGRAM: {
        if (protocol == IPPROTO_IP) {
          protocol = IPPROTO_UDP;
        }

        if (protocol != IPPROTO_UDP) {
          throw upan::exception(XLOC, "invalid protocol %d for dgram-socket", protocol);
        }
      }
        break;

      case SOCK_RAW: {
        if (protocol == IPPROTO_IP) {
          protocol = IPPROTO_ICMP;
        }

        if (protocol != IPPROTO_ICMP && protocol != ETH_P_ARP) {
          throw upan::exception(XLOC, "invalid protocol %d for raw-socket", protocol);
        }
      }
        break;

      default:
        throw upan::exception(XLOC, "unsupported socket type: %d", socketType);
    }

    Process& process = ProcessManager::Instance().GetCurrentPAS();
    auto sd = process.iodTable().allocate([&](int fd) -> SocketDescriptor* {
      switch (socketType) {
        case SOCK_STREAM:
          return new SocketDescriptorStream(process.processID(), fd, family, protocol);
        case SOCK_DGRAM:
          return new SocketDescriptorDataGram(process.processID(), fd, family, protocol);
        case SOCK_RAW:
          if (protocol == ETH_P_ARP) {
            return new SocketDescriptorARP(process.processID(), fd, family, protocol);
          } else if (protocol == IPPROTO_ICMP) {
            return new SocketDescriptorICMP(process.processID(), fd, family, protocol);
          } else {
            throw upan::exception(XLOC, "invalid protocol %d for raw-socket", protocol);
          }

        default:
          throw upan::exception(XLOC, "unsupport socket-type: %d", socketType);
      }
    });

    return sd->id();
  }
}


void NetworkOperations::createSocketPair(SA_FAMILY_TYPE family, SOCKET_TYPE socketType, int protocol, int sv[2]) {
  if (family != AF_LOCAL) {
    throw upan::exception(XLOC, "unsupported socket family: %d - for creating socket pair", family);
  }

  Process& process = ProcessManager::Instance().GetCurrentPAS();
  if (socketType == SOCK_STREAM) {
    auto s1 = process.iodTable().allocate([&](int fd) -> SocketDescriptorStreamBuffer* {
      return new SocketDescriptorStreamBuffer(process.processID(), fd, 2048, O_RDWR);
    }).cast<SocketDescriptorStreamBuffer>();

    auto s2 = process.iodTable().allocate([&](int fd) -> SocketDescriptorStreamBuffer* {
      return new SocketDescriptorStreamBuffer(process.processID(), fd, 2048, O_RDWR);
    }).cast<SocketDescriptorStreamBuffer>();

    s1->setPeer(s2);
    s2->setPeer(s1);

    sv[0] = s1->id();
    sv[1] = s2->id();
  } else if (socketType == SOCK_DGRAM) {
    auto s1 = process.iodTable().allocate([&](int fd) -> SocketDescriptorDataGramBuffer* {
      return new SocketDescriptorDataGramBuffer(process.processID(), fd, 2048, O_RDWR);
    }).cast<SocketDescriptorDataGramBuffer>();

    auto s2 = process.iodTable().allocate([&](int fd) -> SocketDescriptorDataGramBuffer* {
      return new SocketDescriptorDataGramBuffer(process.processID(), fd, 2048, O_RDWR);
    }).cast<SocketDescriptorDataGramBuffer>();

    s1->setPeer(s2);
    s2->setPeer(s1);

    sv[0] = s1->id();
    sv[1] = s2->id();
  } else {
    throw upan::exception(XLOC, "unsupported socket type: %d - for creating socket pair", socketType);
  }
}

void NetworkOperations::bind(sock_t fd, const struct sockaddr& address, socklen_t len) {
  auto descriptor = ProcessManager::Instance().GetCurrentPAS().iodTable().getRealNonDupped(fd);
  dynamic_cast<SocketDescriptor&>(*descriptor).bind(address, len);
}

void NetworkOperations::setSockOpt(sock_t fd, int level, SOCKET_OPTION option, const void* optval, socklen_t len) {
  if (level != SOL_SOCKET) {
    throw upan::exception(XLOC, "unsupported socket option level: %d", level);
  }

  auto& descriptor = dynamic_cast<SocketDescriptor&>(*ProcessManager::Instance().GetCurrentPAS().iodTable().getRealNonDupped(fd));
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

    case SO_SNDTIMEO:
    {
      if (len < sizeof(struct timeval)) {
        throw upan::exception(XLOC, "invalid len(%d) for SO_SNDTIMEO option", len);
      }

      const auto timeout = reinterpret_cast<const struct timeval*>(optval);
      descriptor.setSendTimeout(timeout);
    }
    break;

    case SO_RCVTIMEO:
    {
      if (len < sizeof(struct timeval)) {
        throw upan::exception(XLOC, "invalid len(%d) for SO_RECVTIMEO option", len);
      }

      const auto timeout = reinterpret_cast<const struct timeval*>(optval);
      descriptor.setRecvTimeout(timeout);
    }
    break;

    default:
      throw upan::exception(XLOC, "unsupported socket option: %d", option);
  }
}

void NetworkOperations::getSockOpt(sock_t fd, int level, SOCKET_OPTION option, void* optval, socklen_t* len) {
  if (level != SOL_SOCKET) {
    throw upan::exception(XLOC, "unsupported socket option level: %d", level);
  }

  if (optval == nullptr || len == nullptr) {
    throw upan::exception(XLOC, "invalid optval or len");
  }

  auto& descriptor = dynamic_cast<SocketDescriptor&>(*ProcessManager::Instance().GetCurrentPAS().iodTable().getRealNonDupped(fd));
  switch(option) {
    case SO_BROADCAST:
    {
      *(uint8_t*)optval = descriptor.canBroadcast() ? 1 : 0;
      *len = 1;
    }
      break;

    case SO_SNDTIMEO:
    {
      const auto timeoutMs = descriptor.getSendTimeout();
      auto timeout = reinterpret_cast<struct timeval*>(optval);
      timeout->tv_sec = timeoutMs / 1000;
      timeout->tv_usec = (timeoutMs % 1000) * 1000;
      *len = sizeof(struct timeval);
    }
    break;

    case SO_RCVTIMEO:
    {
      const auto timeoutMs = descriptor.getRecvTimeout();
      auto timeout = reinterpret_cast<struct timeval*>(optval);
      timeout->tv_sec = timeoutMs / 1000;
      timeout->tv_usec = (timeoutMs % 1000) * 1000;
      *len = sizeof(struct timeval);
    }
    break;

    case SO_ERROR:
    {
      *(int*)optval = descriptor.getLastError();
      *len = sizeof(int);
    }
    break;

    default:
      throw upan::exception(XLOC, "unsupported socket option: %d", option);
  }
}

ssize_t NetworkOperations::sendTo(int fd, const void* buf, size_t n, int flags, const struct sockaddr* addr, socklen_t len) {
  auto& descriptor = dynamic_cast<SocketDescriptor&>(*ProcessManager::Instance().GetCurrentPAS().iodTable().getRealNonDupped(fd));
  return descriptor.sendTo(buf, n, flags, addr, len);
}

ssize_t NetworkOperations::recvFrom(int fd, void* buf, size_t n, int flags, struct sockaddr* addr, socklen_t* len) {
  auto& descriptor = dynamic_cast<SocketDescriptor&>(*ProcessManager::Instance().GetCurrentPAS().iodTable().getRealNonDupped(fd));
  return descriptor.recvFrom(buf, n, flags, addr, len);
}

void NetworkOperations::connect(int fd, const struct sockaddr& addr, socklen_t len) {
  auto& descriptor = dynamic_cast<SocketDescriptor&>(*ProcessManager::Instance().GetCurrentPAS().iodTable().getRealNonDupped(fd));
  descriptor.connect(addr, len);
}

void NetworkOperations::listen(int fd, int backlog) {
  auto& descriptor = dynamic_cast<SocketDescriptor&>(*ProcessManager::Instance().GetCurrentPAS().iodTable().getRealNonDupped(fd));
  descriptor.listen(backlog);
}

int NetworkOperations::accept(int fd, struct sockaddr* addr, socklen_t* len) {
  auto& descriptor = dynamic_cast<SocketDescriptor&>(*ProcessManager::Instance().GetCurrentPAS().iodTable().getRealNonDupped(fd));
  return descriptor.accept(addr, len);
}

void NetworkOperations::shutdown(int fd, SOCKET_SHUTDOWN_TYPE type) {
  auto& descriptor = dynamic_cast<SocketDescriptor&>(*ProcessManager::Instance().GetCurrentPAS().iodTable().getRealNonDupped(fd));
  return descriptor.shutdown(type);
}