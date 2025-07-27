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
    _state(NetworkPacket::TCP::TCP_STATE::TCP_CLOSED), _closedByApp(false),
    _srcAddr({(sa_family_t)family, 0, { INADDR_ANY }, { 0 } }),
    _destAddr({(sa_family_t)family, 0, { INADDR_ANY }, { 0 } }),
    _seqNum(1), _ackNum(0), _tcpHandler(nullptr),
    _synQueue(fd, 128), _ackQueue(fd, 1024) {
}

SocketDescriptorStream::~SocketDescriptorStream() {
}

void SocketDescriptorStream::destroy() {
  if (_state == NetworkPacket::TCP::TCP_CLOSED) {
    delete this;
  } else {
    _closedByApp = true;
    NetworkManager::Instance().shutdownTCPConnection(*this);
  }
}

void SocketDescriptorStream::release() {
  NetworkManager::Instance().getTCPSocketResolver().release(*this);
  NetworkManager::Instance().getTCPPortPool().release(_srcAddr.sin_port);
  _state = NetworkPacket::TCP::TCP_CLOSED;
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
  if (_state != NetworkPacket::TCP::TCP_CLOSED) {
    throw upan::exception(XLOC, "connect failed - socket %d is already connected", id());
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

  NetworkManager::Instance().getTCPSocketResolver().setup(*this);
  _tcpHandler = &device.getTCPHandler();

  sendSyn();
  sendAck();
  _state = NetworkPacket::TCP::TCP_ESTABLISHED;
}

void SocketDescriptorStream::listen(int backlog) {
  if (_srcAddr.sin_port == 0) {
    throw upan::exception(XLOC, "listen failed - socket %d is not bound to any port", id());
  }

  if (_srcAddr.sin_addr.s_addr != INADDR_ANY) {
    NetworkManager::Instance().getDevice(_srcAddr, false);
  }

  _state = NetworkPacket::TCP::TCP_LISTEN;
  NetworkManager::Instance().getTCPSocketResolver().listen(*this);
}

upan::shared_ptr<RawNetPacket> SocketDescriptorStream::sendWithRetry(TCPHandler::SendType type, const uint8_t* buf, size_t n) {
  upan::shared_ptr<RawNetPacket> packet = {};

  uint32_t seqInc = 0;
  if (type == TCPHandler::DATA) {
    seqInc = n;
  } else if (type == TCPHandler::SYN || type == TCPHandler::SYN_ACK || type == TCPHandler::FIN) {
    seqInc = 1;
  }

  const uint32_t sendSeqNum = _seqNum;
  uint32_t timeoutInMs = 1000;
  const int MAX_RETRIES = 6;

  for (int i = 0; i < MAX_RETRIES; ++i) {
    _seqNum = sendSeqNum + seqInc;
    _tcpHandler->send(buf, n, _srcAddr, _destAddr, type, sendSeqNum, _ackNum);

    _sendRecvMutex.unlock();
    packet = _ackQueue.pop(timeoutInMs);
    _sendRecvMutex.lock();

    if (packet.isEmpty()) {
      //timeoutInMs *= 2;
      continue;
    } else {
      break;
    }
  }

  if (packet.isEmpty()) {
    release();
    throw upan::exception(XLOC, "sendWithRetry failed - timed out");
  }

  return packet;
}

void SocketDescriptorStream::sendSyn() {
  upan::mutex_guard g(_sendRecvMutex);

  _state = NetworkPacket::TCP::TCP_SYN_SENT;

  auto packet = sendWithRetry(TCPHandler::SYN, nullptr, 0);

  const auto& tcpHeader = packet->getTCPHeader();
  if (tcpHeader._syn != 1) {
    throw upan::exception(XLOC, "connect failed - invalid SYN ACK packet");
  }

  _ackNum = ntohl(tcpHeader._seqNum) + 1;
}

void SocketDescriptorStream::sendSynAck() {
  upan::mutex_guard g(_sendRecvMutex);

  _state = NetworkPacket::TCP::TCP_SYN_RECEIVED;

  auto packet = sendWithRetry(TCPHandler::SYN_ACK, nullptr, 0);

  const auto& tcpHeader = packet->getTCPHeader();
  _ackNum = ntohl(tcpHeader._seqNum);
}

void SocketDescriptorStream::sendFin() {
  upan::mutex_guard g(_sendRecvMutex);

  if (_state == NetworkPacket::TCP::TCP_CLOSE_WAIT) {
    _state = NetworkPacket::TCP::TCP_FIN_WAIT_1;
    auto packet = sendWithRetry(TCPHandler::FIN, nullptr, 0);

    const auto& tcpHeader = packet->getTCPHeader();
    _ackNum = ntohl(tcpHeader._seqNum);

    sendAck();
    release();
  } else {
    _state = NetworkPacket::TCP::TCP_FIN_WAIT_1;
    auto packet = sendWithRetry(TCPHandler::FIN, nullptr, 0);

    const auto& tcpHeader = packet->getTCPHeader();
    _ackNum = ntohl(tcpHeader._seqNum);

    _state = NetworkPacket::TCP::TCP_FIN_WAIT_2;

    if (tcpHeader._fin == 1) {
      _ackNum += 1;
      sendAck();
      release();
    }
  }
}

void SocketDescriptorStream::sendAck() {
  _tcpHandler->send(nullptr, 0, _srcAddr, _destAddr, TCPHandler::ACK, _seqNum, _ackNum);
}

ssize_t SocketDescriptorStream::sendTo(const uint8_t* buf, size_t n, int flags, const struct sockaddr* addr, socklen_t len) {
  upan::mutex_guard g(_sendRecvMutex);

  if (_state != NetworkPacket::TCP::TCP_ESTABLISHED) {
    throw upan::exception(XLOC, "sendPacket failed - socket %d is not connected", id());
  }

  auto packet = sendWithRetry(TCPHandler::DATA, buf, n);

  const auto& tcpHeader = packet->getTCPHeader();
  _ackNum = ntohl(tcpHeader._seqNum);

  return n;
}

ssize_t SocketDescriptorStream::recvFrom(uint8_t* buf, size_t n, int flags, struct sockaddr* addr, socklen_t* len) {
  if (_state != NetworkPacket::TCP::TCP_ESTABLISHED) {
    throw upan::exception(XLOC, "recvPacket failed - socket %d is not connected", id());
  }

  const auto& packet = recvPacket();
  const auto& tcpHeader = packet->getTCPHeader();

  _ackNum = ntohl(tcpHeader._seqNum) + packet->getTCPDataLen();

  const void* srcBuf = packet->getTCPData();
  const auto xferLen = upan::min(n, packet->getTCPDataLen());
  memcpy(buf, srcBuf, xferLen);

  sendAck();

  return xferLen;
}

void SocketDescriptorStream::recvNotify(const upan::shared_ptr<RawNetPacket>& packet) {
  const auto& tcpHeader = packet->getTCPHeader();

  if (_state == NetworkPacket::TCP::TCP_LISTEN) {
    if (tcpHeader._syn != 1) {
      throw upan::exception(XLOC, "recvPacket failed - invalid packet - syn != 1");
    }
    _synQueue.push(packet);
    return;
  }

  if (ntohl(tcpHeader._ackNum) != _seqNum) {
    KLog::warn("received packet with invalid ack num: %d, expected: %d", ntohl(tcpHeader._ackNum), _seqNum);
    return;
  }

  if (tcpHeader._ack == 1 && tcpHeader._psh == 0 && packet->getTCPDataLen() == 0 && tcpHeader._fin == 0) {
    _ackQueue.push(packet);
    return;
  }

  if (tcpHeader._fin == 1) {
    upan::mutex_guard g(_finMutex);
    if (_state == NetworkPacket::TCP::TCP_FIN_WAIT_1) {
      _ackQueue.push(packet);
      return;
    }

    _ackNum = ntohl(tcpHeader._seqNum) + 1;
    sendAck();

    if (_state == NetworkPacket::TCP::TCP_FIN_WAIT_2) {
      release();
      return;
    } else {
      _state = NetworkPacket::TCP::TCP_CLOSE_WAIT;
      return;
    }
  }

  if (tcpHeader._ack != 1) {
    throw upan::exception(XLOC, "recvPacket failed - invalid packet - ack != 1");
  }

  //data
  SocketDescriptor::recvNotify(packet);
}

void SocketDescriptorStream::PacketQueue::push(const upan::shared_ptr<RawNetPacket>& packet) {
  upan::mutex_guard g(_mutex);

  if (_queue.full()) {
    printf("\nsocket (%d) queue is full - dropping packet", _socketId);
    _queue.pop_front();
  }

  _queue.push_back(packet);
}

upan::shared_ptr<RawNetPacket> SocketDescriptorStream::PacketQueue::pop(uint32_t timeoutInMs) {
  while(true) {
    {
      upan::mutex_guard g(_mutex);
      if (!_queue.empty()) {
        const auto& packet = _queue.front();
        _queue.pop_front();
        return packet;
      }
    }

    ProcessManager::Instance().WaitOnIODescriptor(_socketId, IO_OP_TYPES::IO_Read, timeoutInMs);

    auto r = ProcessManager::Instance().GetCurrentPAS().stateInfo().getError();
    if (r == ProcessStateInfo::NO_ERROR) {
      continue;
    } else if (r == ProcessStateInfo::TIMEOUT) {
      return {};
    } else {
      throw upan::exception(XLOC, "socket (%d) receive failed with error code: %d", _socketId, r);
    }
  }
}
