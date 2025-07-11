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
#include <stdio.h>
#include <IPV4Handler.h>
#include <NetworkDevice.h>
#include <NetworkManager.h>

IPV4Handler::IPV4Handler(NetworkDevice& networkDevice) : PacketHandler(networkDevice) {
}

void IPV4Handler::recv(const upan::shared_ptr<RawNetPacket>& packet) {
  KLog::debug("Handling IPV4 Packet");
  const auto& ipv4Header = packet->getIPV4Header();
  verifyChecksum(ipv4Header);

  if (device().isSameSubnet(ipv4Header._header.ip_src.s_addr)) {
    NetworkManager::Instance().updateIPMACTable(ipv4Header._header.ip_src.s_addr, packet->getEthernetHeader()._header.h_source);
  }

  ipv4Header.toHost().print();

  const FragmentKey fragmentKey = { ipv4Header._header.ip_id,
                                    ipv4Header._header.ip_p,
                                    ipv4Header._header.ip_src.s_addr,
                                    ipv4Header._header.ip_dst.s_addr };

  if (ipv4Header.hasMoreFragments()) {
    addFragment(fragmentKey, packet);
  } else {
    upan::shared_ptr<RawNetPacket> assembledPacket(assemblePacket(fragmentKey, packet));

    switch(ipv4Header.type()) {
      case IPPROTO_UDP:
        device().getUDP4Handler().recv(assembledPacket);
        break;
      case IPPROTO_ICMP:
        device().getICMPHandler().recv(assembledPacket);
        break;
      default:
        throw upan::exception(XLOC, "unsupported IPV4 packet type: %d", ipv4Header.type());
    }
  }
}

uint32_t IPV4Handler::headerLen() const {
  //TODO: if IPV4 header has header-options then that must be factored here
  return NetworkPacket::IPV4::HEADER_SIZE + device().getEthernetHandler().headerLen();
}

void IPV4Handler::initHeaderLen(RawNetPacket& packet) {
  //TODO: if IPV4 header has header-options then that must be factored here
  packet.getIPV4Header()._header.ip_hl = NetworkPacket::IPV4::HEADER_SIZE / sizeof(uint32_t);
}

void IPV4Handler::send(RawNetPacket& packet, IPPROTO_TYPE protocol, const struct sockaddr_in& srcAddr, const struct sockaddr_in& destAddr) {
  auto& ipv4Header = packet.getIPV4Header();

  //This should be already done if we are sending a packet from a higher network layer like UDP
  initHeaderLen(packet);

  const auto ipv4HeaderLen = ipv4Header.headerLen();
  const auto maxPayload = device().mtu() - ipv4HeaderLen;

  const auto packetHeaderLen = NetworkPacket::Ethernet::HEADER_SIZE + ipv4HeaderLen;
  const auto totalPayloadSize = packet.len() - packetHeaderLen;

  const auto packetId = (uint16_t)rand();

  size_t offset = 0;
  while (offset < totalPayloadSize) {
    // 8 byte aligned fragments, not applicable for last one
    const auto fragmentLen = (totalPayloadSize - offset) > maxPayload ? maxPayload & 0x7 : (totalPayloadSize - offset);

    ipv4Header._header.ip_v = 4;
    ipv4Header._header.ip_tos = 0;
    ipv4Header._header.ip_len = htons(fragmentLen + ipv4HeaderLen);
    ipv4Header._header.ip_id = htons(packetId);
    // set MF flag if there are more fragments
    ipv4Header._header.ip_off = htons(((offset + fragmentLen) < totalPayloadSize ? 0x2000 : 0x0) | (offset >> 3));
    ipv4Header._header.ip_ttl = 255;
    ipv4Header._header.ip_p = protocol;
    ipv4Header._header.ip_sum = 0;
    ipv4Header._header.ip_src = { device().isConnected() ? device().GetIPAddress() : INADDR_ANY };
    ipv4Header._header.ip_dst = destAddr.sin_addr;

    ipv4Header._header.ip_sum = calcChecksum(ipv4Header);

    if (totalPayloadSize <= maxPayload) { //there is only one fragment - use the main packet
      device().getEthernetHandler().send(packet, ETH_PROTO_TYPE::ETH_P_IP);
    } else {
      RawNetPacket fragmentPacket(packetHeaderLen + fragmentLen);
      fragmentPacket.getIPV4Header() = ipv4Header;
      memcpy(fragmentPacket.getIPV4Data(), packet.getIPV4Data() + offset, fragmentLen);
      device().getEthernetHandler().send(fragmentPacket, ETH_PROTO_TYPE::ETH_P_IP);
    }

    offset += fragmentLen;
  }
}

uint16_t IPV4Handler::calcChecksum(const NetworkPacket::IPV4::Header& ipv4Header) {
  return NetworkUtil::CalculateChecksum((uint16_t *)&ipv4Header,ipv4Header.headerLen(), 0);
}

void IPV4Handler::verifyChecksum(const NetworkPacket::IPV4::Header& ipv4Header) {
  const auto calculatedChecksum = calcChecksum(ipv4Header);
  if (calculatedChecksum != 0) {
    ipv4Header.toHost().print();
    throw upan::exception(XLOC, "Invalid Checksum for IP Packet ID: %d (calc. checksum: 0x%x)", ntohs(ipv4Header._header.ip_id), calculatedChecksum);
  }
}

void IPV4Handler::addFragment(const IPV4Handler::FragmentKey& fragmentKey, const upan::shared_ptr<RawNetPacket>& packet) {
  upan::mutex_guard g(_fragmentMutex);
  _fragments[fragmentKey].push_back(packet);
}

upan::shared_ptr<RawNetPacket> IPV4Handler::assemblePacket(const IPV4Handler::FragmentKey& fragmentKey, const upan::shared_ptr<RawNetPacket>& lastFragment) {
  upan::mutex_guard g(_fragmentMutex);

  auto it = _fragments.find(fragmentKey);
  if (it == _fragments.end()) {
    return lastFragment;
  }

  auto& packets = it->second;
  packets.push_back(lastFragment);

  auto firstPacket = packets.begin();
  const int headerLen = NetworkPacket::Ethernet::HEADER_SIZE + firstPacket->getIPV4Header().headerLen();

  int dataLen = 0;
  for (auto p : packets) {
    dataLen += p->getIPV4Header().dataLen();
  }

  //validate the total bytes received vs the len data on the inbound packet
  upan::shared_ptr<RawNetPacket> finalPacket(new RawNetPacket(headerLen + dataLen));
  finalPacket->getEthernetHeader() = firstPacket->getEthernetHeader();
  auto& finalIPV4Header = finalPacket->getIPV4Header();
  finalIPV4Header = firstPacket->getIPV4Header();

  int dataPos = 0;
  for (auto p : packets) {
    memcpy(finalPacket->getIPV4Data() + dataPos, p->getIPV4Data(), p->getIPV4Header().dataLen());
    dataPos += p->getIPV4Header().dataLen();
  }

  finalIPV4Header._header.ip_len = firstPacket->getIPV4Header().headerLen() + dataLen;
  finalIPV4Header._header.ip_off = 0;
  finalIPV4Header._header.ip_sum = 0;
  finalIPV4Header._header.ip_sum = calcChecksum(finalIPV4Header);

  _fragments.erase(fragmentKey);

  //TODO: cleanup expired entries

  const int calculatedLen = lastFragment->getIPV4Header().fragmentOffset() + lastFragment->getIPV4Header().dataLen();
  if (dataLen != calculatedLen) {
    throw upan::exception("fragment len mismatch for packet-id: %d, src ip: %s, dest ip: %s, received len: %d, calculated len: %d",
                ntohs(fragmentKey._identification),
                upan::net::inet_ntostr(htonl(fragmentKey._srcAddr)).c_str(),
                upan::net::inet_ntostr(htonl(fragmentKey._destAddr)).c_str(),
                dataLen, calculatedLen);
  } else {
    return finalPacket;
  }
}