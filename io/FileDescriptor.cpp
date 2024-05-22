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

#include <fs.h>
#include <exception.h>
#include <FileDescriptor.h>
#include <ProcessManager.h>
#include <StorageDrive.h>

FileDescriptor::FileDescriptor(int pid, int fd, byte mode,
                               const FileNodeRef& fileNodeRef,
                               StorageDrive& diskDrive,
                               uint32_t startSectorId) :
        IODescriptor(pid, fd, mode),
        _fileNodeRef(fileNodeRef),
        _diskDrive(diskDrive),
        _offset(0),
        _lastReadSectorIndex(0),
        _lastReadSectorNo(startSectorId) {
}

int FileDescriptor::read(void* buffer, int len) {
  const int n = _diskDrive.fileSystem().read(_fileNodeRef, *this, (uint8_t*)buffer, len);
  _diskDrive.fileSystem().updateTime(_fileNodeRef, DIR_ACCESS_TIME);
  _offset += n;

  return n;
}

int FileDescriptor::write(const void* buffer, int len) {
  if( !(getMode() & O_WRONLY || getMode() & O_RDWR || getMode() & O_APPEND) ) {
    throw upan::exception(XLOC, "insufficient permission to write file fd: %d", id());
  }

  int incLen = len ;
  const int limit = 1 MB ;

  while(true) {
    auto n = (incLen > limit) ? limit : incLen;
    n = _diskDrive.fileSystem().write(_fileNodeRef, *this, (const uint8_t*)buffer, n);
    incLen -= n;
    if (incLen == 0) { break; }
  }

  _diskDrive.fileSystem().updateTime(_fileNodeRef, DIR_ACCESS_TIME | DIR_MODIFIED_TIME);

  return len;
}

void FileDescriptor::seek(int seekType, int offset) {
  switch(seekType) {
    case SEEK_SET:
      break ;
    case SEEK_CUR:
      offset += _offset;
      break ;
    case SEEK_END:
      offset += getStat().st_size;
      break;
    default:
      throw upan::exception(XLOC, "invalid file seek type: %d", seekType);
  }

  if(offset < 0) {
    throw upan::exception(XLOC, "invalid file offset %d", offset);
  }

  _offset = offset;
}

struct stat FileDescriptor::getStat() {
  return _diskDrive.fileSystem().stats(_fileNodeRef);
}

void FileDescriptor::setLastReadSectorDetails(int sectorIndex, uint32_t sectorId) {
  _lastReadSectorIndex = sectorIndex;
  _lastReadSectorNo = sectorId;
}

void FileDescriptor::getLastReadSectorDetails(int& sectorIndex, uint32_t& sectorId) {
  sectorIndex = _lastReadSectorIndex;
  sectorId = _lastReadSectorNo;
}
