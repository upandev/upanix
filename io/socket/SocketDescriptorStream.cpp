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
#include <NetworkDevice.h>
#include <ProcessManager.h>

SocketDescriptorStream::SocketDescriptorStream(int pid, int fd, SA_FAMILY_TYPE family, int protocol)
  : SocketDescriptor(pid, fd, family, protocol),
    _srcAddr({(sa_family_t)family, 0, { INADDR_ANY }, { 0 } }),
    _destAddr({(sa_family_t)family, 0, { INADDR_ANY }, { 0 } }) {
}

SocketDescriptorStream::~SocketDescriptorStream() {
  _tcpConnection.toOption().ifPresent([](TCPConnection& tcpConnection) { tcpConnection.close(); });
}

void SocketDescriptorStream::bind(const struct sockaddr& address, socklen_t len) {
  if (_srcAddr.sin_port != 0) {
    throw upan::exception(XLOC, "setupRoute failed - socket %d is already bound to port %d", id(), _srcAddr.sin_port);
  }

  if (len != sizeof(struct sockaddr_in)) {
    throw upan::exception(XLOC, "invalid socket len: %d", len);
  }

  memcpy((void*) &_srcAddr, (void*) &address, len);
  if (_srcAddr.sin_port == 0) {
    _srcAddr.sin_port = NetworkManager::Instance().getTCPPortPool().allocate();
  } else {
    NetworkManager::Instance().getTCPPortPool().allocate(_srcAddr.sin_port);
  }
}

void SocketDescriptorStream::connect(const struct sockaddr& address, socklen_t len) {
  if (!_tcpConnection.isEmpty()) {
    throw upan::exception(XLOC, "connect failed - socket %d has already initialized a connection", id());
  }

  if (len != sizeof(struct sockaddr_in)) {
    throw upan::exception(XLOC, "invalid socket len: %d", len);
  }

  _destAddr = reinterpret_cast<const struct sockaddr_in&>(address);
  auto& device = NetworkManager::Instance().getDevice(_destAddr, true);

  if (_srcAddr.sin_port == 0) {
    _srcAddr.sin_port = NetworkManager::Instance().getTCPPortPool().allocate();
  }

  if (_srcAddr.sin_addr.s_addr == INADDR_ANY) {
    _srcAddr.sin_addr.s_addr = device.getIPAddress();
  } else if (_srcAddr.sin_addr.s_addr != device.getIPAddress()) {
    throw upan::exception(XLOC, "connect failed - socket %d is not connected to the same network device", id());
  }

  _tcpConnection.reset(new TCPConnection(device.getTCPHandler(), _srcAddr, _destAddr, id()));
  NetworkManager::Instance().addTCPConnection(_tcpConnection);
  _tcpConnection->connect();
}

void SocketDescriptorStream::listen(int backlog) {
  if (_srcAddr.sin_port == 0) {
    throw upan::exception(XLOC, "listen failed - socket %d is not bound to any port", id());
  }

  if (_srcAddr.sin_addr.s_addr != INADDR_ANY) {
    NetworkManager::Instance().getDevice(_srcAddr, false);
  }

  //_state = NetworkPacket::TCP::TCP_LISTEN;
  NetworkManager::Instance().getTCPSocketResolver().listen(*this);
}

int SocketDescriptorStream::read(void* buffer, int len) {
  if (_tcpConnection.isEmpty()) {
    throw upan::exception(XLOC, "read failed - socket %d is not connected", id());
  }

  while(true) {
    if (auto n = _tcpConnection->recv((uint8_t*)buffer, len)) {
      return n;
    }

    if (getMode() & O_RD_NONBLOCK) {
      return 0;
    }

    ProcessManager::Instance().WaitOnIODescriptor(id(), IO_OP_TYPES::IO_Read, getRecvTimeout());
    if (ProcessManager::Instance().GetCurrentPAS().stateInfo().getError() == ProcessStateInfo::TIMEOUT) {
      throw upan::exception(XLOC, "socket receive timed-out");
    }
  }
}

bool SocketDescriptorStream::canRead() {
  if (_tcpConnection.isEmpty()) {
    return false;
  }
  return _tcpConnection->canRecv();
}

int SocketDescriptorStream::write(const void* buffer, int len) {
  if (_tcpConnection.isEmpty()) {
    throw upan::exception(XLOC, "write failed - socket %d is not connected", id());
  }

  while(true) {
    if (auto n = _tcpConnection->send((const uint8_t*)buffer, len)) {
      return n;
    }

    if (getMode() & O_WR_NONBLOCK) {
      return 0;
    }

    ProcessManager::Instance().WaitOnIODescriptor(id(), IO_OP_TYPES::IO_Read, getSendTimeout());
    if (ProcessManager::Instance().GetCurrentPAS().stateInfo().getError() == ProcessStateInfo::TIMEOUT) {
      throw upan::exception(XLOC, "socket send timed-out");
    }
  }
}

bool SocketDescriptorStream::canWrite() {
  if (_tcpConnection.isEmpty()) {
    return false;
  }
  return _tcpConnection->canSend();
}

ssize_t SocketDescriptorStream::sendTo(const uint8_t* buf, size_t n, int flags, const struct sockaddr* addr, socklen_t len) {
  return write(buf, n);
}

ssize_t SocketDescriptorStream::recvFrom(uint8_t* buf, size_t n, int flags, struct sockaddr* addr, socklen_t* len) {
  return read(buf, n);
}