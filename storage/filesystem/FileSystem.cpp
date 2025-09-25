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
#include <Global.h>
#include <StringUtil.h>
#include <FileSystem.h>
#include <StorageDrive.h>
#include <Process.h>
#include <SystemUtil.h>

#define MAX_SECTORS_PER_RW 8

FileSystem::FileSystem(StorageDrive &diskDrive, uint32_t freePoolSize) :
  _diskDrive(diskDrive),
  _freePoolQueue(freePoolSize),
  _fsTableCache(diskDrive, _bootBlock) {
}

void FileSystem::format() {
  /************************ FAT Boot Block [START] *******************************/
  byte bFSBootBlockBuffer[512] ;
  auto bootBlock = (BootBlock*)(bFSBootBlockBuffer) ;
  bootBlock->initialize(_diskDrive);

  bFSBootBlockBuffer[510] = 0x55 ; /* BootSector Signature */
  bFSBootBlockBuffer[511] = 0xAA ;

  _diskDrive.Write(1, 1, bFSBootBlockBuffer);
  /************************* FAT Boot Block [END] **************************/

  /*********************** FAT Table [START] *************************************/
  byte bSectorBuffer[512] ;
  memset(bSectorBuffer, 0, 512);

  for(uint32_t i = 0; i < bootBlock->getTableSize(); ++i) {
    if(i == 0) {
      ((unsigned *) &bSectorBuffer)[0] = EOC;
    }

    _diskDrive.Write(i + bootBlock->getReservedSectorCount() + 1, 1, bSectorBuffer);

    if(i == 0) {
      ((unsigned *) &bSectorBuffer)[0] = 0;
    }
  }
  /*************************** FAT Table [END] **************************************/

  /*************************** Root Directory [START] *******************************/
  _bootBlock = *bootBlock;

  ((FileNode*)bSectorBuffer)->InitAsRoot();
  _diskDrive.xWrite(bSectorBuffer, ((FileNode*)bSectorBuffer)->ParentSectorID(), 1);
  /*************************** Root Directory [END] ********************************/

  _diskDrive.FlushAllDirtyCacheSectors();
  _root.clear();
}

void FileSystem::mount() {
  _bootBlock.load(_diskDrive);
  loadFreeSectors();
  _fileTree.initialize(_diskDrive);
  _root.set(_fileTree._root);
}

void FileSystem::unmount() {
  _bootBlock.store(_diskDrive);
  _fsTableCache.flush();
  _freePoolQueue.clear();
  _diskDrive.FlushAllDirtyCacheSectors();
  _fileTree.uninitialize();
  _root.clear();
}

void FileSystem::loadFreeSectors() {
  if(_freePoolQueue.full()) return;

  // First do Cache Lookup
  _fsTableCache.loadFreeSectors(_freePoolQueue);
  if(_freePoolQueue.full()) return;

  byte bBuffer[ 4096 ];

  for(unsigned i = 0; i < _bootBlock.getTableSize(); ) {
    if (_fsTableCache.exists(i)) {
      ++i;
      continue;
    }

    unsigned uiBlockSize = (_bootBlock.getTableSize() - i);
    if(uiBlockSize > 8)
      uiBlockSize = 8;

    _diskDrive.Read(i + _bootBlock.getReservedSectorCount() + 1, uiBlockSize, (byte*)bBuffer);

    auto pTable = (unsigned*)bBuffer;

    for(unsigned j = 0; j < ENTRIES_PER_TABLE_SECTOR * uiBlockSize; j++) {
      if(!(pTable[j] & EOC)) {
        const uint32_t uiSectorID = i * ENTRIES_PER_TABLE_SECTOR + j;
        if(!_freePoolQueue.push_back(uiSectorID)) {
          return;
        }
      }
    }

    i += uiBlockSize;
  }
}

uint32_t FileSystem::allocateSector() {
  upan::mutex_guard g(_freePoolMutex);

  if(_freePoolQueue.empty()) {
    loadFreeSectors();
    if(_freePoolQueue.empty())
      throw upan::exception(XLOC, "No free sectors available on disk: %s", _diskDrive.DriveName().c_str());
  }

  auto freeSectorId = _freePoolQueue.front();
  _freePoolQueue.pop_front();

  setSectorEntryValue(freeSectorId, EOC);
  return freeSectorId;
}

uint32_t FileSystem::deallocateSector(uint32_t currentSectorId) {
  upan::mutex_guard g(_freePoolMutex);

  auto nextSectorId = getSectorEntryValue(currentSectorId);
  setSectorEntryValue(currentSectorId, 0);
  _freePoolQueue.push_back(currentSectorId);
  return nextSectorId;
}

uint32_t FileSystem::getRealSectorNumber(uint32_t uiSectorID) const {
  return uiSectorID + 1/*BPB*/
         + _bootBlock.getReservedSectorCount()
         + _bootBlock.getTableSize();
}

void FileSystem::checkIfMounted() {
  if (!_diskDrive.Mounted()) {
    throw upan::exception(XLOC, "drive %s is not mounted", _diskDrive.DriveName().c_str());
  }
}

uint16_t FileSystem::getFileAttr(uint16_t fileType, uint16_t mode) {
  mode = FILE_PERM(mode) ;
  fileType = FILE_TYPE(fileType) ;

  if(!(S_ISFILE(fileType) || S_ISDIR(fileType) || S_ISSOCK(fileType))) {
    throw upan::exception(XLOC, "invalid file attribute: %x", fileType);
  }
  return (uint16_t)(fileType | mode);
}

void FileSystem::create(const FileTree::NodeTokens& fileTokens, const upan::string& newFileName,
                        uint16_t fileType, uint16_t mode,
                        const FileNodeRef& cwd, Process& process) {
  if (newFileName == DIR_SPECIAL_CURRENT || newFileName == DIR_SPECIAL_PARENT || newFileName.empty()) {
    throw upan::exception(XLOC, "invalid file name %s", newFileName.c_str());
  }

  auto parentNodeRef = _fileTree.getFileNodeRef(fileTokens, cwd);
  if (parentNodeRef.empty()) {
    throw upan::exception(XLOC, "invalid path for file %s", newFileName.c_str());
  }

  auto& parentNode = parentNodeRef.nodev();
  if (parentNode.isFile()) {
    throw upan::exception(XLOC, "%s is not a directory", parentNode.name().c_str());
  }

  FileNodeRef::WriteGuard g1(parentNodeRef);

  if (!parentNode.find(newFileName).isEmpty()) {
    throw upan::exception(XLOC, "%s %s already exists", newFileName.c_str(), (S_ISFILE(fileType) ? "file" : "directory"));
  }

  FileNodeRef parentParentNodeRef(parentNodeRef.nodev().parent());
  FileNodeRef::WriteGuard g2(parentParentNodeRef);

  uint8_t parentDirBuffer[FileSystem::SECTOR_SIZE];
  _diskDrive.xRead(parentDirBuffer, parentNode.sectorId(), 1);
  auto& parentFileNode = reinterpret_cast<FileNode*>(parentDirBuffer)[parentNode.sectorOffset()];

  if(!process.hasFilePermission(parentFileNode, O_RDWR)) {
    throw upan::exception(XLOC, "insufficient permission to create file: %s", newFileName.c_str());
  }

  uint32_t newSectorId;
  uint8_t newSectorOffset;
  bool newSector = false;
  if (!parentNode.getFreeSlot(newSectorId, newSectorOffset)) {
    newSector = true;
    newSectorId = allocateSector();
    newSectorOffset = 0;
    auto lastSectorId = parentNode.getDirLastSectorId();
    if (lastSectorId == EOC) {
      parentFileNode.StartSectorID(newSectorId);
      parentNode.startSectorId(newSectorId);
    } else {
      setSectorEntryValue(lastSectorId, newSectorId);
    }
  }

  uint8_t newSectorBuffer[FileSystem::SECTOR_SIZE];
  if (newSector) {
    memset(newSectorBuffer, 0, FileSystem::SECTOR_SIZE);
  } else {
    _diskDrive.xRead(newSectorBuffer, newSectorId, 1);
  }
  auto& newFileNode = reinterpret_cast<FileNode*>(newSectorBuffer)[newSectorOffset];
  newFileNode.Init(newFileName.c_str(), getFileAttr(fileType, mode), process.userID(), parentNode.sectorId(), parentNode.sectorOffset());

  parentFileNode.AddNode();

  _diskDrive.xWrite(parentDirBuffer, parentNode.sectorId(), 1);
  _diskDrive.xWrite(newSectorBuffer, newSectorId, 1);

  _fileTree.addNode(parentNode, newFileNode, newSectorId, newSectorOffset);
}

void FileSystem::remove(const FileTree::NodeTokens& fileTokens, const upan::string& deleteFileName, const FileNodeRef& cwd, Process& process) {
  if (deleteFileName == DIR_SPECIAL_CURRENT || deleteFileName == DIR_SPECIAL_PARENT || deleteFileName.empty()) {
    throw upan::exception(XLOC, "invalid file name %s", deleteFileName.c_str());
  }
  auto parentNodeRef = _fileTree.getFileNodeRef(fileTokens, cwd);
  if (parentNodeRef.empty()) {
    throw upan::exception(XLOC, "invalid path for file %s", deleteFileName.c_str());
  }

  auto& parentNode = parentNodeRef.nodev();
  if (parentNode.isFile()) {
    throw upan::exception(XLOC, "%s is not a directory", parentNode.name().c_str());
  }

  FileNodeRef::WriteGuard g(parentNodeRef);

  FileNodeRef parentParentNodeRef(parentNodeRef.nodev().parent());
  FileNodeRef::WriteGuard g2(parentParentNodeRef);

  uint8_t parentDirBuffer[FileSystem::SECTOR_SIZE];
  _diskDrive.xRead(parentDirBuffer, parentNode.sectorId(), 1);
  auto& parentFileNode = reinterpret_cast<FileNode*>(parentDirBuffer)[parentNode.sectorOffset()];

  if(!process.hasFilePermission(parentFileNode, O_RDWR)) {
    throw upan::exception(XLOC, "insufficient permission to delete file: %s", deleteFileName.c_str());
  }

  if(process.fileUserType(parentFileNode) != USER_OWNER) {
    throw upan::exception(XLOC, "insufficient permission to delete file: %s", deleteFileName.c_str());
  }

  uint32_t prevSectorId;
  bool deallocateSectorBlock;
  auto deleteNode =  _fileTree.removeNode(parentNode, deleteFileName, prevSectorId, deallocateSectorBlock);

  if (deleteNode->isFile()) {
    auto curSectorId = deleteNode->startSectorId();
    while(curSectorId != EOC) {
      curSectorId = deallocateSector(curSectorId);
    }
  }

  if (deallocateSectorBlock) {
    auto nextSectorId = deallocateSector(deleteNode->sectorId());
    if (deleteNode->sectorId() == parentFileNode.StartSectorID()) {
      parentFileNode.StartSectorID(nextSectorId);
      parentNode.startSectorId(nextSectorId);
    } else {
      setSectorEntryValue(prevSectorId, nextSectorId);
    }
  } else {
    uint8_t sectorBuffer[FileSystem::SECTOR_SIZE];
    _diskDrive.xRead(sectorBuffer, deleteNode->sectorId(), 1);
    auto& deleteFileNode = reinterpret_cast<FileNode*>(sectorBuffer)[deleteNode->sectorOffset()];
    deleteFileNode.MarkAsDeleted();
    _diskDrive.xWrite(sectorBuffer, deleteNode->sectorId(), 1);
  }

  delete deleteNode;
  parentFileNode.RemoveNode();

  _diskDrive.xWrite(parentDirBuffer, parentNode.sectorId(), 1);
}

FileNodeRef FileSystem::open(const FileTree::NodeTokens& fileTokens, uint16_t mode, const FileNodeRef& cwd, Process& process) {
  auto fileNodeRef = _fileTree.getFileNodeRef(fileTokens, cwd);
  bool newFileCreated = false;

  if (fileNodeRef.empty()) {
    if ( (mode & O_APPEND) || (mode & O_CREAT) || (mode & O_TRUNC) ) {
      FileTree::NodeTokens dirTokens(fileTokens);
      dirTokens.pop_back();
      const upan::string& fileName = fileTokens.back();
      create(dirTokens, fileName, S_IFREG, ATTR_FILE_DEFAULT, cwd, process);
      fileNodeRef = _fileTree.getFileNodeRef(fileTokens, cwd);
      newFileCreated = true;
    } else {
      return {};
    }
  }

  FileNodeRef::WriteGuard g1(fileNodeRef);
  auto& node = fileNodeRef.nodev();

  if (node.isDirectory()) {
    throw upan::exception(XLOC, "%s is a directory", node.name().c_str());
  }

  FileNodeRef parentNodeRef(node.parent());
  FileNodeRef::WriteGuard g2(parentNodeRef);

  uint8_t sectorBuffer[FileSystem::SECTOR_SIZE];
  _diskDrive.xRead(sectorBuffer, node.sectorId(), 1);
  auto& fileNode = reinterpret_cast<FileNode*>(sectorBuffer)[node.sectorOffset()];

  if(!process.hasFilePermission(fileNode, mode)) {
    throw upan::exception(XLOC, "insufficient permission to open file %s", node.name().c_str());
  }

  if( (mode & O_TRUNC) && !newFileCreated) {
    auto curSectorId = node.startSectorId();
    while(curSectorId != EOC) {
      curSectorId = deallocateSector(curSectorId);
    }
    fileNode.StartSectorID(EOC);
    fileNode.Size(0);
    _diskDrive.xWrite(sectorBuffer, node.sectorId(), 1);
    node.startSectorId(EOC);
    node.size(0);
  }
  return fileNodeRef;
}

void FileSystem::truncate(FileNodeRef fileNodeRef) {
  FileNodeRef::WriteGuard g1(fileNodeRef);
  auto& node = fileNodeRef.nodev();

  if (node.isDirectory()) {
    throw upan::exception(XLOC, "%s is a directory", node.name().c_str());
  }

  FileNodeRef parentNodeRef(node.parent());
  FileNodeRef::WriteGuard g2(parentNodeRef);

  uint8_t sectorBuffer[FileSystem::SECTOR_SIZE];
  _diskDrive.xRead(sectorBuffer, node.sectorId(), 1);
  auto& fileNode = reinterpret_cast<FileNode*>(sectorBuffer)[node.sectorOffset()];

  auto curSectorId = node.startSectorId();
  while(curSectorId != EOC) {
    curSectorId = deallocateSector(curSectorId);
  }
  fileNode.StartSectorID(EOC);
  fileNode.Size(0);
  _diskDrive.xWrite(sectorBuffer, node.sectorId(), 1);
  node.startSectorId(EOC);
  node.size(0);
}

FileNodeRef FileSystem::exists(const FileTree::NodeTokens& fileTokens, const FileNodeRef& cwd) {
  return _fileTree.getFileNodeRef(fileTokens, cwd);
}

upan::option<struct stat> FileSystem::stats(const FileTree::NodeTokens& fileTokens, const FileNodeRef& cwd) {
  FileNodeRef fileNodeRef = _fileTree.getFileNodeRef(fileTokens, cwd);
  if (fileNodeRef.empty()) {
    return upan::option<struct stat>::empty();
  }
  return upan::option<struct stat>(stats(fileNodeRef));
}

struct stat FileSystem::stats(FileNodeRef fileNodeRef) {
  FileNodeRef::ReadGuard g(fileNodeRef);
  auto& node = fileNodeRef.nodev();

  uint8_t sectorBuffer[FileSystem::SECTOR_SIZE];
  _diskDrive.xRead(sectorBuffer, node.sectorId(), 1);
  auto& fileNode = reinterpret_cast<FileNode*>(sectorBuffer)[node.sectorOffset()];

  return stats(fileNode);
}

struct stat FileSystem::stats(const FileNode& fileNode) {
  struct stat fileStat{};

  fileStat.st_dev = _diskDrive.DriveNumber();
  fileStat.st_mode = fileNode.Attribute() ;
  fileStat.st_uid = fileNode.UserID() ;
  fileStat.st_size = fileNode.Size() ;
  fileStat.st_atime.tv_sec = fileNode.AccessedTime() ;
  fileStat.st_mtime.tv_sec = fileNode.ModifiedTime() ;
  fileStat.st_ctime.tv_sec = fileNode.CreatedTime() ;

  fileStat.st_blksize = 512 ;
  fileStat.st_blocks = (fileNode.Size() / 512) + ((fileNode.Size() % 512) ? 1 : 0 ) ;

  fileStat.st_rdev = 0 ;
  fileStat.st_gid = 1 ;
  fileStat.st_nlink = 1 ;
  fileStat.st_ino = 0 ;

  return fileStat;
}

upan::string FileSystem::fullPath(FileNodeRef fileNodeRef) {
  return _fileTree.getFullPath(fileNodeRef.nodev());
}

bool FileSystem::hasFilePermission(const FileTree::NodeTokens& fileTokens, uint8_t mode, const FileNodeRef& cwd, Process& process) {
  auto fileNodeRef = _fileTree.getFileNodeRef(fileTokens, cwd);
  if (fileNodeRef.empty()) {
    throw upan::exception(XLOC, "no such file or directory : %s", fileTokens.back().c_str());
  }

  FileNodeRef::ReadGuard g(fileNodeRef);
  auto& node = fileNodeRef.nodev();

  if (node.isDirectory()) {
    return false;
  }

  uint8_t sectorBuffer[FileSystem::SECTOR_SIZE];
  _diskDrive.xRead(sectorBuffer, node.sectorId(), 1);
  auto& fileNode = reinterpret_cast<FileNode*>(sectorBuffer)[node.sectorOffset()];

  return process.hasFilePermission(fileNode, mode);
}

void FileSystem::listDir(const FileTree::NodeTokens& fileTokens, FileNodeRef cwd, Process& process, FileStats& fileStats) {
  auto fileNodeRef = _fileTree.getFileNodeRef(fileTokens, cwd);
  if (fileNodeRef.empty()) {
    throw upan::exception(XLOC, "no such file or directory : %s", fileTokens.back().c_str());
  }

  FileNodeRef::ReadGuard g(fileNodeRef);
  auto& node = fileNodeRef.nodev();
  uint8_t sectorBuffer[FileSystem::SECTOR_SIZE];

  _diskDrive.xRead(sectorBuffer, node.sectorId(), 1);
  auto& mainFileNode = reinterpret_cast<FileNode*>(sectorBuffer)[node.sectorOffset()];

  if (!process.hasFilePermission(mainFileNode, O_RDONLY)) {
    throw upan::exception(XLOC, "insufficient permission to read %s", node.name().c_str());
  }

  struct stat_ex var_stat;
  if (mainFileNode.IsFile()) {
    strcpy(var_stat._name, mainFileNode.Name());
    var_stat._stat = stats(mainFileNode);
    fileStats.push_back(var_stat);
  } else {
    const auto dirSize = node.size();
    uint32_t dirCount = 0;
    uint32_t currentSectorId = node.startSectorId();

    while (currentSectorId != EOC) {
      _diskDrive.xRead(sectorBuffer, currentSectorId, 1);

      for (uint8_t sectorOffset = 0; sectorOffset < FileSystem::DIR_ENTRIES_PER_SECTOR; ++sectorOffset) {
        auto& fileNode = reinterpret_cast<FileNode*>(sectorBuffer)[sectorOffset];
        if (!fileNode.IsDeleted()) {
          ++dirCount;
          if (dirCount > dirSize) {
            break;
          }

          strcpy(var_stat._name, fileNode.Name());
          var_stat._stat = stats(fileNode);
          fileStats.push_back(var_stat);
        }
      }
      currentSectorId = _diskDrive.fileSystem().getSectorEntryValue(currentSectorId);
    }
  }
}

int FileSystem::read(FileNodeRef fileNodeRef, FileDescriptor& fdEntry, uint8_t* dataBuffer, int size) {
  FileNodeRef::ReadGuard g(fileNodeRef);
  auto& node = fileNodeRef.nodev();

  const int offset = fdEntry.getOffset();
  if (offset >= node.size()) {
    return 0;
  }

  if (node.size() == 0) {
    dataBuffer[0] = '\0';
    return 0;
  }

  int sectorIndex;
  uint32_t currentSectorId;
  fdEntry.getLastReadSectorDetails(sectorIndex, currentSectorId);

  int startReadSectorIndex = offset / FileSystem::SECTOR_SIZE;
  if(currentSectorId == EOC || sectorIndex < 0 || sectorIndex > startReadSectorIndex)	{
    sectorIndex = 0;
    currentSectorId = node.startSectorId();
    fdEntry.setLastReadSectorDetails(sectorIndex, currentSectorId);
  }

  while(sectorIndex != startReadSectorIndex)	{
    currentSectorId = getSectorEntryValue(currentSectorId);
    sectorIndex++;
  }

  fdEntry.setLastReadSectorDetails(sectorIndex, currentSectorId);

  uint32_t lastReadSectorId = currentSectorId;
  int lastReadSectorIndex = sectorIndex;

  const int currentFileSize = node.size();
  int readCount = 0 ;
  int readRemainingCount = (size < (currentFileSize - offset) && size > 0) ? size : (currentFileSize  - offset) ;

  uint8_t sectorBuffer[MAX_SECTORS_PER_RW * FileSystem::SECTOR_SIZE];
  int startReadSectorOffset = offset % FileSystem::SECTOR_SIZE;
  int sectorCount = 0;

  while(true)	{
    if(currentSectorId == EOC) {
      fdEntry.setLastReadSectorDetails(lastReadSectorIndex, lastReadSectorId) ;
      return readCount;
    }

    auto startSectorId = currentSectorId;

    lastReadSectorIndex += sectorCount;
    lastReadSectorId = currentSectorId;

    sectorCount = 1;
    int currentReadSize = 0;

    for(;;) {
      const auto nextSectorId = getSectorEntryValue(currentSectorId);

      if(currentSectorId + 1 == nextSectorId) {
        currentSectorId = nextSectorId ;

        ++sectorCount;
        currentReadSize = sectorCount * FileSystem::SECTOR_SIZE - startReadSectorOffset;

        if(readRemainingCount <= currentReadSize) {
          if((startSectorId + readRemainingCount) <= FileSystem::SECTOR_SIZE) {
            --sectorCount;
          }
          currentReadSize = readRemainingCount;
          break;
        }

        if(sectorCount == MAX_SECTORS_PER_RW) {
          currentSectorId = getSectorEntryValue(currentSectorId);
          break;
        }
      } else {
        currentReadSize = sectorCount * FileSystem::SECTOR_SIZE - startReadSectorOffset;

        if(readRemainingCount <= currentReadSize) {
          currentReadSize = readRemainingCount;
        }

        currentSectorId = nextSectorId;
        break;
      }
    }

    _diskDrive.xRead(sectorBuffer, startSectorId, sectorCount);

    memcpy(dataBuffer + readCount, sectorBuffer + startReadSectorOffset, currentReadSize);

    readCount += currentReadSize;
    readRemainingCount -= currentReadSize;

    startReadSectorOffset = 0 ;

    if(readRemainingCount <= 0) {
      fdEntry.setLastReadSectorDetails(lastReadSectorIndex, lastReadSectorId);
      return readCount;
    }
  }

  throw upan::exception(XLOC, "fs table is corrupted for drive:%s", _diskDrive.DriveName().c_str());
}

void FileSystem::_bufferedWrite(uint32_t sectorId, const uint8_t* dataBuffer, uint8_t* writeBuffer, unsigned& startSectorId, unsigned& prevSectorId, unsigned& count, bool flush) {
  if(flush) {
    if(count > 0) {
      _diskDrive.xWrite(writeBuffer, startSectorId, count);
    }
    count = 0 ;
    return;
  }

  bool newBuffering = false;

  if(count == 0) {
    startSectorId = prevSectorId = sectorId ;
    memcpy(writeBuffer, dataBuffer, 512);
    count = 1 ;
  } else if(prevSectorId + 1 == sectorId) {
    prevSectorId = sectorId ;
    memcpy(writeBuffer + count * 512, dataBuffer, 512);
    ++count;
  } else {
    newBuffering = true ;
  }

  if(count == MAX_SECTORS_PER_RW || newBuffering) {
    _diskDrive.xWrite(writeBuffer, startSectorId, count);

    count = 0 ;

    if(newBuffering) {
      startSectorId = prevSectorId = sectorId ;
      memcpy(writeBuffer, dataBuffer, 512);
      count = 1 ;
    }
  }
}

int FileSystem::_write(FileTree::Node& node, FileDescriptor& fdEntry, const uint8_t* dataBuffer, int size) {
  const uint32_t offset = fdEntry.getOffset();
  const uint32_t fileSize = node.size();
  uint8_t sectorBuffer[FileSystem::SECTOR_SIZE];

  const int startWriteSectorIndex = offset / FileSystem::SECTOR_SIZE;
  const int startWriteSectorOffset = offset % FileSystem::SECTOR_SIZE;

  int sectorIndex;
  uint32_t currentSectorId;
  fdEntry.getLastReadSectorDetails(sectorIndex, currentSectorId);

  if(currentSectorId == EOC || sectorIndex < 0 || sectorIndex > startWriteSectorIndex) {
    sectorIndex = 0 ;
    currentSectorId = node.startSectorId();
    fdEntry.setLastReadSectorDetails(sectorIndex, currentSectorId);
  }

  uint32_t prevSectorId = EOC ;

  while(sectorIndex < startWriteSectorIndex && currentSectorId != EOC) {
    auto nextSectorId = getSectorEntryValue(currentSectorId);
    prevSectorId = currentSectorId;
    currentSectorId = nextSectorId;
    ++sectorIndex;
  }

  if(currentSectorId == EOC) {
    memset(sectorBuffer, 0, FileSystem::SECTOR_SIZE);

    do {
      currentSectorId = allocateSector();

      if(node.startSectorId() == EOC) {
        node.startSectorId(currentSectorId);
      } else {
        setSectorEntryValue(prevSectorId, currentSectorId);
      }

      prevSectorId = currentSectorId;
      _diskDrive.xWrite(sectorBuffer, currentSectorId, 1);

      fdEntry.setLastReadSectorDetails(sectorIndex, currentSectorId);

      ++sectorIndex;
    } while(sectorIndex <= startWriteSectorIndex) ;
  } else {
    fdEntry.setLastReadSectorDetails(sectorIndex, currentSectorId) ;
  }

  int writtenCount = 0 ;
  int writeRemainingCount = size ;

  if(startWriteSectorOffset != 0) {
    _diskDrive.xRead(sectorBuffer, currentSectorId, 1);

    writtenCount = 512 - startWriteSectorOffset;
    if(size <= writtenCount) {
      writtenCount = size;
    }

    memcpy(sectorBuffer + startWriteSectorOffset, dataBuffer, writtenCount);

    _diskDrive.xWrite(sectorBuffer, currentSectorId, 1);

    if(writtenCount == size) {
      return size;
    }

    auto nextSectorId = getSectorEntryValue(currentSectorId);
    prevSectorId = currentSectorId ;
    currentSectorId = nextSectorId ;

    writeRemainingCount -= writtenCount ;
  }

  bool allocationStarted = false ;

  uint32_t uiBufStartSectorId, bufPrevSectorId;
  uint8_t writeBuffer[MAX_SECTORS_PER_RW * FileSystem::SECTOR_SIZE];
  uint32_t bufCount = 0;

  while(true) {
    if(currentSectorId == EOC || allocationStarted == true) {
      allocationStarted = true ;
      currentSectorId = allocateSector();
      setSectorEntryValue(prevSectorId, currentSectorId);
    }

    if(writeRemainingCount < FileSystem::SECTOR_SIZE) {
      if(allocationStarted == false && (offset + size) < fileSize) {
        _diskDrive.xRead(sectorBuffer, currentSectorId, 1);
      }

      memcpy(sectorBuffer, (dataBuffer + writtenCount), writeRemainingCount);

      _bufferedWrite(currentSectorId, sectorBuffer, writeBuffer, uiBufStartSectorId, bufPrevSectorId, bufCount, false);
      _bufferedWrite(EOC, nullptr, writeBuffer, uiBufStartSectorId, bufPrevSectorId, bufCount, true);

      return size;
    }

    _bufferedWrite(currentSectorId, dataBuffer + writtenCount, writeBuffer, uiBufStartSectorId, bufPrevSectorId, bufCount, false);

    writtenCount += FileSystem::SECTOR_SIZE;
    writeRemainingCount -= FileSystem::SECTOR_SIZE;

    if(writeRemainingCount == 0) {
      _bufferedWrite(EOC, nullptr, writeBuffer, uiBufStartSectorId, bufPrevSectorId, bufCount, true);
      return size;
    }

    auto nextSectorId = getSectorEntryValue(currentSectorId);
    prevSectorId = currentSectorId ;
    currentSectorId = nextSectorId ;
  }

  throw upan::exception(XLOC, "fs table is corrupted for drive:%s", _diskDrive.DriveName().c_str());
}

int FileSystem::write(FileNodeRef fileNodeRef, FileDescriptor& fdEntry, const uint8_t* dataBuffer, int size) {
  if (size == 0) {
    return 0;
  }

  FileNodeRef::WriteGuard g1(fileNodeRef);
  auto& node = fileNodeRef.nodev();

  fdEntry.setOffset(fdEntry.getMode() & O_APPEND ? node.size() : fdEntry.getOffset());

  int n = _write(node, fdEntry, dataBuffer, size);

  fdEntry.setOffset(fdEntry.getOffset() + n);

  if(node.size() < fdEntry.getOffset()) {
    FileNodeRef parentNodeRef(node.parent());
    FileNodeRef::WriteGuard g2(parentNodeRef);

    uint8_t sectorBuffer[FileSystem::SECTOR_SIZE];
    _diskDrive.xRead(sectorBuffer, node.sectorId(), 1);
    auto& fileNode = reinterpret_cast<FileNode*>(sectorBuffer)[node.sectorOffset()];

    node.size(fdEntry.getOffset());

    fileNode.Size(node.size());
    fileNode.StartSectorID(node.startSectorId());

    _diskDrive.xWrite(sectorBuffer, node.sectorId(), 1);
  }

  return n;
}


void FileSystem::updateTime(FileNodeRef fileNodeRef, uint8_t timeType) {
  FileNodeRef::WriteGuard g1(fileNodeRef);
  auto& node = fileNodeRef.nodev();

  FileNodeRef parentNodeRef(node.parent());
  FileNodeRef::WriteGuard g2(parentNodeRef);

  uint8_t sectorBuffer[FileSystem::SECTOR_SIZE];
  _diskDrive.xRead(sectorBuffer, node.sectorId(), 1);
  auto& fileNode = reinterpret_cast<FileNode*>(sectorBuffer)[node.sectorOffset()];

  const auto time = SystemUtil_GetTimeOfDay();
  if(timeType & DIR_ACCESS_TIME) {
    fileNode.AccessedTime(time);
  }

  if(timeType & DIR_MODIFIED_TIME) {
    fileNode.ModifiedTime(time);
  }

  _diskDrive.xWrite(sectorBuffer, node.sectorId(), 1);
}
