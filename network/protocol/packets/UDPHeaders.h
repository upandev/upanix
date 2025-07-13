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
  namespace UDP {
    struct Header {
      uint16_t _srcPort;
      uint16_t _destPort;
      uint16_t _len;
      uint16_t _checksum;

      void print() const {
        KLog::debug("Src Port: %d, Dest Port: %d, Len: %d, Checksum: 0x%x", _srcPort, _destPort, _len, _checksum);
      }

      Header toHost() const {
        return Header {
        ntohs(_srcPort),
        ntohs(_destPort),
        ntohs(_len),
        ntohs(_checksum) };
      }
    } PACKED;

    struct IPV4PseudoHeader {
      in_addr_t _srcAddr;
      in_addr_t _destAddr;
      uint8_t _zeros;
      uint8_t _protocol;
      uint16_t _udpLen;
    } PACKED;

    constexpr uint32_t HEADER_SIZE = sizeof(Header);
    constexpr uint32_t IPV4_PSEUDO_HEADER_SIZE = sizeof(IPV4PseudoHeader);
  }
}
