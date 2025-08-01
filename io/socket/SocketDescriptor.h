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

#include <IODescriptor.h>
#include <net/socket.h>
#include <mutex.h>
#include <RawNetPacket.h>
#include <queue.h>
#include <shared_ptr.h>
#include <dtime.h>

class NetworkDevice;

class SocketDescriptor : public IODescriptor {
protected:
  SocketDescriptor(int pid, int fd, SA_FAMILY_TYPE family, int protocol);

public:
  void seek(int seekType, int offset) override { }
  uint32_t getOffset() const override { return 0; }

  virtual void bind(const struct sockaddr& address, socklen_t len) = 0;
  virtual void connect(const struct sockaddr& address, socklen_t len) = 0;
  virtual void listen(int backlog) = 0;
  virtual ssize_t sendTo(const uint8_t* buf, size_t n, int flags, const struct sockaddr* addr, socklen_t len) = 0;
  virtual ssize_t recvFrom(uint8_t* buf, size_t n, int flags, struct sockaddr* addr, socklen_t* len) = 0;

  //socket options
  void setAllowBroadcast(bool val) { _allowBroadcast = val; }
  bool canBroadcast() const { return _allowBroadcast; }

  time_t getSendTimeout() const { return _sendTimeoutInMs; }
  void setSendTimeout(const struct timeval* timeout) {
    _sendTimeoutInMs = 0;
    if (timeout) {
      _sendTimeoutInMs = timeout->tv_sec * 1000 + timeout->tv_usec / 1000;
    }
  }

  time_t getRecvTimeout() const { return _recvTimeoutInMs; }
  void setRecvTimeout(const struct timeval* timeout) {
    _recvTimeoutInMs = 0;
    if (timeout) {
      _recvTimeoutInMs = timeout->tv_sec * 1000 + timeout->tv_usec / 1000;
    }
  }

  SA_FAMILY_TYPE family() const { return _family; }
  int protocol() const { return _protocol; }

protected:
  void validateSendToParams(const void* buf, int flags, const struct sockaddr* addr, socklen_t len);
  void validateRecvFromParams(const void* buf, int flags, struct sockaddr* addr, socklen_t * len);

  void validateSockAddrLen(socklen_t len) const;
  void validateFlags(int flags) const;
  void validateBuf(const void* buf) const;

private:
  const SA_FAMILY_TYPE _family;
  const int _protocol;
  bool _allowBroadcast;
  time_t _sendTimeoutInMs;
  time_t _recvTimeoutInMs;
};
