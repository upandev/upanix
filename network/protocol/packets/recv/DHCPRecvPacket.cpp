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
#include <UDP4RecvPacket.h>
#include <DHCPRecvPacket.h>
#include <NetworkUtil.h>

DHCPRecvPacket::DHCPRecvPacket(const UDP4RecvPacket& udp4RecvPacket) :
  _udp4RecvPacket(udp4RecvPacket),
  _header(reinterpret_cast<NetworkPacket::DHCP::Header&>(*_udp4RecvPacket.PacketData())) {
  if (_udp4RecvPacket.Header()._len < NetworkPacket::DHCP::HEADER_SIZE) {
    throw upan::exception(XLOC, "Invalid DHCP Packet with len: %d", _udp4RecvPacket.Header()._len);
  }
  _header._xid = NetworkUtil::SwitchEndian(_header._xid);
  _header._secs = NetworkUtil::SwitchEndian(_header._secs);
  _header._flags = NetworkUtil::SwitchEndian(_header._flags);
}

void DHCPRecvPacket::Print() const {
  printf("\n DHCP: Op: %d, HType: %d, HLen: %d, Hops: %d", _header._op, _header._hType, _header._hLen, _header._hops);
  printf("\n XID: 0x%x, Secs: %d, Flags: 0x%x", _header._xid, _header._secs, _header._flags);
  printf("\n CIAddr: %s, YIAddr: %s, SIAddr: %s, GIAddr: %s",
         IPAddress(_header._ciAddr).str().c_str(),
         IPAddress(_header._yiAddr).str().c_str(),
         IPAddress(_header._siAddr).str().c_str(),
         IPAddress(_header._giAddr).str().c_str());
  printf("\n CHAddr: %s", MACAddress(_header._chAddr).str().c_str());
}