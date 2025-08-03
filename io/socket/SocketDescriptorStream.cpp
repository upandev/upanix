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
#include <RealNetworkDevice.h>

SocketDescriptorStream::SocketDescriptorStream(int pid, int fd, SA_FAMILY_TYPE family, int protocol)
  : SocketDescriptor(pid, fd, family, protocol),
    _srcAddr({(sa_family_t)family, 0, { INADDR_ANY }, { 0 } }),
    _destAddr({(sa_family_t)family, 0, { INADDR_ANY }, { 0 } }),
    _connectionBacklog(0) {
}

SocketDescriptorStream::~SocketDescriptorStream() {
  NetworkManager::Instance().getTCPSocketResolver().releaseListeningSocket(*this);
  _tcpConnection.toOption().ifPresent([](TCPConnection& tcpConnection) { tcpConnection.close(); });
  for(auto& c : _listenQueue) {
    c->close();
  }
  for(auto& c : _acceptQueue) {
    c->close();
  }
}

void SocketDescriptorStream::bind(const struct sockaddr& address, socklen_t len) {
  if (_srcAddr.sin_port != 0) {
    throw upan::exception(XLOC, "setupRoute failed - socket %d is already bound to port %d", id(), ntohs(_srcAddr.sin_port));
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

  _connectionBacklog = backlog;
  NetworkManager::Instance().getTCPSocketResolver().listen(*this);
  NetworkManager::Instance().getTCPPortPool().addRefCount(_srcAddr.sin_port);
}

int SocketDescriptorStream::accept(struct sockaddr* addr, socklen_t* len) {
  upan::mutex_guard g(_acceptMutex);
  _acceptCond.waitc(_acceptMutex, [&]() { return !_acceptQueue.empty(); });

  auto tcpConnection = _acceptQueue.front();
  _acceptQueue.pop_front();

  SocketDescriptorStream* sd = nullptr;
  Process& process = ProcessManager::Instance().GetCurrentPAS();
  process.iodTable().allocate([&](int fd) -> SocketDescriptorStream* {
    sd = new SocketDescriptorStream(process.processID(), fd, family(), protocol());
    return sd;
  });

  if (!sd) {
    throw upan::exception(XLOC, "accept failed on socket: %d - new socket creation failed", id());
  }

  NetworkManager::Instance().getTCPPortPool().addRefCount(_srcAddr.sin_port);
  tcpConnection->socketId(sd->id());
  sd->_tcpConnection = tcpConnection;
  sd->_srcAddr = tcpConnection->srcAddr();
  sd->_destAddr = tcpConnection->destAddr();
  if (addr && len) {
    *addr = reinterpret_cast<struct sockaddr&>(sd->_destAddr);
    *len = sizeof(struct sockaddr_in);
  }

  return sd->id();
}

void SocketDescriptorStream::acceptResponse(const upan::shared_ptr<RawNetPacket>& rawPacket) {
  const auto& tcpHeader = rawPacket->getTCPHeader();

  if (tcpHeader._syn != 1 || tcpHeader._ack == 1 || rawPacket->getTCPDataLen() > 0) {
    throw upan::exception(XLOC, "invalid TCP packet received - expected SYN on listening socket: %d", id());
  }

  if (_listenQueue.size() == _connectionBacklog) {
    throw upan::exception(XLOC, "accept failed - listening socket %d is full", id());
  }

  const auto& ipv4Header = rawPacket->getIPV4Header();

  struct sockaddr_in srcAddr = { AF_INET, tcpHeader._destPort, ipv4Header._header.ip_dst, 0 };
  struct sockaddr_in destAddr = { AF_INET, tcpHeader._srcPort, ipv4Header._header.ip_src, 0 };

  auto& device = NetworkManager::Instance().getDevice(destAddr, true);

  upan::shared_ptr<TCPConnection> tcpConnection(new TCPConnection(device.getTCPHandler(), srcAddr, destAddr, id()));
  tcpConnection->setAckNum(ntohl(tcpHeader._seqNum) + 1);
  NetworkManager::Instance().addTCPConnection(tcpConnection);
  _listenQueue.push_back(tcpConnection);
  tcpConnection->accept();
}

void SocketDescriptorStream::acceptConnection(TCPConnection& tcpConnection) {
  upan::mutex_guard g(_acceptMutex);
  for(auto it = _listenQueue.begin(); it != _listenQueue.end(); ++it) {
    auto& c = *it;
    if (c.get() == &tcpConnection) {
      _listenQueue.erase(it);
      _acceptQueue.push_back(c);
      _acceptCond.notify_one();
      return;
    }
  }
  throw upan::exception(XLOC, "accept failed - socket %d can't find the connection in listen queue", id());
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
    const auto r = ProcessManager::Instance().GetCurrentPAS().stateInfo().getError();
    if (r == ProcessStateInfo::TIMEOUT) {
      throw upan::exception(XLOC, "socket receive timed-out");
    } else if (r == ProcessStateInfo::INTERRUPTED) {
      throw upan::exception(XLOC, "socket receive interrupted");
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
    const auto r = ProcessManager::Instance().GetCurrentPAS().stateInfo().getError();
    if (r == ProcessStateInfo::TIMEOUT) {
      throw upan::exception(XLOC, "socket receive timed-out");
    } else if (r == ProcessStateInfo::INTERRUPTED) {
      throw upan::exception(XLOC, "socket receive interrupted");
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