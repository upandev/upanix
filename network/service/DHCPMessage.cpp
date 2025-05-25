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

#include <DHCPMessage.h>
#include <NetworkDevice.h>

static constexpr int DHCP_REQUEST_IP_RENEWAL_TIME = 3600; // 1 min

void DHCPMessage::createDiscoverPacket(const NetworkDevice& networkDevice) {
  memset(this, 0, sizeof(DHCPMessage));

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
  uint32_t leaseTime = ntohl(DHCP_REQUEST_IP_RENEWAL_TIME); //1hr
  memcpy(_options + oi, (void*)&leaseTime, sizeof(uint32_t));
  oi += 4;

  _options[oi++] = DHCPOptionType::HostName;
  const int l = strlen(networkDevice.hostName());
  _options[oi++] = l;
  memcpy(_options + oi, networkDevice.hostName(), l);
  oi += l;

  _options[oi] = DHCPOptionType::OptionEnd;
}

void DHCPMessage::createRequestPacket(const NetworkDevice& networkDevice) {
  memset(this, 0, sizeof(DHCPMessage));

  _op = DHCPOperationType::BootRequest;
  _htype = HardwareType::Ethernet;
  _hlen = INADDR_MAC_LEN;
  _xid = htonl(rand());
  _flags = DHCPFlags::Unicast;
  memcpy(_chaddr, networkDevice.GetMACAddress().get(), INADDR_MAC_LEN);

  // Add DHCP options
  _magicCookie = DHCP_MAGIC_COOKIE;

  int oi = 0;
  _options[oi++] = DHCPOptionType::MessageType; // Option: DHCP Message Type
  _options[oi++] = 1;  // Length
  _options[oi++] = DHCPMessageType::Request; // DHCP Discover

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

  _options[oi++] = DHCPOptionType::RequestedIPAddress;
  _options[oi++] = 4;
  uint32_t ipAddress = networkDevice.GetIPAddress();
  memcpy(_options + oi, (void*)&ipAddress, sizeof(uint32_t));
  oi += 4;

  _options[oi++] = DHCPOptionType::LeaseTime;
  _options[oi++] = 4;
  uint32_t leaseTime = ntohl(DHCP_REQUEST_IP_RENEWAL_TIME); //1hr
  memcpy(_options + oi, (void*)&leaseTime, sizeof(uint32_t));
  oi += 4;

  _options[oi++] = DHCPOptionType::HostName;
  const int l = strlen(networkDevice.hostName());
  _options[oi++] = l;
  memcpy(_options + oi, networkDevice.hostName(), l);
  oi += l;

  _options[oi] = DHCPOptionType::OptionEnd;
}

void DHCPMessage::createRenewPacket(const NetworkDevice& networkDevice, in_addr_t dhcpServerIP) {
  memset(this, 0, sizeof(DHCPMessage));

  _op = DHCPOperationType::BootRequest;
  _htype = HardwareType::Ethernet;
  _hlen = INADDR_MAC_LEN;
  _xid = htonl(rand());
  _flags = DHCPFlags::Unicast;
  memcpy(_chaddr, networkDevice.GetMACAddress().get(), INADDR_MAC_LEN);
  _ciaddr = networkDevice.GetIPAddress();
  _siaddr = dhcpServerIP;

  // Add DHCP options
  _magicCookie = DHCP_MAGIC_COOKIE;

  int oi = 0;
  _options[oi++] = DHCPOptionType::MessageType; // Option: DHCP Message Type
  _options[oi++] = 1;  // Length
  _options[oi++] = DHCPMessageType::Request; // DHCP Discover

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

  _options[oi++] = DHCPOptionType::RequestedIPAddress;
  _options[oi++] = 4;
  uint32_t ipAddress = networkDevice.GetIPAddress();
  memcpy(_options + oi, (void*)&ipAddress, sizeof(uint32_t));
  oi += 4;

  _options[oi++] = DHCPOptionType::DHCPServerIdentifier;
  _options[oi++] = 4;
  memcpy(_options + oi, (void*)&dhcpServerIP, sizeof(uint32_t));
  oi += 4;

  _options[oi++] = DHCPOptionType::LeaseTime;
  _options[oi++] = 4;
  uint32_t leaseTime = ntohl(DHCP_REQUEST_IP_RENEWAL_TIME); //1hr
  memcpy(_options + oi, (void*)&leaseTime, sizeof(uint32_t));
  oi += 4;

  _options[oi++] = DHCPOptionType::HostName;
  const int l = strlen(networkDevice.hostName());
  _options[oi++] = l;
  memcpy(_options + oi, networkDevice.hostName(), l);
  oi += l;

  _options[oi] = DHCPOptionType::OptionEnd;
}

const uint8_t* DHCPMessage::getOption(DHCPOptionType optionType) const {
  for(int i = 0; i < DHCP_MAX_OPTION_SIZE; ++i) {
    if (_options[i] == DHCPOptionType::OptionEnd) {
      return nullptr;
    }

    if (i + 1 >= DHCP_MAX_OPTION_SIZE) {
      throw upan::exception(XLOC, "incomplete option: %d, missing len", optionType);
    }

    const auto len = _options[i+1];
    const auto nexti = i + 1 + len;
    if (nexti >= DHCP_MAX_OPTION_SIZE) {
      throw upan::exception(XLOC, "incomplete option: %d", optionType);
    }

    if (_options[i] == optionType) {
      return _options + i + 1;
    } else {
      i = nexti;
    }
  }

  throw upan::exception(XLOC, "invalid option - no option-end found");
}

DHCPMessage::DHCPMessageType DHCPMessage::getMessageType() const {
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
    case DHCPMessageType::ACK:
    case DHCPMessageType::NAK:
    case DHCPMessageType::Decline:
      return (DHCPMessageType)messageType;

    default:
      throw upan::exception(XLOC, "unsupported DHCP message type %d", messageType);
  }
}

in_addr_t DHCPMessage::readIPAddress(DHCPOptionType optionType) const {
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

in_addr_t DHCPMessage::getSubnetMask() const {
  return readIPAddress(DHCPOptionType::SubnetMask);
}

in_addr_t DHCPMessage::getBroadcastAddress() const {
  return readIPAddress(DHCPOptionType::BroadcastAddress);
}

in_addr_t DHCPMessage::getDNSAddress() const {
  return readIPAddress(DHCPOptionType::DNS);
}

in_addr_t DHCPMessage::getRouterAddress() const {
  return readIPAddress(DHCPOptionType::Router);
}

in_addr_t DHCPMessage::getDHCPServerAddress() const {
  return readIPAddress(DHCPOptionType::DHCPServerIdentifier);
}

time_t DHCPMessage::readTime(DHCPOptionType optionType) const {
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

time_t DHCPMessage::getLeaseTime() const {
  return readTime(DHCPOptionType::LeaseTime);
}

time_t DHCPMessage::getLeaseRenewalTime() const {
  return readTime(DHCPOptionType::LeaseRenewalTime);
}

time_t DHCPMessage::getLeaseRebindingTime() const {
  return readTime(DHCPOptionType::LeaseRebindingTime);
}

upan::string DHCPMessage::readString(DHCPOptionType optionType) const {
  auto option = getOption(optionType);
  if (option) {
    const auto len = option[0];
    return { (char*)option + 1, len };
  }
  return upan::string::EMPTY;
}

upan::string DHCPMessage::getMessageText() const {
  return readString(DHCPOptionType::MessageText);
}

void DHCPMessage::validateResponse(const DHCPMessage& request, const NetworkDevice& networkDevice) {
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
  if (clientMacAddress != networkDevice.GetMACAddress()) {
    throw upan::exception(XLOC, "MAC address mismatch. Response: %s", clientMacAddress.str().c_str());
  }
}

