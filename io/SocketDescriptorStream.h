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

#include <list.h>
#include <shared_ptr.h>
#include <SocketDescriptor.h>
#include <TCPHeaders.h>
#include <TCPHandler.h>

class SocketDescriptorStream : public SocketDescriptor {
public:
  SocketDescriptorStream(int pid, int fd, SA_FAMILY_TYPE family, int protocol);
  ~SocketDescriptorStream() override;

  const struct sockaddr_in& srcAddr() { return _srcAddr; }
  const struct sockaddr_in& destAddr() { return _destAddr; }
  NetworkPacket::TCP::TCP_STATE getState() const { return _state; }

private:
  void destroy() override;

  void bind(const struct sockaddr& address, socklen_t len) override;
  void connect(const struct sockaddr& address, socklen_t len) override;
  void listen(int backlog) override;
  ssize_t sendTo(const uint8_t* buf, size_t n, int flags, const struct sockaddr* addr, socklen_t len) override;
  ssize_t recvFrom(uint8_t* buf, size_t n, int flags, struct sockaddr* addr, socklen_t* len) override;
  void recvNotify(const upan::shared_ptr<RawNetPacket>& packet) override;

  upan::shared_ptr<RawNetPacket> sendWithRetry(TCPHandler::SendType type, const uint8_t* buf, size_t n);
  void sendSyn();
  void sendSynAck();
  void sendFin();
  void sendAck();

  void release();
  bool closedByApp() const { return _closedByApp; }

  class PacketQueue {
  public:
    PacketQueue(int socketId, size_t size) : _socketId(socketId), _queue(size) {}
    void push(const upan::shared_ptr<RawNetPacket>& packet);
    upan::shared_ptr<RawNetPacket> pop(uint32_t timeoutInMs);

  private:
    int _socketId;
    upan::queue<upan::shared_ptr<RawNetPacket>> _queue;
    upan::mutex _mutex;
  };

  friend class TCPSocketResolver;
  friend class NetworkManager;

private:
  NetworkPacket::TCP::TCP_STATE _state;
  bool _closedByApp;

  struct sockaddr_in _srcAddr;
  struct sockaddr_in _destAddr;

  uint32_t _seqNum;
  uint32_t _ackNum;
  TCPHandler* _tcpHandler;

  PacketQueue _synQueue;
  PacketQueue _ackQueue;
  upan::mutex _sendRecvMutex;
};