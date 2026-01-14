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

#include <FSPipeDevice.h>
#include <ProcessManager.h>

FSPipeDevice::FSPipeDevice(const upan::string& path, int bufSize, bool isNamed) : FSDevice(path),
    _buffer(bufSize), _isNamed(isNamed) {
  if (_isNamed) {
    upan::string rootPath(ROOT_DRIVE_PREFIX);
    rootPath += path;
    const auto& fileStat = FileOperations::Instance().stats(rootPath);
    if (fileStat.isEmpty()) {
      FileOperations::Instance().create(rootPath, S_IFIFO | 0666);
    } else {
      if (S_ISFIFO(fileStat.value().st_mode) == false) {
        throw upan::exception(XLOC, "a non pipe file already exists at path: %s", path.c_str());
      }
      if (!FileOperations::Instance().fileAccess(rootPath, W_OK)) {
        throw upan::exception(XLOC, "permission denied to access pipe device at path: %s", path.c_str());
      }
    }
  }
}

FSPipeDevice::~FSPipeDevice() {
  if (_isNamed) {
    const upan::string rootPrefix(ROOT_DRIVE_PREFIX);
    unlink((rootPrefix + path()).c_str());
  }
}

bool FSPipeDevice::canRead() const {
  return _buffer.canRead();
}

bool FSPipeDevice::canWrite() const {
  return _buffer.canWrite();
}

int FSPipeDevice::read(int fd, void* buffer, int len, bool block) {
  return _buffer.read(buffer, len, block, [&]() {
    ProcessManager::Instance().WaitOnIODescriptor(fd, IODescriptorTable::IO_OP_TYPES::IO_Read, 0);
  });
}

int FSPipeDevice::write(int fd, const void* buffer, int len, bool block) {
  return _buffer.write(buffer, len, block, [&]() {
    ProcessManager::Instance().WaitOnIODescriptor(fd, IODescriptorTable::IO_OP_TYPES::IO_Write, 0);
  });
}