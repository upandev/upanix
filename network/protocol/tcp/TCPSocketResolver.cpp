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

#include <TCPSocketResolver.h>
#include <NetworkManager.h>
#include <NetworkDevice.h>
#include <TCPSegment.h>

void TCPSocketResolver::listen(SocketDescriptorStream& socket) {
  upan::mutex_guard g(_mutex);
//  const auto& srcAddr = socket.srcAddr();
//  auto r = _socketListenMap.insert(SOCKET_ADDR_MAP::value_type(SOCK_ADDR(srcAddr), &socket));
//  if (r.second == false) {
//    throw upan::exception(XLOC, "listen failed - socket %d is already listening", socket.id());
//  }
}

void TCPSocketResolver::setup(upan::shared_ptr<TCPConnection>& tcpConnection) {
  upan::mutex_guard g(_mutex);

  auto srcAddr = SOCK_ADDR(tcpConnection->srcAddr());
  auto destAddr = SOCK_ADDR(tcpConnection->destAddr());

  auto r = _socketBindMap[srcAddr].insert(TCP_CONNECTION_ADDR_MAP::value_type(destAddr, tcpConnection));
  if (r.second == false) {
    throw upan::exception(XLOC, "setup failed - socket %d is already bound", tcpConnection->socketId());
  }
}

void TCPSocketResolver::release(upan::shared_ptr<TCPConnection>& tcpConnection) {
  upan::mutex_guard g(_mutex);

  const auto& srcAddr = tcpConnection->srcAddr();
  const auto& destAddr = tcpConnection->destAddr();

  if (destAddr.sin_port != 0) {
    auto it = _socketBindMap.find(SOCK_ADDR(srcAddr));
    if (it != _socketBindMap.end()) {
      it->second.erase(SOCK_ADDR(destAddr));
      if (it->second.empty()) {
        _socketBindMap.erase(it);
      }
    }
  } else {
    _tcpConnectionListenMap.erase(SOCK_ADDR(srcAddr));
  }
}

upan::shared_ptr<TCPConnection> TCPSocketResolver::resolve(const upan::shared_ptr<RawNetPacket>& packet) {
  upan::mutex_guard g(_mutex);

  const auto& ipv4Header = packet->getIPV4Header();
  const auto& tcpHeader = packet->getTCPHeader();

  const sockaddr_in destAddr = { AF_INET, tcpHeader._destPort, ipv4Header._header.ip_dst, 0 };
  const sockaddr_in srcAddr = { AF_INET, tcpHeader._srcPort, ipv4Header._header.ip_src, 0 };

  auto it = _socketBindMap.find(SOCK_ADDR(destAddr));
  if (it != _socketBindMap.end()) {
    auto it2 = it->second.find(SOCK_ADDR(srcAddr));
    if (it2 != it->second.end()) {
      return it2->second;
    }
  } else {
    auto lt = _tcpConnectionListenMap.find(SOCK_ADDR(destAddr));
    if (lt != _tcpConnectionListenMap.end()) {
      return lt->second;
    } else {
      struct sockaddr_in anyAddr = { AF_INET, destAddr.sin_port, { INADDR_ANY }, 0 };
      auto it2 = _tcpConnectionListenMap.find(SOCK_ADDR(anyAddr));
      if (it2 != _tcpConnectionListenMap.end()) {
        return it2->second;
      }
    }
  }

  return upan::shared_ptr<TCPConnection> {};
}

void TCPSocketResolver::recv(const upan::shared_ptr<RawNetPacket>& packet) {
  auto conn = resolve(packet);

  if (conn.isEmpty()) {
    const auto& tcpHeader = packet->getTCPHeader();
    const auto& ipv4Header = packet->getIPV4Header();

    struct sockaddr_in srcAddr = { AF_INET, tcpHeader._destPort, ipv4Header._header.ip_dst, 0 };
    struct sockaddr_in destAddr = { AF_INET, tcpHeader._srcPort, ipv4Header._header.ip_src, 0 };

    auto& device = NetworkManager::Instance().getDevice(destAddr, true);
    TCPSegment segment;
    segment.type(TCPSegment::RST);
    segment.seqNum(0);
    segment.ackNum(ntohl(tcpHeader._ackNum) + packet->getTCPDataLen());
    device.getTCPHandler().send(segment, srcAddr, destAddr);
    KLog::warn("send RST for invalid TCP packet from %s:%d", inet_ntoa(destAddr.sin_addr), ntohs(destAddr.sin_port));
  } else {
    conn->recvPacket(packet);
  }
}