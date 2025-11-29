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
#include <sys/un.h>
#include <SocketDescriptor.h>

class SocketDescriptorLocalDataGram : public SocketDescriptor {
public:
  SocketDescriptorLocalDataGram(int pid, int fd, SA_FAMILY_TYPE family);

  const sockaddr_un& srcAddr() const { return _srcAddr; }
  const upan::string& srcPath() const { return _srcPath; }
  const upan::string& destPath() const { return _destPath; }

protected:
  int _read(void* buffer, int len) override;
  bool _canRead_1() override;

  int _write(const void* buffer, int len) override;
  bool _canWrite_1() override;

  void _bind(const struct sockaddr& address, socklen_t len) override;
  void _connect(const struct sockaddr& address, socklen_t len) override;
  void _listen(int backlog) override {
    throw upan::exception(XLOC, "listen not supported for local datagram sockets");
  }
  int _accept(struct sockaddr* addr, socklen_t* len) override {
    throw upan::exception(XLOC, "accept not supported for local datagram sockets");
  }
  ssize_t _sendTo(const void* buf, size_t n, int flags, const struct sockaddr* addr, socklen_t len) override;
  ssize_t _recvFrom(void* buf, size_t n, int flags, struct sockaddr* addr, socklen_t* len) override;
  void _close() override;
  void _shutdown(SOCKET_SHUTDOWN_TYPE type) override;

  int getLastError() const override { return _errorCode; }
  ssize_t sendMessage(const void* buf, size_t n, const upan::string& srcPath);

private:
  struct Message {
    upan::string _msg;
    upan::string _srcPath;
  };

  sockaddr_un _srcAddr;
  upan::list<Message> _messages;
  upan::string _srcPath;
  upan::string _destPath;
  bool _isConnected;
  bool _isBound;
  int _errorCode;
  upan::mutex _ioSync;
};
