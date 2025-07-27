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
#include <net/ip.h>
#include <unet.h>
#include <Global.h>

namespace NetworkPacket {
  namespace IPV4 {
    struct Header {
      struct ip _header;

      IPPROTO_TYPE type() const {
        return static_cast<IPPROTO_TYPE>(_header.ip_p);
      }

      int headerLen() const {
        return _header.ip_hl * sizeof(uint32_t);
      }

      int dataLen() const {
        return ntohs(_header.ip_len) - headerLen();
      }

      uint16_t fragmentOffset() const {
        return (ntohs(_header.ip_off) & 0x1FFF) << 3;
      }

      bool hasMoreFragments() const {
        return (ntohs(_header.ip_off) & 0x2000) != 0;
      }

      Header toHost() const {
        Header h = *this;
        h._header.ip_len = ntohs(_header.ip_len);
        h._header.ip_id = ntohs(_header.ip_id);
        h._header.ip_off = htons(_header.ip_off);
        h._header.ip_sum = ntohs(_header.ip_sum);
        h._header.ip_src = { ntohl(_header.ip_src.s_addr) };
        h._header.ip_dst = { ntohl(_header.ip_dst.s_addr) };;
        return h;
      }

      void print() const {
        KLog::debug("Version: %d, IHL: %d, TOS: %d, TotalLen: %d", _header.ip_v, _header.ip_hl, _header.ip_tos, _header.ip_len);

        KLog::debug("Identification: %d, Flags: 0x%x, FragmentOffset: 0x%x, TTL: %d, Protocol: 0x%x",
               _header.ip_id, _header.ip_off >> 13, _header.ip_off << 3, _header.ip_ttl, _header.ip_p);

        KLog::debug("Checksum: 0x%x", _header.ip_sum);

        KLog::debug("Source Addr: %s, Dest Addr: %s",
               upan::net::inet_ntostr(htonl(_header.ip_src.s_addr)).c_str(),
               upan::net::inet_ntostr(htonl(_header.ip_dst.s_addr)).c_str());
      }
    } PACKED;

    struct HeaderOptions {
      uint32_t _options:24;
      uint32_t _padding:8;
    } PACKED;

    struct IPV4PseudoHeader {
      in_addr_t _srcAddr;
      in_addr_t _destAddr;
      uint8_t _zeros;
      uint8_t _protocol;
      uint16_t _len;
    } PACKED;

    constexpr uint32_t HEADER_SIZE = sizeof(Header);
    constexpr uint32_t DEFAULT_IHL = HEADER_SIZE / sizeof(uint32_t);
    constexpr uint32_t HEADER_OPT_SIZE = sizeof(HeaderOptions);
    constexpr uint32_t IPV4_PSEUDO_HEADER_SIZE = sizeof(IPV4PseudoHeader);
  }
}
