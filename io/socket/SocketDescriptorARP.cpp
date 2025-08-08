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
#include <SocketDescriptorARP.h>
#include <NetworkManager.h>
#include <NetworkDevice.h>
#include <RealNetworkDevice.h>

SocketDescriptorARP::SocketDescriptorARP(int pid, int fd, SA_FAMILY_TYPE family, int protocol)
  : SocketDescriptorPacket(pid, fd, family, protocol) {
}

void SocketDescriptorARP::_close() {
  NetworkManager::Instance().getARPSocketResolver().release(*this);
}

void SocketDescriptorARP::_bind(const struct sockaddr& address, socklen_t len) {
  validateSockAddrLen(len);
  memcpy((void*)&srcAddr(), (void*)&address, len);
}

void SocketDescriptorARP::_connect(const struct sockaddr& address, socklen_t len) {
  _destAddr = address;
}

ssize_t SocketDescriptorARP::_sendTo(const uint8_t* buf, size_t n, int flags, const struct sockaddr* addr, socklen_t len) {
  validateSendToParams(buf, flags, addr, len);
  if (!addr) {
    throw upan::exception(XLOC, "sendPacket/destination address is not specified");
  }

  NetworkManager::Instance().getARPSocketResolver().setup(*this, *reinterpret_cast<const struct ether_arp*>(buf));
  const auto& device = NetworkManager::Instance().getDefaultRealDevice();
  device.value().getARPHandler().send(buf, n, srcAddr(), *addr);

  return n;
}

ssize_t SocketDescriptorARP::_recvFrom(uint8_t* buf, size_t n, int flags, struct sockaddr* addr, socklen_t* len) {
  validateRecvFromParams(buf, flags, addr, len);
  const auto& packet = recvPacket();
  const void* srcBuf = packet->getEthernetData();
  const size_t dataLen = packet->len() - NetworkPacket::Ethernet::HEADER_SIZE;
  const auto xferLen = upan::min(n, dataLen);

  memcpy(buf, srcBuf, xferLen);

  if (addr && len) {
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

  return xferLen;
}

bool SocketDescriptorARP::filterPacket(const upan::shared_ptr<RawNetPacket>& rawPacket) {
  return true;
}