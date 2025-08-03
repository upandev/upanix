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

#include <stdlib.h>
#include <dtime.h>

class TCPSegment {
public:
  typedef enum {
    SYN,
    SYN_ACK,
    ACK,
    DATA,
    FIN,
    RST
  } Type;

public:
  static const int MAX_SEGMENT_SIZE = 1024;
  explicit TCPSegment();
  ~TCPSegment() = default;

  TCPSegment(const TCPSegment& o) = delete;
  TCPSegment& operator=(const TCPSegment& o) = delete;

  void resetTime() { _time = btime(); }
  time_t elapsedTime() const { return btime() - _time; }
  bool isExpired(time_t timeout) const { return elapsedTime() > timeout; }

  uint8_t* buf() { return _buf; }
  const uint8_t* buf() const { return _buf; }

  int len() const { return _len; }
  void len(int n) { _len = n; }

  uint32_t seqNum() const { return _seqNum; }
  void seqNum(uint32_t n) { _seqNum = n; }

  uint32_t ackNum() const { return _ackNum; }
  void ackNum(uint32_t n) { _ackNum = n; }

  void type(Type v) { _type = v; }
  Type type() const { return _type; }

  void incRetryCount() { ++_retryCount; }
  int retryCount() const { return _retryCount; }

  bool psh() const;

private:
  uint8_t _buf[MAX_SEGMENT_SIZE]{};
  int _len;
  uint32_t _seqNum;
  uint32_t _ackNum;
  Type _type;
  time_t _time;
  int _retryCount;
};