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

#include <SocketDescriptorDataGram.h>
#include <NetworkManager.h>

SocketDescriptorDataGram::SocketDescriptorDataGram(int pid, int fd, SA_FAMILY_TYPE family, int protocol)
  : SocketDescriptor(pid, fd, family, protocol),
    _srcAddr({(sa_family_t)family, 0, {INADDR_ANY }, {0 } }) {
}

SocketDescriptorDataGram::~SocketDescriptorDataGram() {
  NetworkManager::Instance().getUDPPortPool().release(_srcAddr.sin_port);
}

void SocketDescriptorDataGram::bind(const struct sockaddr& address, socklen_t len) {
  if (_srcAddr.sin_port != 0) {
    throw upan::exception(XLOC, "setupRoute failed - socket %d is already bound to port %d", id(), _srcAddr.sin_port);
  }

  if (len != sizeof(struct sockaddr_in)) {
    throw upan::exception(XLOC, "invalid socket len: %d", len);
  }

  memcpy((void*)&_srcAddr, (void*)&address, len);
  NetworkManager::Instance().getUDPPortPool().allocate(_srcAddr.sin_port);
  NetworkManager::Instance().setupRoute(*this, &_srcAddr, sizeof(sockaddr_in));
}

ssize_t SocketDescriptorDataGram::sendTo(const uint8_t* buf, size_t n, int flags, const struct sockaddr* addr, socklen_t len) {
  validateSendToParams(buf, flags, addr, len);
  if (!addr) {
    throw upan::exception(XLOC, "sendPacket/destination address is not specified");
  }

  const auto& destAddr = reinterpret_cast<const struct sockaddr_in&>(*addr);
  if (destAddr.sin_addr.s_addr == INADDR_BROADCAST && !canBroadcast()) {
    throw upan::exception(XLOC, "sendPacket failed - broadcast socket-option is not enabled on socket: %d", id());
  }

  if (_srcAddr.sin_port == 0) {
    _srcAddr = { (sa_family_t)family(),
                 NetworkManager::Instance().getUDPPortPool().allocate(),
                 { INADDR_ANY },
                 { 0 }
    };
    NetworkManager::Instance().setupRoute(*this, &_srcAddr, sizeof(sockaddr_in));
  }

  return NetworkManager::Instance().send(buf, n, protocol(), _srcAddr, *addr);
}

ssize_t SocketDescriptorDataGram::recvFrom(uint8_t* buf, size_t n, int flags, struct sockaddr* addr, socklen_t* len) {
  validateRecvFromParams(buf, flags, addr, len);
  const auto& packet = recvPacket();
  const void* srcBuf = packet->getUDP4Data();
  const size_t dataLen = packet->getUDP4Header()._len - NetworkPacket::UDP::HEADER_SIZE;
  const auto xferLen = upan::min(n, dataLen);

  memcpy(buf, srcBuf, xferLen);

  if (addr && len) {
    reinterpret_cast<sockaddr_in&>(*addr) = { AF_INET, packet->getUDP4Header()._srcPort, packet->getIPV4Header()._header.ip_src };
    *len = sizeof(sockaddr_in);
  }

  return xferLen;
}