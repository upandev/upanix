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
#include <net/ip_icmp.h>
#include <SocketDescriptorICMP.h>

class ICMPSocketResolver {
public:
  void recv(const upan::shared_ptr<RawNetPacket>& packet);
  void setup(SocketDescriptorICMP& socket, const struct icmp& header);
  void release(SocketDescriptorICMP& socket);

private:
  upan::option<SocketDescriptorICMP&> resolve(const upan::shared_ptr<RawNetPacket>& packet);

  typedef uint32_t ICMP_PACKET_ID;
  typedef upan::map<ICMP_PACKET_ID, SocketDescriptorICMP*> ICMP_ID_SOCKET_MAP;
  typedef upan::set<ICMP_PACKET_ID> ICMP_ID_SET;
  typedef upan::map<SocketDescriptorICMP*, ICMP_ID_SET> ICMP_ID_SET_MAP;

  ICMP_ID_SOCKET_MAP _icmpIdSocketMap;
  ICMP_ID_SET_MAP _icmpIdSocketMapReverse;
  upan::mutex _mutex;
};