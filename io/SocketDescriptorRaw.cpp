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
#include <SocketDescriptorRaw.h>
#include <NetworkManager.h>
#include <UDP4Handler.h>

SocketDescriptorRaw::SocketDescriptorRaw(int pid, int fd, SA_FAMILY_TYPE family, int protocol) : SocketDescriptor(pid, fd, family, protocol) {
}

ssize_t SocketDescriptorRaw::sendTo(const uint8_t* buf, size_t n, int flags, const struct sockaddr* addr, socklen_t len) {
  validateSendToParams(buf, flags, addr, len);
  if (!addr) {
    throw upan::exception(XLOC, "sendPacket/destination address is not specified");
  }

  if (protocol() != ETH_P_ARP) {
    const auto& destAddr = reinterpret_cast<const struct sockaddr_in&>(*addr);
    if (destAddr.sin_addr.s_addr == INADDR_BROADCAST && !canBroadcast()) {
      throw upan::exception(XLOC, "sendPacket failed - broadcast socket-option is not enabled on socket: %d", id());
    }
  }

  NetworkManager::Instance().bind(*this, buf, n);
  return NetworkManager::Instance().send(buf, n, protocol(), bindAddress(), *addr);
}

ssize_t SocketDescriptorRaw::recvFrom(uint8_t* buf, size_t n, int flags, struct sockaddr* addr, socklen_t* len) {
  validateRecvFromParams(buf, flags, addr, len);
  const auto& packet = recvPacket();
  const void* srcBuf = packet->getEthernetData();
  const size_t dataLen = packet->len() - NetworkPacket::Ethernet::HEADER_SIZE;
  const auto xferLen = upan::min(n, dataLen);

  memcpy(buf, srcBuf, xferLen);

  if (addr && len) {
    if (protocol() != ETH_P_ARP) {
      reinterpret_cast<sockaddr_in&>(*addr) = {(sa_family_t) family(), 0, packet->getIPV4Header()._header.ip_src};
      *len = sizeof(sockaddr_in);
    } else if (protocol() == ETH_P_ARP) {
      auto& etherAddr = reinterpret_cast<struct sockaddr_ll&>(*addr);

      etherAddr.sll_family = (sa_family_t) family();
      etherAddr.sll_protocol = (uint16_t) protocol();
      etherAddr.sll_halen = ETH_ALEN;
      etherAddr.sll_ifindex = 1;//todo
      etherAddr.sll_pkttype = 0;//todo
      etherAddr.sll_hatype = ARPHRD_ETHER;
      memcpy(etherAddr.sll_addr, reinterpret_cast<const struct ether_arp*>(srcBuf)->arp_sha, ETH_ALEN);
      *len = sizeof(struct sockaddr_ll);
    }
  }

  return xferLen;
}