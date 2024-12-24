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
#include <SocketDescriptorDataGram.h>
#include "network/NetworkManager.h"
#include <UDP4Handler.h>

SocketDescriptorDataGram::SocketDescriptorDataGram(int pid, int fd, IPPROTO_TYPE protocol) : SocketDescriptor(pid, fd, protocol) {
}

void SocketDescriptorDataGram::sendTo(const void* buf, size_t n, int flags, const struct sockaddr* addr, socklen_t len) {
  validateSendToParams(buf, flags, addr, len);
  if (!addr) {
    throw upan::exception(XLOC, "send/destination address is not specified");
  }

  const auto& in_addr = reinterpret_cast<const struct sockaddr_in&>(addr);
  if (in_addr.sin_addr.s_addr == INADDR_BROADCAST && !canBroadcast()) {
    throw upan::exception(XLOC, "send failed - broadcast socket-option is not enabled on socket: %d", id());
  }

  ensureBind();

  NetworkManager::Instance().getDefaultDevice().ifPresent([&](NetworkDevice& device) {
    //device.getUDP4Handler().SendPacket()
  });
  //NetworkManager::Instance().send(_bindAddress, in_addr, buf, n);
}
