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
#include <IPV4Handler.h>
#include <NetworkDevice.h>

IPV4Handler::IPV4Handler(NetworkDevice& networkDevice) : PacketHandler(networkDevice) {
}

void IPV4Handler::recv(const upan::shared_ptr<RawNetPacket>& packet) {
  printf("\n Handling IPV4 Packet");
  const auto& ipv4Header = packet->getIPV4Header();
  verifyChecksum(ipv4Header);
  ipv4Header.toHost().print();
  switch(ipv4Header.type()) {
    case IPPROTO_UDP:
      device().getUDP4Handler().recv(packet);
      break;
    default:
      throw upan::exception(XLOC, "unsupported IPV4 packet type: %d", ipv4Header.type());
  }
}

uint32_t IPV4Handler::headerLen() const {
  //TODO: if IPV4 header has header-options then that must be factored here
  return NetworkPacket::IPV4::HEADER_SIZE + device().getEthernetHandler().headerLen();
}

void IPV4Handler::initHeaderLen(RawNetPacket& packet) {
  //TODO: if IPV4 header has header-options then that must be factored here
  packet.getIPV4Header()._ihl = NetworkPacket::IPV4::HEADER_SIZE / sizeof(uint32_t);
}

void IPV4Handler::send(RawNetPacket& packet, IPPROTO_TYPE protocol, const struct sockaddr_in& srcAddr, const struct sockaddr_in& destAddr) {
  auto& ipv4Header = packet.getIPV4Header();
  //This should be already done if we are sending a packet from a higher network layer like UDP
  initHeaderLen(packet);
  ipv4Header._version = 4;
  ipv4Header._tos = 0;
  ipv4Header._totalLen = htons(ntohs(packet.getUDP4Header()._len) + (ipv4Header._ihl * sizeof(uint32_t)));
  ipv4Header._identification = htons(rand());
  ipv4Header._flags = 0;
  ipv4Header._fragmentOffset = 0;
  ipv4Header._ttl = 255;
  ipv4Header._protocol = protocol;
  ipv4Header._checksum = 0;
  ipv4Header._srcAddr = srcAddr.sin_addr.s_addr;
  ipv4Header._destAddr = destAddr.sin_addr.s_addr;

  ipv4Header._checksum = calcChecksum(ipv4Header);

  device().getEthernetHandler().send(packet, NetworkPacket::EthernetPacketType::E_IPV4_T);
}

uint16_t IPV4Handler::calcChecksum(const NetworkPacket::IPV4::Header& ipv4Header) {
  return NetworkUtil::CalculateChecksum((uint16_t *)&ipv4Header,ipv4Header._ihl * sizeof(uint32_t), 0);
}

void IPV4Handler::verifyChecksum(const NetworkPacket::IPV4::Header& ipv4Header) {
  const auto calculatedChecksum = calcChecksum(ipv4Header);
  if (calculatedChecksum ^ (uint16_t)0xFFFF) {
    ipv4Header.print();
    throw upan::exception(XLOC, "Invalid Checksum for IP Packet ID: %d (calc. checksum: 0x%x)", ntohs(ipv4Header._identification), calculatedChecksum);
  }
}