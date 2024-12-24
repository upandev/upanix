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

bool NetworkManager::isPortBounded(in_addr_t ip, in_port_t port) {
  if (ip == INADDR_BROADCAST || ip == INADDR_LOOPBACK) {
    return _socketBindSet.exists(port);
  } else {
    return _socketBindMap[INADDR_BROADCAST].exists(port)
    || _socketBindMap[INADDR_LOOPBACK].exists(port)
    || _socketBindMap[ip].exists(port);
  }
}

void NetworkManager::bind(in_addr_t ip, in_port_t port, SocketDescriptor& socket) {
  upan::mutex_guard g(_nMutex);

  if (isPortBounded(ip, port)) {
    throw upan::exception(XLOC, "port %d is already bound", port);
  }
  _socketBindMap[ip][port] = &socket;
  _socketBindSet.insert(port);
}

void NetworkManager::send(const struct sockaddr_in& from, const struct sockaddr_in& to, const void* buf, socklen_t n) {
}