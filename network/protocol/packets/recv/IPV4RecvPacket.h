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

#include <NetworkPacketComponents.h>
#include <EthernetRecvPacket.h>
#include <IPType.h>

class IPV4RecvPacket {
public:
  explicit IPV4RecvPacket(const EthernetRecvPacket& ethernetPacket);

  IPType Type() const {
    return static_cast<IPType>(_ipv4Header._protocol);
  }

  void Print() const;

  uint8_t* PacketData() const {
    return _ethernetPacket.PacketData() + (sizeof(uint32_t) * _ipv4Header._ihl);
  }

  const NetworkPacket::IPV4::Header& header() const {
    return _ipv4Header;
  }

private:
  void VerifyChecksum();

private:
  const EthernetRecvPacket& _ethernetPacket;
  NetworkPacket::IPV4::Header& _ipv4Header;
};