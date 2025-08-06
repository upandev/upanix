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
#include <NetworkManager.h>
#include <NetworkDevice.h>

SocketDescriptorDataGram::SocketDescriptorDataGram(int pid, int fd, SA_FAMILY_TYPE family, int protocol)
  : SocketDescriptorPacket(pid, fd, family, protocol), _routeSetupCompleted(false) {
}

void SocketDescriptorDataGram::onClose() {
  NetworkManager::Instance().getUDPSocketResolver().release(*this);
  NetworkManager::Instance().getUDPPortPool().release(srcAddr().sin_port);
}

void SocketDescriptorDataGram::bind(const struct sockaddr& address, socklen_t len) {
  if (srcAddr().sin_port != 0) {
    throw upan::exception(XLOC, "setupRoute failed - socket %d is already bound to port %d", id(), srcAddr().sin_port);
  }

  if (len != sizeof(struct sockaddr_in)) {
    throw upan::exception(XLOC, "invalid socket len: %d", len);
  }

  memcpy((void*)&srcAddr(), (void*)&address, len);
  if (srcAddr().sin_port == 0) {
    srcAddr().sin_port = NetworkManager::Instance().getUDPPortPool().allocate();
  } else {
    NetworkManager::Instance().getUDPPortPool().allocate(srcAddr().sin_port);
  }
}

ssize_t SocketDescriptorDataGram::sendTo(const uint8_t* buf, size_t n, int flags, const struct sockaddr* addr, socklen_t len) {
  validateSendToParams(buf, flags, addr, len);
  if (!addr) {
    throw upan::exception(XLOC, "sendPacket/destination address is not specified");
  }

  const auto& destAddr = reinterpret_cast<const struct sockaddr_in&>(*addr);
  if (destAddr.sin_addr.s_addr == INADDR_BROADCAST && !canBroadcast()) {
    throw upan::exception(XLOC, "sendPacket failed - broadcast socket-option is not enabled on socket: %d", id());
  }

  auto& device = NetworkManager::Instance().getDevice(destAddr, true);

  if (srcAddr().sin_port == 0) {
    srcAddr().sin_port = NetworkManager::Instance().getUDPPortPool().allocate();
  }

  if (srcAddr().sin_addr.s_addr != INADDR_ANY && srcAddr().sin_addr.s_addr != device.getIPAddress()) {
    throw upan::exception(XLOC, "sendPacket failed - socket %d is not connected to the same network device", id());
  }

  if (_routeSetupCompleted == false) {
    NetworkManager::Instance().getUDPSocketResolver().setup(*this);
    _routeSetupCompleted = true;
  }

  device.getUDPHandler().send(buf, n, srcAddr(), destAddr);
  return n;
}

ssize_t SocketDescriptorDataGram::recvFrom(uint8_t* buf, size_t n, int flags, struct sockaddr* addr, socklen_t* len) {
  validateRecvFromParams(buf, flags, addr, len);
  const auto& packet = recvPacket();
  const void* srcBuf = packet->getUDPData();
  const size_t dataLen = packet->getUDPHeader()._len - NetworkPacket::UDP::HEADER_SIZE;
  const auto xferLen = upan::min(n, dataLen);

  memcpy(buf, srcBuf, xferLen);

  if (addr && len) {
    reinterpret_cast<sockaddr_in&>(*addr) = { AF_INET, packet->getUDPHeader()._srcPort, packet->getIPV4Header()._header.ip_src };
    *len = sizeof(sockaddr_in);
  }

  return xferLen;
};