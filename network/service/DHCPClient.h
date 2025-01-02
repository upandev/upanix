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

#include <ithread.h>
#include <net/socket.h>
#include <dtime.h>

class NetworkDevice;

class DHCPClient : upan::thread {
private:
  DHCPClient();

  static constexpr int DHCP_SERVER_PORT = 67;
  static constexpr int DHCP_CLIENT_PORT = 68;
  static constexpr uint32_t DHCP_MAGIC_COOKIE = 0x63538263;
  static constexpr int DHCP_MAX_OPTION_SIZE = 308;

  typedef enum {
    Discover = 1,
    Offer = 2,
  } DHCPMessageType;

  typedef enum {
    BootRequest = 1,
    BootResponse = 2,
  } DHCPOperationType;

  typedef enum {
    Ethernet = 1,
  } HardwareType;

  typedef enum {
    Unicast = 0x0,
    Broadcast = 0x0080, //this is network byte order
  } DHCPFlags;

  typedef enum {
    Param_SubnetMask = 1,
    Param_Router = 3,
    Param_DNS = 6,
    Param_DomainName = 15,
    Param_ClasslessStaticRoute = 121,
  } ParameterRequestListItem;

  typedef enum {
    SubnetMask = 1,
    Router = 3,
    DNS = 6,
    HostName = 12,
    BroadcastAddress = 28,
    LeaseTime = 51,
    LeaseRenewalTime = 58,
    LeaseRebindingTime = 59,
    MessageType = 53,
    DHCPServerIndentifier = 54,
    ParameterRequestList = 55,
    MaxMessageSize = 57,
    ClientIdentifier = 61,
    OptionEnd = 255
  } DHCPOptionType;

  struct Message {
    uint8_t _op;
    uint8_t _htype;
    uint8_t _hlen;
    uint8_t _hops;
    uint32_t _xid;
    uint16_t _secs;
    uint16_t _flags;
    in_addr_t _ciaddr;
    in_addr_t _yiaddr;
    in_addr_t _siaddr;
    in_addr_t _giaddr;
    uint8_t _chaddr[16];
    uint8_t _sname[64];
    uint8_t _file[128];
    uint32_t _magicCookie;
    uint8_t _options[DHCP_MAX_OPTION_SIZE];

    uint8_t* getOption(DHCPClient::DHCPOptionType optionType);
    DHCPClient::DHCPMessageType getMessageType();
    in_addr_t readIPAddress(DHCPClient::DHCPOptionType optionType);
    in_addr_t getSubnetMask();
    in_addr_t getBroadcastAddress();
    in_addr_t getDNSAddress();
    in_addr_t getRouterAddress();
    in_addr_t getDHCPServerAddress();

    void createDiscoverPacket(const NetworkDevice&);
    void processResponse(const DHCPClient::Message& request, NetworkDevice&, DHCPClient&);
  } PACKED;

public:
  static DHCPClient& Instance();
  void run() override;

private:
  void discover(NetworkDevice&);

private:
  in_addr_t _dhcpServerAddress;
  time_t _leaseTime;
  time_t _leaseRenewalTime;
  time_t _leaseRebindingTime;
};