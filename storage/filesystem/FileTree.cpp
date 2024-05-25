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
  uninitialize();
}

void FileTree::initialize(StorageDrive& storageDrive) {
  uint8_t sectorBuffer[FileSystem::SECTOR_SIZE];

  storageDrive.xRead(sectorBuffer, 0, 1);
  auto& parentFileNode = reinterpret_cast<FileNode*>(sectorBuffer)[0];

  _root = new Node(nullptr, parentFileNode, 0, 0);
  _root->Load(storageDrive);
}

void FileTree::uninitialize() {
  delete _root;
  _root = nullptr;
}

FileNodeRef FileTree::getFileNodeRef(const FileTree::NodeTokens& nodeTokens, const FileNodeRef& cwd) {
  upan::mutex_guard g(_treeMutex);

  FileNodeRef cur = cwd;

  for(const auto& token : nodeTokens) {
    if (token == DIR_SPECIAL_CURRENT) {
      continue;
    }

    if (token == DIR_SPECIAL_PARENT) {
      if (cur.nodev().isRoot()) {
        continue;
      }
      cur.set(cur.nodev().parent());
      continue;
    }

    auto& node = cur.nodev();
    if (node.isFile()) {
      throw upan::exception(XLOC, "file %s can't be in the directory search path", node.name().c_str());
    }
    auto& subNodes = node.subNodes();

    auto i = subNodes.find(token);
    if (i == subNodes.end()) {
      return {};
    }

    cur.set(i->second);
  }

  return { cur };
}

void FileTree::addNode(FileTree::Node& parent, const FileNode& newFileNode, uint32_t sectorId, uint8_t sectorOffset) {
  upan::mutex_guard g(_treeMutex);
  parent.addSubNode(newFileNode, sectorId, sectorOffset);
}

FileTree::Node* FileTree::removeNode(Node& parent, const upan::string& deleteFileName, uint32_t& prevSectorId, bool& deallocateSectorBlock) {
  upan::mutex_guard g(_treeMutex);
  return parent.removeSubNode(deleteFileName, prevSectorId, deallocateSectorBlock);
}

upan::string FileTree::getFullPath(FileTree::Node& node) {
  upan::mutex_guard g(_treeMutex);
  upan::string fullPath;
  Node* cur = &node;
  while(cur != nullptr) {
    if (fullPath.empty()) {
      fullPath = cur->name();
    } else if (cur->isRoot()) {
      fullPath = cur->name() + fullPath;
    } else {
      fullPath = cur->name() + "/" + fullPath;
    }
    cur = cur->parent();
  }
  return fullPath;
}

class DirSectorBlock {
public:
  DirSectorBlock() : _sectorId(EOC) {
    for (auto& node : _nodes) {
      node = nullptr;
    }
  }

  uint32_t sectorId() const { return _sectorId; }
  void sectorId(uint32_t sectorId) { _sectorId = sectorId; }
  FileTree::Node** nodes() { return _nodes; }
  bool empty() const {
    for (auto node : _nodes) {
      if (node != nullptr) {
        return false;
      }
    }
    return true;
  }

private:
  uint32_t _sectorId;
  FileTree::Node* _nodes[FileSystem::DIR_ENTRIES_PER_SECTOR];
};

void FileTree::Node::Load(StorageDrive& storageDrive) {
  uint8_t sectorBuffer[FileSystem::SECTOR_SIZE];

  auto currentSectorId = _startSectorId;
  uint32_t fileCount = 0;

  while(currentSectorId != EOC && fileCount < _size) {
    storageDrive.xRead(sectorBuffer, currentSectorId, 1);

    auto dirSectorBlock = new DirSectorBlock();
    dirSectorBlock->sectorId(currentSectorId);

    auto fileNodes = reinterpret_cast<FileNode*>(sectorBuffer);
    for(auto sectorOffset = 0; sectorOffset < FileSystem::DIR_ENTRIES_PER_SECTOR && fileCount < _size; ++sectorOffset) {
      const auto& fileNode = fileNodes[sectorOffset];
      if (fileNode.IsDeleted()) {
        dirSectorBlock->nodes()[sectorOffset] = nullptr;
        continue;
      }

      auto node = new FileTree::Node(this, fileNode, currentSectorId, sectorOffset);
      dirSectorBlock->nodes()[sectorOffset] = node;

      _subNodes.insert(SubNodes::value_type(node->name(), node));

      if (fileNode.IsDirectory()) {
        node->Load(storageDrive);
      }
      ++fileCount;
    }

    _dirSectorBlocks.push_back(dirSectorBlock);

    currentSectorId = storageDrive.fileSystem().getSectorEntryValue(currentSectorId);
  }
}

FileTree::Node::Node(Node* parent, const FileNode& fileNode, uint32_t sectorId, uint8_t sectorOffset) :
        _parent(parent),
        _name(fileNode.Name()),
        _startSectorId(fileNode.StartSectorID()),
        _sectorId(sectorId),
        _sectorOffset(sectorOffset),
        _isFile(fileNode.IsFile()),
        _size(fileNode.Size()),
        _refCount(0) {
}

FileTree::Node::~Node() {
  for(auto& node : _subNodes) {
    delete node.second;
  }
  _subNodes.clear();

  for(auto d : _dirSectorBlocks) {
    delete d;
  }
  _dirSectorBlocks.clear();
}

upan::option<FileTree::Node*> FileTree::Node::find(const upan::string &name) {
  auto i = _subNodes.find(name);
  if (i == _subNodes.end()) {
    return upan::option<FileTree::Node*>::empty();
  }
  return upan::option<FileTree::Node*>(i->second);
}

bool FileTree::Node::getFreeSlot(uint32_t& sectorId, uint8_t& sectorOffset) {
  for(auto& dirSectorBlock : _dirSectorBlocks) {
    for(int i = 0; i < FileSystem::DIR_ENTRIES_PER_SECTOR; ++i) {
      if (dirSectorBlock->nodes()[i] == nullptr) {
        sectorId = dirSectorBlock->sectorId();
        sectorOffset = i;
        return true;
      }
    }
  }
  return false;
}

uint32_t FileTree::Node::getDirLastSectorId() {
  if (_dirSectorBlocks.empty()) {
    return EOC;
  }
  return _dirSectorBlocks.back()->sectorId();
}

void FileTree::Node::addSubNode(const FileNode& fileNode, uint32_t sectorId, uint8_t sectorOffset) {
  auto subNode = new Node(this, fileNode, sectorId, sectorOffset);
  if (_startSectorId == EOC) {
    _startSectorId = subNode->sectorId();
    auto dirSectorBlock = new DirSectorBlock();
    dirSectorBlock->sectorId(_startSectorId);
    dirSectorBlock->nodes()[0] = subNode;
    _dirSectorBlocks.push_back(dirSectorBlock);
  } else {
    bool addedToFreeSlot = false;
    for(auto& dirSectorBlock : _dirSectorBlocks) {
      if (dirSectorBlock->sectorId() == subNode->sectorId()) {
        dirSectorBlock->nodes()[subNode->sectorOffset()] = subNode;
        addedToFreeSlot = true;
        break;
      }
    }
    if (!addedToFreeSlot) {
      auto dirSectorBlock = new DirSectorBlock();
      dirSectorBlock->sectorId(subNode->sectorId());
      dirSectorBlock->nodes()[subNode->sectorOffset()] = subNode;
      _dirSectorBlocks.push_back(dirSectorBlock);
    }
  }
  _subNodes.insert(SubNodes::value_type(subNode->name(), subNode));
  ++_size;
}

FileTree::Node* FileTree::Node::removeSubNode(const upan::string& fileName, uint32_t& prevSectorId, bool& deallocateSectorBlock) {
  auto sit = _subNodes.find(fileName);
  if (sit == _subNodes.end()) {
    throw upan::exception(XLOC, "%s does not exists", fileName.c_str());
  }

  auto subNode = sit->second;

  if (subNode->isReferenced()) {
    throw upan::exception(XLOC, "%s file/directory is in use", fileName.c_str());
  }

  if (subNode->isDirectory() && subNode->size() > 0) {
    throw upan::exception(XLOC, "%s directory is not empty", fileName.c_str());
  }

  prevSectorId = EOC;
  deallocateSectorBlock = false;

  for (auto it = _dirSectorBlocks.begin(); it != _dirSectorBlocks.end(); ++it) {
    auto& dirSectorBlock = **it;
    if (dirSectorBlock.sectorId() == subNode->sectorId()) {
      dirSectorBlock.nodes()[subNode->sectorOffset()] = nullptr;
      if (dirSectorBlock.empty()) {
        deallocateSectorBlock = true;
        _dirSectorBlocks.erase(it++);
        if (dirSectorBlock.sectorId() == _startSectorId) {
          _startSectorId = (it == _dirSectorBlocks.end()) ? EOC : it->sectorId();
        }
        delete &dirSectorBlock;
      }
      break;
    }
    prevSectorId = dirSectorBlock.sectorId();
  }

  _subNodes.erase(sit);
  --_size;
  return subNode;
}
