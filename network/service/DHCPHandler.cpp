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
#include <DHCPHandler.h>
#include <UDP4Handler.h>
#include <UDP4RecvPacket.h>
#include <DHCPRecvPacket.h>
#include <DHCPSendPacket.h>

DHCPHandler::DHCPHandler(UDP4Handler &udpHandler) : _udpHandler(udpHandler) {
}

void DHCPHandler::Process(const UDP4RecvPacket& packet) {
  printf("\n Handling DHCP Packet");
  DHCPRecvPacket dhcpPacket(packet);
  dhcpPacket.Print();
}

void DHCPHandler::ObtainIPAddress() {
  uint8_t clientHardwareAddress[16];
  memset(clientHardwareAddress, 0, 16);
  memcpy(clientHardwareAddress, _udpHandler.GetMACAddress().get(), NetworkPacket::MAC_ADDR_LEN);

  DHCPSendPacket dhcpSendPacket(1, 1, NetworkPacket::MAC_ADDR_LEN, 0,
                                0x3903F326, 0, 0,
                                { INADDR_ANY }, { INADDR_ANY }, { INADDR_ANY }, { INADDR_ANY },
                                clientHardwareAddress, nullptr, nullptr);
  _udpHandler.SendPacket(dhcpSendPacket.buf(), dhcpSendPacket.len(), 68, 67);
}