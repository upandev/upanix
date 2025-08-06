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

#include <mutex.h>
#include <list.h>
#include <shared_ptr.h>
#include <atomicop.h>
#include <condition_variable.h>
#include <TCPHandler.h>
#include <TCPSegment.h>
#include <MemPool.h>
#include <queue.h>

class TCPConnection {
public:
  typedef enum {
    TCP_NEW,
    TCP_CLOSED,
    TCP_LISTEN,
    TCP_SYN_SENT,
    TCP_SYN_RECEIVED,
    TCP_ESTABLISHED,
    TCP_FIN_WAIT_1,
    TCP_FIN_WAIT_2,
    TCP_CLOSING,
    TCP_TIME_WAIT,
    TCP_CLOSE_WAIT,
    TCP_LAST_ACK
  } State;

public:
  TCPConnection(TCPHandler& tcpHandler, const struct sockaddr_in& srcAddr, const struct sockaddr_in& destAddr, int socketId, bool blockingSocket);

  State state() { return _state.get(); }
  void state(State state) { _state.set(state); }
  void connect();
  void accept();
  void close();

  int recv(uint8_t* buf, int len);
  bool canRecv();

  int send(const uint8_t* buf, int len);
  bool canSend();
  void processStream();

  int socketId() const { return _socketId; }
  void socketId(int id);
  const struct sockaddr_in& srcAddr() { return _srcAddr; }
  const struct sockaddr_in& destAddr() { return _destAddr; }
  const upan::string& str() const { return _str; }

  void recvPacket(upan::shared_ptr<RawNetPacket> packet);

  void setAckNum(uint32_t ackNum) { _ackNum = ackNum; }
  int errorCode() const { return _errorCode; }

private:
  bool allowAppSend() { return _state.get() == TCP_ESTABLISHED || _state.get() == TCP_CLOSE_WAIT; }
  bool allowAppRecv() { return _state.get() != TCP_CLOSED; }
  void updateToString();

  void processRecvPackets();
  void processPendingAckSegments();
  void processSendStream();

  void processAck(upan::shared_ptr<RawNetPacket> rawPacket);
  void processSynAck(upan::shared_ptr<RawNetPacket> packet);

  void sendSegment(TCPSegment& segment);
  void sendAck();
  void sendReset();
  void prepareFinSegment(TCPSegment& segment);
  void onRecvFin();

private:
  typedef upan::list<upan::shared_ptr<TCPSegment>> STREAM;
  typedef upan::list<upan::shared_ptr<RawNetPacket>> PACKET_LIST;
  typedef upan::map<uint32_t, upan::shared_ptr<RawNetPacket>> PACKET_SEQ_MAP;

  TCPHandler& _tcpHandler;
  const struct sockaddr_in _srcAddr;
  const struct sockaddr_in _destAddr;
  int _socketId;
  bool _blockingSocket;
  upan::string _str;

  upan::atomic::integral<State> _state;
  uint32_t _seqNum;
  uint32_t _ackNum;
  uint32_t _finSeqNum;
  bool _sendFin;
  time_t _timeWaitStart;

  upan::queue<uint8_t> _sendStream;
  upan::queue<uint8_t> _recvStream;
  STREAM _pendingAckSegments;
  PACKET_LIST _recvPackets;
  PACKET_SEQ_MAP _pendingDataPackets;
  upan::mutex _sendRecvMutex;
  upan::condition_variable _sendRecvCond;
  int _errorCode;
};