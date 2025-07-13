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
#include <net/if_ether.h>
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
        Header h {};
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

    constexpr uint32_t HEADER_SIZE = sizeof(Header);
    constexpr uint32_t DEFAULT_IHL = HEADER_SIZE / sizeof(uint32_t);
    constexpr uint32_t HEADER_OPT_SIZE = sizeof(HeaderOptions);
  }

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
