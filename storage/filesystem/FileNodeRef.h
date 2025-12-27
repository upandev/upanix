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

#include <FileTree.h>
#include <option.h>

class FileNodeRef {
public:
  FileNodeRef();
  FileNodeRef(const FileNodeRef& fileNodeRef);
  explicit FileNodeRef(FileTree::Node* node);
  ~FileNodeRef();

  FileNodeRef& operator=(const FileNodeRef& fileNodeRef);
  bool operator==(const FileNodeRef& other) {
    return _node == other._node;
  }
  void set(FileTree::Node* node);
  void clear();
  bool empty() { return _node == nullptr; }

  uint32_t startSectorId() { return nodev().startSectorId(); }
  bool isDirectory() { return nodev().isDirectory(); }
  bool isFile() { return nodev().isFile(); }
  bool isRegularFile() { return nodev().isRegularFile(); }
  bool isChrFile() { return nodev().isChrFile(); }
  bool isSockFile() { return nodev().isSockFile(); }
  uint16_t attribute() { return nodev().attribute(); }

private:
  FileTree::Node& nodev() {
    return node().value();
  }

  upan::option<FileTree::Node&> node() {
    return upan::option<FileTree::Node&>(_node);
  }

  class WriteGuard {
  public:
    WriteGuard(FileNodeRef& ref) : _ref(ref) {
      _ref.node().ifPresent([](FileTree::Node& node) { node.rwlock().write_lock(); });
    }

    ~WriteGuard() {
      _ref.node().ifPresent([](FileTree::Node& node) { node.rwlock().write_unlock(); });
    }
  private:
    WriteGuard() = delete;
    WriteGuard(const WriteGuard&) = delete;
    WriteGuard& operator=(const WriteGuard&) = delete;

    FileNodeRef& _ref;
  };

  class ReadGuard {
  public:
    ReadGuard(FileNodeRef& ref) : _ref(ref) {
      _ref.node().ifPresent([](FileTree::Node& node) { node.rwlock().read_lock(); });
    }

    ~ReadGuard() {
      _ref.node().ifPresent([](FileTree::Node& node) { node.rwlock().read_unlock(); });
    }
  private:
    ReadGuard() = delete;
    ReadGuard(const ReadGuard&) = delete;
    ReadGuard& operator=(const ReadGuard&) = delete;

    FileNodeRef& _ref;
  };

private:
  FileTree::Node* _node;
  friend class FileSystem;
  friend class FileTree;
};