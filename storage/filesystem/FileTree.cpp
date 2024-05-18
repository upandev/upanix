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

#include <FileTree.h>
#include <FileSystem.h>
#include <StorageDrive.h>

FileTree::FileTree() : _root(nullptr) {
}

FileTree::~FileTree() {
  Uninitialize();
}

void FileTree::Initialize(StorageDrive& storageDrive) {
  uint8_t sectorBuffer[FileSystem::SECTOR_SIZE];

  storageDrive.xRead(sectorBuffer, 0, 1);
  auto& parentFileNode = reinterpret_cast<FileNode*>(sectorBuffer)[0];

  _root = new Node(nullptr, parentFileNode);
  _root->Load(storageDrive);
}

void FileTree::Uninitialize() {
  delete _root;
  _root = nullptr;
}

void FileTree::Node::Load(StorageDrive& storageDrive) {
  uint8_t sectorBuffer[FileSystem::SECTOR_SIZE];

  auto currentSectorId = _startSectorId;
  uint32_t fileCount = 0;

  while(currentSectorId != EOC && fileCount < _size) {
    storageDrive.xRead(sectorBuffer, currentSectorId, 1);

    auto fileNodes = reinterpret_cast<FileNode*>(sectorBuffer);
    for(auto sectorOffset = 0; sectorOffset < FileSystem::DIR_ENTRIES_PER_SECTOR && fileCount < _size; ++sectorOffset) {
      const auto& fileNode = fileNodes[sectorOffset];

      auto node = new FileTree::Node(this, fileNode);
      _subNodes.insert(SubNodes::value_type(node->name(), node));

      if (!fileNode.IsDeleted()) {
        if (fileNode.IsDirectory()) {
          node->Load(storageDrive);
        }
        ++fileCount;
      }
    }

    currentSectorId = storageDrive.fileSystem().getSectorEntryValue(currentSectorId);
  }
}

FileTree::Node::Node(const Node* parent, const FileNode& fileNode) :
        _parent(parent),
        _name(fileNode.Name()),
        _startSectorId(fileNode.StartSectorID()),
        _sectorOffset(fileNode.ParentSectorPos()),
        _isFile(fileNode.IsFile()),
        _isDeleted(fileNode.IsDeleted()),
        _size(fileNode.Size()),
        _refCount(0) {
  printf("\n Loading file node: %s", fileNode.Name());
}

FileTree::Node::~Node() {
  for(auto& node : _subNodes) {
    delete node.second;
  }
  _subNodes.clear();
}