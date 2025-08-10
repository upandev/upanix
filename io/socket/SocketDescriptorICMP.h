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

#include <net/if_arp.h>
#include <net/ip_icmp.h>
#include <SocketDescriptorPacket.h>

class SocketDescriptorICMP : public SocketDescriptorPacket {
public:
  SocketDescriptorICMP(int pid, int fd, SA_FAMILY_TYPE family, int protocol);

private:
  void _bind(const struct sockaddr& address, socklen_t len) override;
  void _connect(const struct sockaddr& address, socklen_t len) override;
  ssize_t _sendTo(const void* buf, size_t n, int flags, const struct sockaddr* addr, socklen_t len) override;
  ssize_t _recvFrom(void* buf, size_t n, int flags, struct sockaddr* addr, socklen_t* len) override;
  void _close() override;
  bool filterPacket(const upan::shared_ptr<RawNetPacket>& rawPacket) override;

private:
  struct sockaddr_in _destAddr;
};