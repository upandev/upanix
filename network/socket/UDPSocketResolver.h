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
#include <bitset.h>
#include <SocketDescriptor.h>
#include <SocketResolver.h>

class UDPSocketResolver : public SocketResolver {
public:
  upan::option<SocketDescriptor&> resolve(const upan::shared_ptr<RawNetPacket>& packet) override;
  void setup(SocketDescriptor& socket, const void* protocolData, size_t len) override;
  void release(SocketDescriptor& socket) override;

private:
  bool isPortBounded(in_addr_t ip, in_port_t port);
  upan::option<SocketDescriptor&> findBindingSocket(in_addr_t addr, in_port_t port);

private:
  typedef upan::map<in_port_t, SocketDescriptor*> SOCKET_PORT_MAP;
  typedef upan::map<in_addr_t, SOCKET_PORT_MAP> SOCKET_BIND_MAP;
  typedef upan::map<in_port_t, int> SOCKET_BIND_SET;
  typedef upan::map<SocketDescriptor*, sockaddr_in> SOCKET_SRC_ADDR_MAP;

  SOCKET_BIND_SET _socketBindSet;
  SOCKET_BIND_MAP _socketBindMap;
  SOCKET_SRC_ADDR_MAP _socketSrcAddrMap;
};