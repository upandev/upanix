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
#include <DeviceDrive.h>
#include <Directory.h>

FileDescriptor::FileDescriptor(int pid, int fd, byte mode,
                               const upan::string& nodeId,
                               const upan::string& fileName,
                               DiskDrive& diskDrive,
                               uint32_t startSectorID) :
        IODescriptor(pid, fd, mode),
        _fileName(fileName),
        _nodeId(nodeId),
        _diskDrive(diskDrive),
        _offset(0),
        _lastReadSectorIndex(0),
        _lastReadSectorNo(startSectorID) {
}

void FileDescriptor::readCWD(FileSystem::CWD& cwd) {
  auto& pas = ProcessManager::Instance().GetCurrentPAS();

  if(pas.driveID() == _diskDrive.Id()) {
    cwd.pDirEntry = &(pas.processPWD().DirEntry);
    cwd.uiSectorNo = pas.processPWD().uiSectorNo;
    cwd.bSectorEntryPosition = pas.processPWD().bSectorEntryPosition;
  } else {
    cwd.pDirEntry = &(_diskDrive._fileSystem.FSpwd.DirEntry);
    cwd.uiSectorNo = _diskDrive._fileSystem.FSpwd.uiSectorNo;
    cwd.bSectorEntryPosition = _diskDrive._fileSystem.FSpwd.bSectorEntryPosition;
  }
}

int FileDescriptor::read(void* buffer, int len) {
  upan::rlock_gaurd rlockGaurd(_diskDrive.GetFileLock(_nodeId));

  FileSystem::CWD cwd;
  readCWD(cwd);

  int readLen = Directory_FileRead(&_diskDrive, &cwd, *this, (byte*)buffer, len);
  FileOperations_UpdateTime(_diskDrive, cwd, getFileName().c_str(), DIR_ACCESS_TIME);
  _offset += readLen;

  return readLen;
}

int FileDescriptor::write(const void* buffer, int len) {
  upan::wlock_gaurd wlockGaurd(_diskDrive.GetFileLock(_nodeId));

  printf("\n writing started by thread: %d", getPid());

  if( !(getMode() & O_WRONLY || getMode() & O_RDWR || getMode() & O_APPEND) )
    throw upan::exception(XLOC, "insufficient permission to write file fd: %d", id());

  FileSystem::CWD cwd;
  readCWD(cwd);

  unsigned uiIncLen = len ;
  const unsigned uiLimit = 1 MB ;

  while(true) {
    auto n = (uiIncLen > uiLimit) ? uiLimit : uiIncLen ;

    Directory_FileWrite(&_diskDrive, &cwd, *this, (byte*)buffer, n);

    uiIncLen -= n ;

    if(uiIncLen == 0)
      break ;
  }

  FileOperations_UpdateTime(_diskDrive, cwd, getFileName().c_str(), DIR_ACCESS_TIME | DIR_MODIFIED_TIME);

  printf("\n writing completed by thread: %d", getPid());
  return len;
}

void FileDescriptor::seek(int seekType, int offset) {
  switch(seekType)
  {
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

  if(offset < 0)
    throw upan::exception(XLOC, "invalid file offset %d", offset);

  _offset = offset;
}

FileSystem_FileStat FileDescriptor::getStat() {
  FileSystem::CWD cwd;
  readCWD(cwd);
  return FileOperations_GetStat(_diskDrive, cwd, getFileName().c_str());
}