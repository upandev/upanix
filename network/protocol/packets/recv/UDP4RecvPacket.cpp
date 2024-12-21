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
#include <IPV4RecvPacket.h>
#include <UDP4RecvPacket.h>
#include <NetworkUtil.h>

UDP4RecvPacket::UDP4RecvPacket(const IPV4RecvPacket& ipv4RecvPacket) :
    _ipv4RecvPacket(ipv4RecvPacket), _udpHeader(reinterpret_cast<NetworkPacket::UDP::Header&>(*ipv4RecvPacket.PacketData())) {
  VerifyChecksum();
  _udpHeader._srcPort = NetworkUtil::SwitchEndian(_udpHeader._srcPort);
  _udpHeader._destPort = NetworkUtil::SwitchEndian(_udpHeader._destPort);
  _udpHeader._len = NetworkUtil::SwitchEndian(_udpHeader._len);
  _udpHeader._checksum = NetworkUtil::SwitchEndian(_udpHeader._checksum);
}

void UDP4RecvPacket::VerifyChecksum() {
  if (_udpHeader._checksum) {
    NetworkPacket::UDP::IPV4PseudoHeader pseudoHeader;
    memcpy(pseudoHeader._srcAddr, _ipv4RecvPacket.header()._srcAddr, NetworkPacket::IPV4_ADDR_LEN);
    memcpy(pseudoHeader._destAddr, _ipv4RecvPacket.header()._destAddr, NetworkPacket::IPV4_ADDR_LEN);
    pseudoHeader._zeros = 0;
    pseudoHeader._protocol = IPType::UDP;
    pseudoHeader._udpLen = _udpHeader._len;

    const uint32_t len = NetworkUtil::SwitchEndian(_udpHeader._len);
    const uint16_t calculatedChecksum = NetworkUtil::CalculateChecksum((uint16_t *) _ipv4RecvPacket.PacketData(),
                                                                       len,
                                                                       NetworkUtil::AddForChecksum(
                                                                           (uint16_t *) &pseudoHeader,
                                                                           NetworkPacket::UDP::IPV4_PSEUDO_HEADER_SIZE, 0));

    const uint16_t r = calculatedChecksum ^ (uint16_t)0xFFFF;
    if (r) {
      Print();
      throw upan::exception(XLOC, "Invalid Checksum for UDP Packet, IP Packet ID: %d (calc. checksum: 0x%x)",
                            _ipv4RecvPacket.header()._identification, calculatedChecksum);
    }
  }
}

void UDP4RecvPacket::Print() const {
  printf("\n Src Port: %d, Dest Port: %d, Len: %d, Checksum: 0x%x",
         _udpHeader._srcPort, _udpHeader._destPort, _udpHeader._len, _udpHeader._checksum);
}