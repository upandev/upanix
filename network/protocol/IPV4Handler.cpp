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
#include <IPV4Handler.h>
#include <NetworkDevice.h>

IPV4Handler::IPV4Handler(NetworkDevice& networkDevice) : PacketHandler(networkDevice) {
}

void IPV4Handler::recv(const RawNetPacket& packet) {
  printf("\n Handling IPV4 Packet");
  const auto& ipv4Header = packet.getIPV4Header();
  ipv4Header.print();

  device().getHandler(ipv4Header.type()).ifPresent([&packet](PacketHandler& handler) { handler.recv(packet); });
}