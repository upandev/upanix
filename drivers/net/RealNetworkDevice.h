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

#include <uniq_ptr.h>
#include <NetworkDevice.h>
#include <DHCPClient.h>
#include <ARPClient.h>
#include <ARPHandler.h>
#include <EthernetHandler.h>

class SocketBuffer;
class PCIEntry;

class RealNetworkDevice : public NetworkDevice {
private:
  static constexpr char DEFAULT_HOST_NAME[] = "Upanix";

public:

  explicit RealNetworkDevice(const PCIEntry& pciEntry);
  ~RealNetworkDevice() override = default;

  virtual void sendPacket(const RawNetPacket& packet) = 0;
  void send(RawNetPacket& packet, ETH_PROTO_TYPE eType) override;
  void print() const override;

  uint32_t deviceLayerHeaderLen() const override { return getEthernetHandler().headerLen(); };
  const char* hostName() const override { return DEFAULT_HOST_NAME; }

  const MACAddress& getGatewayMACAddress() const { return _gatewayMacAddress; }
  in_addr_t getGatewayAddress() const { return _gatewayAddress; }
  in_addr_t getBroadcastAddress() const { return _broadcastAddress; }
  in_addr_t getDNSAddress() const { return _dnsAddress; }

  EthernetHandler& getEthernetHandler() { return _ethernetHandler; }
  const EthernetHandler& getEthernetHandler() const { return _ethernetHandler; }

  ARPHandler& getARPHandler() { return _arpHandler; }
  const ARPHandler& getARPHandler() const { return _arpHandler; }

  ARPClient& getARPClient() { return *_arpClient; }

protected:
  void setName(const upan::string& name) { _name = name; }
  void setId(int id) { _id = id; }

  void connectToNetwork();
  void onConnected();
  void setMACAddress(const MACAddress& macAddress) { _macAddress = macAddress; }
  void setIPAddress(in_addr_t ip) { _ipAddress = ip; }
  void setGatewayAddress(in_addr_t ip) { _gatewayAddress = ip; }
  void setSubnetMask(in_addr_t ip) { _subnetMask = ip; }
  void setBroadcastAddress(in_addr_t ip) { _broadcastAddress = ip; }
  void setDNSAddress(in_addr_t ip) { _dnsAddress = ip; }

  friend class DHCPClient;
  friend class NetworkManager;
protected:
  const PCIEntry& _pciEntry;

  MACAddress _gatewayMacAddress;
  in_addr_t _gatewayAddress;
  in_addr_t _broadcastAddress;
  in_addr_t _dnsAddress;

  EthernetHandler _ethernetHandler;
  ARPHandler _arpHandler;

  upan::uniq_ptr<DHCPClient> _dhcpClient;
  upan::uniq_ptr<ARPClient> _arpClient;
};
