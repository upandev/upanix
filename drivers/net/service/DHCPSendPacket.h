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
#include <EtherType.h>

class DHCPSendPacket {
private:
  uint32_t _len;
  uint8_t* _buf;

public:
  DHCPSendPacket(uint8_t op, uint8_t hType, uint8_t hLen, uint8_t hops,
                 uint32_t xid, uint16_t secs, uint16_t flags,
                 uint8_t* ciAddr, uint8_t* yiAddr, uint8_t* siAddr, uint8_t* giAddr,
                 uint8_t* chAddr, uint8_t* sName, uint8_t* file);

  ~DHCPSendPacket();

  uint8_t* buf() { return _buf; }
  uint32_t len() { return _len; }
};