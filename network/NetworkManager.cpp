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
#include <UDPSocketResolver.h>
#include <ICMPSocketResolver.h>
#include "ARPSocketResolver.h"

NetworkManager& NetworkManager::Instance() {
  static NetworkManager instance;
  return instance;
}

NetworkManager::NetworkManager() : _interfaceId(0) {

}

void NetworkManager::Initialize() {
  for(auto pPCIEntry : PCIBusHandler::Instance().PCIEntries())   {
    if(pPCIEntry->bHeaderType & PCI_HEADER_BRIDGE) {
      continue;
    }
    Probe(*pPCIEntry);
  }

  _socketResolvers.insert(SOCKET_RESOLVER_MAP::value_type(IPPROTO_TYPE::IPPROTO_TCP, new TCPSocketResolver()));
  _socketResolvers.insert(SOCKET_RESOLVER_MAP::value_type(IPPROTO_TYPE::IPPROTO_UDP, new UDPSocketResolver()));
  _socketResolvers.insert(SOCKET_RESOLVER_MAP::value_type(IPPROTO_TYPE::IPPROTO_ICMP, new ICMPSocketResolver()));
  _socketResolvers.insert(SOCKET_RESOLVER_MAP::value_type(ETH_PROTO_TYPE::ETH_P_ARP, new ARPSocketResolver()));

  getDefaultDevice().ifPresent([](NetworkDevice& networkDevice) {
    networkDevice.connectToNetwork();
  });

  if (_dnsClient.isEmpty()) {
    _dnsClient.reset(new DNSClient());
  }
}

upan::option<NetworkDevice&> NetworkManager::getDefaultDevice() {
  if (_devices.empty()) {
    return upan::option<NetworkDevice&>::empty();
  }
  return upan::option<NetworkDevice&>(*_devices.front());
}

upan::option<NetworkDevice&> NetworkManager::getDeviceById(int id) {
  for(auto d : _devices) {
    if (d->id() == id) {
      return upan::option<NetworkDevice&>(*d);
    }
  }
  return upan::option<NetworkDevice&>::empty();
}

upan::option<NetworkDevice&> NetworkManager::getDeviceByName(const upan::string& name) {
  for(auto d : _devices) {
    if (d->name() == name) {
      return upan::option<NetworkDevice&>(*d);
    }
  }
  return upan::option<NetworkDevice&>::empty();
}

void NetworkManager::Probe(const PCIEntry& pciEntry) {
  try {
    if(pciEntry.usVendorID == 0x168C && pciEntry.usDeviceID == 0x36) {
      printf("ATH9K network-card detected");
      //return new ATH9KDevice(pciEntry);
    } else if(pciEntry.usVendorID == INTEL_VENDOR_ID && pciEntry.usDeviceID == 0x100E) {
      E1000NICDevice::Create(pciEntry);

      auto& device = E1000NICDevice::Instance();
      device.setName("eth0");
      device.setId(++_interfaceId);

      _devices.push_back(&device);
    } else if(pciEntry.usVendorID == INTEL_VENDOR_ID && pciEntry.usDeviceID == 0x153A) {
      printf("Ethernet i217-v network-card detected");
    }
  } catch(const upan::exception& e) {
    e.Print();
  }
}

void NetworkManager::updateIPMACTable(in_addr_t ip, const MACAddress& mac) {
  if (ip != INADDR_BROADCAST && mac != INADDR_MAC_BROADCAST) {
    _ipMACTable.insert(IP_MAP_TABLE::value_type(ip, mac));
  }
}

upan::option<const MACAddress&> NetworkManager::lookupMAC(in_addr_t ip) {
  auto i = _ipMACTable.find(ip);
  if (i == _ipMACTable.end()) {
    return upan::option<const MACAddress&>::empty();
  }
  return upan::option<const MACAddress&>(i->second);
}

void NetworkManager::bind(SocketDescriptor& socket, const uint8_t* buf, size_t len) {
  upan::mutex_guard g(_nMutex);

  auto it = _socketResolvers.find(socket.protocol());
  if (it != _socketResolvers.end()) {
    it->second->bind(socket, buf, len);
  }
}

void NetworkManager::unbind(SocketDescriptor& socket) {
  upan::mutex_guard g(_nMutex);

  auto it = _socketResolvers.find(socket.protocol());
  if (it != _socketResolvers.end()) {
    it->second->unbind(socket);
  }
}

ssize_t NetworkManager::send(const uint8_t* buf, size_t n, int protocol, const struct sockaddr_in& srcAddr, const struct sockaddr& destAddr) {
  switch (protocol) {
    case IPPROTO_UDP:
      getDefaultDevice().value().getUDP4Handler().send(buf, n, srcAddr, reinterpret_cast<const struct sockaddr_in&>(destAddr));
      break;
    case IPPROTO_ICMP:
      getDefaultDevice().value().getICMPHandler().send(buf, n, srcAddr, reinterpret_cast<const struct sockaddr_in&>(destAddr));
      break;
    case ETH_P_ARP:
      getDefaultDevice().value().getARPHandler().send(buf, n, srcAddr, destAddr);
      break;
    default:
      throw upan::exception(XLOC, "packet send failed - unsupported protocol: %d", protocol);
  }
  //todo: handle partial send?
  return n;
}

void NetworkManager::recv(const upan::shared_ptr<RawNetPacket>& packet, int protocol) {
  auto it = _socketResolvers.find(protocol);
  if (it != _socketResolvers.end()) {
    it->second->resolve(packet).ifPresent([&packet](SocketDescriptor& socket) {
      socket.recvNotify(packet);
    });
  }
}