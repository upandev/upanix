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
  readRootDirectory();
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

void FileSystem::readRootDirectory() {
  byte bDataBuffer[512];
  _diskDrive.xRead(bDataBuffer, 0, 1);
  _pwd.Init(*reinterpret_cast<FileNode*>(bDataBuffer), 0, 0);
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
  if(_freePoolQueue.empty()) {
    loadFreeSectors();
    if(_freePoolQueue.empty())
      throw upan::exception(XLOC, "No free sectors available on disk: %s", _diskDrive.DriveName().c_str());
  }

  auto uiFreeSectorID = _freePoolQueue.front();
  _freePoolQueue.pop_front();

  setSectorEntryValue(uiFreeSectorID, EOC);
  return uiFreeSectorID;
}

uint32_t FileSystem::deallocateSector(uint32_t currentSectorId) {
  auto uiNextSectorID = getSectorEntryValue(currentSectorId);
  setSectorEntryValue(currentSectorId, 0);
  _freePoolQueue.push_back(currentSectorId);
  return uiNextSectorID;
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

  if(!(fileType == ATTR_TYPE_FILE || fileType == ATTR_TYPE_DIRECTORY)) {
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

  FileNodeRef::WriteGuard g(parentNodeRef);

  if (!parentNode.find(newFileName).isEmpty()) {
    throw upan::exception(XLOC, "%s %s already exists", newFileName.c_str(), (FILE_TYPE(fileType) == ATTR_TYPE_FILE ? "file" : "directory"));
  }

  uint8_t parentDirBuffer[FileSystem::SECTOR_SIZE];
  _diskDrive.xRead(parentDirBuffer, parentNode.sectorId(), 1);
  auto& parentFileNode = reinterpret_cast<FileNode*>(parentDirBuffer)[parentNode.sectorOffset()];

  if(!process.hasFilePermission(parentFileNode, O_RDWR)) {
    throw upan::exception(XLOC, "insufficient permission to create file: %s", newFileName.c_str());
  }

  uint32_t newSectorId;
  uint8_t newSectorOffset;

  if (!parentNode.getFreeSlot(newSectorId, newSectorOffset)) {
    newSectorId = allocateSector();
    newSectorOffset = 0;
    auto lastSectorId = parentNode.getDirLastSectorId();
    if (lastSectorId == EOC) {
      parentFileNode.StartSectorID(newSectorId);
    } else {
      setSectorEntryValue(lastSectorId, newSectorId);
    }
  }

  uint8_t newSectorBuffer[FileSystem::SECTOR_SIZE];
  _diskDrive.xRead(newSectorBuffer, newSectorId, 1);
  auto& newFileNode = reinterpret_cast<FileNode*>(newSectorBuffer)[newSectorOffset];
  newFileNode.Init(newFileName.c_str(), getFileAttr(fileType, mode), process.userID(), newSectorId, newSectorOffset);

  parentFileNode.AddNode();

  _diskDrive.xWrite(parentDirBuffer, parentNode.sectorId(), 1);
  _diskDrive.xWrite(newSectorBuffer, newSectorId, 1);

  _fileTree.addNode(parentNode, newFileNode);
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
    if ( (mode & O_APPEND) || (mode & O_CREAT) ) {
      FileTree::NodeTokens dirTokens(fileTokens);
      dirTokens.pop_back();
      const upan::string& fileName = fileTokens.back();
      create(dirTokens, fileName, ATTR_TYPE_FILE, ATTR_FILE_DEFAULT, cwd, process);
      fileNodeRef = _fileTree.getFileNodeRef(fileTokens, cwd);
      newFileCreated = true;
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
  }
  return fileNodeRef;
}

FileNodeRef FileSystem::exists(const FileTree::NodeTokens& fileTokens, const FileNodeRef& cwd) {
  return _fileTree.getFileNodeRef(fileTokens, cwd);
}

struct stat FileSystem::stats(const FileTree::NodeTokens& fileTokens, const FileNodeRef& cwd) {
  FileNodeRef fileNodeRef = _fileTree.getFileNodeRef(fileTokens, cwd);
  if (fileNodeRef.empty()) {
    throw upan::exception(XLOC, "no such file or directory : %s", fileTokens.back().c_str());
  }
  return stats(fileNodeRef);
}

struct stat FileSystem::stats(FileNodeRef fileNodeRef) {
  FileNodeRef::ReadGuard g(fileNodeRef);
  auto& node = fileNodeRef.nodev();

  uint8_t sectorBuffer[FileSystem::SECTOR_SIZE];
  _diskDrive.xRead(sectorBuffer, node.sectorId(), 1);
  auto& fileNode = reinterpret_cast<FileNode*>(sectorBuffer)[node.sectorOffset()];
  
  struct stat fileStat{};

  fileStat.st_dev = _diskDrive.DriveNumber();
  fileStat.st_mode = fileNode.Attribute() ;
  fileStat.st_uid = fileNode.UserID() ;
  fileStat.st_size = fileNode.Size() ;
  fileStat.st_atime = fileNode.AccessedTime() ;
  fileStat.st_mtime = fileNode.ModifiedTime() ;
  fileStat.st_ctime = fileNode.CreatedTime() ;

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
