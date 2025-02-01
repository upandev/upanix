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
  constexpr int IPV4_ADDR_LEN = 4;

  typedef enum {
    E_IPV4_T = 0x0800,
    E_ARP_T = 0x0806,
  } EthernetPacketType;

  namespace ARP {
    struct Header {
      uint16_t _hType;
      uint16_t _pType;
      uint8_t _hLen;
      uint8_t _pLen;
      uint16_t _opCode;

      EthernetPacketType type() const {
        return static_cast<EthernetPacketType>(ntohs(_pType));
      }

      Header toHost() const {
        return Header {
        ntohs(_hType),
        ntohs(_pType),
        _hLen,
        _pLen,
        ntohs(_opCode) };
      }

      bool isRequest() const { return _opCode == 1; }
      bool isResponse() const { return _opCode == 2; }

      void print() const {
        klog_debug("HType: %x, PType: %x, HLen: %d, PLen: %d, OpCode: %d", _hType, _pType, _hLen, _pLen, _opCode);
      }
    } PACKED;

    struct IPV4 {
      uint8_t _senderHardwareAddress[INADDR_MAC_LEN];
      in_addr_t _senderProtocolAddress;
      uint8_t _targetHardwareAddress[INADDR_MAC_LEN];
      in_addr_t _targetProtocolAddress;

      IPV4 toHost() const {
        IPV4 h {};
        memcpy(h._senderHardwareAddress, _senderHardwareAddress, INADDR_MAC_LEN);
        memcpy(h._targetHardwareAddress, _targetHardwareAddress, INADDR_MAC_LEN);
        h._senderProtocolAddress = ntohl(_senderProtocolAddress);
        h._targetProtocolAddress = ntohl(_targetProtocolAddress);
        return h;
      }

      void print() const {
        char buf[1024];
        upan::string msg("SHA: ");
        for (int i = 0; i < INADDR_MAC_LEN; i++) {
          sprintf(buf, "%02x%s", _senderHardwareAddress[i], i < INADDR_MAC_LEN - 1 ? ":" : "");
          msg += buf;
        }
        sprintf(buf, ", SPA: %s", inet_ntoa({_senderProtocolAddress}));
        msg += buf;

        msg += "\n THA: ";
        for (int i = 0; i < INADDR_MAC_LEN; i++) {
          sprintf(buf, "%02x%s", _targetHardwareAddress[i], i < INADDR_MAC_LEN - 1 ? ":" : "");
          msg += buf;
        }
        sprintf(buf, ", TPA: %s", inet_ntoa({_targetProtocolAddress}));
        msg += buf;
        klog_debug(msg.c_str());
      }
    } PACKED;

    constexpr uint32_t HEADER_SIZE = sizeof(Header);
    constexpr uint32_t IPV4_SIZE = sizeof(IPV4);
  }

  namespace Ethernet {
    struct Header {
      uint8_t _destinationMAC[INADDR_MAC_LEN];
      uint8_t _sourceMAC[INADDR_MAC_LEN];
      uint16_t _type;

      EthernetPacketType type() const {
        return static_cast<EthernetPacketType>(ntohs(_type));
      }

      Header toHost() const {
        Header h{};
        memcpy(h._destinationMAC, _destinationMAC, INADDR_MAC_LEN);
        memcpy(h._sourceMAC, _sourceMAC, INADDR_MAC_LEN);
        h._type = ntohs(_type);
        return h;
      }

      void print() const {
        char buf[1024];
        upan::string msg("Ethernet - Type: %d", ntohs(_type));
        msg += "\n Src MAC: ";
        for (int i = 0; i < INADDR_MAC_LEN; i++) {
          sprintf(buf, "%02x%s", _sourceMAC[i], i < INADDR_MAC_LEN - 1 ? ":" : "");
          msg += buf;
        }

        msg += "\n Dest MAC: ";
        for (int i = 0; i < INADDR_MAC_LEN; i++) {
          sprintf(buf, "%02x%s", _destinationMAC[i], i < INADDR_MAC_LEN - 1 ? ":" : "");
          msg += buf;
        }
        klog_debug(msg.c_str());
      }
    } PACKED;

    constexpr uint32_t HEADER_SIZE = sizeof(Header);
  }

  namespace IPV4 {
    struct Header {
      uint8_t _ihl:4; // Internet Header Length
      uint8_t _version:4;
      uint8_t _tos; // Type Of Service
      uint16_t _totalLen;
      uint16_t _identification;
      uint16_t _flags:3;
      uint16_t _fragmentOffset:13;
      uint8_t _ttl; // Time to live
      uint8_t _protocol;
      uint16_t _checksum;
      in_addr_t _srcAddr;
      in_addr_t _destAddr;

      IPPROTO_TYPE type() const {
        return static_cast<IPPROTO_TYPE>(_protocol);
      }

      Header toHost() const {
        Header h = *this;
        h._totalLen = ntohs(_totalLen);
        h._identification = ntohs(_identification);
        h._checksum = ntohs(_checksum);
        h._srcAddr = ntohl(_srcAddr);
        h._destAddr = ntohl(_destAddr);
        return h;
      }

      void print() const {
        klog_debug("Version: %d, IHL: %d, TOS: %d, TotalLen: %d", _version, _ihl, _tos, _totalLen);

        klog_debug("Identification: %d, Flags: 0x%x, FragmentOffset: 0x%x, TTL: %d, Protocol: 0x%x",
               _identification, _flags, _fragmentOffset, _ttl, _protocol);

        klog_debug("Checksum: 0x%x", _checksum);

        klog_debug("Source Addr: %s, Dest Addr: %s",
               upan::net::inet_ntostr(htonl(_srcAddr)).c_str(),
               upan::net::inet_ntostr(htonl(_destAddr)).c_str());
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
        klog_debug("Src Port: %d, Dest Port: %d, Len: %d, Checksum: 0x%x", _srcPort, _destPort, _len, _checksum);
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
