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
#include <EtherType.h>
#include <NetworkPacketComponents.h>
#include <PacketHandler.h>
#include <EthernetRecvPacket.h>
#include <option.h>

class RawNetPacket;
class EtherPacketHandler;
class NetworkDevice;
class ARPSendPacket;

class EthernetHandler : public PacketHandler<RawNetPacket> {
public:
  EthernetHandler(NetworkDevice& networkDevice);
  void Process(const RawNetPacket& packet);

  template <typename T>
  upan::option<T&> GetHandler() {
    auto i = _etherPacketHandlers.find(T::HandlerType());
    if (i == _etherPacketHandlers.end()) {
      return upan::option<T&>::empty();
    }
    return upan::option<T&>(dynamic_cast<T&>(*i->second));
  }

  void SendPacket(RawNetPacket& packet, EtherType pType, const uint8_t* destMac);

  private:
    typedef upan::map<EtherType, PacketHandler<EthernetRecvPacket>*> EtherPacketHandlerMap;
    EtherPacketHandlerMap _etherPacketHandlers;

    const static uint32_t MIN_ETHERNET_PACKET_LEN = NetworkPacket::MAC_ADDR_LEN /*dmac*/ + NetworkPacket::MAC_ADDR_LEN /*smac*/ + 2 /*eType*/ + 1 /*payload*/;
};
