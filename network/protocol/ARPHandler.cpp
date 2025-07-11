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
#include <stdio.h>
#include <ARPHandler.h>
#include <NetworkManager.h>
#include <NetworkDevice.h>

ARPHandler::ARPHandler(NetworkDevice& networkDevice) : PacketHandler(networkDevice) {
}

uint32_t ARPHandler::headerLen() const {
  return device().getEthernetHandler().headerLen();
}

void ARPHandler::recv(const upan::shared_ptr<RawNetPacket>& packet) {
  const auto& arpHeader = packet->getARPHeader();
  if (arpHeader.isResponse()) {
    KLog::debug("Handling ARP packet reply");
    arpHeader.toHost().print();
    NetworkManager::Instance().recv(packet, ETH_PROTO_TYPE::ETH_P_ARP);
  } else if (arpHeader.isRequest()) {
    if (device().isConnected()) {
      if (arpHeader._header.arp_tpa == device().GetIPAddress()) {
        KLog::debug("Handling ARP packet request");
        struct ether_arp arp_reply {};
        arp_reply.ea_hdr.ar_op = htons(ARPOP_REPLY);

        memcpy(arp_reply.arp_sha, device().GetMACAddress().get(), ETH_ALEN);
        arp_reply.arp_spa = device().GetIPAddress();

        memcpy(arp_reply.arp_tha, arpHeader._header.arp_sha, ETH_ALEN);
        arp_reply.arp_tpa = arpHeader._header.arp_spa;
        send((uint8_t*)&arp_reply, sizeof(arp_reply), sockaddr_in{}, sockaddr {});
      }
    }
  } else {
    //KLog::debug("Ignoring ARP packet : %s", inet_ntoa( { arpHeader._header.arp_spa }));
  }
}

void ARPHandler::send(const uint8_t* buf, uint32_t len, const struct sockaddr_in& srcAddr, const struct sockaddr& destAddr) {
  RawNetPacket packet(len + headerLen());

  memcpy(packet.getEthernetData(), buf, len);

  auto& arpHeader = packet.getARPHeader();
  arpHeader._header.ea_hdr.ar_hrd = htons(ARPHRD_ETHER);
  arpHeader._header.ea_hdr.ar_pro = htons(ETH_PROTO_TYPE::ETH_P_IP);
  arpHeader._header.ea_hdr.ar_hln = ETH_ALEN;
  arpHeader._header.ea_hdr.ar_pln = NetworkPacket::IPV4_ADDR_LEN;

  device().getEthernetHandler().send(packet, ETH_PROTO_TYPE::ETH_P_ARP);
}