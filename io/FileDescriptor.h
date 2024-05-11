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

#include <IODescriptor.h>
#include <FileSystem.h>

class StorageDrive;

class FileDescriptor : public IODescriptor {
public:
  FileDescriptor(int pid, int fd, byte mode,
                 const upan::string& nodeId,
                 const upan::string& fileName,
                 StorageDrive& diskDrive, uint32_t startSectorID);

  int read(void* buffer, int len) override;
  bool canRead() override {
    return true;
  }

  int write(const void* buffer, int len) override;
  bool canWrite() override {
    return true;
  }

  void seek(int seekType, int offset) override;

  FileSystem_FileStat getStat();

  const upan::string& getFileName() const {
    return _fileName;
  }

  uint32_t getOffset() const override {
    return _offset;
  }

  int getLastReadSectorIndex() const {
    return _lastReadSectorIndex;
  }

  void setLastReadSectorIndex(int v) {
    _lastReadSectorIndex = v;
  }

  uint32_t getLastReadSectorNo() const {
    return _lastReadSectorNo;
  }

  void setLastReadSectorNo(uint32_t v) {
    _lastReadSectorNo = v;
  }

  void getLastReadSectorDetails(FileSystem::Node&, int& sectorIndex, uint32_t& sectorId);
  void setLastReadSectorDetails(int sectorIndex, uint32_t sectorId);

  void setOffset(uint32_t offset) {
    _offset = offset;
  }

private:
  FileSystem::PresentWorkingDirectory& getWorkingDirectory();

private:
  const upan::string _fileName;
  const upan::string _nodeId;
  StorageDrive& _diskDrive;
  uint32_t _offset;
  int _lastReadSectorIndex;
  uint32_t _lastReadSectorNo;
};
