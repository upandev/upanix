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

#include <IODescriptorTable.h>
#include <mutex.h>
#include <mosstd.h>
#include <ProcessManager.h>
#include <RedirectDescriptor.h>
#include <StreamBufferDescriptor.h>
#include <NullDescriptor.h>

constexpr int PROC_SYS_MAX_OPEN_FILES = 4096;

IODescriptorTable::IODescriptorTable(int pid, int parentPid) : _pid(pid), _descIdCounter(0) {
  if (pid == NO_PROCESS_ID) {
    allocate([pid](int fd) { return new StreamBufferDescriptor(pid, fd, 4096, O_WR_NONBLOCK); });
    auto& stdoutFD = allocate([pid](int fd) { return new StreamBufferDescriptor(pid, fd, 4096, O_WR_NONBLOCK); });
    allocate([pid, &stdoutFD](int fd) { return new RedirectDescriptor(pid, fd, stdoutFD); });
  } else {
    auto& parentProcess = ProcessManager::Instance().GetProcess(parentPid).value();
    allocate([&](int fd) { return new RedirectDescriptor(pid, fd, parentProcess.iodTable().get(STDIN)); });
    allocate([&](int fd) { return new RedirectDescriptor(pid, fd, parentProcess.iodTable().get(STDOUT)); });
    allocate([&](int fd) { return new RedirectDescriptor(pid, fd, parentProcess.iodTable().get(STDERR)); });
  }
}

IODescriptorTable::~IODescriptorTable() noexcept {
  for(auto& x : _iodMap) {
    delete x.second;
  }
}

void IODescriptorTable::setupStreamedStdio() {
  upan::mutex_guard g(_ioMutex);
  delete _iodMap[STDOUT];
  _iodMap[STDOUT] = new StreamBufferDescriptor(_pid, STDOUT, 4096, O_WR_NONBLOCK);

  delete _iodMap[STDIN];
  _iodMap[STDIN] = new StreamBufferDescriptor(_pid, STDIN, 4096, O_WR_NONBLOCK);
}

void IODescriptorTable::setupNullStdio() {
  upan::mutex_guard g(_ioMutex);

  delete _iodMap[STDOUT];
  _iodMap[STDOUT] = new NullDescriptor(_pid, STDOUT);

  delete _iodMap[STDIN];
  _iodMap[STDIN] = new NullDescriptor(_pid, STDIN);
}

IODescriptor& IODescriptorTable::allocate(const upan::function<IODescriptor*, int>& descriptorBuilder) {
  upan::mutex_guard g(_ioMutex);

  if(_iodMap.size() >= PROC_SYS_MAX_OPEN_FILES) {
    throw upan::exception(XLOC, "can't open new file - max open files limit %d reached", PROC_SYS_MAX_OPEN_FILES);
  }

  const auto fd = _descIdCounter++;
  auto i = _iodMap.insert(IODMap::value_type(fd, descriptorBuilder(fd)));

  if (!i.second) {
    throw upan::exception(XLOC, "failed to create an entry in File IODescriptor table");
  }

  return *i.first->second;
}

IODescriptorTable::IODMap::iterator IODescriptorTable::getItr(int fd) {
  auto i = _iodMap.find(fd);
  if (i == _iodMap.end()) {
    throw upan::exception(XLOC, "invalid file descriptor: %d", fd);
  }
  return i;
}

IODescriptor& IODescriptorTable::get(int fd) {
  upan::mutex_guard g(_ioMutex);
  return *(getItr(fd)->second);
}

IODescriptor& IODescriptorTable::getRealNonDupped(int fd) {
  upan::mutex_guard g(_ioMutex);
  return get(fd).getRealDescriptor();
}

void IODescriptorTable::free(int fd) {
  upan::mutex_guard g(_ioMutex);
  auto e = getItr(fd);

  if (e->second->getRefCount() > 1) {
    throw upan::exception(XLOC, "file descriptor is open - refcount: %d", e->second->getRefCount());
  }

  e->second->decrementRefCount();
  e->second->getParentDescriptor().ifPresent([](IODescriptor& p) { p.decrementRefCount(); });

  delete e->second;
  _iodMap.erase(e);
}

void IODescriptorTable::dup2(int oldFD, int newFD) {
  upan::mutex_guard g(_ioMutex);
  auto& oldF = get(oldFD);
  auto& newF = get(newFD);
  free(newFD);
  _iodMap.insert(IODMap::value_type(newFD, new RedirectDescriptor(_pid, newFD, oldF)));
}

upan::vector<io_descriptor> IODescriptorTable::select(const upan::vector<io_descriptor>& ioDescriptors) {
  const auto& result = selectCheck(ioDescriptors);
  if (result.empty()) {
    ProcessManager::Instance().WaitOnIODescriptors(ioDescriptors);
    return ProcessManager::Instance().GetCurrentPAS().stateInfo().GetIODescriptors();
  } else {
    return result;
  }
}

upan::vector<io_descriptor> IODescriptorTable::selectCheck(const upan::vector<io_descriptor>& ioDescriptors) {
  upan::vector<io_descriptor> result;
  for(const auto& ioDescriptor : ioDescriptors) {
    auto& d = get(ioDescriptor._fd);
    switch(ioDescriptor._ioType) {
      case IO_OP_TYPES::IO_Read:
        if (d.canRead()) {
          result.push_back(ioDescriptor);
        }
        break;
      case IO_OP_TYPES::IO_Write:
        if (d.canWrite()) {
          result.push_back(ioDescriptor);
        }
        break;
    }
  }
  return result;
}