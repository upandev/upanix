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
#include <ARPHandler.h>
#include <ARPRecvPacket.h>
#include <ARPSendPacket.h>
#include <EthernetHandler.h>
#include <NetworkUtil.h>

ARPHandler::ARPHandler(EthernetHandler &ethernetHandler)
  : PacketHandler<EthernetRecvPacket>(ethernetHandler.GetNetworkDevice()), _ethernetHandler(ethernetHandler) {
}

void ARPHandler::Process(const EthernetRecvPacket& packet) {
  ARPRecvPacket arpPacket(packet);
  if (arpPacket.isResponse()) {
    printf("\n Handling ARP packet");
    arpPacket.Print();
  }
}

void ARPHandler::SendRequestForMAC(const IPAddress& ipAddress) {
  const uint8_t spa[] = { 0, 0, 0, 0 };
  const uint8_t broadcast[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
  const uint8_t tha[] = { 0, 0, 0, 0, 0, 0 };

  ARPSendPacket arpSendPacket(1, EtherType::IPV4,
                              NetworkPacket::MAC_ADDR_LEN, NetworkPacket::IPV4_ADDR_LEN, 1,
                              _ethernetHandler.GetMACAddress().get(),
                              spa, tha, ipAddress.get());
  _ethernetHandler.SendPacket(arpSendPacket.buf(), arpSendPacket.len(), EtherType::ARP, broadcast);
}

void ARPHandler::SendRARP() {
  const uint8_t spa[] = { 255, 255, 255, 255 };
  const uint8_t broadcast[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
  const uint8_t* mac = GetMACAddress().get();

  ARPSendPacket arpSendPacket(1, EtherType::IPV4,
                              NetworkPacket::MAC_ADDR_LEN, NetworkPacket::IPV4_ADDR_LEN, 3,
                              mac, spa, mac, spa);
  _ethernetHandler.SendPacket(arpSendPacket.buf(), arpSendPacket.len(), EtherType::ARP, broadcast);
}
