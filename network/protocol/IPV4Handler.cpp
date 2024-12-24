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

void IPV4Handler::recv(RawNetPacket& packet) {
  printf("\n Handling IPV4 Packet");
  verifyChecksum(packet);
  auto& ipv4Header = packet.getIPV4Header();
  ipv4Header.switchNetworkOrder();
  ipv4Header.print();
  ipv4Header.switchNetworkOrder();
  device().getHandler(ipv4Header.type()).ifPresent([&packet](PacketHandler& handler) { handler.recv(packet); });
}

void IPV4Handler::verifyChecksum(RawNetPacket& packet) {
  const auto& ipv4Header = packet.getIPV4Header();
  const uint32_t calculatedChecksum = NetworkUtil::CalculateChecksum((uint16_t *)(packet.getEthernetData()),
                                                                     ipv4Header._ihl * sizeof(uint32_t),
                                                                     0);
  if (calculatedChecksum ^ (uint16_t)0xFFFF) {
    ipv4Header.print();
    throw upan::exception(XLOC, "Invalid Checksum for IP Packet ID: %d (calc. checksum: 0x%x)", ntohs(ipv4Header._identification), calculatedChecksum);
  }
}