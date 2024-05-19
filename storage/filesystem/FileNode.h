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
#include <ustring.h>
#include <fs.h>

class StorageDrive;

class FileNode {
public:
  void Init(const char *szDirName, uint16_t usDirAttribute, int iUserID, uint32_t uiParentSecNo, uint8_t bParentSecPos);

  void InitAsRoot();

  upan::string FullPath(StorageDrive &diskDrive);

  const char *Name() const { return (const char *) _name; }
  const struct timeval &CreatedTime() const { return _createdTime; }
  const struct timeval &AccessedTime() const { return _accessedTime; }
  void AccessedTime(const uint32_t tSec) { _accessedTime.tSec = tSec; }
  const struct timeval &ModifiedTime() const { return _modifiedTime; }
  void ModifiedTime(const uint32_t tSec) { _modifiedTime.tSec = tSec; }
  uint16_t ParentSectorPos() const { return _parentSectorPos; }
  uint16_t Attribute() const { return _attribute; }
  uint32_t ParentSectorID() const { return _parentSectorID; }
  int UserID() const { return _userID; }
  uint32_t Size() const { return _size; }
  uint32_t StartSectorID() const { return _startSectorID; }

  bool IsDirectory() const { return (_attribute & ATTR_TYPE_DIRECTORY) == ATTR_TYPE_DIRECTORY; }
  bool IsFile() const { return (_attribute & ATTR_TYPE_FILE) == ATTR_TYPE_FILE; }
  bool IsDeleted() const { return (_attribute & ATTR_DELETED_DIR) != 0; }

  void Size(uint32_t s) { _size = s; }
  void AddNode() { ++_size; }
  void RemoveNode() { --_size; }
  void MarkAsDeleted() { _attribute |= ATTR_DELETED_DIR; }
  void StartSectorID(const uint32_t sectorId) { _startSectorID = sectorId; }

private:
  char            _name[33];
  struct timeval  _createdTime;
  struct timeval  _accessedTime;
  struct timeval  _modifiedTime;
  uint8_t         _parentSectorPos;
  uint16_t        _attribute;
  uint32_t        _size;
  uint32_t        _startSectorID;
  uint32_t        _parentSectorID;
  int             _userID;
} PACKED;