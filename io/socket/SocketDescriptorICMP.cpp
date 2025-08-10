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
#include <SocketDescriptorICMP.h>
#include <NetworkManager.h>
#include <NetworkDevice.h>
#include <RealNetworkDevice.h>

SocketDescriptorICMP::SocketDescriptorICMP(int pid, int fd, SA_FAMILY_TYPE family, int protocol)
  : SocketDescriptorPacket(pid, fd, family, protocol),
    _destAddr({(sa_family_t)family, 0, { INADDR_ANY }, { 0 } }) {
}

void SocketDescriptorICMP::_close() {
  NetworkManager::Instance().getICMPSocketResolver().release(*this);
}

void SocketDescriptorICMP::_bind(const struct sockaddr& address, socklen_t len) {
  validateSockAddrLen(len);
  memcpy((void*)&srcAddr(), (void*)&address, len);
}

void SocketDescriptorICMP::_connect(const struct sockaddr& address, socklen_t len) {
  _destAddr = extractDestAddr(address, len, false);
}

ssize_t SocketDescriptorICMP::_sendTo(const void* buf, size_t n, int flags, const struct sockaddr* addr, socklen_t len) {
  validateFlags(flags);
  validateBuf(buf);

  const struct sockaddr_in& destAddr = resolveDestAddr(_destAddr, addr, len, false);

  if (destAddr.sin_addr.s_addr == INADDR_BROADCAST && !canBroadcast()) {
    throw upan::exception(XLOC, "sendPacket failed - broadcast socket-option is not enabled on socket: %d", id());
  }
  NetworkManager::Instance().getICMPSocketResolver().setup(*this, *reinterpret_cast<const struct icmp*>(buf));
  auto& device = NetworkManager::Instance().getDevice(destAddr, true);
  device.getICMPHandler().send(buf, n, srcAddr(), destAddr);

  return n;
}

ssize_t SocketDescriptorICMP::_recvFrom(void* buf, size_t n, int flags, struct sockaddr* addr, socklen_t* len) {
  validateRecvFromParams(buf, flags, addr, len);
  const auto& packet = recvPacket();
  const void* srcBuf = packet->getEthernetData();
  const size_t dataLen = packet->len() - NetworkPacket::Ethernet::HEADER_SIZE;
  const auto xferLen = upan::min(n, dataLen);

  memcpy(buf, srcBuf, xferLen);

  if (addr && len) {
    reinterpret_cast<sockaddr_in&>(*addr) = {(sa_family_t) family(), 0, packet->getIPV4Header()._header.ip_src};
    *len = sizeof(sockaddr_in);
  }

  return xferLen;
}

bool SocketDescriptorICMP::filterPacket(const upan::shared_ptr<RawNetPacket>& rawPacket) {
  const auto& ipv4Header = rawPacket->getIPV4Header();

  if (_destAddr.sin_addr.s_addr != INADDR_ANY) {
    if (ipv4Header._header.ip_src.s_addr != _destAddr.sin_addr.s_addr) {
      return false;
    }
  }

  return true;
}