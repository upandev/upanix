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
#include <ConfigFileDB.h>
#include <result.h>
#include <DHCPMessage.h>

class NetworkDevice;

class DHCPClient : upan::thread {
private:
  static constexpr int DHCP_SERVER_PORT = 67;
  static constexpr int DHCP_CLIENT_PORT = 68;

  typedef enum {
    TIMEOUT,
    REJECTED,
    INVALID,
  } DHCPResponseErrorCode;

public:
  explicit DHCPClient(NetworkDevice& networkDevice);
  void run() override;

private:
  void dhcpDiscover();
  void dhcpRequest();
  void dhcpRenew();
  void dhcpRebind();

  upan::result<DHCPMessage> sendDiscover();
  upan::result<DHCPMessage> sendRequest();
  upan::result<DHCPMessage> sendRenew();

  void loadFromConfig();
  void updateFromDHCPResponse(const DHCPMessage& response);

private:
  static const upan::string CFG_DHCP_LEASE_TIME;
  static const upan::string CFG_DHCP_LEASE_RENEWAL_TIME;
  static const upan::string CFG_DHCP_LEASE_REBINDING_TIME;
  static const upan::string CFG_DHCP_SERVER_IP_ADDRESS;
  static const upan::string CFG_HOST_IP_ADDRESS;
  static const upan::string CFG_GATEWAY_IP_ADDRESS;
  static const upan::string CFG_BROADCAST_IP_ADDRESS;
  static const upan::string CFG_SUBNET_MASK;
  static const upan::string CFG_DNS_IP_ADDRESS;

  enum DHCPFlowState {
    FlowState_Discover,
    FlowState_Request,
    FlowState_Renew,
    FlowState_Rebind
  };

  NetworkDevice& _networkDevice;
  DHCPFlowState _flowState;
  in_addr_t _dhcpServerAddress;
  time_t _leaseTime;
  time_t _leaseRenewalTime;
  time_t _leaseRebindingTime;
  time_t _leaseExpiry;
  time_t _leaseRenewalExpiry;

  int _testRenewalCount;
  int _testRebindCount;

  upan::ConfigFileDB _config;
};