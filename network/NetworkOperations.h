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

#include <sys/socket.h>

class NetworkOperations {
private:
  NetworkOperations();

public:
  static NetworkOperations& Instance();

  int createSocket(SA_FAMILY_TYPE family, SOCKET_TYPE socketType, int protocol);
  void createSocketPair(SA_FAMILY_TYPE family, SOCKET_TYPE socketType, int protocol, int sv[2]);
  void bind(sock_t fd, const struct sockaddr& address, socklen_t len);
  void setSockOpt(sock_t fd, int level, SOCKET_OPTION option, const void* optval, socklen_t len);
  void getSockOpt(sock_t fd, int level, SOCKET_OPTION option, void* optval, socklen_t* len);
  ssize_t sendTo(int fd, const void* buf, size_t n, int flags, const struct sockaddr* addr, socklen_t len);
  ssize_t recvFrom(int fd, void* buf, size_t n, int flags, struct sockaddr* addr, socklen_t* len);
  void connect(int fd, const struct sockaddr& addr, socklen_t len);
  void listen(int fd, int backlog);
  int accept(int fd, struct sockaddr* addr, socklen_t* len);
  void shutdown(int fd, SOCKET_SHUTDOWN_TYPE type);
};