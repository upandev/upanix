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

class NetworkDevice;

class SocketDescriptor : public IODescriptor {
protected:
  SocketDescriptor(int pid, int fd, IPPROTO_TYPE protocol);

public:
  ~SocketDescriptor() override;

  int read(void* buffer, int len) override;
  bool canRead() override { return true; }

  int write(const void* buffer, int len) override;
  bool canWrite() override { return true; }

  void seek(int seekType, int offset) override { }
  uint32_t getOffset() const override { return 0; }

  void bind(const struct sockaddr& address, socklen_t len);
  virtual void sendTo(const void *buf, size_t n, int flags, const struct sockaddr* addr, socklen_t len) = 0;

  void setAllowBroadcast(bool val) { _allowBroadcast = val; }
  bool canBroadcast() const { return _allowBroadcast; }

protected:
  void validateSendToParams(const void* buf, int flags, const struct sockaddr* addr, socklen_t len);
  void ensureBind();

private:
  bool isBound() const { return _bindAddress.sin_port != 0; }
  void validateSockAddrLen(socklen_t len) const;
  void validateFlags(int flags) const;
  void validateBuf(const void* buf) const;

private:

  const IPPROTO_TYPE _protocol;
  struct sockaddr_in _bindAddress;
  upan::mutex _mutex;

  bool _allowBroadcast;
};
