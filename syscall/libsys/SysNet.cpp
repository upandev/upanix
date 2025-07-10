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
# include <SysCall.h>

sock_t SysNet_CreateSocket(SA_FAMILY_TYPE sa_family, SOCKET_TYPE socket_type, int protocol) {
  uint64_t retStatus;
  SysCallNet_Handle(&retStatus, SYS_CALL_SOCKET_CREATE, false, (uint64_t)sa_family, (uint64_t)socket_type, (uint64_t)protocol, 4, 5);
  return (sock_t)retStatus;
}

int SysNet_Bind(sock_t fd, struct sockaddr* client_addr, socklen_t len) {
  uint64_t retStatus;
  SysCallNet_Handle(&retStatus, SYS_CALL_SOCKET_BIND, false, (uint64_t)fd, (uint64_t)client_addr, (uint64_t)len, 4, 5);
  return (int)retStatus;
}

int SysNet_SetSockOpt(sock_t fd, int level, SOCKET_OPTION option, const void* optval, socklen_t len) {
  uint64_t retStatus;
  SysCallNet_Handle(&retStatus, SYS_CALL_SOCKET_SET_OPT, false, (uint64_t)fd, (uint64_t)level, (uint64_t)option, (uint64_t)optval, (uint64_t)len);
  return (int) retStatus;
}

int SysNet_SendTo(int fd, const void *buf, size_t n, int flags, const struct sockaddr* addr, socklen_t len) {
  uint64_t retStatus;
  uint64_t ext_param[] = { (uint64_t)addr, (uint64_t)len };
  SysCallNet_Handle(&retStatus, SYS_CALL_SOCKET_SEND_TO, false, (uint64_t)fd, (uint64_t)buf, (uint64_t)n, (uint64_t)flags, (uint64_t)&ext_param);
  return (int) retStatus;
}

int SysNet_RecvFrom(int fd, void *buf, size_t n, int flags, struct sockaddr* addr, socklen_t* len) {
  uint64_t retStatus;
  uint64_t ext_param[] = { (uint64_t)addr, (uint64_t)len };
  SysCallNet_Handle(&retStatus, SYS_CALL_SOCKET_RECV_FROM, false, (uint64_t)fd, (uint64_t)buf, (uint64_t)n, (uint64_t)flags, (uint64_t)&ext_param);
  return (int) retStatus;
}