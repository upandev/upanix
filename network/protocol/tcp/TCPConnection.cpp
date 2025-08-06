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
#include <TCPConnection.h>
#include <RawNetPacket.h>
#include <NetworkManager.h>

constexpr int MAX_RETRY_PER_SEGMENT = 6;
constexpr int RETRANSMIT_TIMEOUT = 1000; //ms
constexpr int MAX_PENDING_PACKETS_SIZE = 128 KB; //ms

TCPConnection::TCPConnection(TCPHandler& tcpHandler,
                             const struct sockaddr_in& srcAddr,
                             const struct sockaddr_in& destAddr,
                             int socketId,
                             bool blockingSocket)
  : _tcpHandler(tcpHandler), _srcAddr(srcAddr), _destAddr(destAddr), _socketId(socketId), _blockingSocket(blockingSocket),
    _state(TCP_NEW), _seqNum(1), _ackNum(0), _finSeqNum(0), _sendFin(false), _timeWaitStart(0),
    _sendStream(64 KB), _recvStream(64 KB), _errorCode(0) {
  updateToString();
  KLog::info("TCP connection created for %s", _str.c_str());
}

void TCPConnection::updateToString() {
  char buf[1024];
  sprintf(buf, "socket: %d, src: %s:%u, dest: %s:%u", _socketId,
          upan::net::inet_ntostr(_srcAddr.sin_addr.s_addr).c_str(), ntohs(_srcAddr.sin_port),
          upan::net::inet_ntostr(_destAddr.sin_addr.s_addr).c_str(), ntohs(_destAddr.sin_port));
  _str = buf;
}

void TCPConnection::socketId(int socketId) {
  _socketId = socketId;
  updateToString();
  KLog::info("TCP connection accepted for %s", _str.c_str());
}

void TCPConnection::connect() {
  if (_state.get() != TCP_NEW) {
    throw upan::exception(XLOC, "connect failed - state: %d", _state.get());
  }

  upan::shared_ptr<TCPSegment> packet(new TCPSegment());

  packet->type(TCPSegment::SYN);
  packet->seqNum(_seqNum);

  upan::mutex_guard g(_sendRecvMutex);
  sendSegment(*packet);
  _state.set(TCP_SYN_SENT);
  _seqNum++;

  packet->resetTime();
  _pendingAckSegments.push_back(packet);

  if (_blockingSocket) {
    struct timeval timeout;
    timeout.tv_sec = 3;
    timeout.tv_usec = 0;
    _sendRecvCond.wait(_sendRecvMutex, &timeout);

    if (_state.get() != TCP_ESTABLISHED) {
      throw upan::exception(XLOC, "connect failed - state: %d", _state.get());
    }
  }
}

void TCPConnection::accept() {
  upan::mutex_guard g(_sendRecvMutex);

  upan::shared_ptr<TCPSegment> segment(new TCPSegment());

  segment->type(TCPSegment::SYN_ACK);
  segment->seqNum(_seqNum);

  sendSegment(*segment);

  _state.set(TCP_LISTEN);
  _seqNum++;

  segment->resetTime();

  _pendingAckSegments.push_back(segment);
}

void TCPConnection::close() {
  upan::mutex_guard g(_sendRecvMutex);
  _sendFin = true;
}

int TCPConnection::recv(uint8_t* data, int len) {
  upan::mutex_guard g(_sendRecvMutex);
  if (!allowAppRecv()) {
    throw upan::exception(XLOC, "recv failed - connection not active, state: %d", _state.get());
  }
  return _recvStream.read(data, len);
}

bool TCPConnection::canRecv() {
  upan::mutex_guard g(_sendRecvMutex);
  if (!allowAppRecv()) {
    return false;
  }
  return !_recvStream.empty();
}

int TCPConnection::send(const uint8_t* data, int len) {
  upan::mutex_guard g(_sendRecvMutex);
  if (!allowAppSend()) {
    throw upan::exception(XLOC, "send failed - connection not active, state: %d", _state.get());
  }
  return _sendStream.write(data, len);
}

bool TCPConnection::canSend() {
  upan::mutex_guard g(_sendRecvMutex);
  if (!allowAppSend()) {
    return false;
  }
  return !_sendStream.full();
}

void TCPConnection::recvPacket(upan::shared_ptr<RawNetPacket> rawPacket) {
  upan::mutex_guard g(_sendRecvMutex);
  if (_state.get() == TCP_CLOSED ||
      _state.get() == TCP_CLOSE_WAIT ||
      _state.get() == TCP_TIME_WAIT) {
    KLog::warn("received packet for closed connection in state: %d, socket: %d", _state.get(), _socketId);
    return;
  }

  if (_state.get() == TCP_CLOSING || _state.get() == TCP_LAST_ACK) {
    const auto& tcpHeader = rawPacket->getTCPHeader();
    if (tcpHeader._ack == 1 && rawPacket->getTCPDataLen() > 0) {
      KLog::warn("received data packet for closed connection in state: %d, socket: %d", _state.get(), _socketId);
      return;
    }
  }

  _recvPackets.push_back(rawPacket);
}

void TCPConnection::processStream() {
  upan::mutex_guard g(_sendRecvMutex);

  if (_state.get() == TCP_TIME_WAIT) {
    if ((btime() - _timeWaitStart) > 60000) { // 1 min
      _state.set(TCP_CLOSED);
      return;
    }
  }

  if (_state.get() == TCP_CLOSED) {
    return;
  }

  processRecvPackets();

  if (_state.get() == TCP_CLOSED) {
    return;
  }

  processPendingAckSegments();
  processSendStream();

  size_t pendingSendDataSize = 0;
  for (auto& segment : _pendingAckSegments) {
    pendingSendDataSize += segment->len();
  }

  if (pendingSendDataSize > MAX_PENDING_PACKETS_SIZE) {
    sendReset();
    KLog::warn("send RST after max pending send data size didn't receive any ack for socket: %d", _socketId);
  }

  size_t pendingRecvDataSize = 0;
  for (auto& e : _pendingDataPackets) {
    pendingRecvDataSize += e.second->getTCPDataLen();
  }

  if (pendingRecvDataSize > MAX_PENDING_PACKETS_SIZE) {
    sendReset();
    KLog::warn("send RST after max pending recv data size didn't receive any ack for socket: %d", _socketId);
  }
}

void TCPConnection::processRecvPackets() {
  PACKET_SEQ_MAP dataPackets;

  while (!_recvPackets.empty()) {
    auto packet = _recvPackets.front();
    _recvPackets.pop_front();

    auto recvSeqNum = ntohl(packet->getTCPHeader()._seqNum);
    if (recvSeqNum < _ackNum) {
      KLog::warn("received ack with seq num %d < expected ack num %d for socket: %d", recvSeqNum, _ackNum, _socketId);;
      continue;
    }

    bool processed = false;

    auto& tcpHeader = packet->getTCPHeader();
    if (tcpHeader._rst == 1) {
      _state.set(TCP_CLOSED);
      break;
    } else if (tcpHeader._syn == 1 && tcpHeader._ack == 1) {
      processSynAck(packet);
      continue;
    }

    if (packet->getTCPDataLen() > 0 || tcpHeader._fin == 1) {
      processed = true;
      dataPackets.insert(PACKET_SEQ_MAP::value_type(ntohl(tcpHeader._seqNum), packet));
    }

    if (tcpHeader._ack == 1) {
      processed = true;
      processAck(packet);
    }

    if (!processed) {
      KLog::warn("received unexpected TCP packet type. Seq: %d, Ack: %d for socket: %d", tcpHeader._seqNum, tcpHeader._ackNum, _socketId);
      packet->getIPV4Header().print();
      tcpHeader.print();
    }
  }

  for (auto& dataPacket : dataPackets) {
    const auto& packet = dataPacket.second;
    auto& tcpHeader = packet->getTCPHeader();
    const auto recvSeqNum = ntohl(tcpHeader._seqNum);

    if (recvSeqNum == _ackNum) {
      if (tcpHeader._fin == 1) {
        const_cast<NetworkPacket::TCP::Header&>(tcpHeader)._fin = 0;
        onRecvFin();
      }

      const auto dataLen = packet->getTCPDataLen();
      _ackNum += dataLen;

      if (dataLen > 0) {
        if (_recvStream.availableSize() >= dataLen) {
          _recvStream.write(packet->getTCPData(), dataLen);
        } else {
          _pendingDataPackets.insert(PACKET_SEQ_MAP::value_type(recvSeqNum, packet));
        }
      }
    } else {
      _pendingDataPackets.insert(PACKET_SEQ_MAP::value_type(recvSeqNum, packet));
    }

    sendAck();
  }

  for (auto it = _pendingDataPackets.begin(); it != _pendingDataPackets.end();) {
    const auto& packet = it->second;
    auto& tcpHeader = packet->getTCPHeader();
    const auto recvSeqNum = ntohl(tcpHeader._seqNum);

    if (recvSeqNum <= _ackNum) {
      const auto dataLen = packet->getTCPDataLen();
      if (_recvStream.availableSize() >= dataLen) {
        _recvStream.write(packet->getTCPData(), dataLen);
        _pendingDataPackets.erase(it++);

        if (recvSeqNum == _ackNum) {
          if (tcpHeader._fin == 1) {
            onRecvFin();
          }
          _ackNum += dataLen;
          sendAck();
        }
        continue;
      }
    }

    break;
  }
}

void TCPConnection::processPendingAckSegments() {
  for(auto segment : _pendingAckSegments) {
    if (segment->retryCount() == MAX_RETRY_PER_SEGMENT) {
      sendReset();
      KLog::warn("send RST after max retransmit didn't receive any ack for socket: %d", _socketId);
    } else if (segment->isExpired(RETRANSMIT_TIMEOUT)) {
      segment->resetTime();
      sendSegment(*segment);
      segment->incRetryCount();
    }
  }
}

void TCPConnection::processSendStream() {
  while (!_sendStream.empty()) {
    upan::shared_ptr<TCPSegment> segment(new TCPSegment());
    int n = _sendStream.read(segment->buf(), TCPSegment::MAX_SEGMENT_SIZE);
    segment->len(n);
    segment->type(TCPSegment::DATA);
    segment->seqNum(_seqNum);
    _seqNum += n;

    if (_sendStream.empty() && _sendFin) {
      prepareFinSegment(*segment);
    }

    sendSegment(*segment);
    _pendingAckSegments.push_back(segment);
  }

  if (_sendFin) {
    upan::shared_ptr<TCPSegment> segment(new TCPSegment());
    segment->seqNum(_seqNum);
    prepareFinSegment(*segment);
    sendSegment(*segment);
    _pendingAckSegments.push_back(segment);
  }
}

void TCPConnection::processAck(upan::shared_ptr<RawNetPacket> rawPacket) {
  auto& tcpHeader = rawPacket->getTCPHeader();
  //recvSeqNum == _ackNum --> expected ack num
  //recvSeqNum > _ackNum --> gap in data - don't update ackNum - wait for the missing segment
  auto recvAckNum = ntohl(tcpHeader._ackNum);

  if (!_pendingAckSegments.empty() && recvAckNum < _pendingAckSegments.front()->seqNum()) {
    KLog::warn("received ack for a segment that is already Acked - ignoring it for socket: %d", _socketId);;
  }

  for (auto it = _pendingAckSegments.begin(); it != _pendingAckSegments.end();) {
    const auto& segment = *it;
    if (recvAckNum < segment->seqNum()) {
      KLog::warn("received ack with invalid out-of-sequence ack number: %d for socket: %d", recvAckNum, _socketId);
      break;
    }

    _pendingAckSegments.erase(it++);
    if (_finSeqNum && recvAckNum >= _finSeqNum) {
      if (_state.get() == TCP_FIN_WAIT_1) {
        _state.set(TCP_FIN_WAIT_2);
      } else if (_state.get() == TCP_LAST_ACK) {
        _state.set(TCP_CLOSED);
      } else if (_state.get() == TCP_CLOSING) {
        _state.set(TCP_TIME_WAIT);
      }
    } else if (_state.get() == TCP_LISTEN) {
      _state.set(TCP_ESTABLISHED);
      NetworkManager::Instance().getTCPSocketResolver().connectionAccepted(*this);
    }

    if (recvAckNum == segment->seqNum()) {
      break;
    }
  }
}

void TCPConnection::processSynAck(upan::shared_ptr<RawNetPacket> rawPacket) {
  try {
    if (_pendingAckSegments.size() != 1) {
      throw upan::exception(XLOC, "received unexpected syn-ack");
    }

    auto packet = _pendingAckSegments.front();

    if (packet->type() != TCPSegment::SYN) {
      throw upan::exception(XLOC, "received unexpected syn-ack. pending packet is of type: %d", packet->type());
    }

    const auto& tcpHeader = rawPacket->getTCPHeader();

    if (ntohl(tcpHeader._ackNum) != _seqNum) {
      throw upan::exception(XLOC, "received unexpected syn-ack. ack num: %d != expected: %d", ntohl(tcpHeader._ackNum), _seqNum);
    }

    _ackNum = ntohl(tcpHeader._seqNum) + 1;

    _pendingAckSegments.clear();

    sendAck();

    _state.set(TCP_ESTABLISHED);
  } catch(const upan::exception& e) {
    KLog::exception(e);
    _state.set(TCP_CLOSED);
    _errorCode = -1;
  }

  if (_blockingSocket) {
    _sendRecvCond.notify_one();
  }
}

void TCPConnection::sendSegment(TCPSegment& segment) {
  segment.ackNum(_ackNum);
  _tcpHandler.send(segment, _srcAddr, _destAddr);
}

void TCPConnection::sendAck() {
  TCPSegment packet;
  packet.type(TCPSegment::ACK);
  packet.seqNum(_seqNum);
  sendSegment(packet);
}

void TCPConnection::sendReset() {
  TCPSegment rstSegment;
  rstSegment.type(TCPSegment::RST);
  rstSegment.seqNum(_seqNum);
  sendSegment(rstSegment);
  _state.set(TCP_CLOSED);
}

void TCPConnection::prepareFinSegment(TCPSegment& segment) {
  segment.type(TCPSegment::FIN);
  _sendFin = false;
  _seqNum++;
  _finSeqNum = _seqNum;

  if (_state.get() == TCP_CLOSE_WAIT) {
    _state.set(TCP_LAST_ACK);
  } else {
    _state.set(TCP_FIN_WAIT_1);
  }
}

void TCPConnection::onRecvFin() {
  _ackNum += 1;
  if (_state.get() == TCP_FIN_WAIT_1) {
    _state.set(TCP_CLOSING);
  } else if (_state.get() == TCP_FIN_WAIT_2) {
    _state.set(TCP_TIME_WAIT);
    _timeWaitStart = btime();
  } else {
    _state.set(TCP_CLOSE_WAIT);
  }
}