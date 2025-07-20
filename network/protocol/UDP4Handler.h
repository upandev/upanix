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
#include <option.h>
#include <shared_ptr.h>
#include <net/socket.h>
#include <PacketHandler.h>

class IPV4Handler;

class UDP4Handler : public PacketHandler<> {
public:
  explicit UDP4Handler(NetworkDevice& networkDevice);
  void recv(const upan::shared_ptr<RawNetPacket>& packet) override;
  uint32_t headerLen() const override;
  void send(const uint8_t* buf, uint32_t len, const struct sockaddr_in& srcAddr, const struct sockaddr_in& destAddr);

private:
  void calcChecksum(RawNetPacket& packet, in_addr_t srcAddr, in_addr_t destAddr);
  void verifyChecksum(const RawNetPacket& packet);
};