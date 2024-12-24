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
#include <UDP4Handler.h>
#include <IPV4Handler.h>
#include <RawNetPacket.h>
#include <NetworkUtil.h>

UDP4Handler::UDP4Handler(NetworkDevice& networkDevice) : PacketHandler(networkDevice) {
}

void UDP4Handler::recv(RawNetPacket& packet) {
  printf("\n Handling UDP Packet");
  verifyChecksum(packet);
  auto& udpHeader = packet.getUDP4Header();
  udpHeader.switchNetworkOrder();
  udpHeader.print();
}

void UDP4Handler::SendPacket(uint8_t* buf, uint32_t len, uint16_t srcPort, uint16_t destPort) {
}

void UDP4Handler::verifyChecksum(RawNetPacket& packet) {
  const auto& udpHeader = packet.getUDP4Header();
  const auto& ipv4Header = packet.getIPV4Header();
  if (udpHeader._checksum) {
    const NetworkPacket::UDP::IPV4PseudoHeader pseudoHeader {
      ipv4Header._srcAddr,
      ipv4Header._destAddr,
      0,
      NetworkPacket::PacketType::UDP4_TYPE,
      udpHeader._len
    };

    const uint32_t len = ntohs(udpHeader._len);
    const uint32_t partialChecksum = NetworkUtil::CalculatePartialChecksum((uint16_t*) &pseudoHeader, NetworkPacket::UDP::IPV4_PSEUDO_HEADER_SIZE, 0);
    const uint16_t calculatedChecksum = NetworkUtil::CalculateChecksum((uint16_t *) packet.getIPV4Data(),len, partialChecksum);

    const uint16_t r = calculatedChecksum ^ (uint16_t)0xFFFF;
    if (r) {
      udpHeader.print();
      throw upan::exception(XLOC, "Invalid Checksum for UDP Packet, IP Packet ID: %d (calc. checksum: 0x%x)", ntohs(ipv4Header._identification), calculatedChecksum);
    }
  }
}