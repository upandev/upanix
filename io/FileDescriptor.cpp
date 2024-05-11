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
#include <Directory.h>

FileDescriptor::FileDescriptor(int pid, int fd, byte mode,
                               const upan::string& nodeId,
                               const upan::string& fileName,
                               StorageDrive& diskDrive,
                               uint32_t startSectorID) :
        IODescriptor(pid, fd, mode),
        _fileName(fileName),
        _nodeId(nodeId),
        _diskDrive(diskDrive),
        _offset(0),
        _lastReadSectorIndex(0),
        _lastReadSectorNo(startSectorID) {
}

FileSystem::PresentWorkingDirectory& FileDescriptor::getWorkingDirectory() {
  auto& pas = ProcessManager::Instance().GetCurrentPAS();
  return (pas.driveID() == _diskDrive.Id()) ? pas.processPWD() : _diskDrive._fileSystem.pwd();
}

int FileDescriptor::read(void* buffer, int len) {
  upan::rlock_gaurd rlockGaurd(_diskDrive.GetFileLock(_nodeId));

  FileSystem::WorkingDirectory cwd = getWorkingDirectory();

  int readLen = Directory_FileRead(&_diskDrive, cwd, *this, (byte*)buffer, len);
  FileOperations_UpdateTime(_diskDrive, cwd, getFileName().c_str(), DIR_ACCESS_TIME);
  _offset += readLen;

  return readLen;
}

int FileDescriptor::write(const void* buffer, int len) {
  upan::wlock_gaurd wlockGaurd(_diskDrive.GetFileLock(_nodeId));

  if( !(getMode() & O_WRONLY || getMode() & O_RDWR || getMode() & O_APPEND) ) {
    throw upan::exception(XLOC, "insufficient permission to write file fd: %d", id());
  }

  FileSystem::WorkingDirectory cwd = getWorkingDirectory();

  unsigned uiIncLen = len ;
  const unsigned uiLimit = 1 MB ;

  while(true) {
    auto n = (uiIncLen > uiLimit) ? uiLimit : uiIncLen;
    Directory_FileWrite(&_diskDrive, cwd, *this, (byte*)buffer, n);
    uiIncLen -= n;
    if (uiIncLen == 0) break;
  }

  FileOperations_UpdateTime(_diskDrive, cwd, getFileName().c_str(), DIR_ACCESS_TIME | DIR_MODIFIED_TIME);

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

FileSystem_FileStat FileDescriptor::getStat() {
  upan::rlock_gaurd rlockGaurd(_diskDrive.GetFileLock(_nodeId));

  FileSystem::WorkingDirectory cwd;
  getWorkingDirectory();
  return FileOperations_GetStat(_diskDrive, cwd, getFileName().c_str());
}

void FileDescriptor::setLastReadSectorDetails(int sectorIndex, uint32_t sectorId) {
  _lastReadSectorIndex = sectorIndex;
  _lastReadSectorNo = sectorId;
}

void FileDescriptor::getLastReadSectorDetails(FileSystem::Node& node, int &sectorIndex, uint32_t &sectorId) {
  if (_lastReadSectorNo == EOC) {
    if (node.Size() > 0) {
      _lastReadSectorIndex = 0;
      _lastReadSectorNo = node.StartSectorID();
    }
  }

  sectorIndex = _lastReadSectorIndex;
  sectorId = _lastReadSectorNo;
}
