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

#include <UDPSocketResolver.h>
#include <NetworkManager.h>
#include <RealNetworkDevice.h>

upan::option<SocketDescriptorDataGram&> UDPSocketResolver::findBindingSocket(in_addr_t addr, in_port_t port) {
  auto e = _socketBindMap.find(addr);
  if (e != _socketBindMap.end()) {
    auto i = e->second.find(port);
    if (i != e->second.end()) {
      return upan::option<SocketDescriptorDataGram&>(i->second);
    }
  }
  return upan::option<SocketDescriptorDataGram&>::empty();
}

void UDPSocketResolver::setup(SocketDescriptorDataGram& socket) {
  upan::mutex_guard g(_mutex);
  _socketBindMap[socket.srcAddr().sin_addr.s_addr][socket.srcAddr().sin_port] = &socket;
}

void UDPSocketResolver::release(SocketDescriptorDataGram& socket) {
  upan::mutex_guard g(_mutex);

  const auto& srcAddr = socket.srcAddr();
  const in_addr_t ip = srcAddr.sin_addr.s_addr;
  const in_port_t port = srcAddr.sin_port;

  auto ipIt = _socketBindMap.find(ip);
  if (ipIt != _socketBindMap.end()) {
    ipIt->second.erase(port);
    if (ipIt->second.empty()) {
      _socketBindMap.erase(ipIt);
    }
  }
}

upan::option<SocketDescriptorDataGram&> UDPSocketResolver::resolve(const upan::shared_ptr<RawNetPacket>& packet) {
  upan::mutex_guard g(_mutex);

  const in_addr_t destIP = packet->getIPV4Header()._header.ip_dst.s_addr;
  const in_port_t destPort = packet->getUDPHeader()._destPort;

  if (destIP == INADDR_BROADCAST) {
    for(auto& e : _socketBindMap) {
      //there can be multiple network devices with different IP addresses and hence we can have multiple ip<->port mapping
      auto r = findBindingSocket(e.first, destPort);
      if (!r.isEmpty()) {
        return r;
      }
    }
  } else {
    auto r = findBindingSocket(INADDR_ANY, destPort);
    if (r.isEmpty()) {
      return findBindingSocket(destIP, destPort);
    } else {
      return r;
    }
  }
  return upan::option<SocketDescriptorDataGram&>::empty();
}

void UDPSocketResolver::recv(const upan::shared_ptr<RawNetPacket>& packet) {
  resolve(packet).ifPresent([&packet](SocketDescriptorDataGram& socket) { socket.recvNotify(packet); });
}