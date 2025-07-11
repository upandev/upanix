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
#include <unet.h>
#include <PCIBusHandler.h>
#include <NetworkDevice.h>

constexpr char NetworkDevice::DEFAULT_HOST_NAME[];

NetworkDevice::NetworkDevice(const PCIEntry& pciEntry)
  : _pciEntry(pciEntry), _id(0), _connected(false), _macAddress(nullptr), _ipAddress(INADDR_ANY),
    _ethernetHandler(*this), _ipv4Handler(*this), _udp4Handler(*this), _icmpHandler(*this), _arpHandler(*this) {
}

NetworkDevice::~NetworkDevice() {
}

void NetworkDevice::connectToNetwork() {
  _dhcpClient.reset(new DHCPClient(*this));
  _dhcpClient->start();
}

void NetworkDevice::onConnected() {
  KLog::info("DHCP completed. IP address: %s, Gateway address: %s",
         upan::net::inet_ntostr(_ipAddress).c_str(),
         upan::net::inet_ntostr(_gatewayAddress).c_str());
  try {
    if (_arpClient.isEmpty()) {
      _arpClient.reset(new ARPClient(*this));
    }
    _gatewayMacAddress = _arpClient->resolveMacAddress(_gatewayAddress);
    _connected = true;
    KLog::info("Network device connected. Gateway MAC address: %s", _gatewayMacAddress.str().c_str());
  } catch(const upan::exception& e) {
    KLog::error("ARP ERROR: %s", e.ErrorMsg().c_str());
  }
}

bool NetworkDevice::isSameSubnet(const in_addr_t& ip) const {
  return (_ipAddress & _subnetMask) == (ip & _subnetMask);
}
