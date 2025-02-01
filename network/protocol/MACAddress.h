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

#include <ustring.h>
#include <vector.h>
#include <NetworkPacketComponents.h>

class MACAddress {
public:
  MACAddress();
  MACAddress(const upan::string& macAddr);
  MACAddress(const upan::vector<uint8_t>& macAddr);
  MACAddress(const uint8_t* macAddr);
  MACAddress(const MACAddress&);
  MACAddress& operator=(const MACAddress&);

  bool operator==(const MACAddress&) const;
  bool operator!=(const MACAddress& r) const {
    return !this->operator==(r);
  }

  bool isBroadcast() const;
  const upan::string& str() const { return _macAddrStr; }
  const uint8_t* get() const { return _macAddr; }

private:
  template <typename MACAddr>
  void convert(const MACAddr& macAddr) {
    char c[5];
    for(int i = 0; i < INADDR_MAC_LEN; ++i) {
      sprintf(c, "%02x%s", macAddr[i], i < INADDR_MAC_LEN - 1 ? ":" : "");
      _macAddrStr += c;
      _macAddr[i] = macAddr[i];
    }
  }
  void copy(const MACAddress&);

  upan::string _macAddrStr;
  uint8_t _macAddr[INADDR_MAC_LEN];
};