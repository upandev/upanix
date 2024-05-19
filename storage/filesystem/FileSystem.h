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
#include <map.h>
#include <FileNode.h>
#include <BootBlock.h>
#include <FSTableCache.h>
#include <FileTree.h>
#include <FileNodeRef.h>

#define MEDIA_REMOVABLE	0xF0
#define MEDIA_FIXED		0xF8

#define EOC		0x0FFFFFFF
#define EOC_B	0xFF

#define FS_ROOT_DIR "/"
#define DIR_SPECIAL_CURRENT		"."
#define DIR_SPECIAL_PARENT		".."

class StorageDrive;
class Process;

class FileSystem {
public:
  static const int SECTOR_SIZE = 512;
  static const int DIR_ENTRIES_PER_SECTOR = SECTOR_SIZE / sizeof(FileNode);

  FileSystem(StorageDrive &diskDrive, uint32_t freePoolSize);

  ~FileSystem() = default;

  uint64_t totalSize() const { return _bootBlock.getTableSize() * ENTRIES_PER_TABLE_SECTOR * 512; }
  uint64_t usedSize() const { return _bootBlock.getUsedSectors() * 512; }

  uint32_t allocateSector();
  uint32_t deallocateSector(uint32_t currentSectorId);

  uint32_t getRealSectorNumber(uint32_t uiSectorID) const;
  uint32_t getSectorEntryValue(uint32_t uiSectorID) {
    return _fsTableCache.get(uiSectorID);
  }

  void setSectorEntryValue(uint32_t uiSectorID, uint32_t uiSectorEntryValue) {
    _fsTableCache.set(uiSectorID, uiSectorEntryValue);
  }

  FileNodeRef root() { return _root; }

  void create(const FileTree::NodeTokens &fileTokens, const upan::string &newFileName,
              uint16_t fileType, uint16_t mode,
              const FileNodeRef &cwd, Process &process);

private:
  uint16_t getFileAttr(uint16_t fileType, uint16_t mode);
  void readRootDirectory();
  void loadFreeSectors();

public:
  class PresentWorkingDirectory {
  public:
    void Init(const FileNode &node, uint32_t sectorId, uint16_t sectorEntryPos) {
      _node = node;
      _sectorId = sectorId;
      _sectorEntryPos = sectorEntryPos;
    }

    FileNode &getNode() { return _node; }
    const FileNode &getNode() const { return _node; }
    void setNode(const FileNode &node) { _node = node; }
    uint32_t getSectorId() const { return _sectorId; }
    uint16_t getSectorEntryPos() const { return _sectorEntryPos; }

  private:
    FileNode _node;
    uint32_t _sectorId;
    uint8_t _sectorEntryPos;
  };

  class WorkingDirectory {
  public:
    WorkingDirectory() : _node(nullptr), _sectorId(0), _sectorEntryPos(0) {}

    WorkingDirectory(FileNode *node, uint32_t sectorId, uint8_t sectorEntryPos) : _node(node), _sectorId(sectorId),
                                                                                  _sectorEntryPos(sectorEntryPos) {
    }

    WorkingDirectory(PresentWorkingDirectory &pwd) {
      *this = pwd;
    }

    WorkingDirectory &operator=(PresentWorkingDirectory &pwd) {
      _node = &pwd.getNode();
      _sectorId = pwd.getSectorId();
      _sectorEntryPos = pwd.getSectorEntryPos();
      return *this;
    }

    FileNode *getNode() { return _node; }
    const FileNode *getNode() const { return _node; }
    uint32_t getSectorId() const { return _sectorId; }
    uint16_t getSectorEntryPos() const { return _sectorEntryPos; }

  private:
    FileNode *_node;
    uint32_t _sectorId;
    uint8_t _sectorEntryPos;
  };

private:
  void format();
  void mount();
  void unmount();
  void checkIfMounted();

public:
  PresentWorkingDirectory &pwd() { return _pwd; }

private:
  StorageDrive &_diskDrive;
  BootBlock _bootBlock;
  upan::queue<uint32_t> _freePoolQueue;
  FSTableCache _fsTableCache;
  FileTree _fileTree;

  FileNodeRef _root;
  PresentWorkingDirectory _pwd;

  friend class StorageDrive;
};

