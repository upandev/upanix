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

#include <Global.h>
#include <queue.h>
#include <vector.h>
#include <fs.h>

#define MEDIA_REMOVABLE	0xF0
#define MEDIA_FIXED		0xF8

#define EOC		0x0FFFFFFF
#define EOC_B	0xFF
#define DIR_ENTRIES_PER_SECTOR 7

#define FS_ROOT_DIR "/"

#define ENTRIES_PER_TABLE_SECTOR	(128)

class StorageDrive;

class SectorBlockEntry {
public:
  uint32_t* SectorBlock() { return _sectorBlock; }
  const uint32_t* SectorBlock() const { return _sectorBlock; }
  const uint32_t BlockId() const { return _blockId; }
  const uint32_t ReadCount() const { return _readCount; }
  const uint32_t WriteCount() const { return _writeCount; }

  void Load(StorageDrive& diskDrive, uint32_t sectortId);
  uint32_t Read(uint32_t sectorId);
  void Write(uint32_t sectorId, uint32_t value);

private:
  uint32_t _sectorBlock[ENTRIES_PER_TABLE_SECTOR];
  uint32_t _blockId;
  uint32_t _readCount;
  uint32_t _writeCount;
} PACKED;

class FileSystem
{
  public:
    FileSystem(StorageDrive& diskDrive) : _diskDrive(diskDrive), _freePoolQueue(nullptr) {
    }
    ~FileSystem() {
      delete _freePoolQueue;
    }

    uint64_t TotalSize() const { return _fsBootBlock.BPB_FSTableSize * ENTRIES_PER_TABLE_SECTOR * 512; }
    uint64_t UsedSize() const { return _fsBootBlock._usedSectors * 512; }

    void Format();
    void Mount(uint32_t freePoolSize);
    void Unmount();

    uint32_t AllocateSector();
    uint32_t DeallocateSector(uint32_t currentSectorId);

    uint32_t GetTableSectorId(uint32_t uiSectorID) const;
    uint32_t GetRealSectorNumber(uint32_t uiSectorID) const;
    uint32_t GetSectorEntryValue(uint32_t uiSectorID);
    void SetSectorEntryValue(uint32_t uiSectorID, uint32_t uiSectorEntryValue);

    void DisplayCache();

private:
  static const int MAX_SECTORS_IN_TABLE_CACHE = 2048;

  void ReadFSBootBlock();
  void WriteFSBootBlock();

  void LoadFreeSectors();
  void AddToTableCache(unsigned uiSectorEntry);
  void FlushTableCache(int iFlushSize);
  void AddToFreePoolCache(uint32_t sectorId) { _freePoolQueue->push_back(sectorId); }

  SectorBlockEntry* GetSectorEntryFromCache(unsigned uiSectorEntry);

public:
    class Node {
    public:
      void Init(const char* szDirName, unsigned short usDirAttribute, int iUserID, unsigned uiParentSecNo, byte bParentSecPos);
      void InitAsRoot(uint32_t parentSectorId);
      upan::string FullPath(StorageDrive& diskDrive);

      bool IsDirectory() const { return (_fsnode._attribute & ATTR_TYPE_DIRECTORY) == ATTR_TYPE_DIRECTORY; }
      bool IsFile() const { return (_fsnode._attribute & ATTR_TYPE_FILE) == ATTR_TYPE_FILE; }

      const char* Name() const { return (const char*)_fsnode._name; }
      const struct timeval& CreatedTime() const { return _fsnode._createdTime; }

      const struct timeval& AccessedTime() const { return _fsnode._accessedTime; }
      void AccessedTime(const uint32_t tSec) { _fsnode._accessedTime.tSec = tSec; }

      const struct timeval& ModifiedTime() const { return _fsnode._modifiedTime; }
      void ModifiedTime(const uint32_t tSec) { _fsnode._modifiedTime.tSec = tSec; }

      uint16_t ParentSectorPos() const { return _fsnode._parentSectorPos; }

      uint16_t Attribute() const { return _fsnode._attribute; }
      bool IsDeleted() const { return (_fsnode._attribute & ATTR_DELETED_DIR) != 0; }
      void MarkAsDeleted() { _fsnode._attribute |= ATTR_DELETED_DIR ; }

      uint32_t Size() const { return _fsnode._size; }
      void Size(uint32_t s) { _fsnode._size = s; }
      void AddNode() { ++_fsnode._size; }
      void RemoveNode() { --_fsnode._size; }

      uint32_t StartSectorID() const { return _fsnode._startSectorID; }
      void StartSectorID(const uint32_t sectorId) { _fsnode._startSectorID = sectorId; }

      uint32_t ParentSectorID() const { return _fsnode._parentSectorID; }
      int UserID() const { return _fsnode._userID; }

    private:
      FS_Node _fsnode;
    } PACKED;

    typedef struct
    {
      Node DirEntry ;
      unsigned uiSectorNo ;
      byte bSectorEntryPosition ;
    } PACKED PresentWorkingDirectory ;

    typedef struct
    {
      Node* pDirEntry ;
      unsigned uiSectorNo ;
      byte bSectorEntryPosition ;
    } PACKED CWD ;

    PresentWorkingDirectory FSpwd;

private:
    struct BootBlock {
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

private:
    void InitBootBlock(BootBlock&);
    void UpdateUsedSectors(uint32_t uiSectorEntryValue);

    StorageDrive& _diskDrive;
    BootBlock _fsBootBlock;
    upan::queue<uint32_t>* _freePoolQueue;
    upan::vector<SectorBlockEntry> _fsTableCache;
};

typedef struct {
  int 	    st_dev;     /* ID of device containing file */
  int     	st_ino;     /* inode number */
  uint16_t 	st_mode;    /* protection */
  int   		st_nlink;   /* number of hard links */
  int     	st_uid;     /* user ID of owner */
  int     	st_gid;     /* group ID of owner */
  int     	st_rdev;    /* device ID (if special file) */
  uint32_t  st_size;    /* total size, in bytes */
  uint32_t  st_blksize; /* blocksize for filesystem I/O */
  uint32_t  st_blocks;  /* number of blocks allocated */

  struct timeval st_atime;   /* time of last access */
  struct timeval st_mtime;   /* time of last modification */
  struct timeval st_ctime;   /* time of last status change */
} FileSystem_FileStat;

uint32_t FileSystem_DeAllocateSector(StorageDrive* pDiskDrive, unsigned uiCurrentSectorID) ;
unsigned FileSystem_GetSizeForTableCache(unsigned uiNoOfSectorsInTableCache) ;

