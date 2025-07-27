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

#include <ustring.h>
#include <option.h>
#include <NetworkUtil.h>
#include <RawNetPacket.h>
#include <IPV4Handler.h>
#include <TCPHandler.h>
#include <UDPHandler.h>
#include <ICMPHandler.h>
#include <MACAddress.h>

class SocketBuffer;

class NetworkDevice {
private:
  static constexpr uint16_t DEFAULT_MTU = 1500;

public:

  NetworkDevice(int id, const upan::string& name, bool connected, in_addr_t ipAddress, in_addr_t subnetMask);
  virtual ~NetworkDevice() = default;

  const upan::string& name() const { return _name; }
  int id() const { return _id; }
  bool isConnected() const { return _connected; }

  bool isSameSubnet(const in_addr_t& ip) const;
  virtual uint16_t mtu() const { return DEFAULT_MTU; }

  virtual void print() const;

  virtual void send(RawNetPacket& packet, ETH_PROTO_TYPE eType) = 0;
  virtual uint32_t deviceLayerHeaderLen() const = 0;
  virtual const char* hostName() const = 0;

  in_addr_t getIPAddress() const { return _ipAddress; }
  in_addr_t getSubnetMask() const { return _subnetMask; }
  const MACAddress& getMACAddress() const { return _macAddress; }

  IPV4Handler& getIPV4Handler() { return _ipv4Handler; }
  const IPV4Handler& getIPV4Handler() const { return _ipv4Handler; }

  TCPHandler& getTCPHandler() { return _tcpHandler; }
  const TCPHandler& getTCPHandler() const { return _tcpHandler; }

  UDPHandler& getUDPHandler() { return _udp4Handler; }
  const UDPHandler& getUDPHandler() const { return _udp4Handler; }

  ICMPHandler& getICMPHandler() { return _icmpHandler; }
  const ICMPHandler& getICMPHandler() const { return _icmpHandler; }

  friend class NetworkManager;

protected:
  int _id;
  upan::string _name;
  bool _connected;

  in_addr_t _ipAddress;
  in_addr_t _subnetMask;
  MACAddress _macAddress;

  IPV4Handler _ipv4Handler;
  TCPHandler _tcpHandler;
  UDPHandler _udp4Handler;
  ICMPHandler _icmpHandler;
};
