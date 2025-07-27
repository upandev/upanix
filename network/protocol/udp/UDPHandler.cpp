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
#include <UDPHandler.h>
#include <IPV4Handler.h>
#include <RawNetPacket.h>
#include <NetworkUtil.h>
#include <NetworkDevice.h>
#include <NetworkManager.h>
#include <Global.h>

UDPHandler::UDPHandler(NetworkDevice& networkDevice) : PacketHandler(networkDevice) {
}

void UDPHandler::recv(const upan::shared_ptr<RawNetPacket>& packet) {
  KLog::debug("Handling UDP Packet");
  verifyChecksum(*packet);
  const auto& udpHeader = packet->getUDPHeader();
  udpHeader.toHost().print();

  NetworkManager::Instance().getUDPSocketResolver().recv(packet);
}

uint32_t UDPHandler::headerLen() const {
  return NetworkPacket::UDP::HEADER_SIZE + device().getIPV4Handler().headerLen();
}

void UDPHandler::send(const uint8_t* buf, uint32_t len, const struct sockaddr_in& srcAddr, const struct sockaddr_in& destAddr) {
  RawNetPacket packet(len + headerLen());
  //IPV4 header can potentially have varying length because of header-options
  //therefore, we need to initialize the IPV4 header length at the very beginning before constructing the packet bottom up
  device().getIPV4Handler().initHeaderLen(packet);

  memcpy(packet.getUDPData(), buf, len);

  auto& udpHeader = packet.getUDPHeader();
  udpHeader._srcPort = srcAddr.sin_port;
  udpHeader._destPort = destAddr.sin_port;
  udpHeader._len = htons(len + NetworkPacket::UDP::HEADER_SIZE);
  udpHeader._checksum = 0;
  calcChecksum(packet, (device().isConnected() ? device().getIPAddress() : INADDR_ANY), destAddr.sin_addr.s_addr);

  device().getIPV4Handler().send(packet, IPPROTO_UDP, srcAddr, destAddr);
}

void UDPHandler::calcChecksum(RawNetPacket& packet, in_addr_t srcAddr, in_addr_t destAddr) {
  auto& udpHeader = packet.getUDPHeader();
  const NetworkPacket::IPV4::IPV4PseudoHeader pseudoHeader {
          srcAddr,
          destAddr,
          0,
          IPPROTO_UDP,
          udpHeader._len
  };

  const uint32_t len = ntohs(udpHeader._len);
  const uint32_t partialChecksum = NetworkUtil::CalculatePartialChecksum((uint16_t*) &pseudoHeader, NetworkPacket::IPV4::IPV4_PSEUDO_HEADER_SIZE, 0);
  udpHeader._checksum = NetworkUtil::CalculateChecksum((uint16_t *) packet.getIPV4Data(),len, partialChecksum);
}

void UDPHandler::verifyChecksum(const RawNetPacket& packet) {
  const auto& udpHeader = packet.getUDPHeader();
  const auto& ipv4Header = packet.getIPV4Header();
  if (udpHeader._checksum) {
    const NetworkPacket::IPV4::IPV4PseudoHeader pseudoHeader {
      ipv4Header._header.ip_src.s_addr,
      ipv4Header._header.ip_dst.s_addr,
      0,
      IPPROTO_UDP,
      udpHeader._len
    };

    const uint32_t len = ntohs(udpHeader._len);
    const uint32_t partialChecksum = NetworkUtil::CalculatePartialChecksum((uint16_t*) &pseudoHeader, NetworkPacket::IPV4::IPV4_PSEUDO_HEADER_SIZE, 0);
    const uint16_t calculatedChecksum = NetworkUtil::CalculateChecksum((uint16_t *) packet.getIPV4Data(),len, partialChecksum);

    if (calculatedChecksum != 0) {
      udpHeader.print();
      throw upan::exception(XLOC, "Invalid Checksum for UDP Packet, IP Packet ID: %d (calc. checksum: 0x%x)", ntohs(ipv4Header._header.ip_id), calculatedChecksum);
    }
  }
}