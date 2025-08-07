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

#include <IODescriptor.h>

class RedirectDescriptor : public IODescriptor {
public:
  RedirectDescriptor(int pid, int id, IODescriptor::Ptr parentDesc);

  IODescriptor::Ptr getParentDescriptor() override {
    return _parentDesc;
  }

  void changeRedirection(IODescriptor::Ptr ioDescriptor) {
    _parentDesc->decrementRefCount();
    _parentDesc = upan::move(ioDescriptor);
    _parentDesc->incrementRefCount();
  }

private:
  int _read(void* buffer, int len) override {
    return _parentDesc->read(buffer, len);
  }

  bool _canRead() override {
    return _parentDesc->canRead();
  }

  int _write(const void* buffer, int len) override {
    return _parentDesc->write(buffer, len);
  }

  bool _canWrite() override {
    return _parentDesc->canWrite();
  }

  void _seek(int seekType, int offset) override {
    _parentDesc->seek(seekType, offset);
  }

  uint32_t _getOffset() const override {
    return _parentDesc->getOffset();
  }

  void _close() override {
    _parentDesc->decrementRefCount();
  }

private:
  IODescriptor::Ptr _parentDesc;
};
