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

#include <SocketDescriptorStream.h>
#include <NetworkManager.h>

SocketDescriptorStream::SocketDescriptorStream(int pid, int fd, SA_FAMILY_TYPE family, int protocol)
  : SocketDescriptor(pid, fd, family, protocol),
    _state(NetworkPacket::TCP::TCP_STATE::TCP_CLOSED),
    _srcAddr({(sa_family_t)family, 0, { INADDR_ANY }, { 0 } }),
    _destAddr({(sa_family_t)family, 0, { INADDR_ANY }, { 0 } }) {
}

SocketDescriptorStream::~SocketDescriptorStream() {
  NetworkManager::Instance().getTCPPortPool().release(_srcAddr.sin_port);
}

void SocketDescriptorStream::bind(const struct sockaddr& address, socklen_t len) {
  if (_srcAddr.sin_port != 0) {
    throw upan::exception(XLOC, "setupRoute failed - socket %d is already bound to port %d", id(), _srcAddr.sin_port);
  }

  if (len != sizeof(struct sockaddr_in)) {
    throw upan::exception(XLOC, "invalid socket len: %d", len);
  }

  memcpy((void*) &_srcAddr, (void*) &address, len);
  NetworkManager::Instance().getTCPPortPool().allocate(_srcAddr.sin_port);
}

void SocketDescriptorStream::connect(const struct sockaddr& address, socklen_t len) {
  if (_state != NetworkPacket::TCP::TCP_CLOSED) {
    throw upan::exception(XLOC, "connect failed - socket %d is already connected", id());
  }

  if (len != sizeof(struct sockaddr_in)) {
    throw upan::exception(XLOC, "invalid socket len: %d", len);
  }

  if (_srcAddr.sin_port == 0) {
    _srcAddr = {(sa_family_t) family(),
                NetworkManager::Instance().getTCPPortPool().allocate(),
                {INADDR_ANY },
                { 0 }
    };
  }

  _destAddr = reinterpret_cast<const struct sockaddr_in&>(address);

  //send SYN
  //send code
  _state = NetworkPacket::TCP::TCP_SYN_SENT;

  //Receive ACK
  //recv code

  //Send ACK
  //send ack code
  _state = NetworkPacket::TCP::TCP_ESTABLISHED;

  NetworkManager::Instance().setupRoute(*this, nullptr, 0);
}

ssize_t SocketDescriptorStream::sendTo(const uint8_t* buf, size_t n, int flags, const struct sockaddr* addr, socklen_t len) {
  if (_state != NetworkPacket::TCP::TCP_ESTABLISHED) {
    throw upan::exception(XLOC, "sendPacket failed - socket %d is not connected", id());
  }

  validateSendToParams(buf, flags, addr, len);
  if (!addr) {
    throw upan::exception(XLOC, "sendPacket/destination address is not specified");
  }

  const auto& destAddr = reinterpret_cast<const struct sockaddr_in&>(*addr);
  if (destAddr.sin_addr.s_addr == INADDR_BROADCAST) {
    throw upan::exception(XLOC, "sendPacket failed - tcp can not broadcast - socket: %d", id());
  }

  return NetworkManager::Instance().send(buf, n, protocol(), _srcAddr, reinterpret_cast<struct sockaddr&>(_destAddr));
}