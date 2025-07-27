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
#include <TCPHandler.h>
#include <IPV4Handler.h>
#include <RawNetPacket.h>
#include <NetworkUtil.h>
#include <NetworkDevice.h>
#include <NetworkManager.h>

TCPHandler::TCPHandler(NetworkDevice& networkDevice) : PacketHandler(networkDevice) {
}

void TCPHandler::recv(const upan::shared_ptr<RawNetPacket>& packet) {
  KLog::debug("Handling TCP Packet");
  verifyChecksum(*packet);
  const auto& tcpHeader = packet->getTCPHeader();
  tcpHeader.toHost().print();

  NetworkManager::Instance().getTCPSocketResolver().recv(packet);
}

uint32_t TCPHandler::headerLen() const {
  return NetworkPacket::TCP::HEADER_SIZE + device().getIPV4Handler().headerLen();
}

void TCPHandler::send(const uint8_t* buf, uint32_t len,
                      const struct sockaddr_in& srcAddr, const struct sockaddr_in& destAddr,
                      TCPHandler::SendType sendType,
                      uint32_t seqNum, uint32_t ackNum) {
  if (sendType != SendType::DATA) {
    if (buf != nullptr || len != 0) {
      throw upan::exception(XLOC, "sendPacket/buf can't be specified for SYN/ACK packets");
    }
  }

  RawNetPacket packet(len + headerLen());
  //IPV4 header can potentially have varying length because of header-options
  //therefore, we need to initialize the IPV4 header length at the very beginning before constructing the packet bottom up
  device().getIPV4Handler().initHeaderLen(packet);

  memcpy(packet.getTCPData(), buf, len);

  auto& tcpHeader = packet.getTCPHeader();

  tcpHeader._srcPort = srcAddr.sin_port;
  tcpHeader._destPort = destAddr.sin_port;
  tcpHeader._seqNum = htonl(seqNum);
  tcpHeader._ackNum = htonl(ackNum);
  tcpHeader._dataOffset = NetworkPacket::TCP::HEADER_SIZE / sizeof(uint32_t);
  tcpHeader._reserved = 0;
  tcpHeader._windowSize = htons(5840);
  tcpHeader._checksum = 0;
  tcpHeader._urgentPtr = 0;

  if (sendType == SendType::SYN) {
    tcpHeader._syn = 1;
  } else if (sendType == SendType::SYN_ACK) {
    tcpHeader._syn = 1;
    tcpHeader._ack = 1;
  } else if (sendType == SendType::ACK) {
    tcpHeader._ack = 1;
  } else if (sendType == SendType::FIN) {
    tcpHeader._fin = 1;
  } else if (sendType == SendType::RST) {
    tcpHeader._rst = 1;
  } else if (sendType == SendType::DATA) {
    tcpHeader._ack = 1;
    tcpHeader._psh = 1;
  } else {
    throw upan::exception(XLOC, "invalid send type: %d", sendType);
  }

  calcChecksum(packet, (device().isConnected() ? device().getIPAddress() : INADDR_ANY), destAddr.sin_addr.s_addr);

  device().getIPV4Handler().send(packet, IPPROTO_TCP, srcAddr, destAddr);
}

void TCPHandler::calcChecksum(RawNetPacket& packet, in_addr_t srcAddr, in_addr_t destAddr) {
  const uint16_t len = packet.len() + NetworkPacket::TCP::HEADER_SIZE - headerLen();
  auto& tcpHeader = packet.getTCPHeader();
  const NetworkPacket::IPV4::IPV4PseudoHeader pseudoHeader{
          srcAddr,
          destAddr,
          0,
          IPPROTO_TCP,
          htons(len)
  };

  const uint32_t partialChecksum = NetworkUtil::CalculatePartialChecksum((uint16_t*) &pseudoHeader, NetworkPacket::IPV4::IPV4_PSEUDO_HEADER_SIZE, 0);
  tcpHeader._checksum = NetworkUtil::CalculateChecksum((uint16_t *) packet.getIPV4Data(),len, partialChecksum);
}

void TCPHandler::verifyChecksum(const RawNetPacket& packet) {
  const auto& tcpHeader = packet.getTCPHeader();
  const auto& ipv4Header = packet.getIPV4Header();
  const uint16_t len = ipv4Header.dataLen();
  if (tcpHeader._checksum) {
    const NetworkPacket::IPV4::IPV4PseudoHeader pseudoHeader {
      ipv4Header._header.ip_src.s_addr,
      ipv4Header._header.ip_dst.s_addr,
      0,
      IPPROTO_TCP,
      htons(len)
    };

    const uint32_t partialChecksum = NetworkUtil::CalculatePartialChecksum((uint16_t*) &pseudoHeader, NetworkPacket::IPV4::IPV4_PSEUDO_HEADER_SIZE, 0);
    const uint16_t calculatedChecksum = NetworkUtil::CalculateChecksum((uint16_t *) packet.getIPV4Data(),len, partialChecksum);

    if (calculatedChecksum != 0) {
      tcpHeader.print();
      throw upan::exception(XLOC, "Invalid Checksum for TCP Packet, IP Packet ID: %d (calc. checksum: 0x%x)", ntohs(ipv4Header._header.ip_id), calculatedChecksum);
    }
  }
}