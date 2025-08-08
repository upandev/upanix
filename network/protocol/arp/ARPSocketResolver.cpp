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

#include <ARPSocketResolver.h>
#include <arpa/inet.h>

void ARPSocketResolver::setup(SocketDescriptorARP& socket, const struct ether_arp& header) {
  upan::mutex_guard g(_mutex);

  in_addr_t dest_ip = header.arp_tpa;

  auto it = _destIpSocketMap.find(dest_ip);
  if (it != _destIpSocketMap.end() && it->second != &socket) {
    throw upan::exception(XLOC, "ARPSocketResolver::setupRoute: duplicate destination ip: %s", inet_ntoa({ dest_ip }));
  }

  _destIpSocketMap.insert(DEST_IP_SOCKET_MAP ::value_type(dest_ip, &socket));
  _destIpSocketMapReverse[&socket].insert(dest_ip);
}

void ARPSocketResolver::release(SocketDescriptorARP& socket) {
  upan::mutex_guard g(_mutex);

  auto it = _destIpSocketMapReverse.find(&socket);
  if (it != _destIpSocketMapReverse.end()) {
    for (auto id : it->second) {
      _destIpSocketMap.erase(id);
    }
    _destIpSocketMapReverse.erase(it);
  }
}

upan::option<SocketDescriptorARP&> ARPSocketResolver::resolve(const upan::shared_ptr<RawNetPacket>& packet) {
  upan::mutex_guard g(_mutex);

  const auto arpDataLen = packet->len() - NetworkPacket::Ethernet::HEADER_SIZE;
  if (arpDataLen < (int)sizeof(struct ether_arp)) {
    throw upan::exception(XLOC, "ARPSocketResolver::resolve: packet len is too small");
  }

  const auto arp = reinterpret_cast<const NetworkPacket::ARP::Header*>(packet->getEthernetData())->toHost();
  if (arp._header.ea_hdr.ar_hrd == ARPHRD_ETHER
  && arp._header.ea_hdr.ar_op == ARPOP_REPLY
  && arp._header.ea_hdr.ar_hln == ETH_ALEN
  && arp._header.ea_hdr.ar_pro == ETH_P_IP) {
    in_addr_t dest_ip = htonl(arp._header.arp_spa);

    auto it = _destIpSocketMap.find(dest_ip);
    if (it != _destIpSocketMap.end()) {
      return upan::option<SocketDescriptorARP&>(it->second);
    }
  }

  return upan::option<SocketDescriptorARP&>::empty();
}

void ARPSocketResolver::recv(const upan::shared_ptr<RawNetPacket>& packet) {
  resolve(packet).ifPresent([&packet](SocketDescriptorARP& socket) { socket.recvNotify(packet); });
}