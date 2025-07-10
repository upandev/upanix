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
#include <ICMPHandler.h>
#include <IPV4Handler.h>
#include <RawNetPacket.h>
#include <NetworkUtil.h>
#include <NetworkDevice.h>
#include <NetworkManager.h>
#include <Global.h>

ICMPHandler::ICMPHandler(NetworkDevice& networkDevice) : PacketHandler(networkDevice) {
}

void ICMPHandler::recv(const upan::shared_ptr<RawNetPacket>& packet) {
  KLog::debug("Handling ICMP Packet");
  verifyChecksum(*packet);
  NetworkManager::Instance().recv(packet, AF_INET);
}

void ICMPHandler::send(const uint8_t* buf, uint32_t len, const struct sockaddr_in& srcAddr, const struct sockaddr_in& destAddr) {
  RawNetPacket packet(len + device().getIPV4Handler().headerLen());
  //IPV4 header can potentially have varying length because of header-options
  //therefore, we need to initialize the IPV4 header length at the very beginning before constructing the packet bottom up
  device().getIPV4Handler().initHeaderLen(packet);
  memcpy(packet.getIPV4Data(), buf, len);
  device().getIPV4Handler().send(packet, IPPROTO_ICMP, srcAddr, destAddr);
}

void ICMPHandler::verifyChecksum(const RawNetPacket& packet) {
  const auto& ipv4Header = packet.getIPV4Header();
  if (ipv4Header.dataLen() < (int)sizeof(struct icmp)) {
    throw upan::exception(XLOC, "ICMPHandler::verifyChecksum: packet len is too small");
  }

  auto checksum = NetworkUtil::CalculateChecksum(reinterpret_cast<const uint16_t*>(packet.getIPV4Data()), ipv4Header.dataLen(), 0);
  if (checksum != 0) {
    auto icmpHeader = reinterpret_cast<const struct icmp*>(packet.getIPV4Data());
    throw upan::exception(XLOC, "Invalid Checksum for ICMP Packet Id: %d, Seq: %d(calc. checksum: 0x%x)",
                          ntohs(icmpHeader->icmp_id),
                          ntohs(icmpHeader->icmp_seq),
                          checksum);
  }
}