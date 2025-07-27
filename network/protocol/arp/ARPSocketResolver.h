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
#include <net/if_arp.h>
#include <SocketDescriptorRaw.h>

class ARPSocketResolver {
public:
  void recv(const upan::shared_ptr<RawNetPacket>& packet);
  void setup(SocketDescriptorRaw& socket, const struct ether_arp& header);
  void release(SocketDescriptorRaw& socket);

private:
  upan::option<SocketDescriptorRaw&> resolve(const upan::shared_ptr<RawNetPacket>& packet);

  typedef upan::map<in_addr_t, SocketDescriptorRaw*> DEST_IP_SOCKET_MAP;
  typedef upan::set<in_addr_t> DEST_IP_SET;
  typedef upan::map<SocketDescriptorRaw*, DEST_IP_SET> SOCKET_DEST_IP_SET_MAP;

  DEST_IP_SOCKET_MAP _destIpSocketMap;
  SOCKET_DEST_IP_SET_MAP _destIpSocketMapReverse;
  upan::mutex _mutex;
};