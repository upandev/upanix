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

#include <DHCPClient.h>
#include <NetworkManager.h>
#include <RealNetworkDevice.h>
#include <PIT.h>

static constexpr int MAX_BUFFER_SIZE = 1024;

const upan::string DHCPClient::CFG_DHCP_LEASE_TIME("DHCP_LEASE_TIME");
const upan::string DHCPClient::CFG_DHCP_LEASE_RENEWAL_TIME("DHCP_LEASE_RENEWAL_TIME");
const upan::string DHCPClient::CFG_DHCP_LEASE_REBINDING_TIME("DHCP_LEASE_REBINDING_TIME");
const upan::string DHCPClient::CFG_DHCP_SERVER_IP_ADDRESS("DHCP_SERVER_IP_ADDRESS");
const upan::string DHCPClient::CFG_HOST_IP_ADDRESS("HOST_IP_ADDRESS");
const upan::string DHCPClient::CFG_GATEWAY_IP_ADDRESS("GATEWAY_IP_ADDRESS");
const upan::string DHCPClient::CFG_BROADCAST_IP_ADDRESS("BROADCAST_IP_ADDRESS");
const upan::string DHCPClient::CFG_SUBNET_MASK("SUBNET_MASK");
const upan::string DHCPClient::CFG_DNS_IP_ADDRESS("DNS_IP_ADDRESS");

DHCPClient::DHCPClient(RealNetworkDevice& networkDevice) :
  _networkDevice(networkDevice),
  _flowState(FlowState_Discover),
  _dhcpServerAddress(INADDR_NONE),
  _leaseTime(0), _leaseRenewalTime(0), _leaseRebindingTime(0), _leaseExpiry(0), _leaseRenewalExpiry(0),
  _testRenewalCount(0), _testRebindCount(0),
  _config("/var/db/dhcpclient.cfg", upan::ConfigFileDB::OpType::RDWR) {
  loadFromConfig();
}

void DHCPClient::loadFromConfig() {
  _config.get(CFG_DHCP_LEASE_TIME).ifPresent([&](const upan::string& val) { _leaseTime = atol(val.c_str()); });
  _config.get(CFG_DHCP_LEASE_RENEWAL_TIME).ifPresent([&](const upan::string& val) { _leaseRenewalTime = atol(val.c_str()); });
  _config.get(CFG_DHCP_LEASE_REBINDING_TIME).ifPresent([&](const upan::string& val) { _leaseRebindingTime = atol(val.c_str()); });

  _config.get(CFG_DHCP_SERVER_IP_ADDRESS).ifPresent([&](const upan::string& val) { _dhcpServerAddress = upan::net::inet_strton(val); });

  _config.get(CFG_HOST_IP_ADDRESS).ifPresent([&](const upan::string& val) { _networkDevice.setIPAddress(upan::net::inet_strton(val)); });
  _config.get(CFG_GATEWAY_IP_ADDRESS).ifPresent([&](const upan::string& val) { _networkDevice.setGatewayAddress(upan::net::inet_strton(val)); });
  _config.get(CFG_BROADCAST_IP_ADDRESS).ifPresent([&](const upan::string& val) { _networkDevice.setBroadcastAddress(upan::net::inet_strton(val)); });
  _config.get(CFG_SUBNET_MASK).ifPresent([&](const upan::string& val) { _networkDevice.setSubnetMask(upan::net::inet_strton(val)); });
  _config.get(CFG_DNS_IP_ADDRESS).ifPresent([&](const upan::string& val) { _networkDevice.setDNSAddress(upan::net::inet_strton(val)); });

  if (_dhcpServerAddress == INADDR_NONE) {
    _dhcpServerAddress = _networkDevice.getGatewayAddress();
  }

  if (_leaseRenewalTime == 0) {
    _leaseRenewalTime = _leaseTime / 2;
    _leaseRebindingTime = _leaseTime / 2;
  }

  KLog::info("DHCP config loaded");
  KLog::info("Lease Time: %u, Renewal Time: %u, Rebinding Time: %u", _leaseTime, _leaseRenewalTime, _leaseRebindingTime);
  KLog::info("IP: %s, Gateway: %s, Subnet Mask: %s",
             upan::net::inet_ntostr(_networkDevice.getIPAddress()).c_str(),
             upan::net::inet_ntostr(_networkDevice.getGatewayAddress()).c_str(),
             upan::net::inet_ntostr(_networkDevice.getSubnetMask()).c_str());
  KLog::info("Broadcast: %s, DNS: %s",
             upan::net::inet_ntostr(_networkDevice.getBroadcastAddress()).c_str(),
             upan::net::inet_ntostr(_networkDevice.getDNSAddress()).c_str());
}

void DHCPClient::updateFromDHCPResponse(const DHCPMessage& response) {
  {
    upan::ConfigFileDB::BatchWriteGuard g(_config);
    _config.set(CFG_DHCP_LEASE_TIME, upan::string::to_string(response.getLeaseTime()), "");
    _config.set(CFG_DHCP_LEASE_RENEWAL_TIME, upan::string::to_string(response.getLeaseRenewalTime()), "");
    _config.set(CFG_DHCP_LEASE_REBINDING_TIME, upan::string::to_string(response.getLeaseRebindingTime()), "");

    _config.set(CFG_HOST_IP_ADDRESS, upan::net::inet_ntostr(response.getYourIPAddress()), "");
    _config.set(CFG_GATEWAY_IP_ADDRESS, upan::net::inet_ntostr(response.getRouterAddress()), "");
    _config.set(CFG_BROADCAST_IP_ADDRESS, upan::net::inet_ntostr(response.getBroadcastAddress()), "");
    _config.set(CFG_SUBNET_MASK, upan::net::inet_ntostr(response.getSubnetMask()), "");
    _config.set(CFG_DNS_IP_ADDRESS, upan::net::inet_ntostr(response.getDNSAddress()), "");
    _config.set(CFG_DHCP_SERVER_IP_ADDRESS, upan::net::inet_ntostr(response.getDHCPServerAddress()), "");
  }

  KLog::info("DHCP config updated");
  loadFromConfig();
  _networkDevice.onConnected();

  const time_t curTime = btime() / 1000;
  _leaseExpiry = curTime + _leaseTime;
  _leaseRenewalExpiry = curTime + _leaseRebindingTime;
}

void DHCPClient::run() {
  KLog::info("DHCP service started");
  if (_networkDevice.getIPAddress() == INADDR_ANY) {
    _flowState = FlowState_Discover;
  } else {
    _flowState = FlowState_Request;
  }

  try {
    while (is_active()) {
      if (state() == running) {
        KLog::info("processing flow-state: %d", _flowState);
        switch (_flowState) {
          case FlowState_Discover:
            dhcpDiscover();
            break;

          case FlowState_Request:
            dhcpRequest();
            break;

          case FlowState_Renew:
            dhcpRenew();
            break;

          case FlowState_Rebind:
            dhcpRebind();
            break;
        }
      }
    }
  } catch(const upan::exception& e) {
    KLog::error("DHCP ERROR: %s", e.ErrorMsg().c_str());
  }
}

//send discover
//On failure of no response, re-try once every minute
//On NAK, re-try once every minute
//On Offer, update the IP details and go-to wait until renewal time
void DHCPClient::dhcpDiscover() {
  KLog::info("sending DHCP discover");
  const auto& result = sendDiscover();
  if (result.isBad()) {
    KLog::error(result.badValue().Msg().c_str());
    sleep(60); //re-try every minute
    _flowState = FlowState_Discover;
  } else {
    KLog::info("DHCP discover completed");
    updateFromDHCPResponse(result.goodValue());
    sleep(_leaseRenewalTime); //sleep for renewal time
    _flowState = FlowState_Renew;
  }
}

//send request
//On failure of no response, re-try once every minute
//On NAK, go-to discover loop
//On ACK, update the IP details and go-to wait until renewal time
void DHCPClient::dhcpRequest() {
  KLog::info("sending DHCP request");
  const auto& result = sendRequest();
  if (result.isBad()) {
    KLog::error(result.badValue().Msg().c_str());
    if (result.badValue().Val() == DHCPResponseErrorCode::REJECTED) {
      _flowState = FlowState_Discover;
    } else {
      sleep(60); //re-try every minute
      _flowState = FlowState_Request;
    }
  } else {
    KLog::info("DHCP request completed");
    updateFromDHCPResponse(result.goodValue());
    sleep(_leaseRenewalTime); //sleep for renewal time
    _flowState = FlowState_Renew;
  }
}

//send request
//On failure of no response, re-try 1/10th the (time of rebind - renewal)
//On NAK, re-try 1/10th the time of (rebind - renewal)
//On ACK, update the IP details and go-to wait until renewal time
void DHCPClient::dhcpRenew() {
  KLog::info("sending DHCP renew");
  const auto& result = _testRenewalCount >= 1 ? upan::error("force renewal failure") : sendRenew();
  if (result.isBad()) {
    KLog::error(result.badValue().Msg().c_str());
    time_t sleepTime = (_leaseRebindingTime - _leaseRenewalTime) / 10;
    sleepTime = sleepTime > 0 ? sleepTime : 60;
    sleep(sleepTime); //sleep for 1/10th the time interval between rebind - renewal lease time
    if (btime() < _leaseRenewalExpiry) { //still before rebind but after renewal
      _flowState = FlowState_Renew;
    } else { //if rebind period
      _flowState = FlowState_Rebind;
    }
  } else {
    KLog::info("DHCP renew completed");
    updateFromDHCPResponse(result.goodValue());
    sleep(_leaseRenewalTime); //sleep for renewal time
    _flowState = FlowState_Renew;
    //_testRenewalCount++;
  }
}

//If breach the rebind time, then send broadcast
//On failure or NAK, re-try every minute
//On ACK, update the IP details and go-to wait until renewal time
void DHCPClient::dhcpRebind() {
  KLog::info("sending DHCP rebind");
  const auto& result = _testRebindCount >= 1 ? upan::error("force rebind failure") : sendRequest();
  if (result.isBad()) {
    KLog::error(result.badValue().Msg().c_str());
    sleep(60); //re-try every minute
    if (btime() < _leaseExpiry) { //still before expiry
      _flowState = FlowState_Rebind;
    } else {
      _flowState = FlowState_Discover;
    }
  } else {
    KLog::info("DHCP rebind completed");
    updateFromDHCPResponse(result.goodValue());
    sleep(_leaseRenewalTime); //sleep for renewal time
    _flowState = FlowState_Renew;
    //_testRebindCount++;
  }
}

upan::result<DHCPMessage> DHCPClient::sendDiscover() {
  const auto sd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
  if (sd < 0) {
    throw upan::exception(XLOC, "socket creation failed");
  }

  // Bind the socket to the DHCP client port
  struct sockaddr_in client_addr {};
  memset(&client_addr, 0, sizeof(client_addr));
  client_addr.sin_family = AF_INET;
  client_addr.sin_port = htons(DHCP_CLIENT_PORT);
  client_addr.sin_addr.s_addr = INADDR_ANY;

  if (bind(sd, (struct sockaddr *)&client_addr, sizeof(client_addr)) < 0) {
    close(sd);
    throw upan::exception(XLOC, "failed to bind socket");
  }

  struct sockaddr_in server_addr {};
  // Configure the DHCP server address
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(DHCP_SERVER_PORT);
  server_addr.sin_addr.s_addr = INADDR_BROADCAST;

  //allow socket to broadcast
  int broadcast = 1;
  if (setsockopt(sd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) < 0) {
    close(sd);
    throw upan::exception(XLOC, "failed to set socket option: SO_BROADCAST");
  }

  struct timeval timeout {};
  timeout.tv_sec = 10;
  if (setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
    close(sd);
    throw upan::exception(XLOC, "failed to set socket option: SO_RCVTIMEO");
  }

  //create and send DHCP Discover
  DHCPMessage dhcpDiscover {};
  dhcpDiscover.createDiscoverPacket(_networkDevice);
  ssize_t len;

  len = sendto(sd, &dhcpDiscover, sizeof(dhcpDiscover), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
  if (len < 0) {
    close(sd);
    throw upan::exception(XLOC, "failed to sendPacket DHCP Discover");
  }

  char buffer[MAX_BUFFER_SIZE];
  // Receive DHCP Offer
  len = recvfrom(sd, buffer, MAX_BUFFER_SIZE, 0, nullptr, nullptr);
  if (len < 0) {
    close(sd);
    return { upan::error(DHCPResponseErrorCode::TIMEOUT, "failed to receive DHCP offer") };
  }

  close(sd);

  DHCPMessage dhcpResponse {};
  memcpy(&dhcpResponse, buffer, sizeof(dhcpResponse));
  switch(dhcpResponse.getMessageType()) {
    case DHCPMessage::DHCPMessageType::Offer: {
      try {
        dhcpResponse.validateResponse(dhcpDiscover, _networkDevice);
        return {dhcpResponse};
      } catch (const upan::exception& e) {
        return { upan::error(DHCPResponseErrorCode::INVALID, e.ErrorMsg()) };
      }
    }

    case DHCPMessage::DHCPMessageType::NAK:
    case DHCPMessage::DHCPMessageType::Decline:
      return { upan::error(DHCPResponseErrorCode::REJECTED, dhcpResponse.getMessageText() )};

    default:
      return { upan::error(DHCPResponseErrorCode::INVALID, "invalid DHCP response - of message-type: %d", dhcpResponse.getMessageType()) };
  }
}

upan::result<DHCPMessage> DHCPClient::sendRequest() {
  const auto sd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
  if (sd < 0) {
    throw upan::exception(XLOC, "socket creation failed");
  }

  // Bind the socket to the DHCP client port
  struct sockaddr_in client_addr {};
  memset(&client_addr, 0, sizeof(client_addr));
  client_addr.sin_family = AF_INET;
  client_addr.sin_port = htons(DHCP_CLIENT_PORT);
  client_addr.sin_addr.s_addr = INADDR_ANY;

  if (bind(sd, (struct sockaddr *)&client_addr, sizeof(client_addr)) < 0) {
    close(sd);
    throw upan::exception(XLOC, "failed to bind socket");
  }

  struct sockaddr_in server_addr {};
  // Configure the DHCP server address
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(DHCP_SERVER_PORT);
  server_addr.sin_addr.s_addr = INADDR_BROADCAST;

  //allow socket to broadcast
  int broadcast = 1;
  if (setsockopt(sd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) < 0) {
    close(sd);
    throw upan::exception(XLOC, "failed to set socket option: SO_BROADCAST");
  }

  struct timeval timeout {};
  timeout.tv_sec = 10;
  if (setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
    close(sd);
    throw upan::exception(XLOC, "failed to set socket option: SO_RCVTIMEO");
  }

  //create and send DHCP Discover
  DHCPMessage dhcpRequest {};
  dhcpRequest.createRequestPacket(_networkDevice);
  ssize_t len;

  len = sendto(sd, &dhcpRequest, sizeof(dhcpRequest), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
  if (len < 0) {
    close(sd);
    throw upan::exception(XLOC, "failed to sendPacket DHCP Discover");
  }

  char buffer[MAX_BUFFER_SIZE];
  // Receive DHCP Offer
  len = recvfrom(sd, buffer, MAX_BUFFER_SIZE, 0, nullptr, nullptr);
  if (len < 0) {
    close(sd);
    return { upan::error(DHCPResponseErrorCode::TIMEOUT, "failed to receive DHCP ACK") };
  }

  close(sd);

  DHCPMessage dhcpResponse {};
  memcpy(&dhcpResponse, buffer, sizeof(dhcpResponse));
  switch(dhcpResponse.getMessageType()) {
    case DHCPMessage::DHCPMessageType::ACK: {
      try {
        dhcpResponse.validateResponse(dhcpRequest, _networkDevice);
        return {dhcpResponse};
      } catch (const upan::exception& e) {
        return { upan::error(DHCPResponseErrorCode::INVALID, e.ErrorMsg()) };
      }
    }

    case DHCPMessage::DHCPMessageType::NAK:
    case DHCPMessage::DHCPMessageType::Decline:
      return { upan::error(DHCPResponseErrorCode::REJECTED, dhcpResponse.getMessageText() )};

    default:
      return { upan::error(DHCPResponseErrorCode::INVALID, "invalid DHCP response - of message-type: %d", dhcpResponse.getMessageType()) };
  }
}

upan::result<DHCPMessage> DHCPClient::sendRenew() {
  const auto sd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
  if (sd < 0) {
    throw upan::exception(XLOC, "socket creation failed");
  }

  // Bind the socket to the DHCP client port
  struct sockaddr_in client_addr {};
  memset(&client_addr, 0, sizeof(client_addr));
  client_addr.sin_family = AF_INET;
  client_addr.sin_port = htons(DHCP_CLIENT_PORT);
  client_addr.sin_addr.s_addr = INADDR_ANY;

  if (bind(sd, (struct sockaddr *)&client_addr, sizeof(client_addr)) < 0) {
    close(sd);
    throw upan::exception(XLOC, "failed to bind socket");
  }

  struct sockaddr_in server_addr {};
  // Configure the DHCP server address
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(DHCP_SERVER_PORT);
  server_addr.sin_addr.s_addr = _dhcpServerAddress;

  struct timeval timeout {};
  timeout.tv_sec = 10;
  if (setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
    close(sd);
    throw upan::exception(XLOC, "failed to set socket option: SO_RCVTIMEO");
  }

  //create and send DHCP Discover
  DHCPMessage dhcpRequest {};
  dhcpRequest.createRenewPacket(_networkDevice, _dhcpServerAddress);
  ssize_t len;

  len = sendto(sd, &dhcpRequest, sizeof(dhcpRequest), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
  if (len < 0) {
    close(sd);
    throw upan::exception(XLOC, "failed to sendPacket DHCP Discover");
  }

  char buffer[MAX_BUFFER_SIZE];
  // Receive DHCP Offer
  len = recvfrom(sd, buffer, MAX_BUFFER_SIZE, 0, nullptr, nullptr);
  if (len < 0) {
    close(sd);
    return { upan::error(DHCPResponseErrorCode::TIMEOUT, "failed to receive DHCP ACK") };
  }

  close(sd);

  DHCPMessage dhcpResponse {};
  memcpy(&dhcpResponse, buffer, sizeof(dhcpResponse));
  switch(dhcpResponse.getMessageType()) {
    case DHCPMessage::DHCPMessageType::ACK: {
      try {
        dhcpResponse.validateResponse(dhcpRequest, _networkDevice);
        return {dhcpResponse};
      } catch (const upan::exception& e) {
        return { upan::error(DHCPResponseErrorCode::INVALID, e.ErrorMsg()) };
      }
    }

    case DHCPMessage::DHCPMessageType::NAK:
    case DHCPMessage::DHCPMessageType::Decline:
      return { upan::error(DHCPResponseErrorCode::REJECTED, dhcpResponse.getMessageText() )};

    default:
      return { upan::error(DHCPResponseErrorCode::INVALID, "invalid DHCP response - of message-type: %d", dhcpResponse.getMessageType()) };
  }
}