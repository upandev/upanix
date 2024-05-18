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

  ((FileNode*)bSectorBuffer)->InitAsRoot(0/*uiSec*/);
  _diskDrive.xWrite(bSectorBuffer, 0, 1);
  /*************************** Root Directory [END] ********************************/

  _diskDrive.FlushAllDirtyCacheSectors();
  _root.clear();
}

void FileSystem::mount() {
  _bootBlock.load(_diskDrive);
  loadFreeSectors();
  readRootDirectory();
  _fileTree.Initialize(_diskDrive);
  _root = _fileTree._root;
}

void FileSystem::unmount() {
  _bootBlock.store(_diskDrive);
  _fsTableCache.flush();
  _freePoolQueue.clear();
  _diskDrive.FlushAllDirtyCacheSectors();
  _fileTree.Uninitialize();
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
