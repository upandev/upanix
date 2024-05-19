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
#include <FileNodeRef.h>

FileNodeRef::FileNodeRef() : _node(nullptr) {}

FileNodeRef::FileNodeRef(const FileNodeRef& fileNodeRef) : _node(nullptr) {
  set(fileNodeRef._node);
}

FileNodeRef::FileNodeRef(FileTree::Node* node) : _node(nullptr) {
  set(node);
}

FileNodeRef::~FileNodeRef() {
  clear();
}

FileNodeRef& FileNodeRef::operator=(const FileNodeRef& fileNodeRef) {
  if (this == &fileNodeRef) {
    return *this;
  }
  set(fileNodeRef._node);
  return *this;
}

void FileNodeRef::set(FileTree::Node* node) {
  if (node) node->incRefCount();
  if (_node) _node->decRecCount();
  _node = node;
}

void FileNodeRef::clear() {
  if (_node) _node->decRecCount();
  _node = nullptr;
}