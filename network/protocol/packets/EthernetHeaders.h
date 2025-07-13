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
#include <net/if_ether.h>
#include <unet.h>
#include <Global.h>

namespace NetworkPacket {
  namespace Ethernet {
    struct Header {
      struct ethhdr _header;

      ETH_PROTO_TYPE type() const {
        return static_cast<ETH_PROTO_TYPE>(ntohs(_header.h_proto));
      }

      Header toHost() const {
        Header h {};
        memcpy(h._header.h_dest, _header.h_dest, ETH_ALEN);
        memcpy(h._header.h_source, _header.h_source, ETH_ALEN);
        h._header.h_proto = ntohs(_header.h_proto);
        return h;
      }

      void print() const {
        char buf[1024];
        upan::string msg("Ethernet - Type: %d", ntohs(_header.h_proto));
        msg += "\n Src MAC: ";
        for (int i = 0; i < ETH_ALEN; i++) {
          sprintf(buf, "%02x%s", _header.h_source[i], i < ETH_ALEN - 1 ? ":" : "");
          msg += buf;
        }

        msg += "\n Dest MAC: ";
        for (int i = 0; i < ETH_ALEN; i++) {
          sprintf(buf, "%02x%s", _header.h_dest[i], i < ETH_ALEN - 1 ? ":" : "");
          msg += buf;
        }
        KLog::debug(msg.c_str());
      }
    } PACKED;

    constexpr uint32_t HEADER_SIZE = sizeof(Header);
  }
}
