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
#ifndef _PARTITION_MANAGER_H_
#define _PARTITION_MANAGER_H_

#include <Global.h>
#include <ATADeviceController.h>
#include <DeviceDrive.h>

#define MAX_NO_OF_PRIMARY_PARTITIONS	4
#define MAX_NO_OF_EXT_PARTITIONS		32

class GPTHeader
{
  public:
    void CheckSignature() const;
    uint64_t PartArrStartLBA() const { return _partArrStartLBA; }
  private:
    uint8_t  _signature[8]; //"EFI PART", 45h 46h 49h 20h 50h 41h 52h 54h or 0x5452415020494645ULL[a] on little-endian machines
    uint32_t _revision; //for GPT version 1.0 (through at least UEFI version 2.3.1), the value is 00h 00h 01h 00h
    uint32_t _headerSize; //usually 92 bytes
    uint32_t _crc32; //offset +0 up to header size, with this field zeroed during calculation
    uint32_t _reserved; //must be zero
    uint64_t _currentLBA; //location of this header copy
    uint64_t _backupLBA; //location of the other header copy
    uint64_t _firstUsableLBA; //primary partition table last LBA + 1
    uint64_t _lastUsableLBA; //secondary partition table first LBA - 1
    uint8_t  _diskGUID[16]; //also referred as UUID on UNIXes
    uint64_t _partArrStartLBA; //Starting LBA of array of partition entries (always 2 in primary copy)
    uint32_t _partArrSize; //Number of partition entries in array
    uint32_t _partEntrySize; //Size of a single partition entry (usually 80h or 128)
    uint32_t _partArrCRC32; //CRC32 of partition array
} PACKED;

class GPTPartitionEntry
{
  public:
    bool IsEmpty() const;
    uint64_t FirstLBA() const { return _firstLBA; }
    uint64_t Size() const { return _lastLBA - _firstLBA + 1; }
  private:
    uint8_t  _partitionTypeGUID[16];
    uint8_t  _uniquePartitionGUID[16];
    uint64_t _firstLBA;
    uint64_t _lastLBA; //inclusive, usually odd
    uint64_t _attrFlags; //Attribute flags (e.g. bit 60 denotes read-only)
    uint8_t  _partitionName[72];
};

class MBRPartitionInfo
{
  public:
    enum Types { ACTIVE, EXTENEDED, NORMAL };

    MBRPartitionInfo() :
      BootIndicator(0),
      _StartHead(0),
      _StartSector(0),
      _StartCylinder(0),
      SystemIndicator(0),
      _EndHead(0),
      _EndSector(0),
      _EndCylinder(0),
      LBAStartSector(0),
      LBANoOfSectors(0)
    {
    }

    MBRPartitionInfo(unsigned lbaStart, unsigned size, Types type);
	  bool IsEmpty() const 
    {
      return SystemIndicator == 0x0 || LBANoOfSectors == 0x0;
    }

	byte		BootIndicator;

	byte		_StartHead;
	byte		_StartSector;
	byte		_StartCylinder;

	byte		SystemIndicator;

	byte		_EndHead;
	byte		_EndSector;
	byte		_EndCylinder;

	unsigned 	LBAStartSector;
	unsigned 	LBANoOfSectors;

} PACKED;

class PartitionEntry
{
  public:
    PartitionEntry(unsigned startSector, unsigned sizeInSectors, byte fsId) :
      _uiLBAStartSector(startSector), _uiLBASize(sizeInSectors), _fsId(fsId)
    {
    }

    unsigned LBAStartSector() const { return _uiLBAStartSector; }
    unsigned LBASize() const { return _uiLBASize; }
    byte FileSystemID() const { return _fsId; }

  private:
    const unsigned _uiLBAStartSector;
    const unsigned _uiLBASize;
    const byte     _fsId;
};

class ExtPartitionTable
{
  public:
    ExtPartitionTable(const MBRPartitionInfo& cur, unsigned actualStartSec)
      : _currentPartition(cur), _uiActualStartSector(actualStartSec)
    {
    }
    const MBRPartitionInfo& CurrentPartition() const { return _currentPartition; }
    unsigned ActualStartSector() const { return _uiActualStartSector; }
  private:
    MBRPartitionInfo _currentPartition;
    unsigned	_uiActualStartSector;
};

class PartitionTable
{
  public:
    PartitionTable(RawDiskDrive& disk);
    ~PartitionTable();

    void VerbosePrint() const;
    bool IsEmpty() const { return _partitions.empty(); }
    const upan::list<PartitionEntry> GetPartitions() const { return _partitions; }
    void ClearPartitionTable();
    void CreatePrimaryPartitionEntry(unsigned uiSizeInSectors, MBRPartitionInfo::Types);
    void DeletePrimaryPartition();
    void CreateExtPartitionEntry(unsigned uiSizeInSectors);
    void DeleteExtPartition();

  private:
    void ReadGPT();
    void ReadMBRPartition(const MBRPartitionInfo*);
    void VerifyMBR(const MBRPartitionInfo*);

    bool _bIsGPTPartition;
    bool _bIsMBRPartition;
    bool _bIsExtPartitionPresent;
    RawDiskDrive& _disk;
    MBRPartitionInfo _extPartitionEntry;
    upan::list<MBRPartitionInfo*> _primaryPartitions;
    upan::list<ExtPartitionTable*> _extPartitions;
    upan::list<PartitionEntry> _partitions;
};

#endif
