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
#include <NetworkPacketComponents.h>
#include <vector.h>

class NetworkUtil {
public:
  static uint8_t SwitchEndian(uint8_t val);
  static uint16_t SwitchEndian(uint16_t val);
  static uint32_t SwitchEndian(uint32_t val);
  static uint16_t CalculateChecksum(const uint16_t* buf, uint32_t lengthInBytes, uint32_t initSum);
  static uint32_t AddForChecksum(const uint16_t* buf, uint32_t lengthInBytes, uint32_t initSum);
};

class IPAddress {
public:
  IPAddress(const upan::string& ipAddr);
  IPAddress(const upan::vector<uint8_t>& ipAddr);
  IPAddress(const uint8_t* ipAddr);
  IPAddress(const IPAddress&);
  IPAddress& operator=(const IPAddress&);

  IPAddress(const IPAddress&&) = delete;
  IPAddress& operator=(const IPAddress&&) = delete;

  const upan::string str() const {
    return _ipAddrStr;
  }
  const uint8_t* get() const {
    return _ipAddr;
  }
private:
  template <typename IPAddr>
  void convert(const IPAddr& ipAddr) {
    char c[5];
    for(uint32_t i = 0; i < NetworkPacket::IPV4_ADDR_LEN; ++i) {
      sprintf(c, "%u%s", ipAddr[i], i < NetworkPacket::IPV4_ADDR_LEN - 1 ? "." : "");
      _ipAddrStr += c;
      _ipAddr[i] = ipAddr[i];
    }
  }

  void copy(const IPAddress&);

  upan::string _ipAddrStr;
  uint8_t _ipAddr[NetworkPacket::IPV4_ADDR_LEN];
};