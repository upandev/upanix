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

#include <MACAddress.h>

MACAddress::MACAddress(const upan::string &macAddr) : _macAddrStr(macAddr) {
  upan::vector<upan::string> tokens;
  macAddr.tokenize(":", false, tokens);
  if (tokens.size() != NetworkPacket::MAC_ADDR_LEN) {
    throw upan::exception(XLOC, "Invalid MAC Address: %s", macAddr.c_str());
  }
  for(int i = 0; i < tokens.size(); ++i) {
    _macAddr[i] = atoi(tokens[i].c_str());
  }
}

MACAddress::MACAddress(const upan::vector<uint8_t>& macAddr) {
  if (macAddr.size() != NetworkPacket::MAC_ADDR_LEN) {
    throw upan::exception(XLOC, "Invalid MAC Address Len: %d", macAddr.size());
  }
  convert(macAddr);
}

MACAddress::MACAddress(const uint8_t* macAddr) {
  convert(macAddr);
}

MACAddress::MACAddress(const MACAddress& r) {
  copy(r);
}

MACAddress& MACAddress::operator=(const MACAddress& r) {
  copy(r);
  return *this;
}

bool MACAddress::operator==(const MACAddress& r) const {
  for (int i = 0; i < NetworkPacket::MAC_ADDR_LEN; ++i) {
    if (_macAddr[i] != r._macAddr[i]) {
      return false;
    }
  }
  return true;
}

bool MACAddress::isBroadcast() const {
  for(unsigned char i : _macAddr) {
    if (i != 0xFF) {
      return false;
    }
  }
  return true;
}

void MACAddress::copy(const MACAddress& r) {
  this->_macAddrStr = r._macAddrStr;
  memcpy(this->_macAddr, r._macAddr, NetworkPacket::MAC_ADDR_LEN);
}

