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

#include "option.h"
#include "map.h"
#include "io/SocketDescriptor.h"

class SocketResolver {
public:
  virtual upan::option<SocketDescriptor&> resolve(const upan::shared_ptr<RawNetPacket>& packet) = 0;
  virtual void setup(SocketDescriptor& socket, const void* protocolData, size_t len) = 0;
  virtual void release(SocketDescriptor& socket) = 0;
protected:
  upan::mutex _mutex;
};

class TCPSocketResolver : public SocketResolver {
public:
  upan::option<SocketDescriptor&> resolve(const upan::shared_ptr<RawNetPacket>& packet) override { return upan::option<SocketDescriptor&>::empty(); }
  void setup(SocketDescriptor& socket, const void* protocolData, size_t len) override {}
  void release(SocketDescriptor& socket) override {}
};