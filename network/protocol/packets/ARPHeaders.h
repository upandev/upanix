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
#include <net/if_arp.h>
#include <unet.h>
#include <Global.h>

namespace NetworkPacket {
  namespace ARP {
    struct Header {
      struct ether_arp _header;

      ETH_PROTO_TYPE type() const {
        return static_cast<ETH_PROTO_TYPE>(ntohs(_header.ea_hdr.ar_pro));
      }

      Header toHost() const {
        Header h{};
        h._header.ea_hdr.ar_hrd = ntohs(_header.ea_hdr.ar_hrd);
        h._header.ea_hdr.ar_pro = ntohs(_header.ea_hdr.ar_pro);
        h._header.ea_hdr.ar_hln = _header.ea_hdr.ar_hln;
        h._header.ea_hdr.ar_pln = _header.ea_hdr.ar_pln;
        h._header.ea_hdr.ar_op = ntohs(_header.ea_hdr.ar_op);
        memcpy(h._header.arp_sha, _header.arp_sha, ETH_ALEN);
        memcpy(h._header.arp_tha, _header.arp_tha, ETH_ALEN);
        h._header.arp_spa = ntohl(_header.arp_spa);
        h._header.arp_tpa = ntohl(_header.arp_tpa);
        return h;
      }

      bool isRequest() const { return ntohs(_header.ea_hdr.ar_op) == ARPOP_REQUEST; }

      bool isResponse() const { return ntohs(_header.ea_hdr.ar_op) == ARPOP_REPLY; }

      void print() const {
        KLog::debug("HType: %x, PType: %x, HLen: %d, PLen: %d, OpCode: %d", _header.ea_hdr.ar_hrd,
                    _header.ea_hdr.ar_pro,
                    _header.ea_hdr.ar_hln,
                    _header.ea_hdr.ar_pln,
                    _header.ea_hdr.ar_op);

        char buf[256];
        upan::string msg("SHA: ");
        for (int i = 0; i < ETH_ALEN; i++) {
          sprintf(buf, "%02x%s", _header.arp_sha[i], i < ETH_ALEN - 1 ? ":" : "");
          msg += buf;
        }
        sprintf(buf, ", SPA: %s", upan::net::inet_ntostr(htonl(_header.arp_spa)).c_str());
        msg += buf;

        KLog::debug(msg.c_str());

        msg = "THA: ";
        for (int i = 0; i < ETH_ALEN; i++) {
          sprintf(buf, "%02x%s", _header.arp_tha[i], i < ETH_ALEN - 1 ? ":" : "");
          msg += buf;
        }
        sprintf(buf, ", TPA: %s", upan::net::inet_ntostr(htonl(_header.arp_tpa)).c_str());
        msg += buf;

        KLog::debug(msg.c_str());
      }
    } PACKED;

    constexpr uint32_t HEADER_SIZE = sizeof(Header);
  }
}