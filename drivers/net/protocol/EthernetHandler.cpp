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
#include <IPV4Handler.h>
#include <EthernetHandler.h>
#include <EthernetRecvPacket.h>
#include <ARPSendPacket.h>
#include <NetworkDevice.h>
#include <NetworkUtil.h>

EthernetHandler::EthernetHandler(NetworkDevice& networkDevice) : PacketHandler<RawNetPacket>(networkDevice) {
  _etherPacketHandlers.insert(EtherPacketHandlerMap::value_type(ARPHandler::HandlerType(), new ARPHandler(*this)));
  _etherPacketHandlers.insert(EtherPacketHandlerMap::value_type(IPV4Handler::HandlerType(), new IPV4Handler(*this)));
}

void EthernetHandler::Process(const RawNetPacket& packet) {
  if (packet.len() < MIN_ETHERNET_PACKET_LEN) {
    throw upan::exception(XLOC, "Invalid packet: Len %d < min ethernet-packet len %d", packet.len(), MIN_ETHERNET_PACKET_LEN);
  }
  const EthernetRecvPacket ethernetPacket(packet);

  const MACAddress& destMAC = ethernetPacket.DestinationMAC();
  if (!destMAC.isBroadcast() && destMAC != GetMACAddress()) {
    return;
  }

  //ethernetPacket.Print();
  EtherPacketHandlerMap::const_iterator it = _etherPacketHandlers.find(ethernetPacket.Type());
  if (it == _etherPacketHandlers.end()) {
    //throw upan::exception(XLOC, "Unhandled Ethernet Packet Type: %x", ethernetPacket.Type());
  } else {
    it->second->Process(ethernetPacket);
  }
}

void EthernetHandler::SendPacket(uint8_t* buf, uint32_t len, EtherType pType, const uint8_t* destMac) {
  auto header = reinterpret_cast<NetworkPacket::Ethernet::Header*>(buf);
  memcpy(header->_destinationMAC, destMac, NetworkPacket::MAC_ADDR_LEN);
  memcpy(header->_sourceMAC, GetMACAddress().get(), NetworkPacket::MAC_ADDR_LEN);
  header->_type = NetworkUtil::SwitchEndian((uint16_t)pType);
  GetNetworkDevice().SendPacket(buf, len);
}
