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
#include <net/ip.h>
#include <RawNetPacket.h>
#include <ARPHandler.h>
#include <EthernetHandler.h>
#include <NetworkManager.h>
#include <RealNetworkDevice.h>

EthernetHandler::EthernetHandler(RealNetworkDevice& networkDevice) : PacketHandler(networkDevice) {
}

void EthernetHandler::recv(const upan::shared_ptr<RawNetPacket>& packet) {
  if (packet->len() < (int)MIN_ETHERNET_PACKET_LEN) {
    throw upan::exception(XLOC, "Invalid packet: Len %d < min ethernet-packet len %d", packet->len(), MIN_ETHERNET_PACKET_LEN);
  }

  const auto& ethernetHeader = packet->getEthernetHeader();
  const MACAddress& destMAC = ethernetHeader._header.h_dest;
  if (!destMAC.isBroadcast() && destMAC != device().getMACAddress()) {
    KLog::trace("Ignoring ARP packet : DestMac: %s != %s", destMAC.str().c_str(), device().getMACAddress().str().c_str());
    return;
  }

  switch (ethernetHeader.type()) {
    case ETH_PROTO_TYPE::ETH_P_IP:
      device().getIPV4Handler().recv(packet);
      break;
    case ETH_PROTO_TYPE::ETH_P_ARP:
      device().getARPHandler().recv(packet);
      break;
    default:
      throw upan::exception(XLOC, "unsupported ethernet packet type: %d", ethernetHeader.type());
  }
}

uint32_t EthernetHandler::headerLen() const {
  return NetworkPacket::Ethernet::HEADER_SIZE;
}

void EthernetHandler::send(RawNetPacket& packet, ETH_PROTO_TYPE eType) {
  auto& ethernetHeader = packet.getEthernetHeader();
  memcpy(ethernetHeader._header.h_source, device().getMACAddress().get(), ETH_ALEN);
  ethernetHeader._header.h_proto = htons(eType);

  bool isBroadcast = false;
  switch(eType) {
    case ETH_PROTO_TYPE::ETH_P_ARP:
      isBroadcast = true;
      break;
    case ETH_PROTO_TYPE::ETH_P_IP:
      isBroadcast = packet.getIPV4Header()._header.ip_dst.s_addr == INADDR_BROADCAST;
      break;
    default:
      throw upan::exception(XLOC, "unsupported ethernet packet type: %d", eType);
  }

  if (isBroadcast) {
    memcpy(ethernetHeader._header.h_dest, INADDR_MAC_BROADCAST, ETH_ALEN);
  } else {
    in_addr_t dest_ip = packet.getIPV4Header()._header.ip_dst.s_addr;
    if (device().isSameSubnet(dest_ip)) {
      //TODO: instead of failing, lookup - try ARP
      const auto& mac = NetworkManager::Instance().lookupMAC(dest_ip).valueOrThrow(XLOC,
                                                                            upan::error("unable to determine the dest MAC for ip: %s",
                                                                                        inet_ntoa( { dest_ip })).Msg());
      memcpy(ethernetHeader._header.h_dest, mac.get(), ETH_ALEN);
    } else {
      memcpy(ethernetHeader._header.h_dest, device().getGatewayMACAddress().get(), ETH_ALEN);
    }
  }

  device().sendPacket(packet);
}