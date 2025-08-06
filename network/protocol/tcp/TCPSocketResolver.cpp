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

#include <arpa/inet.h>
#include <TCPSocketResolver.h>
#include <NetworkManager.h>
#include <NetworkDevice.h>
#include <TCPSegment.h>

void TCPSocketResolver::listen(SocketDescriptorStream& socket) {
  upan::mutex_guard g(_mutex);
  const auto& srcAddr = socket.srcAddr();
  auto r = _socketListenMap.insert(SOCKET_LISTEN_MAP::value_type(SOCK_ADDR(srcAddr), &socket));
  if (r.second == false) {
    throw upan::exception(XLOC, "listen failed - socket %d is already listening", socket.id());
  }
  _socketAcceptMap.insert(SOCKET_ACCEPT_MAP::value_type(socket.id(), &socket));
}

void TCPSocketResolver::setup(upan::shared_ptr<TCPConnection>& tcpConnection) {
  upan::mutex_guard g(_mutex);

  auto srcAddr = SOCK_ADDR(tcpConnection->srcAddr());
  auto destAddr = SOCK_ADDR(tcpConnection->destAddr());

  auto r = _connectionBindMap[srcAddr].insert(CONNECTION_ADDR_MAP::value_type(destAddr, tcpConnection));
  if (r.second == false) {
    throw upan::exception(XLOC, "setup failed - socket %d is already bound", tcpConnection->socketId());
  }
}

void TCPSocketResolver::releaseConnection(upan::shared_ptr<TCPConnection>& tcpConnection) {
  upan::mutex_guard g(_mutex);

  const auto& srcAddr = tcpConnection->srcAddr();
  const auto& destAddr = tcpConnection->destAddr();

  auto it = _connectionBindMap.find(SOCK_ADDR(srcAddr));
  if (it != _connectionBindMap.end()) {
    it->second.erase(SOCK_ADDR(destAddr));
    if (it->second.empty()) {
      _connectionBindMap.erase(it);
    }
  }

  NetworkManager::Instance().getTCPPortPool().release(tcpConnection->srcAddr().sin_port);
}

void TCPSocketResolver::releaseListeningSocket(SocketDescriptorStream& socket) {
  upan::mutex_guard g(_mutex);
  const auto& srcAddr = socket.srcAddr();
  _socketListenMap.erase(SOCK_ADDR(srcAddr));
  _socketAcceptMap.erase(socket.id());

  NetworkManager::Instance().getTCPPortPool().release(srcAddr.sin_port);
}

void TCPSocketResolver::connectionAccepted(TCPConnection& tcpConnection) {
  upan::mutex_guard g(_mutex);
  auto it = _socketAcceptMap.find(tcpConnection.socketId());
  if (it != _socketAcceptMap.end()) {
    try {
      it->second->acceptConnection(tcpConnection);
    } catch(upan::exception& e) {
      KLog::exception(e);
      tcpConnection.close();
    }
  } else {
    tcpConnection.close();
  }
}

upan::shared_ptr<TCPConnection> TCPSocketResolver::resolveConnection(const upan::shared_ptr<RawNetPacket>& packet) {
  upan::mutex_guard g(_mutex);

  const auto& ipv4Header = packet->getIPV4Header();
  const auto& tcpHeader = packet->getTCPHeader();

  const sockaddr_in destAddr = { AF_INET, tcpHeader._destPort, ipv4Header._header.ip_dst, 0 };
  const sockaddr_in srcAddr = { AF_INET, tcpHeader._srcPort, ipv4Header._header.ip_src, 0 };

  auto it = _connectionBindMap.find(SOCK_ADDR(destAddr));
  if (it != _connectionBindMap.end()) {
    auto it2 = it->second.find(SOCK_ADDR(srcAddr));
    if (it2 != it->second.end()) {
      return it2->second;
    }
  }

  return upan::shared_ptr<TCPConnection> {};
}

upan::option<SocketDescriptorStream&> TCPSocketResolver::resolveListeningSocket(const upan::shared_ptr<RawNetPacket>& packet) {
  upan::mutex_guard g(_mutex);

  const auto& ipv4Header = packet->getIPV4Header();
  const auto& tcpHeader = packet->getTCPHeader();

  const sockaddr_in destAddr = { AF_INET, tcpHeader._destPort, ipv4Header._header.ip_dst, 0 };

  auto lt = _socketListenMap.find(SOCK_ADDR(destAddr));
  if (lt != _socketListenMap.end()) {
    return upan::option<SocketDescriptorStream&>(lt->second);
  } else {
    struct sockaddr_in anyAddr = { AF_INET, destAddr.sin_port, { INADDR_ANY }, 0 };
    auto it2 = _socketListenMap.find(SOCK_ADDR(anyAddr));
    if (it2 != _socketListenMap.end()) {
      return upan::option<SocketDescriptorStream&>(it2->second);
    }
  }

  return upan::option<SocketDescriptorStream&>::empty();
}

void TCPSocketResolver::recv(const upan::shared_ptr<RawNetPacket>& packet) {
  auto conn = resolveConnection(packet);

  if (conn.isEmpty()) {
    auto socket = resolveListeningSocket(packet);
    if (socket.isEmpty()) {
      sendReset(packet);
    } else {
      try {
        socket.value().acceptResponse(packet);
      } catch(const upan::exception& e) {
        KLog::exception(e);
        sendReset(packet);
      }
    }
  } else {
    conn->recvPacket(packet);
  }
}

void TCPSocketResolver::sendReset(const upan::shared_ptr<RawNetPacket>& packet) {
  const auto& tcpHeader = packet->getTCPHeader();
  const auto& ipv4Header = packet->getIPV4Header();

  struct sockaddr_in srcAddr = {AF_INET, tcpHeader._destPort, ipv4Header._header.ip_dst, 0};
  struct sockaddr_in destAddr = {AF_INET, tcpHeader._srcPort, ipv4Header._header.ip_src, 0};

  auto& device = NetworkManager::Instance().getDevice(destAddr, true);
  TCPSegment segment;
  segment.type(TCPSegment::RST);
  segment.seqNum(0);
  segment.ackNum(ntohl(tcpHeader._seqNum) + packet->getTCPDataLen());
  device.getTCPHandler().send(segment, srcAddr, destAddr);
  KLog::warn("send RST for packet from %s:%d", inet_ntoa(destAddr.sin_addr), ntohs(destAddr.sin_port));
}