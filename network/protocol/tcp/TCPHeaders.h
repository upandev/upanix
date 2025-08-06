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
#include <sys/socket.h>
#include <unet.h>
#include <Global.h>

namespace NetworkPacket {
  namespace TCP {
    typedef enum {
      TCP_FLAG_FIN = 0x01,
      TCP_FLAG_SYN = 0x02,
      TCP_FLAG_RST = 0x04,
      TCP_FLAG_PSH = 0x08,
      TCP_FLAG_ACK = 0x10,
    } TCP_FLAG_MASK;

    struct Header {
      uint16_t _srcPort;
      uint16_t _destPort;
      uint32_t _seqNum;
      uint32_t _ackNum;
      uint8_t _reserved:4;       // Reserved
      uint8_t _dataOffset:4;       // Data offset (size of TCP header in 32-bit words)
      uint8_t _fin:1;
      uint8_t _syn:1;
      uint8_t _rst:1;
      uint8_t _psh:1;
      uint8_t _ack:1;
      uint8_t _urg:1;
      uint8_t _ece:1;
      uint8_t _cwr:1;
      uint16_t _windowSize;
      uint16_t _checksum;
      uint16_t _urgentPtr;

      void print() const {
        KLog::debug("Src Port: %u, Dest Port: %u, Seq: %u, Ack: %u, Data Offset: %u", _srcPort, _destPort, _seqNum, _ackNum, _dataOffset);
        KLog::debug("Window Size: %u, Checksum: 0x%x, Urgent Ptr: %u", _windowSize, _checksum, _urgentPtr);
        KLog::debug("Flags: fin(%d), syn(%d), rst(%d), psh(%d), ack(%d), urg(%d), ece(%d), cwr(%d)", _fin, _syn, _rst, _psh, _ack, _urg, _ece, _cwr);
      }

      Header toHost() const {
        return Header {
        ntohs(_srcPort),
        ntohs(_destPort),
        ntohl(_seqNum),
        ntohl(_ackNum),
        _reserved, _dataOffset,
        _fin, _syn, _rst, _psh, _ack, _urg, _ece, _cwr,
        ntohs(_windowSize),
        ntohs(_checksum),
        ntohs(_urgentPtr)
        };
      }
    } PACKED;

    constexpr uint32_t HEADER_SIZE = sizeof(Header);
  }
}
