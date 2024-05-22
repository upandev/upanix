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

#include <FSTableCache.h>
#include <StorageDrive.h>

#define BLOCK_ID(SectorID) ((SectorID) / ENTRIES_PER_TABLE_SECTOR)
#define BLOCK_OFFSET(SectorID) ((SectorID) % ENTRIES_PER_TABLE_SECTOR)

void FSTableCache::loadFreeSectors(upan::queue<uint32_t>& pool) {
  upan::wlock_gaurd g(_rwlock);

  for(const auto& block : _tableCache) {
    auto uiSectorBlock = block.second->Block();
    for(int j = 0; j < ENTRIES_PER_TABLE_SECTOR; j++) {
      if(!(uiSectorBlock[j] & EOC)) {
        const uint32_t uiSectorID = block.second->BlockId() * ENTRIES_PER_TABLE_SECTOR + j;
        if(!pool.push_back(uiSectorID)) {
          return;
        }
      }
    }
  }
}

uint32_t FSTableCache::get(uint32_t uiSectorID) {
  upan::rlock_gaurd g(_rwlock);

  if(uiSectorID > (_bootBlock.getTableSize() * _bootBlock.getBytesPerSector() / 4)) {
    throw upan::exception(XLOC, "invalid cluster id: %u", uiSectorID);
  }

  SectorBlock* pSectorBlockEntry = getSectorBlock(uiSectorID) ;

  if(pSectorBlockEntry == nullptr) {
    _rwlock.read_unlock();
    add(uiSectorID);
    _rwlock.read_lock();
    pSectorBlockEntry = getSectorBlock(uiSectorID) ;
  }

  if(pSectorBlockEntry == nullptr) {
    throw upan::exception(XLOC, "sector entry value not found in cache for sector:%u", uiSectorID);
  }

  return pSectorBlockEntry->Read(uiSectorID);
}

void FSTableCache::set(const uint32_t uiSectorID, uint32_t uiSectorEntryValue) {
  upan::wlock_gaurd g(_rwlock);

  if(uiSectorID > (_bootBlock.getTableSize() * _bootBlock.getBytesPerSector() / 4))
    throw upan::exception(XLOC, "invalid cluster id: %u", uiSectorID);

  if(uiSectorEntryValue == EOC) {
    _bootBlock.incUserSectors();
  } else if(uiSectorEntryValue == 0) {
    _bootBlock.decUserSectors();
  }

  SectorBlock* pSectorBlockEntry = getSectorBlock(uiSectorID) ;

  if(pSectorBlockEntry == nullptr) {
    add(uiSectorID);
    pSectorBlockEntry = getSectorBlock(uiSectorID) ;
  }

  if(pSectorBlockEntry == nullptr) {
    throw upan::exception(XLOC, "Sector block for sector id %d is not in cache", uiSectorID);
  }

  pSectorBlockEntry->Write(uiSectorID, uiSectorEntryValue);
}

uint32_t FSTableCache::getTableSectorId(uint32_t uiSectorID) const {
  return uiSectorID + 1/*BPB*/ + _bootBlock.getReservedSectorCount();
}

FSTableCache::SectorBlock* FSTableCache::getSectorBlock(uint32_t sectorId) {
  upan::rlock_gaurd g(_rwlock);

  if(_tableCache.empty()) {
    return nullptr;
  }
  auto r = _tableCache.find(BLOCK_ID(sectorId));
  return (r != _tableCache.end()) ? r->second : nullptr;
}

void FSTableCache::add(uint32_t sectorId) {
  upan::wlock_gaurd g(_rwlock);

  if(_tableCache.size() == MAX_SECTORS_IN_TABLE_CACHE) {
    flush(1);
  }

  const auto blockId = BLOCK_ID(sectorId);
  auto r = _tableCache.find(blockId);
  if (r != _tableCache.end()) {
    return;
  }

  const auto tableSectorId = getTableSectorId(blockId);
  _tableCache.insert(TableCache::value_type(blockId, new SectorBlock(_storageDrive, tableSectorId, blockId)));
}

void FSTableCache::flush(int flushSize) {
  upan::wlock_gaurd g(_rwlock);

  if(flushSize > _tableCache.size()) {
    flushSize = _tableCache.size();
  }

  for(auto i = _tableCache.begin(); i != _tableCache.end() && flushSize > 0;) {
    auto e = i->second;
    if (e->WriteCount() != 0) {
      _storageDrive.Write(e->BlockId() + _bootBlock.getReservedSectorCount() + 1, 1, (byte*)(e->Block()));
    }
    delete e;
    _tableCache.erase(i++);
    --flushSize;
  }
}

void FSTableCache::debugPrint() {
  upan::rlock_gaurd g(_rwlock);

  printf("\nSTART\n");
  for(const auto& block : _tableCache)
    printf(", %u", block.second->BlockId());
  printf(" :: SIZE = %d", _tableCache.size());
}

FSTableCache::SectorBlock::SectorBlock(StorageDrive& diskDrive, uint32_t tableSectorId, uint32_t blockId) : _blockId(blockId), _readCount(0), _writeCount(0) {
  diskDrive.Read(tableSectorId, 1, (byte*)_block);
}

uint32_t FSTableCache::SectorBlock::Read(uint32_t sectorId) const {
  _readCount.inc();
  return _block[BLOCK_OFFSET(sectorId)] & EOC ;
}

void FSTableCache::SectorBlock::Write(uint32_t sectorId, uint32_t value) {
  auto index = BLOCK_OFFSET(sectorId) ;
  _block[index] = _block[index] & 0xF0000000;
  _block[index] = _block[index] | (value & EOC);
  _writeCount.inc();
}
