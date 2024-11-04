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

#include <NetworkUtil.h>
#include <stringalgo.h>

uint8_t NetworkUtil::SwitchEndian(const uint8_t val) {
  return (val >> 4) | (val << 4);
}

uint16_t NetworkUtil::SwitchEndian(const uint16_t val) {
    return (val >> 8) | (val << 8);
}

uint32_t NetworkUtil::SwitchEndian(const uint32_t val) {
    return ((val >> 24) & 0xFF) | ((val >> 8) & 0xFF00) | ((val << 8) & 0xFF0000) | ((val << 24) & 0xFF000000);
}

uint16_t NetworkUtil::CalculateChecksum(const uint16_t* buf, uint32_t lengthInBytes, uint32_t initSum) {
  uint32_t sum = AddForChecksum(buf, lengthInBytes, initSum);
  while((sum >> 16)) {
    sum = (sum & 0xFFFF) + (sum >> 16);
  }
  return sum & 0xFFFF;
}

uint32_t NetworkUtil::AddForChecksum(const uint16_t* buf, uint32_t lengthInBytes, uint32_t initSum) {
  const uint32_t lengthInWords = lengthInBytes / 2;
  uint32_t sum = initSum;
  for(uint32_t i = 0; i < lengthInWords; ++i) {
    sum += buf[i];
  }
  if (lengthInBytes % 2) {
    sum += ((uint8_t*)buf)[lengthInBytes - 1];
  }
  return sum;
}

IPAddress::IPAddress(const upan::string &ipAddr) : _ipAddrStr(ipAddr) {
  upan::vector<upan::string> tokens = upan::tokenize(ipAddr.c_str(), '.');
  if (tokens.size() != NetworkPacket::IPV4_ADDR_LEN) {
    throw upan::exception(XLOC, "Invalid IP Address: %s", ipAddr.c_str());
  }
  for(int i = 0; i < tokens.size(); ++i) {
    _ipAddr[i] = atoi(tokens[i].c_str());
  }
}

IPAddress::IPAddress(const upan::vector<uint8_t>& ipAddr) {
  if (ipAddr.size() != NetworkPacket::IPV4_ADDR_LEN) {
    throw upan::exception(XLOC, "Invalid IP Address Len: %d", ipAddr.size());
  }
  convert(ipAddr);
}

IPAddress::IPAddress(const uint8_t* ipAddr) {
  convert(ipAddr);
}

IPAddress::IPAddress(const IPAddress& r) {
  copy(r);
}

IPAddress& IPAddress::operator=(const IPAddress& r) {
  copy(r);
  return *this;
}

void IPAddress::copy(const IPAddress& r) {
  this->_ipAddrStr = r._ipAddrStr;
  memcpy(this->_ipAddr, r._ipAddr, NetworkPacket::IPV4_ADDR_LEN);
}
