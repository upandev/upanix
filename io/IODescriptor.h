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
#include <option.h>
#include <shared_ptr.h>
#include <fs.h>

class IODescriptor {
protected:
  IODescriptor(int pid, int id, uint32_t mode) : _pid(pid), _id(id), _mode(mode), _refCount(1), _isClosed(false) {
  }

public:
  typedef upan::shared_ptr<IODescriptor> Ptr;

  virtual ~IODescriptor() = default;

  int id() const {
    return _id;
  }

  int getPid() const {
    return _pid;
  }

  virtual IODescriptor::Ptr getParentDescriptor() {
    return {};
  }

  uint32_t getMode() const {
    return _mode;
  }

  int getRefCount() const {
    return _refCount;
  }

  void decrementRefCount() {
    --_refCount;
  }

  void incrementRefCount() {
    ++_refCount;
  }

  int read(void* buffer, int len);
  bool canRead();
  int write(const void* buffer, int len);
  bool canWrite();
  void seek(int seekType, int offset);
  uint32_t getOffset() const;
  struct stat getStat();

protected:
  virtual int _read(void* buffer, int len) = 0;
  virtual bool _canRead() = 0;
  virtual int _write(const void* buffer, int len) = 0;
  virtual bool _canWrite() = 0;
  virtual void _seek(int seekType, int offset) = 0;
  virtual uint32_t _getOffset() const = 0;
  virtual struct stat _getStat() { return {}; }
  virtual void _close() = 0;

  bool isClosed() const { return _isClosed; }
  void closeCheckAndThrow() const;

private:
  void close();

private:
  const int _pid;
  const int _id;
  uint32_t _mode;
  int _refCount;
  bool _isClosed;

  friend class IODescriptorTable;
};
