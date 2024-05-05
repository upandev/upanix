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

#define BLOCK_ID(SectorID) (SectorID / ENTRIES_PER_TABLE_SECTOR)
#define BLOCK_OFFSET(SectorID) (SectorID % ENTRIES_PER_TABLE_SECTOR)

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

void FileSystem::InitBootBlock(BootBlock& fsBootBlock)
{
  fsBootBlock.BPB_jmpBoot[0] = 0xEB ; /****************/
  fsBootBlock.BPB_jmpBoot[1] = 0xFE ; /* JMP $ -- ARR */
  fsBootBlock.BPB_jmpBoot[2] = 0x90 ; /****************/

  fsBootBlock.BPB_BytesPerSec = 0x200; // 512 ;
  fsBootBlock.BPB_RsvdSecCnt = 2 ;

  if(_diskDrive.DeviceType() == DEV_FLOPPY)
    fsBootBlock.BPB_Media  = MEDIA_REMOVABLE ;
  else
    fsBootBlock.BPB_Media  = MEDIA_FIXED ;

  fsBootBlock.BPB_SecPerTrk = _diskDrive.SectorsPerTrack();
  fsBootBlock.BPB_NumHeads = _diskDrive.NoOfHeads();
  fsBootBlock.BPB_HiddSec  = 0 ;
  fsBootBlock.BPB_TotSec32 = _diskDrive.SizeInSectors();

/*	pFSBootBlock->BPB_FSTableSize ; ---> Calculated */
  fsBootBlock.BPB_ExtFlags  = 0x0080 ;
  fsBootBlock.BPB_FSVer = 0x0100 ;  //version 1.0
  fsBootBlock.BPB_FSInfo  = 1 ;  //Typical Value for FSInfo Sector

  fsBootBlock.BPB_BootSig = 0x29 ;
  fsBootBlock.BPB_VolID = 0x01 ;  //TODO: Required to be set to current Date/Time of system ---- Not Mandatory
  strcpy((char*)fsBootBlock.BPB_VolLab, "No Name   ") ;  //10 + 1(\0) characters only -- ARR

  fsBootBlock._usedSectors = 1 ;

  fsBootBlock.BPB_FSTableSize = (fsBootBlock.BPB_TotSec32 - fsBootBlock.BPB_RsvdSecCnt - 1) / (ENTRIES_PER_TABLE_SECTOR + 1) ;
}

void FileSystem::Format()
{
  /************************ FAT Boot Block [START] *******************************/
  byte bFSBootBlockBuffer[512] ;
  BootBlock* pFSBootBlock = (BootBlock*)(bFSBootBlockBuffer) ;
  InitBootBlock(*pFSBootBlock);

  bFSBootBlockBuffer[510] = 0x55 ; /* BootSector Signature */
  bFSBootBlockBuffer[511] = 0xAA ;

  _diskDrive.Write(1, 1, bFSBootBlockBuffer);
  /************************* FAT Boot Block [END] **************************/

  /*********************** FAT Table [START] *************************************/
  byte bSectorBuffer[512] ;
  memset(bSectorBuffer, 0, 512);

  for(uint32_t i = 0; i < pFSBootBlock->BPB_FSTableSize; i++)
  {
    if(i == 0)
      ((unsigned*)&bSectorBuffer)[0] = EOC ;

    _diskDrive.Write(i + pFSBootBlock->BPB_RsvdSecCnt + 1, 1, bSectorBuffer);

    if(i == 0)
      ((unsigned*)&bSectorBuffer)[0] = 0 ;
  }
  /*************************** FAT Table [END] **************************************/

  /*************************** Root Directory [START] *******************************/
  memcpy(&_fsBootBlock, pFSBootBlock, sizeof(BootBlock));

  unsigned uiSec = GetRealSectorNumber(0);

  ((FileSystem::Node*)bSectorBuffer)->InitAsRoot(uiSec);

  _diskDrive.Write(uiSec, 1, bSectorBuffer);
  /*************************** Root Directory [END] ********************************/
}

void FileSystem::Mount(uint32_t freePoolSize) {
  _freePoolQueue = new upan::queue<uint32_t>(freePoolSize);
  ReadFSBootBlock();
  LoadFreeSectors();
}

void FileSystem::Unmount() {
  WriteFSBootBlock();
  FlushTableCache(MAX_SECTORS_IN_TABLE_CACHE);
  if(_freePoolQueue) {
    delete _freePoolQueue;
    _freePoolQueue = nullptr;
  }
}

void FileSystem::ReadFSBootBlock() {
  byte bArrFSBootBlock[512];

  _diskDrive.Read(1, 1, bArrFSBootBlock);

  if(bArrFSBootBlock[510] != 0x55 || bArrFSBootBlock[511] != 0xAA)
    throw upan::exception(XLOC, "invalid BPB signature - %x, %x", bArrFSBootBlock[510], bArrFSBootBlock[511]);

  memcpy(&_fsBootBlock, bArrFSBootBlock, sizeof(BootBlock));

  if(_fsBootBlock.BPB_BootSig != 0x29)
    throw upan::exception(XLOC, "invalid BOOT signature: %x", _fsBootBlock.BPB_BootSig);

  // TODO: A write to HD image file from mos fs util is changing the CHS value !!
  // Needs to be fixed. So, this check is skipped for the time being

  /*
  if(fsBootBlock.BPB_SecPerTrk != pDiskDrive->uiSectorsPerTrack)
    return FileSystem_ERR_INVALID_SECTORS_PER_TRACK;

  if(fsBootBlock.BPB_NumHeads != pDiskDrive->uiNoOfHeads)
    return FileSystem_ERR_INVALID_NO_OF_HEADS;
  */

  if(_fsBootBlock.BPB_TotSec32 != _diskDrive.SizeInSectors())
    throw upan::exception(XLOC, "invalid BPB_TotSec32: %d", _fsBootBlock.BPB_TotSec32);

  if(_fsBootBlock.BPB_FSTableSize == 0)
    throw upan::exception(XLOC, "invalid BPB_FSTableSize: %d", _fsBootBlock.BPB_FSTableSize);

  if(_fsBootBlock.BPB_BytesPerSec != 0x200)
    throw upan::exception(XLOC, "invalid BPB_BytesPerSec: %d", _fsBootBlock.BPB_BytesPerSec);

  if(_fsBootBlock.BPB_jmpBoot[0] != 0xEB || _fsBootBlock.BPB_jmpBoot[1] != 0xFE || _fsBootBlock.BPB_jmpBoot[2] != 0x90)
    throw upan::exception(XLOC, "invalid BPB_jmpBoot");

  if(_diskDrive.DeviceType() == DEV_FLOPPY)
    if(_fsBootBlock.BPB_Media != 0xF0)
      throw upan::exception(XLOC, "invalid BPB_Media: %x", _fsBootBlock.BPB_Media);

  if(_fsBootBlock.BPB_ExtFlags != 0x0080)
    throw upan::exception(XLOC, "invalid BPB_ExtFlags: %x", _fsBootBlock.BPB_ExtFlags);

  if(_fsBootBlock.BPB_FSVer != 0x0100)
    throw upan::exception(XLOC, "invalid BPB_FSVer: %x", _fsBootBlock.BPB_FSVer);

  if(_fsBootBlock.BPB_FSInfo != 1)
    throw upan::exception(XLOC, "invalid BPB_FSInfo: %d", _fsBootBlock.BPB_FSInfo);

  if(_fsBootBlock.BPB_VolID != 0x01)
    throw upan::exception(XLOC, "invalid BPB_VolID: %x", _fsBootBlock.BPB_VolID);
}

void FileSystem::WriteFSBootBlock()
{
  byte bSectorBuffer[512];

  bSectorBuffer[510] = 0x55; /* BootSector Signature */
  bSectorBuffer[511] = 0xAA;

  memcpy(bSectorBuffer, &_fsBootBlock, sizeof(BootBlock));

  _diskDrive.Write(1, 1, bSectorBuffer);
}

void FileSystem::LoadFreeSectors() {
  if(_freePoolQueue->full())
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
        if(!_freePoolQueue->push_back(uiSectorID)) {
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

  for(unsigned i = 0; i < _fsBootBlock.BPB_FSTableSize; ) {
    if(bStop) {
      break;
    }

    if (_fsTableCache.exists(i)) {
      ++i;
      continue;
    }

    unsigned uiBlockSize = (_fsBootBlock.BPB_FSTableSize - i);
    if(uiBlockSize > 8)
      uiBlockSize = 8;

    _diskDrive.Read(i + _fsBootBlock.BPB_RsvdSecCnt + 1, uiBlockSize, (byte*)bBuffer);

    auto pTable = (unsigned*)bBuffer;

    for(unsigned j = 0; j < ENTRIES_PER_TABLE_SECTOR * uiBlockSize; j++) {
      if(!(pTable[j] & EOC)) {
        const uint32_t uiSectorID = i * ENTRIES_PER_TABLE_SECTOR + j;
        if(!_freePoolQueue->push_back(uiSectorID)) {
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

  for(auto i = _fsTableCache.begin(); i != _fsTableCache.end() && flushSize > 0; ++i) {
    auto e = i->second;
    if (e->WriteCount() != 0) {
      _diskDrive.Write(e->BlockId() + _fsBootBlock.BPB_RsvdSecCnt + 1, 1, (byte*)(e->SectorBlock()));
    }
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
  return r == _fsTableCache.end() ? nullptr : r->second;
}

uint32_t FileSystem::AllocateSector() {
  if(_freePoolQueue->empty()) {
    LoadFreeSectors();
    if(_freePoolQueue->empty())
      throw upan::exception(XLOC, "No free sectors available on disk: %s", _diskDrive.DriveName().c_str());
  }

  auto uiFreeSectorID = _freePoolQueue->front();
  _freePoolQueue->pop_front();

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
  return uiSectorID + 1/*BPB*/ + _fsBootBlock.BPB_RsvdSecCnt;
}

uint32_t FileSystem::GetRealSectorNumber(uint32_t uiSectorID) const
{
  return uiSectorID + 1/*BPB*/
          + _fsBootBlock.BPB_RsvdSecCnt
          + _fsBootBlock.BPB_FSTableSize;
}

void FileSystem::UpdateUsedSectors(uint32_t uiSectorEntryValue)
{
  if(uiSectorEntryValue == EOC)
    _fsBootBlock._usedSectors++;
  else if(uiSectorEntryValue == 0)
    _fsBootBlock._usedSectors--;
}

uint32_t FileSystem::GetSectorEntryValue(const uint32_t uiSectorID) {
  if(uiSectorID > (_fsBootBlock.BPB_FSTableSize * _fsBootBlock.BPB_BytesPerSec / 4)) {
    throw upan::exception(XLOC, "invalid cluster id: %u", uiSectorID);
  }

  SectorBlockEntry* pSectorBlockEntry = GetSectorEntryFromCache(uiSectorID) ;

  if(pSectorBlockEntry == nullptr) {
    AddToTableCache(uiSectorID);
    pSectorBlockEntry = GetSectorEntryFromCache(uiSectorID) ;
  }

  if(pSectorBlockEntry == nullptr) {
    throw upan::exception(XLOC, "sectory entry value not found in cache for sector:%u", uiSectorID);
  }

  return pSectorBlockEntry->Read(uiSectorID);
}

void FileSystem::SetSectorEntryValue(const uint32_t uiSectorID, uint32_t uiSectorEntryValue)
{
  if(uiSectorID > (_fsBootBlock.BPB_FSTableSize * _fsBootBlock.BPB_BytesPerSec / 4))
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

void FileSystem::Node::Init(const char* szDirName, unsigned short usDirAttribute, int iUserID, unsigned uiParentSecNo, byte bParentSecPos)
{
  strcpy((char*)_fsnode._name, szDirName) ;

  _fsnode._attribute = usDirAttribute ;

  _fsnode._createdTime.tSec = SystemUtil_GetTimeOfDay();
  _fsnode._accessedTime.tSec = _fsnode._createdTime.tSec;
  _fsnode._modifiedTime.tSec = _fsnode._createdTime.tSec;

  _fsnode._startSectorID = EOC ;
  _fsnode._size = 0 ;

  _fsnode._parentSectorID = uiParentSecNo ;
  _fsnode._parentSectorPos = bParentSecPos ;

  _fsnode._userID = iUserID ;
}

void FileSystem::Node::InitAsRoot(uint32_t parentSectorId)
{
  Init(FS_ROOT_DIR, ATTR_DIR_DEFAULT | ATTR_TYPE_DIRECTORY, ROOT_USER_ID, parentSectorId, 0);
}

upan::string FileSystem::Node::FullPath(StorageDrive& diskDrive)
{
  byte bSectorBuffer[512] ;

  const FileSystem::Node* pParseDirEntry = this;

  upan::string fullPath = "";
  upan::string temp = "";

  bool bFirst = true ;

  while(true)
  {
    if(strcmp(pParseDirEntry->Name(), FS_ROOT_DIR) == 0)
    {
      return upan::string(FS_ROOT_DIR) + fullPath;
    }
    else
    {
      upan::string curDir = pParseDirEntry->Name();
      if(!bFirst)
      {
        fullPath = curDir + FS_ROOT_DIR + fullPath;
      }
      else
      {
        fullPath = curDir;
        bFirst = false ;
      }
    }

    unsigned uiParSectorNo = pParseDirEntry->ParentSectorID() ;
    byte bParSectorPos = pParseDirEntry->ParentSectorPos() ;

    diskDrive.xRead(bSectorBuffer, uiParSectorNo, 1);

    pParseDirEntry = &((const FileSystem::Node*)bSectorBuffer)[bParSectorPos] ;
  }

  throw upan::exception(XLOC, "failed to find full path for directory/file %s", _fsnode._name);
}
