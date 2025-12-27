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
#include <FSConstants.h>
#include <option.h>
#include <dirent.h>

class StorageDrive;
class Process;
class FileDescriptor;

class FileSystem {
public:
  static const int SECTOR_SIZE = 512;
  static const int DIR_ENTRIES_PER_SECTOR = SECTOR_SIZE / sizeof(FileNode);

  FileSystem(StorageDrive& diskDrive, uint32_t freePoolSize);

  ~FileSystem() = default;

  uint64_t totalSize() const { return _bootBlock.getTableSize() * ENTRIES_PER_TABLE_SECTOR * SECTOR_SIZE; }
  uint64_t usedSize() const { return _bootBlock.getUsedSectors() * SECTOR_SIZE; }

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

  void create(const FileTree::NodeTokens& fileTokens, const upan::string& newFileName, mode_t mode, const FileNodeRef& cwd, Process& process);
  void remove(const FileTree::NodeTokens& fileTokens, const FileNodeRef& cwd, Process& process,
              bool skipDeleteFile);
  FileNodeRef open(const FileTree::NodeTokens& fileTokens, int flags, mode_t mode, const FileNodeRef& cwd, Process& process);
  void renameFile(const FileTree::NodeTokens& srcFileTokens, const FileTree::NodeTokens& destFileTokens,
                  FileNodeRef& srcCWD, FileNodeRef& destCWD, Process& process);
  void truncate(FileNodeRef fileNodeRef);
  FileNodeRef exists(const FileTree::NodeTokens& fileTokens, const FileNodeRef& cwd);
  upan::option<struct stat> stats(const FileTree::NodeTokens& fileTokens, const FileNodeRef& cwd);
  struct stat stats(FileNodeRef fileNodeRef);
  struct stat stats(const FileNode& node);
  upan::string fullPath(FileNodeRef fileNodeRef);
  bool hasFilePermission(const FileTree::NodeTokens& fileTokens, int mode, const FileNodeRef& cwd, Process& process);
  FileNodeRef openDir(const FileTree::NodeTokens& fileTokens, FileNodeRef cwd, Process& process, DIR& dir);
  void readDir(FileNodeRef fileNodeRef, FileDescriptor& fdEntry, Process& process, DIR& dir);
  int read(FileNodeRef fileNodeRef, FileDescriptor& fdEntry, uint8_t* dataBuffer, int size);
  int write(FileNodeRef fileNodeRef, FileDescriptor& fdEntry, const uint8_t* dataBuffer, int size);
  void updateTime(FileNodeRef fileNodeRef, uint8_t timeType);

private:
  uint16_t getFileAttr(uint16_t fileType, mode_t mode);
  void loadFreeSectors();
  void _bufferedWrite(uint32_t sectorId, const uint8_t* dataBuffer, uint8_t* writeBuffer, unsigned& startSectorId, unsigned& prevSectorId, unsigned& count, bool flush);
  int _write(FileTree::Node& node, FileDescriptor& fdEntry, const uint8_t* dataBuffer, int size);

private:
  void format();
  void mount();
  void unmount();
  void checkIfMounted();

private:
  StorageDrive& _diskDrive;
  BootBlock _bootBlock;
  upan::queue<uint32_t> _freePoolQueue;
  upan::mutex _freePoolMutex;
  FSTableCache _fsTableCache;
  FileTree _fileTree;

  FileNodeRef _root;

  friend class StorageDrive;
};

