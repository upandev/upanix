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
#include "TCPConnection.h"

class SocketDescriptorStream : public SocketDescriptor {
public:
  SocketDescriptorStream(int pid, int fd, SA_FAMILY_TYPE family, int protocol);
  ~SocketDescriptorStream() override;

  int read(void* buffer, int len) override;
  bool canRead() override;

  int write(const void* buffer, int len) override;
  bool canWrite() override;

  const struct sockaddr_in& srcAddr() { return _srcAddr; }
  const struct sockaddr_in& destAddr() { return _destAddr; }

private:
  void bind(const struct sockaddr& address, socklen_t len) override;
  void connect(const struct sockaddr& address, socklen_t len) override;
  void listen(int backlog) override;
  int accept(struct sockaddr* addr, socklen_t* len) override;
  ssize_t sendTo(const uint8_t* buf, size_t n, int flags, const struct sockaddr* addr, socklen_t len) override;
  ssize_t recvFrom(uint8_t* buf, size_t n, int flags, struct sockaddr* addr, socklen_t* len) override;
  void acceptResponse(const upan::shared_ptr<RawNetPacket>& rawPacket);
  void acceptConnection(TCPConnection& tcpConnection);

  friend class TCPSocketResolver;
  friend class NetworkManager;

private:
  struct sockaddr_in _srcAddr;
  struct sockaddr_in _destAddr;
  upan::shared_ptr<TCPConnection> _tcpConnection;

  uint32_t _connectionBacklog;
  upan::mutex _acceptMutex;
  upan::condition_variable _acceptCond;
  upan::list<upan::shared_ptr<TCPConnection>> _acceptQueue;
  upan::list<upan::shared_ptr<TCPConnection>> _listenQueue;
};