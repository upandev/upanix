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

#include <FSDevice.h>
#include <queue.h>

class Process;
class FSTerminalDevice : public FSDevice {
public:
  typedef enum {
    CANONICAL,
    RAW
  } Mode;

  typedef enum {
    TERMINAL_IN_READ,
    TERMINAL_IN_WRITE,
    TERMINAL_OUT_READ,
    TERMINAL_OUT_WRITE,
  } TERMINAL_IO_TYPES;

  typedef struct {
    upan::string _path;
    TERMINAL_IO_TYPES _waitType;
  } WaitInfo;

  FSTerminalDevice(Process& owner, const upan::string& path, int inBufSize, int outBufSize);

  bool isReady(TERMINAL_IO_TYPES ioType) const;
  bool canReadInStream() const;
  bool canWriteInStream() const;
  int readInStream(void* buffer, int len);
  int writeInStream(const void* buffer, int len);

  bool canReadOutStream() const;
  bool canWriteOutStream() const;
  int readOutStream(void* buffer, int len);
  int writeOutStream(const void* buffer, int len);

private:
  typedef enum {
    SB_IN,
    SB_OUT,
  } STREAM_BUFFER_TYPE;

  class StreamBuffer {
  public:
    StreamBuffer(int bufSize, STREAM_BUFFER_TYPE type, const upan::string& path);
    int read(void* buffer, int len, bool block);
    bool canRead() const;
    int write(const void* buffer, int len, bool block);
    bool canWrite() const;

  private:
    upan::queue<uint8_t> _queue;
    STREAM_BUFFER_TYPE _type;
    upan::string _path;
    upan::mutex _ioSync;
  };

private:
  Process& _owner;
  StreamBuffer _inBuffer;
  StreamBuffer _outBuffer;
  Mode _mode;
  bool _echo;
};