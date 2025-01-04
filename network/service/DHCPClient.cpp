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
#include <NetworkDevice.h>
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

DHCPClient::DHCPClient(NetworkDevice& networkDevice) :
  _networkDevice(networkDevice),
  _dhcpServerAddress(INADDR_NONE),
  _leaseTime(0), _leaseRenewalTime(0), _leaseRebindingTime(0),
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
    _dhcpServerAddress = _networkDevice.GetGatewayAddress();
  }
}

void DHCPClient::updateFromDHCPResponse(const DHCPClient::Message& response) {
  {
    upan::ConfigFileDB::BatchWriteGuard g(_config);
    _config.set(CFG_DHCP_LEASE_TIME, upan::string::to_string(response.getLeaseTime()), "");
    _config.set(CFG_DHCP_LEASE_RENEWAL_TIME, upan::string::to_string(response.getLeaseRenewalTime()), "");
    _config.set(CFG_DHCP_LEASE_REBINDING_TIME, upan::string::to_string(response.getLeaseRebindingTime()), "");

    _config.set(CFG_HOST_IP_ADDRESS, upan::net::inet_ntostr(response._yiaddr), "");
    _config.set(CFG_GATEWAY_IP_ADDRESS, upan::net::inet_ntostr(response.getRouterAddress()), "");
    _config.set(CFG_BROADCAST_IP_ADDRESS, upan::net::inet_ntostr(response.getBroadcastAddress()), "");
    _config.set(CFG_SUBNET_MASK, upan::net::inet_ntostr(response.getSubnetMask()), "");
    _config.set(CFG_DNS_IP_ADDRESS, upan::net::inet_ntostr(response.getDNSAddress()), "");
    _config.set(CFG_DHCP_SERVER_IP_ADDRESS, upan::net::inet_ntostr(response.getDHCPServerAddress()), "");
  }

  loadFromConfig();
}

void DHCPClient::run() {
  while(is_active()) {
    if (state() == running) {
      try {
        //Load details from dhcp.info
        //If it is the first time after boot and IP Address is present from dhcp.info
          //send request
          //On failure of no response, re-try once every minute
          //On NAK, go-to discover loop
          //On ACK, update the IP details and go-to wait until renewal time
        //If there is no IP then
          //send discover
          //On failure of no response, re-try once every minute
          //On NAK, re-try once every minute
          //On Offer, update the IP details and go-to wait until renewal time
        //** wait until renewal time
        //after this wait, send request
        //On failure of no response, re-try 1/10th the (time of rebind - renewal)
        //On NAK, re-try 1/10th the time of (rebind - renewal)
          //If breach the rebind time, then send broadcast
          //On failure or NAK, re-try every minute
          //On ACK, update the IP details and go-to wait until renewal time
        //On ACK, update the IP details and go-to wait until renewal time

        if (_networkDevice.GetIPAddress() == INADDR_NONE) {
          _leaseTime = 0;
        }

        const time_t currentTime = PIT::Instance().GetCurrentTimeFromBoot();
        if (currentTime >= _leaseTime) {
          discover();
          const auto sleepDuration = _leaseRenewalTime - currentTime;
          if (sleepDuration > 0) {
            sleepms(sleepDuration);
          }
        } else if (currentTime >= _leaseRenewalTime) {
          //request to extend with DHCP server
        } else if (currentTime >= _leaseRebindingTime) {
          //request to extend with broadcast
        } else {
          const auto sleepDuration = _leaseRenewalTime - currentTime;
          if (sleepDuration > 0) {
            sleep(sleepDuration);
          }
          sleep(60);
        }
        //Last known IP
        //Request specific IP
        //Update records
        //Track Lease
        //Extend Lease

      } catch (const upan::exception& e) {
        e.Print();
        sleep(60);
      }
    }
  }

  //Release IP on Shutdown
}

void DHCPClient::Message::createDiscoverPacket(const NetworkDevice& networkDevice) {
  memset(this, 0, sizeof(DHCPClient::Message));

  _op = DHCPOperationType::BootRequest;
  _htype = HardwareType::Ethernet;
  _hlen = INADDR_MAC_LEN;
  _xid = htonl(rand());
  _flags = DHCPFlags::Broadcast;
  memcpy(_chaddr, networkDevice.GetMACAddress().get(), INADDR_MAC_LEN);

  // Add DHCP options
  _magicCookie = DHCP_MAGIC_COOKIE;

  int oi = 0;
  _options[oi++] = DHCPOptionType::MessageType; // Option: DHCP Message Type
  _options[oi++] = 1;  // Length
  _options[oi++] = DHCPMessageType::Discover; // DHCP Discover

  _options[oi++] = DHCPOptionType::ParameterRequestList;
  _options[oi++] = 5;
  _options[oi++] = ParameterRequestListItem::Param_SubnetMask;
  _options[oi++] = ParameterRequestListItem::Param_Router;
  _options[oi++] = ParameterRequestListItem::Param_DNS;
  _options[oi++] = ParameterRequestListItem::Param_DomainName;
  _options[oi++] = ParameterRequestListItem::Param_ClasslessStaticRoute;

  _options[oi++] = DHCPOptionType::MaxMessageSize;
  _options[oi++] = 2;
  auto mtu = htons(networkDevice.mtu());
  memcpy(_options + oi, (void*)&mtu, sizeof(uint16_t));
  oi += 2;

  _options[oi++] = DHCPOptionType::ClientIdentifier;
  _options[oi++] = 7;
  _options[oi++] = HardwareType::Ethernet;
  memcpy(_options + oi, networkDevice.GetMACAddress().get(), INADDR_MAC_LEN);
  oi += 6;

  _options[oi++] = DHCPOptionType::LeaseTime;
  _options[oi++] = 4;
  uint32_t leaseTime = ntohl(3600); //1hr
  memcpy(_options + oi, (void*)&leaseTime, sizeof(uint32_t));
  oi += 4;

  _options[oi++] = DHCPOptionType::HostName;
  const int l = strlen(networkDevice.hostName());
  _options[oi++] = l;
  memcpy(_options + oi, networkDevice.hostName(), l);
  oi += l;

  _options[oi] = DHCPOptionType::OptionEnd;
}

const uint8_t* DHCPClient::Message::getOption(DHCPClient::DHCPOptionType optionType) const {
  for(int i = 0; i < DHCP_MAX_OPTION_SIZE; ++i) {
    if (_options[i] == DHCPOptionType::OptionEnd) {
      return nullptr;
    }

    if (_options[i] == optionType) {
      if (i + 1 >= DHCP_MAX_OPTION_SIZE) {
        throw upan::exception(XLOC, "incomplete option: %d, missing len", optionType);
      }

      const auto len = _options[i+1];
      if (i + 1 + len >= DHCP_MAX_OPTION_SIZE) {
        throw upan::exception(XLOC, "incomplete option: %d", optionType);
      }

      return _options + i + 1;
    }
  }

  throw upan::exception(XLOC, "invalid option - no option-end found");
}

DHCPClient::DHCPMessageType DHCPClient::Message::getMessageType() const {
  auto option = getOption(DHCPOptionType::MessageType);
  if (!option) {
    throw upan::exception(XLOC, "DHCP Message Type option not found");
  }

  const auto len = option[0];
  if (len != 1) {
    throw upan::exception(XLOC, "invalid DHCP message type option length: %d (expected 1)", len);
  }

  const auto messageType = option[1];
  switch(messageType) {
    case DHCPMessageType::Discover:
    case DHCPMessageType::Offer:
      return (DHCPMessageType)messageType;

    default:
      throw upan::exception(XLOC, "unsupported DHCP message type %d", messageType);
  }
}

in_addr_t DHCPClient::Message::readIPAddress(DHCPClient::DHCPOptionType optionType) const {
  auto option = getOption(optionType);
  if (option) {
    const auto len = option[0];
    if (len != sizeof(in_addr_t)) {
      throw upan::exception(XLOC, "invalid IP address len: %d", len);
    }
    return *reinterpret_cast<const in_addr_t*>(option + 1);
  }
  return INADDR_NONE;
}

in_addr_t DHCPClient::Message::getSubnetMask() const {
  return readIPAddress(DHCPOptionType::SubnetMask);
}

in_addr_t DHCPClient::Message::getBroadcastAddress() const {
  return readIPAddress(DHCPOptionType::BroadcastAddress);
}

in_addr_t DHCPClient::Message::getDNSAddress() const {
  return readIPAddress(DHCPOptionType::DNS);
}

in_addr_t DHCPClient::Message::getRouterAddress() const {
  return readIPAddress(DHCPOptionType::Router);
}

in_addr_t DHCPClient::Message::getDHCPServerAddress() const {
  return readIPAddress(DHCPOptionType::DHCPServerIndentifier);
}

time_t DHCPClient::Message::readTime(DHCPClient::DHCPOptionType optionType) const {
  auto option = getOption(optionType);
  if (option) {
    const auto len = option[0];
    if (len != sizeof(uint32_t)) {
      throw upan::exception(XLOC, "invalid time len: %d", len);
    }
    return ntohl(*reinterpret_cast<const uint32_t*>(option + 1));
  }
  return 0;
}

time_t DHCPClient::Message::getLeaseTime() const {
  return readTime(DHCPOptionType::LeaseTime);
}

time_t DHCPClient::Message::getLeaseRenewalTime() const {
  return readTime(DHCPOptionType::LeaseRenewalTime);
}

time_t DHCPClient::Message::getLeaseRebindingTime() const {
  return readTime(DHCPOptionType::LeaseRebindingTime);
}

void DHCPClient::Message::processResponse(const DHCPClient::Message& request, DHCPClient& dhcpClient) {
  if (_op != DHCPOperationType::BootResponse) {
    throw upan::exception(XLOC, "unsupported DHCP message type: %d", _op);
  }

  if (_htype != HardwareType::Ethernet) {
    throw upan::exception(XLOC, "unsupported hardware type: %d", _htype);
  }

  if (_hlen != INADDR_MAC_LEN) {
    throw upan::exception(XLOC, "unsupported hardware address length: %d", _hlen);
  }

  if (_xid != request._xid) {
    throw upan::exception(XLOC, "transaction-id mismatch. Response xid: 0x%x, Request xid: 0x%x", _xid, request._xid);
  }

  if (_magicCookie != DHCP_MAGIC_COOKIE) {
    throw upan::exception(XLOC, "invalid DHCP magic cookie - 0x%x", _magicCookie);
  }

  const MACAddress clientMacAddress(_chaddr);
  if (clientMacAddress != dhcpClient._networkDevice.GetMACAddress()) {
    throw upan::exception(XLOC, "MAC address mismatch. Response: %s", clientMacAddress.str().c_str());
  }

  const auto messageType = getMessageType();
  if (messageType == DHCPMessageType::Offer) {
    dhcpClient.updateFromDHCPResponse(*this);
  }
}

void DHCPClient::discover() {
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
  DHCPClient::Message dhcpDiscover {};
  dhcpDiscover.createDiscoverPacket(_networkDevice);
  ssize_t len;

  len = sendto(sd, &dhcpDiscover, sizeof(dhcpDiscover), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
  if (len < 0) {
    close(sd);
    throw upan::exception(XLOC, "failed to send DHCP Discover");
  }

  printf("\nDHCP Discover sent");
  char buffer[MAX_BUFFER_SIZE];
  // Receive DHCP Offer
  len = recvfrom(sd, buffer, MAX_BUFFER_SIZE, 0, nullptr, nullptr);
  if (len < 0) {
    close(sd);
    throw upan::exception(XLOC, "failed to receive DHCP offer");
  }

  close(sd);

  DHCPClient::Message dhcpResponse {};
  memcpy(&dhcpResponse, buffer, sizeof(dhcpResponse));
  dhcpResponse.processResponse(dhcpDiscover, *this);
}