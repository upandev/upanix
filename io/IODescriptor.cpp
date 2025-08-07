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

#include <IODescriptor.h>

int IODescriptor::read(void* buffer, int len) {
  closeCheckAndThrow();
  return _read(buffer, len);
}

bool IODescriptor::canRead() {
  closeCheckAndThrow();
  return _canRead();
}

int IODescriptor::write(const void* buffer, int len) {
  closeCheckAndThrow();
  return _write(buffer, len);
}

bool IODescriptor::canWrite() {
  closeCheckAndThrow();
  return _canWrite();
}

void IODescriptor::seek(int seekType, int offset) {
  closeCheckAndThrow();
  _seek(seekType, offset);
}

uint32_t IODescriptor::getOffset() const {
  closeCheckAndThrow();
  return _getOffset();
}

struct stat IODescriptor::getStat() {
  closeCheckAndThrow();
  return _getStat();
}

void IODescriptor::close() {
  closeCheckAndThrow();
  decrementRefCount();
  _close();
  _isClosed = true;
}

void IODescriptor::closeCheckAndThrow() const {
  if (_isClosed) {
    throw upan::exception(XLOC, "IODescriptor %d is already closed", id());
  }
}