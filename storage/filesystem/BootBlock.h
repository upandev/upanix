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

class StorageDrive;

class BootBlock {
public:
  void initialize(StorageDrive& storageDrive);
  void load(StorageDrive& storageDrive);
  void store(StorageDrive& storageDrive);

  uint32_t getTableSize() const { return BPB_FSTableSize; }
  uint16_t getReservedSectorCount() const { return BPB_RsvdSecCnt; }
  uint16_t getBytesPerSector() const { return BPB_BytesPerSec; }
  uint32_t getUsedSectors() const { return _usedSectors; }

  void incUserSectors() { ++_usedSectors; }
  void decUserSectors() { --_usedSectors; }

private:
  uint8_t  BPB_jmpBoot[3];

  uint8_t  BPB_Media;
  uint16_t BPB_SecPerTrk;
  uint16_t BPB_NumHeads;

  uint16_t BPB_BytesPerSec;
  uint32_t BPB_TotSec32;
  uint32_t BPB_HiddSec;

  uint16_t BPB_RsvdSecCnt;
  uint32_t BPB_FSTableSize;

  uint16_t BPB_ExtFlags;
  uint16_t BPB_FSVer;
  uint16_t BPB_FSInfo;

  uint8_t  BPB_BootSig;
  uint32_t BPB_VolID;
  uint8_t  BPB_VolLab[11 + 1];

  uint32_t _usedSectors;
} PACKED;
