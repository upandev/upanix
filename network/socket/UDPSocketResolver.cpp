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

uint16_t UDPSocketResolver::allocatePort() {
  return _portPool.allocate(49152, 65535);
}

bool UDPSocketResolver::isPortAllocated(in_port_t port) const {
  return _portPool.test(port);
}

void UDPSocketResolver::releasePort(in_port_t port) {
  return _portPool.set(port);
}

bool UDPSocketResolver::isPortBounded(in_addr_t ip, in_port_t port) {
  if (ip == INADDR_ANY || ip == INADDR_LOOPBACK) {
    return _socketBindSet.exists(port);
  } else {
    return _socketBindMap[INADDR_ANY].exists(port)
           || _socketBindMap[INADDR_LOOPBACK].exists(port)
           || _socketBindMap[ip].exists(port);
  }
}

upan::option<SocketDescriptor&> UDPSocketResolver::findBindingSocket(in_addr_t addr, in_port_t port) {
  auto e = _socketBindMap.find(addr);
  if (e != _socketBindMap.end()) {
    auto i = e->second.find(port);
    if (i != e->second.end()) {
      return upan::option<SocketDescriptor&>(i->second);
    }
  }
  return upan::option<SocketDescriptor&>::empty();
}

void UDPSocketResolver::bind(SocketDescriptor& socket, const uint8_t* buf, size_t len) {
  if (socket.bindAddress().sin_port == 0) {
    socket.setBindAddress({ AF_INET, htons(allocatePort()), INADDR_ANY, 0 });
  }

  const in_addr_t ip = socket.bindAddress().sin_addr.s_addr;
  const in_port_t port = socket.bindAddress().sin_port;

  if (isPortBounded(ip, port)) {
    throw upan::exception(XLOC, "port %d is already bound", ntohs(port));
  }

  if (ip != INADDR_ANY && ip != INADDR_LOOPBACK) {
    const auto networkDeviceIP = NetworkManager::Instance().getDefaultRealDevice().value().getIPAddress();
    if (networkDeviceIP == INADDR_ANY) {
      throw upan::exception(XLOC, "network device doesn't have an IP address yet");
    }
    if (ip != networkDeviceIP) {
      throw upan::exception(XLOC, "invalid IP %s to bind. Network device IP is %s",
                            upan::net::inet_ntostr(ip).c_str(),
                            upan::net::inet_ntostr(networkDeviceIP).c_str());
    }
  }

  _socketBindMap[ip][port] = &socket;
  ++_socketBindSet[port];
}

void UDPSocketResolver::unbind(SocketDescriptor& socket) {
  if (socket.bindAddress().sin_port == 0) {
    return;
  }

  const in_addr_t ip = socket.bindAddress().sin_addr.s_addr;
  const in_port_t port = socket.bindAddress().sin_port;

  releasePort(ntohs(port));

  auto ipIt = _socketBindMap.find(ip);
  if (ipIt != _socketBindMap.end()) {
    ipIt->second.erase(port);
    if (ipIt->second.empty()) {
      _socketBindMap.erase(ipIt);
    }
  }

  auto portIt = _socketBindSet.find(port);
  if (portIt != _socketBindSet.end()) {
    if (portIt->second > 1) {
      --portIt->second;
    } else {
      _socketBindSet.erase(portIt);
    }
  }
}
upan::option<SocketDescriptor&> UDPSocketResolver::resolve(const upan::shared_ptr<RawNetPacket>& packet) {
  upan::mutex_guard g(_mutex);
  const in_addr_t destIP = packet->getIPV4Header()._header.ip_dst.s_addr;
  const in_port_t destPort = packet->getUDP4Header()._destPort;

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

  return upan::option<SocketDescriptor&>::empty();
}