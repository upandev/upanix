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
#pragma once

#include <stdlib.h>
#include <map.h>
#include <queue.h>

#define ENTRIES_PER_TABLE_SECTOR	(128)

class StorageDrive;
class BootBlock;

class FSTableCache {
public:
  FSTableCache(StorageDrive& storageDrive, BootBlock& bootBlock) : _storageDrive(storageDrive), _bootBlock(bootBlock) {}

  static const int MAX_SECTORS_IN_TABLE_CACHE = 2048;

  void loadFreeSectors(upan::queue<uint32_t>& pool);
  uint32_t get(uint32_t uiSectorID);
  void set(uint32_t uiSectorID, uint32_t uiSectorEntryValue);
  bool exists(uint32_t sectorId) { return _tableCache.exists(sectorId); }
  void flush() {
    flush(MAX_SECTORS_IN_TABLE_CACHE);
  }

private:
  class SectorBlock {
  public:
    SectorBlock() = delete;
    SectorBlock(StorageDrive &diskDrive, uint32_t tableSectorId, uint32_t blockId);

    uint32_t* Block() { return _block; }
    uint32_t BlockId() const { return _blockId; }
    uint32_t ReadCount() const { return _readCount; }
    uint32_t WriteCount() const { return _writeCount; }

    uint32_t Read(uint32_t sectorId);
    void Write(uint32_t sectorId, uint32_t value);

  private:
    uint32_t _block[ENTRIES_PER_TABLE_SECTOR];
    uint32_t _blockId;
    uint32_t _readCount;
    uint32_t _writeCount;
  };

private:
  uint32_t getTableSectorId(uint32_t uiSectorID) const;
  SectorBlock* getSectorBlock(uint32_t sectorId);
  void add(uint32_t sectorId);
  void flush(int flushSize);
  void debugPrint();

private:
    StorageDrive& _storageDrive;
    BootBlock& _bootBlock;

    typedef upan::map<uint32_t, SectorBlock*> TableCache;
    TableCache _tableCache;
};

