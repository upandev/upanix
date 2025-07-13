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
  
#include <stdlib.h>
#include <ustring.h>
#include <net/socket.h>
#include <unet.h>
#include <Global.h>

namespace NetworkPacket {
  namespace DHCP {
    struct Header {
      uint8_t _op;
      uint8_t _hType;
      uint8_t _hLen;
      uint8_t _hops;
      uint32_t _xid;
      uint16_t _secs; // seconds elapsed since client started the protocol
      uint16_t _flags;
      struct in_addr _ciAddr; // Client IP Address
      struct in_addr _yiAddr; // Your (Client) IP Address
      struct in_addr _siAddr; // Server IP Address;
      struct in_addr _giAddr; // Relay agent (Gateway) IP Address
      uint8_t _chAddr[16]; // Client Hardware Address;
      uint8_t _sName[64]; // Optional server host name (null terminated string)
      uint8_t _file[128]; // boot file name (null terminated string)
    } PACKED;

    constexpr uint32_t HEADER_SIZE = sizeof(Header);
  }
}
