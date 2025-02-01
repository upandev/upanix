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
#include <NetworkDevice.h>
#include <bitset.h>
#include <set.h>
#include <map.h>
#include <uniq_ptr.h>
#include <SocketDescriptor.h>

class IRQ;

class NetworkManager
{
private:
  NetworkManager() ;
  NetworkManager(const NetworkManager&) = delete;
  NetworkManager& operator=(const NetworkManager&) = delete;

public:
  static NetworkManager& Instance()
  {
    static NetworkManager instance;
    return instance;
  }

  void Initialize();
  upan::list<NetworkDevice*>& Devices() { return _devices; }
  upan::option<NetworkDevice&> getDefaultDevice();

  uint16_t allocatePort();
  bool isPortAllocated(uint16_t port) const;
  void releasePort(in_port_t port);
  void updateIPMACTable(const RawNetPacket&);
  upan::option<MACAddress> lookupMAC(in_addr_t ip);

  void bind(in_addr_t ip, in_port_t port, SocketDescriptor& socket);
  void unbind(in_addr_t ip, in_port_t);
  void send(const uint8_t* buf, size_t n, IPPROTO_TYPE protocol, const struct sockaddr_in& srcAddr, const struct sockaddr_in& destAddr);
  void recv(const upan::shared_ptr<RawNetPacket>& packet, const struct sockaddr_in& destAddr);
private:
  typedef upan::map<in_port_t, SocketDescriptor*> SOCKET_PORT_MAP;
  typedef upan::map<in_addr_t, SOCKET_PORT_MAP> SOCKET_BIND_MAP;
  typedef upan::map<in_port_t, int> SOCKET_BIND_SET;
  typedef upan::map<in_port_t, MACAddress> IP_MAP_TABLE;

  void Probe(const PCIEntry& pciEntry);
  bool isPortBounded(in_addr_t ip, in_port_t port);
  upan::option<SocketDescriptor*> findBindingSocket(in_addr_t addr, in_port_t port);

  upan::list<NetworkDevice*> _devices;
  upan::mutex _nMutex;
  upan::bitset<UINT16_MAX + 1> _portPool;
  SOCKET_BIND_SET _socketBindSet;
  SOCKET_BIND_MAP _socketBindMap;
  IP_MAP_TABLE _ipMACTable;
};