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
#include <stdio.h>
#include <IrqManager.h>
#include <PCIBusHandler.h>
#include <ATH9KDevice.h>
#include <E1000NICDevice.h>
#include <NetworkManager.h>
#include <unet.h>
#include <vector.h>

NetworkManager::NetworkManager() {
  //initialize();
}

void NetworkManager::Initialize() {
  for(auto pPCIEntry : PCIBusHandler::Instance().PCIEntries())   {
    if(pPCIEntry->bHeaderType & PCI_HEADER_BRIDGE) {
      continue;
    }
    Probe(*pPCIEntry);
  }

  getDefaultDevice().ifPresent([this](NetworkDevice& networkDevice) {
    _dhcpClient.reset(new DHCPClient(networkDevice));
    _dhcpClient->start();
  });
}

upan::option<NetworkDevice&> NetworkManager::getDefaultDevice() {
  if (_devices.empty()) {
    return upan::option<NetworkDevice&>::empty();
  }
  return upan::option<NetworkDevice&>(*_devices.front());
}

void NetworkManager::Probe(const PCIEntry& pciEntry) {
  try {
    if(pciEntry.usVendorID == 0x168C && pciEntry.usDeviceID == 0x36) {
      printf("ATH9K network-card detected");
      //return new ATH9KDevice(pciEntry);
    } else if(pciEntry.usVendorID == INTEL_VENDOR_ID && pciEntry.usDeviceID == 0x100E) {
      E1000NICDevice::Create(pciEntry);
      _devices.push_back(&E1000NICDevice::Instance());
    } else if(pciEntry.usVendorID == INTEL_VENDOR_ID && pciEntry.usDeviceID == 0x153A) {
      printf("Ethernet i217-v network-card detected");
    }
  } catch(const upan::exception& e) {
    e.Print();
  }
}

uint16_t NetworkManager::allocatePort() {
  upan::mutex_guard g(_nMutex);
  return _portPool.allocate(49152, 65535);
}

bool NetworkManager::isPortAllocated(in_port_t port) const {
  upan::mutex_guard g(_nMutex);
  return _portPool.test(port);
}

void NetworkManager::releasePort(in_port_t port) {
  upan::mutex_guard g(_nMutex);
  return _portPool.set(port);
}

void NetworkManager::updateIPMACTable(const RawNetPacket& packet) {
  const auto& ip = packet.getIPV4Header()._srcAddr;
  if (ip != INADDR_BROADCAST) {
    const MACAddress mac = packet.getEthernetHeader()._sourceMAC;
    if (mac != INADDR_MAC_BROADCAST) {
      _ipMACTable.insert(IP_MAP_TABLE::value_type(ip, mac));
    }
  }
}

upan::option<MACAddress> NetworkManager::lookupMAC(in_addr_t ip) {
  auto i = _ipMACTable.find(ip);
  if (i == _ipMACTable.end()) {
    return upan::option<MACAddress>::empty();
  }
  return upan::option<MACAddress>(i->second);
}

bool NetworkManager::isPortBounded(in_addr_t ip, in_port_t port) {
  if (ip == INADDR_ANY || ip == INADDR_LOOPBACK) {
    return _socketBindSet.exists(port);
  } else {
    return _socketBindMap[INADDR_ANY].exists(port)
    || _socketBindMap[INADDR_LOOPBACK].exists(port)
    || _socketBindMap[ip].exists(port);
  }
}

upan::option<SocketDescriptor*> NetworkManager::findBindingSocket(in_addr_t addr, in_port_t port) {
  auto e = _socketBindMap.find(addr);
  if (e != _socketBindMap.end()) {
    auto i = e->second.find(port);
    if (i != e->second.end()) {
      return upan::option<SocketDescriptor*>(i->second);
    }
  }
  return upan::option<SocketDescriptor*>::empty();
}

void NetworkManager::bind(in_addr_t ip, in_port_t port, SocketDescriptor& socket) {
  upan::mutex_guard g(_nMutex);

  if (isPortBounded(ip, port)) {
    throw upan::exception(XLOC, "port %d is already bound", ntohs(port));
  }

  if (ip != INADDR_ANY && ip != INADDR_LOOPBACK) {
    const auto networkDeviceIP = getDefaultDevice().value().GetIPAddress();
    if (networkDeviceIP == INADDR_NONE) {
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

void NetworkManager::unbind(in_addr_t ip, in_port_t port) {
  upan::mutex_guard g(_nMutex);

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

void NetworkManager::send(const uint8_t* buf, size_t n, IPPROTO_TYPE protocol, const struct sockaddr_in& srcAddr, const struct sockaddr_in& destAddr) {
  switch (protocol) {
    case IPPROTO_UDP:
      getDefaultDevice().value().getUDP4Handler().send(buf, n, srcAddr, destAddr);
      break;
    default:
      throw upan::exception(XLOC, "packet send failed - unsupported protocol: %d", protocol);
  }
}

void NetworkManager::recv(const upan::shared_ptr<RawNetPacket>& packet, const struct sockaddr_in& destAddr) {
  upan::mutex_guard g(_nMutex);
  if (destAddr.sin_addr.s_addr == INADDR_BROADCAST) {
    for(auto& e : _socketBindMap) {
      //there can be multiple network devices with different IP addresses and hence we can have multiple ip<->port mapping
      findBindingSocket(e.first, destAddr.sin_port).ifPresent([&packet](SocketDescriptor* socket) {
        socket->recvNotify(packet);
      });
    }
  } else {
    auto r = findBindingSocket(INADDR_ANY, destAddr.sin_port);
    if (r.isEmpty()) {
      findBindingSocket(destAddr.sin_addr.s_addr, destAddr.sin_port).ifPresent([&packet](SocketDescriptor* socket) {
        socket->recvNotify(packet);
      });
    } else {
      r.value()->recvNotify(packet);
    }
  }
}