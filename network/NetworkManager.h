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

#include <bitset.h>
#include <set.h>
#include <map.h>
#include <uniq_ptr.h>
#include <ithread.h>
#include <SocketDescriptor.h>
#include <TCPSocketResolver.h>
#include <DNSClient.h>
#include <MACAddress.h>
#include <PCIBusHandler.h>
#include <TCPSocketResolver.h>
#include <UDPSocketResolver.h>
#include <ICMPSocketResolver.h>
#include <ARPSocketResolver.h>
#include <TCPSocketResolver.h>
#include <TCPStreamWorker.h>

class IRQ;
class NetworkDevice;
class RealNetworkDevice;
class LoopbackNetworkDevice;

class NetworkManager {
public:
  typedef upan::map<in_addr_t, MACAddress> IP_MAP_TABLE;

  NetworkManager(const NetworkManager&) = delete;
  NetworkManager& operator=(const NetworkManager&) = delete;

private:
  NetworkManager();

public:
  static NetworkManager& Instance();

  void Initialize();
  void addTCPConnection(upan::shared_ptr<TCPConnection>& connection);

  upan::list<NetworkDevice*>& devices() { return _devices; }
  upan::option<RealNetworkDevice&> getDefaultRealDevice();
  upan::option<LoopbackNetworkDevice&> getLoopbackDevice();
  NetworkDevice& getDevice(const struct sockaddr_in& addr, bool isDestination);
  upan::option<NetworkDevice&> getDeviceById(int);
  upan::option<NetworkDevice&> getDeviceByName(const upan::string&);

  void updateIPMACTable(in_addr_t ip, const MACAddress& mac);
  upan::option<const MACAddress&> lookupMAC(in_addr_t ip);
  const IP_MAP_TABLE& getIPMACTable() { return _ipMACTable; }

  DNSClient& getDNSClient() { return *_dnsClient; }

  class PortPool {
  public:
    uint16_t allocate();
    void allocate(in_port_t port);
    void release(in_port_t port);

  private:
    upan::mutex _mutex;
    upan::bitset<UINT16_MAX + 1> _portPool;
  };

  PortPool& getUDPPortPool() { return _udpPortPool; }
  PortPool& getTCPPortPool() { return _tcpPortPool; }

  TCPSocketResolver& getTCPSocketResolver() { return _tcpSocketResolver; }
  UDPSocketResolver& getUDPSocketResolver() { return _udpSocketResolver; }
  ICMPSocketResolver& getICMPSocketResolver() { return _icmpSocketResolver; }
  ARPSocketResolver& getARPSocketResolver() { return _arpSocketResolver; }

private:
  void Probe(const PCIEntry& pciEntry);

  upan::mutex _nMutex;
  int _interfaceId;
  IP_MAP_TABLE _ipMACTable;
  upan::list<NetworkDevice*> _devices;
  RealNetworkDevice* _defaultRealDevice;
  LoopbackNetworkDevice* _loopbackDevice;

  upan::uniq_ptr<DNSClient> _dnsClient;
  PortPool _udpPortPool;
  PortPool _tcpPortPool;

  TCPSocketResolver _tcpSocketResolver;
  UDPSocketResolver _udpSocketResolver;
  ICMPSocketResolver _icmpSocketResolver;
  ARPSocketResolver _arpSocketResolver;

  TCPStreamWorker _tcpStreamWorker;
};