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

#include <option.h>
#include <map.h>
#include <SocketDescriptorStream.h>

class TCPSocketResolver {
public:
  void recv(const upan::shared_ptr<RawNetPacket>& packet);
  void listen(SocketDescriptorStream& socket);
  void setup(SocketDescriptorStream& socket);
  void release(SocketDescriptorStream& socket);

private:
  upan::option<SocketDescriptorStream&> resolve(const upan::shared_ptr<RawNetPacket>& packet);

  typedef uint64_t sock_addr_t;
  constexpr sock_addr_t SOCK_ADDR(const sockaddr_in& addr) { return ((uint64_t)addr.sin_addr.s_addr << 32) | addr.sin_port; }

  typedef upan::map<sock_addr_t, SocketDescriptorStream*> SOCKET_ADDR_MAP;
  typedef upan::map<sock_addr_t, SOCKET_ADDR_MAP> SOCKET_BIND_MAP;

  SOCKET_ADDR_MAP _socketListenMap;
  SOCKET_BIND_MAP _socketBindMap;
  upan::mutex _mutex;
};