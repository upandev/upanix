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
#include <UserManager.h>
#include <SystemUtil.h>
#include <DMM.h>

#define BLOCK_ID(SectorID) ((SectorID) / ENTRIES_PER_TABLE_SECTOR)
#define BLOCK_OFFSET(SectorID) ((SectorID) % ENTRIES_PER_TABLE_SECTOR)

SectorBlockEntry::SectorBlockEntry(StorageDrive& diskDrive, uint32_t tableSectorId, uint32_t blockId) : _blockId(blockId), _readCount(0), _writeCount(0) {
  diskDrive.Read(tableSectorId, 1, (byte*)_sectorBlock);
}

uint32_t SectorBlockEntry::Read(uint32_t sectorId) {
  ++_readCount;
  return _sectorBlock[BLOCK_OFFSET(sectorId)] & EOC ;
}

void SectorBlockEntry::Write(uint32_t sectorId, uint32_t value) {
  auto index = BLOCK_OFFSET(sectorId) ;
  _sectorBlock[index] = _sectorBlock[index] & 0xF0000000;
  _sectorBlock[index] = _sectorBlock[index] | (value & EOC);
  ++_writeCount;
}

FileSystem::FileSystem(StorageDrive &diskDrive, uint32_t freePoolSize) : _diskDrive(diskDrive), _freePoolQueue(freePoolSize) {
}

void FileSystem::Format() {
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

  auto uiSec = GetRealSectorNumber(0);
  ((FileNode*)bSectorBuffer)->InitAsRoot(uiSec);
  _diskDrive.Write(uiSec, 1, bSectorBuffer);
  /*************************** Root Directory [END] ********************************/

  _diskDrive.FlushAllDirtyCacheSectors();
}

void FileSystem::Mount() {
  _bootBlock.load(_diskDrive);
  LoadFreeSectors();
  ReadRootDirectory();
}

void FileSystem::Unmount() {
  _bootBlock.store(_diskDrive);
  FlushTableCache(MAX_SECTORS_IN_TABLE_CACHE);
  _freePoolQueue.clear();
  _diskDrive.FlushAllDirtyCacheSectors();
}

void FileSystem::ReadRootDirectory() {
  byte bDataBuffer[512];
  _diskDrive.xRead(bDataBuffer, 0, 1);
  _pwd.Init(*reinterpret_cast<FileNode*>(bDataBuffer), 0, 0);
}

void FileSystem::LoadFreeSectors() {
  if(_freePoolQueue.full())
    return;

  bool bStop = false;

  // First do Cache Lookup
  for(const auto& block : _fsTableCache) {
    if(bStop) {
      break;
    }

    auto uiSectorBlock = block.second->SectorBlock();
    for(int j = 0; j < ENTRIES_PER_TABLE_SECTOR; j++) {
      if(!(uiSectorBlock[j] & EOC)) {
        const uint32_t uiSectorID = block.second->BlockId() * ENTRIES_PER_TABLE_SECTOR + j;
        if(!_freePoolQueue.push_back(uiSectorID)) {
          bStop = true;
          break;
        }
      }
    }
  }

  if(bStop) {
    return;
  }

  byte bBuffer[ 4096 ];

  for(unsigned i = 0; i < _bootBlock.getTableSize(); ) {
    if(bStop) {
      break;
    }

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
          bStop = true;
          break;
        }
      }
    }

    i += uiBlockSize;
  }
}

void FileSystem::FlushTableCache(int flushSize) {
  if(flushSize > _fsTableCache.size()) {
    flushSize = _fsTableCache.size();
  }

  for(auto i = _fsTableCache.begin(); i != _fsTableCache.end() && flushSize > 0;) {
    auto e = i->second;
    if (e->WriteCount() != 0) {
      _diskDrive.Write(e->BlockId() + _bootBlock.getReservedSectorCount() + 1, 1, (byte*)(e->SectorBlock()));
    }
    delete e;
    _fsTableCache.erase(i++);
    --flushSize;
  }
}

void FileSystem::AddToTableCache(uint32_t sectorId) {
  if(_fsTableCache.size() == MAX_SECTORS_IN_TABLE_CACHE) {
    FlushTableCache(1);
  }

  const auto blockId = BLOCK_ID(sectorId);
  auto r = _fsTableCache.find(blockId);
  if (r != _fsTableCache.end()) {
    return;
  }

  const auto tableSectorId = GetTableSectorId(blockId);
  _fsTableCache.insert(TableCache::value_type(blockId, new SectorBlockEntry(_diskDrive, tableSectorId, blockId)));
}

SectorBlockEntry* FileSystem::GetSectorEntryFromCache(uint32_t sectorId) {
  if(_fsTableCache.empty()) {
    return nullptr;
  }
  auto r = _fsTableCache.find(BLOCK_ID(sectorId));
  return (r != _fsTableCache.end()) ? r->second : nullptr;
}

uint32_t FileSystem::AllocateSector() {
  if(_freePoolQueue.empty()) {
    LoadFreeSectors();
    if(_freePoolQueue.empty())
      throw upan::exception(XLOC, "No free sectors available on disk: %s", _diskDrive.DriveName().c_str());
  }

  auto uiFreeSectorID = _freePoolQueue.front();
  _freePoolQueue.pop_front();

  SetSectorEntryValue(uiFreeSectorID, EOC);
  return uiFreeSectorID;
}

uint32_t FileSystem::DeallocateSector(uint32_t currentSectorId) {
  auto uiNextSectorID = GetSectorEntryValue(currentSectorId);
  SetSectorEntryValue(currentSectorId, 0);
  AddToFreePoolCache(currentSectorId);
  return uiNextSectorID;
}

void FileSystem::DisplayCache() {
  printf("\nSTART\n");
  for(const auto& block : _fsTableCache)
    printf(", %u", block.second->BlockId());
  printf(" :: SIZE = %d", _fsTableCache.size());
}

uint32_t FileSystem::GetTableSectorId(uint32_t uiSectorID) const
{
  return uiSectorID + 1/*BPB*/ + _bootBlock.getReservedSectorCount();
}

uint32_t FileSystem::GetRealSectorNumber(uint32_t uiSectorID) const
{
  return uiSectorID + 1/*BPB*/
          + _bootBlock.getReservedSectorCount()
         + _bootBlock.getTableSize();
}

void FileSystem::UpdateUsedSectors(uint32_t uiSectorEntryValue)
{
  if(uiSectorEntryValue == EOC)
    _bootBlock.incUserSectors();
  else if(uiSectorEntryValue == 0)
    _bootBlock.decUserSectors();
}

uint32_t FileSystem::GetSectorEntryValue(const uint32_t uiSectorID) {
  if(uiSectorID > (_bootBlock.getTableSize() * _bootBlock.getBytesPerSector() / 4)) {
    throw upan::exception(XLOC, "invalid cluster id: %u", uiSectorID);
  }

  SectorBlockEntry* pSectorBlockEntry = GetSectorEntryFromCache(uiSectorID) ;

  if(pSectorBlockEntry == nullptr) {
    AddToTableCache(uiSectorID);
    pSectorBlockEntry = GetSectorEntryFromCache(uiSectorID) ;
  }

  if(pSectorBlockEntry == nullptr) {
    throw upan::exception(XLOC, "sector entry value not found in cache for sector:%u", uiSectorID);
  }

  return pSectorBlockEntry->Read(uiSectorID);
}

void FileSystem::SetSectorEntryValue(const uint32_t uiSectorID, uint32_t uiSectorEntryValue)
{
  if(uiSectorID > (_bootBlock.getTableSize() * _bootBlock.getBytesPerSector() / 4))
    throw upan::exception(XLOC, "invalid cluster id: %u", uiSectorID);

  UpdateUsedSectors(uiSectorEntryValue);

  SectorBlockEntry* pSectorBlockEntry = GetSectorEntryFromCache(uiSectorID) ;

  if(pSectorBlockEntry == NULL)
  {
    AddToTableCache(uiSectorID);
    pSectorBlockEntry = GetSectorEntryFromCache(uiSectorID) ;
  }

  if(pSectorBlockEntry == NULL)
    throw upan::exception(XLOC, "Sector block for sector id %d is not in cache", uiSectorID);

  pSectorBlockEntry->Write(uiSectorID, uiSectorEntryValue);
}