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
#include <option.h>
#include <list.h>

class FileSystem;
class FileNodeRef;
class DirSectorBlock;

class FileTree {
public:
  FileTree();
  ~FileTree();

  typedef upan::list<upan::string> NodeTokens;
  upan::option<FileNodeRef> getFileNodeRef(const FileTree::NodeTokens &nodeTokens, const FileNodeRef &cwd);

  class Node {
  public:
    Node(const Node* parent, const FileNode& fileNode);
    ~Node();

    bool isRoot() const { return _parent == nullptr; }
    const upan::string& name() const { return _name; }
    uint32_t startSectorId() const { return _startSectorId; }
    uint8_t sectorOffset() const { return _sectorOffset; }
    uint32_t sectorId() const { return isRoot() ? 0 : _parent->startSectorId(); }
    bool isFile() const { return _isFile; };
    bool isDirectory() const { return !isFile(); }
    uint32_t size() const { return _size; }

    void name(const upan::string& name) { _name = name; }
    void size(const uint32_t size) { _size = size; }

    void incRefCount() { _refCount.inc(); }
    void decRecCount() { _refCount.dec(); }
    bool isReferenced() { return _refCount.get() > 0; }

    upan::option<Node*> find(const upan::string& name);
    bool getFreeSlot(uint32_t& sectorId, uint8_t& sectorOffset);
    uint32_t getDirLastSectorId();
    void addSubNode(const FileNode& fileNode);

    upan::rwlock& rwlock() { return _rwlock; }

  private:
    const Node* _parent;
    upan::string _name;
    uint32_t _startSectorId;
    const uint8_t _sectorOffset:6;
    uint8_t _isFile:1;
    uint32_t _size;
    upan::atomic::integral<int> _refCount;
    upan::rwlock _rwlock;

    typedef upan::list<DirSectorBlock*> DirSectorBlocks;
    DirSectorBlocks _dirSectorBlocks;

    typedef upan::map<upan::string, Node*> SubNodes;
    SubNodes _subNodes;

  private:
    void Load(StorageDrive& storageDrive);
    SubNodes& subNodes() { return _subNodes; }

    friend class FileTree;
  };

  Node* root() { return _root; }

private:
  void initialize(StorageDrive& storageDrive);
  void uninitialize();

private:
  Node* _root;
  upan::mutex _treeMutex;

  friend class FileSystem;
};