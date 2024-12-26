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

#include <exception.h>

#include <RawNetPacket.h>
#include <ARPHandler.h>
#include <EthernetHandler.h>
#include <NetworkDevice.h>

EthernetHandler::EthernetHandler(NetworkDevice& networkDevice) : PacketHandler(networkDevice) {
}

void EthernetHandler::recv(RawNetPacket& packet) {
  if (packet.len() < MIN_ETHERNET_PACKET_LEN) {
    throw upan::exception(XLOC, "Invalid packet: Len %d < min ethernet-packet len %d", packet.len(), MIN_ETHERNET_PACKET_LEN);
  }

  auto& ethernetHeader = packet.getEthernetHeader();
  const MACAddress& destMAC = ethernetHeader._destinationMAC;
  if (!destMAC.isBroadcast() && destMAC != device().GetMACAddress()) {
    return;
  }

  switch (ethernetHeader.type()) {
    case NetworkPacket::Ethernet::PacketType::E_IPV4_T:
      device().getIPV4Handler().recv(packet);
      break;
    case NetworkPacket::Ethernet::PacketType::E_ARP_T:
      device().getARPHandler().recv(packet);
      break;
    default:
      throw upan::exception(XLOC, "unsupported ethernet packet type: %d", ethernetHeader.type());
  }
}

void EthernetHandler::send(RawNetPacket& packet, NetworkPacket::Ethernet::PacketType eType) {
  auto& ethernetHeader = packet.getEthernetHeader();
  memcpy(ethernetHeader._sourceMAC, device().GetMACAddress().get(), INADDR_MAC_LEN);
  ethernetHeader._type = htons(eType);

  bool isBroadcast = false;
  switch(eType) {
    case NetworkPacket::Ethernet::PacketType::E_ARP_T:
      isBroadcast = true;
      break;
    case NetworkPacket::Ethernet::PacketType::E_IPV4_T:
      isBroadcast = packet.getIPV4Header()._destAddr == INADDR_BROADCAST;
      break;
    default:
      throw upan::exception(XLOC, "unsupported ethernet packet type: %d", eType);
  }

  if (isBroadcast) {
    memcpy(ethernetHeader._destinationMAC, INADDR_MAC_BROADCAST, INADDR_MAC_LEN);
  } else {
    throw upan::exception(XLOC, "unable to determine the target/destination MAC");
  }

  device().SendPacket(packet);
}