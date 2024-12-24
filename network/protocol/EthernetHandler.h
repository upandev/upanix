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
#pragma once

#include <map.h>
#include <NetworkPacketComponents.h>
#include <PacketHandler.h>
#include <EthernetRecvPacket.h>
#include <option.h>

class RawNetPacket;
class NetworkDevice;

class EthernetHandler : public PacketHandler {
public:
  explicit EthernetHandler(NetworkDevice& networkDevice);
  void recv(const RawNetPacket& packet) override;

  void SendPacket(RawNetPacket& packet, NetworkPacket::PacketType pType, const uint8_t* destMac);

  private:
    const static uint32_t MIN_ETHERNET_PACKET_LEN = NetworkPacket::MAC_ADDR_LEN /*dmac*/ + NetworkPacket::MAC_ADDR_LEN /*smac*/ + 2 /*eType*/ + 1 /*payload*/;
};
