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

#include <IPV4RecvPacket.h>

IPV4RecvPacket::IPV4RecvPacket(const EthernetRecvPacket& ethernetPacket) :
  _ethernetPacket(ethernetPacket),
  _ipv4Header(reinterpret_cast<NetworkPacket::IPV4::Header&>(*ethernetPacket.PacketData())) {
  VerifyChecksum();
  _ipv4Header._totalLen = ntohs(_ipv4Header._totalLen);
  _ipv4Header._identification = ntohs(_ipv4Header._identification);
  _ipv4Header._checksum = ntohs(_ipv4Header._checksum);
  //_ipv4Header._fragmentOffset = NetworkUtil::SwitchEndian(_ipv4Header._fragmentOffset);
}

void IPV4RecvPacket::VerifyChecksum() {
  const uint32_t calculatedChecksum = NetworkUtil::CalculateChecksum((uint16_t *)(_ethernetPacket.PacketData()),
                                                                     _ipv4Header._ihl * sizeof(uint32_t), 0);
  if (calculatedChecksum ^ (uint16_t)0xFFFF) {
    Print();
    throw upan::exception(XLOC, "Invalid Checksum for IP Packet ID: %d (calc. checksum: 0x%x)",
                          ntohs(_ipv4Header._identification), calculatedChecksum);
  }
}

void IPV4RecvPacket::Print() const {
  printf("\n Version: %d, IHL: %d, TOS: %d, TotalLen: %d",
         _ipv4Header._version, _ipv4Header._ihl, _ipv4Header._tos, _ipv4Header._totalLen);

  printf("\nIdentification: %d, Flags: 0x%x, FragmentOffset: 0x%x, TTL: %d, Protocol: 0x%x",
         _ipv4Header._identification, _ipv4Header._flags, _ipv4Header._fragmentOffset, _ipv4Header._ttl, _ipv4Header._protocol);

  printf("\nChecksum: 0x%x", _ipv4Header._checksum);

  printf("\nSource Addr: %d.%d.%d.%d, Dest Addr: %d.%d.%d.%d",
         _ipv4Header._srcAddr[0], _ipv4Header._srcAddr[1], _ipv4Header._srcAddr[2], _ipv4Header._srcAddr[3],
         _ipv4Header._destAddr[0], _ipv4Header._destAddr[1], _ipv4Header._destAddr[2], _ipv4Header._destAddr[3]);
}