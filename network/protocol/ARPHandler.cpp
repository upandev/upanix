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
    printf("\n Handling ARP packet");
    arpHeader.print();
    if (arpHeader.type() == NetworkPacket::EthernetPacketType::E_IPV4_T) {
      const auto& arpIPV4Header = packet->getARPIPV4Header();
      arpIPV4Header.print();
    }
  }
}

RawNetPacket ARPHandler::CreatePacket(uint16_t hType, NetworkPacket::EthernetPacketType pType, uint8_t hLen, uint8_t pLen, uint16_t opCode,
                                      const uint8_t* sha, const struct in_addr& spa, const uint8_t* tha, const struct in_addr& tpa) {

  RawNetPacket packet(NetworkPacket::Ethernet::HEADER_SIZE + NetworkPacket::ARP::HEADER_SIZE + NetworkPacket::ARP::IPV4_SIZE);

  auto& arpHeader = packet.getARPHeader();
  arpHeader._hType = htons(hType);
  arpHeader._pType = htons((uint16_t) pType);
  arpHeader._hLen = hLen;
  arpHeader._pLen = pLen;
  arpHeader._opCode = htons(opCode);

  auto& arpIPV4Header = packet.getARPIPV4Header();

  memcpy(arpIPV4Header._senderHardwareAddress, sha, INADDR_MAC_LEN);
  arpIPV4Header._senderProtocolAddress = spa.s_addr;

  memcpy(arpIPV4Header._targetHardwareAddress, tha, INADDR_MAC_LEN);
  arpIPV4Header._targetProtocolAddress = tpa.s_addr;

  return packet;
}

void ARPHandler::SendRequestForMAC(const struct in_addr& ipAddress) {
  const struct in_addr spa = { INADDR_ANY };
  const uint8_t tha[] = { 0, 0, 0, 0, 0, 0 };

  auto packet = CreatePacket(1, NetworkPacket::EthernetPacketType::E_IPV4_T,
                             INADDR_MAC_LEN, NetworkPacket::IPV4_ADDR_LEN, 1,
                             device().GetMACAddress().get(), spa, tha, ipAddress);
  device().getEthernetHandler().send(packet, NetworkPacket::EthernetPacketType::E_ARP_T);
}

void ARPHandler::SendRARP() {
  const struct in_addr spa = { INADDR_BROADCAST };
  const uint8_t* mac = device().GetMACAddress().get();

  auto packet = CreatePacket(1, NetworkPacket::EthernetPacketType::E_IPV4_T,
                              INADDR_MAC_LEN, NetworkPacket::IPV4_ADDR_LEN, 3,
                              mac, spa, mac, spa);
  device().getEthernetHandler().send(packet, NetworkPacket::EthernetPacketType::E_ARP_T);
}
