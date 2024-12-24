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
#include <PCIBusHandler.h>
#include <NetworkDevice.h>

NetworkDevice::NetworkDevice(const PCIEntry& pciEntry)
  : _pciEntry(pciEntry),
    _ethernetHandler(*this), _ipv4Handler(*this), _udp4Handler(*this), _arpHandler(*this) {
}

NetworkDevice::~NetworkDevice() {
}

upan::option<PacketHandler&> NetworkDevice::getHandler(const NetworkPacket::PacketType type) {
  switch (type) {
    case NetworkPacket::PacketType::ETHER_TYPE: return upan::option<PacketHandler&>(_ethernetHandler);
    case NetworkPacket::PacketType::IPV4_TYPE: return upan::option<PacketHandler&>(_ipv4Handler);
    case NetworkPacket::PacketType::UDP4_TYPE: return upan::option<PacketHandler&>(_udp4Handler);
    case NetworkPacket::PacketType::ARP_TYPE: return upan::option<PacketHandler&>(_arpHandler);
  }
  return upan::option<PacketHandler&>::empty();
  //throw upan::exception(XLOC, "no network protocol handler found for packet-type: %d", type);
}