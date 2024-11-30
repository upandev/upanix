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
#include <IPV4RecvPacket.h>
#include <UDP4RecvPacket.h>

UDP4Handler::UDP4Handler(IPV4Handler &ipv4Handler)
  : PacketHandler<IPV4RecvPacket>(ipv4Handler.GetNetworkDevice()), _ipv4Handler(ipv4Handler) {
}

void UDP4Handler::Process(const IPV4RecvPacket& packet) {
  printf("\n Handling UDP Packet");
  UDP4RecvPacket udpPacket(packet);
  udpPacket.Print();
}

void UDP4Handler::SendPacket(uint8_t* buf, uint32_t len, uint16_t srcPort, uint16_t destPort) {
}

//NetProtocolType UDP4Handler::Type(const UDP4RecvPacket& packet) const {
//  const auto& header = packet.Header();
//  if ((header._srcPort == 67 && header._destPort == 68) || (header._srcPort == 68 && header._destPort == 67)) {
//    return NetProtocolType::DHCP;
//  }
//  return NetProtocolType::Unknown;
//}