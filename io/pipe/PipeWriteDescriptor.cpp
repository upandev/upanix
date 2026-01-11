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

#include <PipeWriteDescriptor.h>

PipeWriteDescriptor::PipeWriteDescriptor(int pid, int id, uint32_t mode, const upan::string& pipeName)
  : PipeDescriptor(pid, id, O_APPEND | mode, pipeName) {
}

int PipeWriteDescriptor::_read(void* buffer, int len) {
  throw upan::exception(XLOC, "read not supported for pipe read descriptors");
}

bool PipeWriteDescriptor::_canRead() {
  return false;
}

int PipeWriteDescriptor::_write(const void* buffer, int len) {
  return _pipeDevice->write(id(), buffer, len, !(getMode() & O_WR_NONBLOCK || getMode() & O_NONBLOCK));
}

bool PipeWriteDescriptor::_canWrite() {
  return _pipeDevice->canWrite();
}