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
#include <SocketDescriptorDataGram.h>

class UDPSocketResolver {
public:
  void recv(const upan::shared_ptr<RawNetPacket>& packet);
  void setup(SocketDescriptorDataGram& socket);
  void release(SocketDescriptorDataGram& socket);

private:
  upan::option<SocketDescriptorDataGram&> resolve(const upan::shared_ptr<RawNetPacket>& packet);
  upan::option<SocketDescriptorDataGram&> findBindingSocket(in_addr_t addr, in_port_t port);

private:
  typedef upan::map<in_port_t, SocketDescriptorDataGram*> SOCKET_PORT_MAP;
  typedef upan::map<in_addr_t, SOCKET_PORT_MAP> SOCKET_BIND_MAP;

  SOCKET_BIND_MAP _socketBindMap;
  upan::mutex _mutex;
};