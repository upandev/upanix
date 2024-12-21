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
#include <EthernetRecvPacket.h>
#include <ARPHandler.h>
#include <ARPRecvPacket.h>
#include <EthernetHandler.h>

ARPHandler::ARPHandler(EthernetHandler &ethernetHandler)
  : PacketHandler<EthernetRecvPacket>(ethernetHandler.GetNetworkDevice()), _ethernetHandler(ethernetHandler) {
}

void ARPHandler::Process(const EthernetRecvPacket& packet) {
  ARPRecvPacket arpPacket(packet);
  if (arpPacket.isResponse()) {
    printf("\n Handling ARP packet");
    arpPacket.Print();
  }
}

RawNetPacket ARPHandler::CreatePacket(uint16_t hType, EtherType pType, uint8_t hLen, uint8_t pLen, uint16_t opCode,
                                      const uint8_t* sha, const struct in_addr& spa, const uint8_t* tha, const struct in_addr& tpa) {

  RawNetPacket packet(NetworkPacket::Ethernet::HEADER_SIZE + NetworkPacket::ARP::HEADER_SIZE + NetworkPacket::ARP::IPV4_SIZE);

  auto arpHeader = reinterpret_cast<NetworkPacket::ARP::Header*>(packet.buf() + NetworkPacket::Ethernet::HEADER_SIZE);
  arpHeader->_hType = htons(hType);
  arpHeader->_pType = htons((uint16_t) pType);
  arpHeader->_hLen = hLen;
  arpHeader->_pLen = pLen;
  arpHeader->_opCode = htons(opCode);

  auto _arpIPV4 = reinterpret_cast<NetworkPacket::ARP::IPV4*>(
          packet.buf() + NetworkPacket::Ethernet::HEADER_SIZE + NetworkPacket::ARP::HEADER_SIZE);

  memcpy(_arpIPV4->_senderHardwareAddress, sha, NetworkPacket::MAC_ADDR_LEN);
  _arpIPV4->_senderProtocolAddress = spa;

  memcpy(_arpIPV4->_targetHardwareAddress, tha, NetworkPacket::MAC_ADDR_LEN);
  _arpIPV4->_targetProtocolAddress = tpa;

  return packet;
}

void ARPHandler::SendRequestForMAC(const struct in_addr& ipAddress) {
  const struct in_addr spa = { INADDR_ANY };
  const uint8_t broadcast[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
  const uint8_t tha[] = { 0, 0, 0, 0, 0, 0 };

  auto packet = CreatePacket(1, EtherType::IPV4,
                             NetworkPacket::MAC_ADDR_LEN, NetworkPacket::IPV4_ADDR_LEN, 1,
                             _ethernetHandler.GetMACAddress().get(), spa, tha, ipAddress);
  _ethernetHandler.SendPacket(packet, EtherType::ARP, broadcast);
}

void ARPHandler::SendRARP() {
  const struct in_addr spa = { INADDR_BROADCAST };
  const uint8_t broadcast[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
  const uint8_t* mac = GetMACAddress().get();

  auto packet = CreatePacket(1, EtherType::IPV4,
                              NetworkPacket::MAC_ADDR_LEN, NetworkPacket::IPV4_ADDR_LEN, 3,
                              mac, spa, mac, spa);
  _ethernetHandler.SendPacket(packet, EtherType::ARP, broadcast);
}
