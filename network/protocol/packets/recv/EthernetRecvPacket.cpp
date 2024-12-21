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
#include <NetworkUtil.h>
#include <EthernetRecvPacket.h>

EthernetRecvPacket::EthernetRecvPacket(const RawNetPacket& rawNetPacket) :
  _rawNetPacket(rawNetPacket),
  _header(reinterpret_cast<NetworkPacket::Ethernet::Header&>(*rawNetPacket.buf())) {
  _header._type = NetworkUtil::SwitchEndian(_header._type);
}

void EthernetRecvPacket::Print() const {
  printf("\n ETHERNET PACKET: D ");
  for(int  i = 0; i < NetworkPacket::MAC_ADDR_LEN; i++) {
    printf("%02x%s", _header._destinationMAC[i], i < NetworkPacket::MAC_ADDR_LEN - 1 ? ":" : "");
  }
  printf(", S ");
  for(int  i = 0; i < NetworkPacket::MAC_ADDR_LEN; i++) {
    printf("%02x%s", _header._sourceMAC[i], i < NetworkPacket::MAC_ADDR_LEN - 1 ? ":" : "");
  }
  printf(", Type: %x", _header._type);
}