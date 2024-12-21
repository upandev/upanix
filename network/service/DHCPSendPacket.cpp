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

#include <DHCPSendPacket.h>
#include <NetworkPacketComponents.h>
#include <memory/DMM.h>
#include <newalloc.h>

DHCPSendPacket::DHCPSendPacket(uint8_t op, uint8_t hType, uint8_t hLen, uint8_t hops,
                               uint32_t xid, uint16_t secs, uint16_t flags,
                               const struct in_addr& ciAddr, const struct in_addr& yiAddr,
                               const struct in_addr& siAddr, const struct in_addr& giAddr,
                               uint8_t* chAddr, uint8_t* sName, uint8_t* file)
                               : _len(0), _buf(nullptr) {
  _len = NetworkPacket::Ethernet::HEADER_SIZE
      + NetworkPacket::IPV4::HEADER_SIZE
      + NetworkPacket::UDP::HEADER_SIZE
      + NetworkPacket::DHCP::HEADER_SIZE;

  _buf = new ((void*)KernelDMM::Instance().allocate(_len, 16))uint8_t[_len];
  memset(_buf, 0, _len);

  auto header = reinterpret_cast<NetworkPacket::DHCP::Header*>(
      _buf + _len - NetworkPacket::DHCP::HEADER_SIZE);
  header->_op = op;
  header->_hType = hType;
  header->_hLen = hLen;
  header->_hops = hops;
  header->_xid = htons(xid);
  header->_secs = htons(secs);
  header->_flags = htons(flags);
  header->_ciAddr = ciAddr;
  header->_yiAddr = yiAddr;
  header->_siAddr = siAddr;
  header->_giAddr = giAddr;

  printf("\n %d: %d: %d", sizeof(header->_chAddr), sizeof(header->_sName), sizeof(header->_file));
  if (chAddr) {
    memcpy(header->_chAddr, chAddr, sizeof(header->_chAddr));
  }
  if (sName) {
    memcpy(header->_sName, sName, sizeof(header->_sName));
  }
  if (file) {
    memcpy(header->_file, file, sizeof(header->_file));
  }
}

DHCPSendPacket::~DHCPSendPacket() {
  delete[] _buf;
}