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

#include <LocalDataGramResolver.h>

LocalDataGramResolver& LocalDataGramResolver::Instance() {
  static LocalDataGramResolver instance;
  return instance;
}

void LocalDataGramResolver::setup(SocketDescriptorLocalDataGram& socket, const upan::string& path) {
  upan::mutex_guard g(_mutex);
  auto r = _socketBindMap.insert(SOCKET_PATH_MAP::value_type(path, &socket));
  if (r.second == false) {
    throw upan::exception(XLOC, "setup failed - socket %s is already bound", path.c_str());
  }
}

void LocalDataGramResolver::release(SocketDescriptorLocalDataGram& socket) {
  upan::mutex_guard g(_mutex);
  auto it = _socketBindMap.find(socket.srcPath());
  if (it != _socketBindMap.end()) {
    _socketBindMap.erase(it);
  }
}

upan::option<SocketDescriptorLocalDataGram&> LocalDataGramResolver::resolve(const upan::string& path) {
  upan::mutex_guard g(_mutex);
  auto it = _socketBindMap.find(path);
  if (it != _socketBindMap.end()) {
    return upan::option<SocketDescriptorLocalDataGram&>(*it->second);
  }
  return upan::option<SocketDescriptorLocalDataGram&>::empty();
}