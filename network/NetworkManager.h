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

#include <list.h>
#include <bitset.h>
#include <set.h>
#include <map.h>
#include <uniq_ptr.h>
#include <SocketDescriptor.h>
#include <SocketResolver.h>
#include <DNSClient.h>
#include <MACAddress.h>
#include <PCIBusHandler.h>

class IRQ;
class NetworkDevice;
class RealNetworkDevice;
class LoopbackNetworkDevice;

class NetworkManager
{
public:
  NetworkManager(const NetworkManager&) = delete;
  NetworkManager& operator=(const NetworkManager&) = delete;

private:
  NetworkManager();

public:
  static NetworkManager& Instance();

  void Initialize();
  upan::list<NetworkDevice*>& devices() { return _devices; }
  upan::option<RealNetworkDevice&> getDefaultRealDevice();
  upan::option<LoopbackNetworkDevice&> getLoopbackDevice();
  NetworkDevice& getDevice(const struct sockaddr_in& addr);
  upan::option<NetworkDevice&> getDeviceById(int);
  upan::option<NetworkDevice&> getDeviceByName(const upan::string&);

  void updateIPMACTable(in_addr_t ip, const MACAddress& mac);
  upan::option<const MACAddress&> lookupMAC(in_addr_t ip);

  void bind(SocketDescriptor& socket, const uint8_t* buf, size_t len);
  void unbind(SocketDescriptor& socket);

  ssize_t send(const uint8_t* buf, size_t n, int protocol, const struct sockaddr_in& srcAddr, const struct sockaddr& destAddr);
  void recv(const upan::shared_ptr<RawNetPacket>& packet, int protocol);

  DNSClient& getDNSClient() { return *_dnsClient; }

private:
  typedef upan::map<in_addr_t, MACAddress> IP_MAP_TABLE;
  typedef upan::map<int, SocketResolver*> SOCKET_RESOLVER_MAP;

  void Probe(const PCIEntry& pciEntry);

  upan::mutex _nMutex;
  int _interfaceId;
  IP_MAP_TABLE _ipMACTable;
  upan::list<NetworkDevice*> _devices;
  RealNetworkDevice* _defaultRealDevice;
  LoopbackNetworkDevice* _loopbackDevice;
  SOCKET_RESOLVER_MAP _socketResolvers;
  upan::uniq_ptr<DNSClient> _dnsClient;
};