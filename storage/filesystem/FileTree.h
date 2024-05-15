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
#include <ustring.h>
#include <rwlock.h>
#include <FileNode.h>
#include <mutex.h>

class FileSystem;

class FileTree {
public:
  FileTree();
  ~FileTree();

  class Node {
  public:
    Node(const Node* parent, const FileNode& fileNode);
    ~Node();

    bool isRoot() const { return _parent == nullptr; }
    const upan::string& name() const { return _name; }
    uint32_t startSectorId() const { return _startSectorId; }
    uint8_t sectorOffset() const { return _sectorOffset; }
    bool isFile() const { return _isFile; };
    bool isDirectory() const { return !isFile(); }
    bool isDeleted() const { return _isDeleted; }
    uint32_t size() const { return _size; }

    void name(const upan::string& name) { _name = name; }
    void size(const uint32_t size) { _size = size; }

    void markAsDeleted() { _isDeleted = true; }

  private:
    void Load(StorageDrive& storageDrive);

  private:
    const Node* _parent;
    upan::string _name;
    const uint32_t _startSectorId;
    const uint8_t _sectorOffset:6;
    uint8_t _isFile:1;
    uint8_t _isDeleted:1;
    uint32_t _size;
    int _openCount;
    upan::rwlock _rwlock;

    typedef upan::map<upan::string, Node*> SubNodes;
    SubNodes _subNodes;

    friend class FileTree;
  };

private:
  void Initialize(StorageDrive& storageDrive);
  void Uninitialize();

private:
  Node* _root;
  upan::mutex _treeMutex;

  friend class FileSystem;
};