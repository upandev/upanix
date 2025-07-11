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
#include <set.h>
#include <SocketResolver.h>

class ARPSocketResolver : public SocketResolver {
public:
  upan::option<SocketDescriptor&> resolve(const upan::shared_ptr<RawNetPacket>& packet) override;
  void bind(SocketDescriptor& socket, const uint8_t* buf, size_t len) override;
  void unbind(SocketDescriptor& socket) override;

private:
  typedef uint32_t ICMP_PACKET_ID;
  typedef upan::map<in_addr_t, SocketDescriptor*> DEST_IP_SOCKET_MAP;
  typedef upan::set<in_addr_t> DEST_IP_SET;
  typedef upan::map<SocketDescriptor*, DEST_IP_SET> SOCKET_DEST_IP_SET_MAP;

  DEST_IP_SOCKET_MAP _destIpSocketMap;
  SOCKET_DEST_IP_SET_MAP _destIpSocketMapReverse;
};