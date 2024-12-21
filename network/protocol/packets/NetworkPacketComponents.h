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

namespace NetworkPacket {
  constexpr int MAC_ADDR_LEN = 6;
  constexpr int IPV4_ADDR_LEN = 4;

  namespace ARP {
    struct Header {
      uint16_t _hType;
      uint16_t _pType;
      uint8_t _hLen;
      uint8_t _pLen;
      uint16_t _opCode;
    } PACKED;

    struct IPV4 {
      uint8_t _senderHardwareAddress[MAC_ADDR_LEN];
      struct in_addr _senderProtocolAddress;
      uint8_t _targetHardwareAddress[MAC_ADDR_LEN];
      struct in_addr _targetProtocolAddress;
    } PACKED;

    constexpr uint32_t HEADER_SIZE = sizeof(Header);
    constexpr uint32_t IPV4_SIZE = sizeof(IPV4);
  }

  namespace Ethernet {
    struct Header {
      uint8_t _destinationMAC[MAC_ADDR_LEN];
      uint8_t _sourceMAC[MAC_ADDR_LEN];
      uint16_t _type;
    } PACKED;

    constexpr uint32_t HEADER_SIZE = sizeof(Header);
  }

  namespace IPV4 {
    struct Header {
      uint32_t _ihl:4; // Internet Header Length
      uint32_t _version:4;
      uint8_t _tos; // Type Of Service
      uint16_t _totalLen;
      uint16_t _identification;
      uint32_t _flags:3;
      uint32_t _fragmentOffset:13;
      uint8_t _ttl; // Time to live
      uint8_t _protocol;
      uint16_t _checksum;
      uint8_t _srcAddr[4];
      uint8_t _destAddr[4];
    } PACKED;

    struct HeaderOptions {
      uint32_t _options:24;
      uint32_t _padding:8;
    } PACKED;

    constexpr uint32_t HEADER_SIZE = sizeof(Header);
    constexpr uint32_t HEADER_OPT_SIZE = sizeof(HeaderOptions);
  }

  namespace UDP {
    struct Header {
      uint16_t _srcPort;
      uint16_t _destPort;
      uint16_t _len;
      uint16_t _checksum;
    } PACKED;

    struct IPV4PseudoHeader {
      uint8_t _srcAddr[4];
      uint8_t _destAddr[4];
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
