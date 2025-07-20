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

#include <map.h>
#include <shared_ptr.h>
#include <option.h>
#include <mutex.h>
#include <vector.h>
#include <net/socket.h>
#include <IPV4Headers.h>
#include <PacketHandler.h>

class EthernetHandler;

class IPV4Handler : public PacketHandler<> {
public:
  explicit IPV4Handler(NetworkDevice& networkDevice);
  void recv(const upan::shared_ptr<RawNetPacket>& packet) override;
  uint32_t headerLen() const override;
  void send(RawNetPacket& packet, IPPROTO_TYPE protocol, const struct sockaddr_in& srcAddr, const struct sockaddr_in& destAddr);
  void initHeaderLen(RawNetPacket& packet);

private:
  uint16_t calcChecksum(const NetworkPacket::IPV4::Header& ipv4Header);
  void verifyChecksum(const NetworkPacket::IPV4::Header& ipv4Header);

  struct FragmentKey {
    uint16_t _identification;
    uint8_t _protocol;
    in_addr_t _srcAddr;
    in_addr_t _destAddr;

    bool operator<(const FragmentKey& r) const {
      if (_identification != r._identification) return _identification < r._identification;
      if (_protocol != r._protocol) return _protocol < r._protocol;
      if (_srcAddr != r._srcAddr) return _srcAddr < r._srcAddr;
      return _destAddr < r._destAddr;
    }
  };

  void addFragment(const IPV4Handler::FragmentKey& fragmentKey, const upan::shared_ptr<RawNetPacket>& packet);
  upan::shared_ptr<RawNetPacket> assemblePacket(const IPV4Handler::FragmentKey& fragmentKey, const upan::shared_ptr<RawNetPacket>& lastFragment);

private:
  upan::mutex _fragmentMutex;
  upan::map<FragmentKey, upan::vector<upan::shared_ptr<RawNetPacket>>> _fragments;
};