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
#include <RealNetworkDevice.h>

constexpr char RealNetworkDevice::DEFAULT_HOST_NAME[];

RealNetworkDevice::RealNetworkDevice(const PCIEntry& pciEntry)
  : NetworkDevice(0, "", false, INADDR_ANY, INADDR_ANY),
    _pciEntry(pciEntry), _ethernetHandler(*this), _arpHandler(*this) {
}

void RealNetworkDevice::connectToNetwork() {
  _dhcpClient.reset(new DHCPClient(*this));
  _dhcpClient->start();
}

void RealNetworkDevice::onConnected() {
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

void RealNetworkDevice::send(RawNetPacket& packet, ETH_PROTO_TYPE eType) {
  _ethernetHandler.send(packet, eType);
}

void RealNetworkDevice::print() const {
  NetworkDevice::print();
  printf("\n ether: %s", getMACAddress().str().c_str());
  printf("\n broadcast: %s, gateway: %s, ether(gateway): %s",
         upan::net::inet_ntostr(_broadcastAddress).c_str(),
         upan::net::inet_ntostr(_gatewayAddress).c_str(),
         _gatewayMacAddress.str().c_str());
  printf("\n dns: %s", upan::net::inet_ntostr(_dnsAddress).c_str());
}