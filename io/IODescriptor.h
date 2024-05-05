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

class IODescriptor {
protected:
  IODescriptor(int pid, int id, uint32_t mode) : _pid(pid), _id(id), _mode(mode), _refCount(1) {
  }

public:
  virtual ~IODescriptor() {}

  int id() const {
    return _id;
  }

  int getPid() const {
    return _pid;
  }

  virtual IODescriptor& getRealDescriptor() {
    return *this;
  }

  virtual upan::option<IODescriptor&> getParentDescriptor() {
    return upan::option<IODescriptor&>::empty();
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

  virtual int read(void* buffer, int len) = 0;
  virtual bool canRead() = 0;
  virtual int write(const void* buffer, int len) = 0;
  virtual bool canWrite() = 0;
  virtual void seek(int seekType, int offset) = 0;
  virtual uint32_t getOffset() const = 0;

private:
  const int _pid;
  const int _id;
  uint32_t _mode;
  int _refCount;
};
