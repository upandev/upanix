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
#include <EthernetHandler.h>
#include <NetworkDevice.h>

ARPHandler::ARPHandler(NetworkDevice& networkDevice) : PacketHandler(networkDevice) {
}

void ARPHandler::recv(const upan::shared_ptr<RawNetPacket>& packet) {
  const auto& arpHeader = packet->getARPHeader();
  if (arpHeader.isResponse()) {
    KLog::debug("Handling ARP packet");
    arpHeader.print();
  }
}

uint32_t ARPHandler::headerLen() const {
  return NetworkPacket::ARP::HEADER_SIZE + device().getEthernetHandler().headerLen();
}

RawNetPacket ARPHandler::CreatePacket(uint16_t hType, ETH_PROTO_TYPE pType, uint8_t hLen, uint8_t pLen, uint16_t opCode,
                                      const uint8_t* sha, const struct in_addr& spa, const uint8_t* tha, const struct in_addr& tpa) {

  RawNetPacket packet(NetworkPacket::Ethernet::HEADER_SIZE + NetworkPacket::ARP::HEADER_SIZE);

  auto& arpHeader = packet.getARPHeader();
  arpHeader._header.ea_hdr.ar_hrd = htons(hType);
  arpHeader._header.ea_hdr.ar_pro = htons((uint16_t) pType);
  arpHeader._header.ea_hdr.ar_hln = hLen;
  arpHeader._header.ea_hdr.ar_pln = pLen;
  arpHeader._header.ea_hdr.ar_op = htons(opCode);

  memcpy(arpHeader._header.arp_sha, sha, ETH_ALEN);
  arpHeader._header.arp_spa = spa.s_addr;

  memcpy(arpHeader._header.arp_tha, tha, ETH_ALEN);
  arpHeader._header.arp_tpa = tpa.s_addr;

  return packet;
}

void ARPHandler::SendRequestForMAC(const struct in_addr& ipAddress) {
  const struct in_addr spa = { INADDR_ANY };
  const uint8_t tha[] = { 0, 0, 0, 0, 0, 0 };

  auto packet = CreatePacket(1, ETH_PROTO_TYPE::ETH_P_IP,
                             ETH_ALEN, NetworkPacket::IPV4_ADDR_LEN, 1,
                             device().GetMACAddress().get(), spa, tha, ipAddress);
  device().getEthernetHandler().send(packet, ETH_PROTO_TYPE::ETH_P_ARP);
}

void ARPHandler::SendRARP() {
  const struct in_addr spa = { INADDR_BROADCAST };
  const uint8_t* mac = device().GetMACAddress().get();

  auto packet = CreatePacket(1, ETH_PROTO_TYPE::ETH_P_IP,
                              ETH_ALEN, NetworkPacket::IPV4_ADDR_LEN, 3,
                              mac, spa, mac, spa);
  device().getEthernetHandler().send(packet, ETH_PROTO_TYPE::ETH_P_ARP);
}
