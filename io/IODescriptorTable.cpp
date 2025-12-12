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
#include <ProcessManager.h>
#include <RedirectDescriptor.h>
#include <StreamBufferDescriptor.h>
#include <NullDescriptor.h>
#include <StorageDrive.h>
#include "errno_values.h"

constexpr int PROC_SYS_MAX_OPEN_FILES = 4096;

IODescriptorTable::IODescriptorTable(int pid) : _pid(pid), _descIdCounter(0) {
}

IODescriptorTable::~IODescriptorTable() noexcept {
  for(auto& x : _iodMap) {
    x.second->close();
  }
}

void IODescriptorTable::closeAllFiles(StorageDrive& drive) {
  upan::mutex_guard g(_ioMutex);
  upan::vector<int> fds;
  for(auto& x : _iodMap) {
    auto fileDescriptor = dynamic_cast<FileDescriptor*>(x.second.get());
    if (fileDescriptor && fileDescriptor->diskDrive().Id() == drive.Id()) {
      fds.push_back(x.first);
    }
  }

  for(auto fd : fds) {
    free(fd);
  }
}

void IODescriptorTable::setupStreamedStdio() {
  upan::mutex_guard g(_ioMutex);
  _iodMap[STDOUT]->close();
  _iodMap[STDOUT] = new StreamBufferDescriptor(_pid, STDOUT, 4096, O_WR_NONBLOCK);

  _iodMap[STDIN]->close();
  _iodMap[STDIN] = new StreamBufferDescriptor(_pid, STDIN, 4096, O_WR_NONBLOCK);
}

void IODescriptorTable::setupNullStdio() {
  upan::mutex_guard g(_ioMutex);

  _iodMap[STDOUT]->close();
  _iodMap[STDOUT] = new NullDescriptor(_pid, STDOUT);

  _iodMap[STDIN]->close();
  _iodMap[STDIN] = new NullDescriptor(_pid, STDIN);
}

IODescriptor::Ptr IODescriptorTable::allocate(const upan::function<IODescriptor::Ptr, int>& descriptorBuilder) {
  upan::mutex_guard g(_ioMutex);

  if(_iodMap.size() >= PROC_SYS_MAX_OPEN_FILES) {
    throw upan::exception(XLOC, "can't open new file - max open files limit %d reached", PROC_SYS_MAX_OPEN_FILES);
  }

  const auto fd = _descIdCounter++;
  auto i = _iodMap.insert(IODMap::value_type(fd, descriptorBuilder(fd)));

  if (!i.second) {
    throw upan::exception(XLOC, "failed to create an entry in File IODescriptor table");
  }

  return i.first->second;
}

IODescriptorTable::IODMap::iterator IODescriptorTable::getItr(int fd) {
  auto i = _iodMap.find(fd);
  if (i == _iodMap.end()) {
    throw upan::exception(XLOC, "invalid file descriptor: %d", fd);
  }
  return i;
}

IODescriptor::Ptr IODescriptorTable::get(int fd) {
  upan::mutex_guard g(_ioMutex);
  return getItr(fd)->second;
}

IODescriptor::Ptr IODescriptorTable::getRealNonDupped(int fd) {
  upan::mutex_guard g(_ioMutex);
  auto ptr = get(fd);
  while (!ptr->getParentDescriptor().isEmpty()) {
    ptr = ptr->getParentDescriptor();
  }
  return ptr;
}

void IODescriptorTable::free(int fd) {
  upan::mutex_guard g(_ioMutex);
  auto e = getItr(fd);

  if (e->second->getRefCount() > 1) {
    throw upan::exception(XLOC, "descriptor is open - refcount: %d", e->second->getRefCount());
  }

  e->second->close();
  _iodMap.erase(e);
}

void IODescriptorTable::updateRedirections(int srcFD, IODescriptor::Ptr targetDesc) {
  upan::mutex_guard g(_ioMutex);
  for(auto& x : _iodMap) {
    auto d = x.second.cast<RedirectDescriptor>();
    if (!d.isEmpty() && d->getParentDescriptor()->id() == srcFD) {
      d->changeRedirection(targetDesc);
    }
  }
}

void IODescriptorTable::dup2(int oldFD, int newFD) {
  upan::mutex_guard g(_ioMutex);
  auto oldF = get(oldFD);
  auto newF = get(newFD);

  IODescriptor::Ptr targetF(new RedirectDescriptor(_pid, newFD, oldF));
  ProcessManager::Instance().updateAllIODescriptorRedirections(_pid, newFD, targetF);
  free(newFD);
  _iodMap.insert(IODMap::value_type(newFD, targetF));
}

upan::vector<IODescriptorTable::io_descriptor> IODescriptorTable::select(const upan::vector<io_descriptor>& ioDescriptors, struct timeval* timeout, int& retCode) {
  retCode = 0;
  const auto& result = selectCheck(ioDescriptors);
  if (result.empty()) {
    const time_t timeoutInMs = timeout ? timeout->tv_sec * 1000 + timeout->tv_usec / 1000 : 0;
    ProcessManager::Instance().WaitOnIODescriptors(ioDescriptors, timeoutInMs);
    const auto err = ProcessManager::Instance().GetCurrentPAS().stateInfo().getError();
    if (err == ProcessStateInfo::INTERRUPTED) {
      //throw upan::exception(XLOC, "IO select interrupted");
      retCode = -EINTR;
      return {};
    } else if (err == ProcessStateInfo::TIMEOUT) {
      retCode = 0;
      return {};
    }
    return ProcessManager::Instance().GetCurrentPAS().stateInfo().GetIODescriptors();
  } else {
    retCode = result.size();
    return result;
  }
}

upan::vector<IODescriptorTable::io_descriptor> IODescriptorTable::selectCheck(const upan::vector<io_descriptor>& ioDescriptors) {
  upan::vector<io_descriptor> result;
  for(const auto& ioDescriptor : ioDescriptors) {
    auto d = get(ioDescriptor._fd);
    switch(ioDescriptor._ioType) {
      case IO_OP_TYPES::IO_Read:
        if (d->canRead()) {
          result.push_back(ioDescriptor);
        }
        break;
      case IO_OP_TYPES::IO_Write:
        if (d->canWrite()) {
          result.push_back(ioDescriptor);
        }
        break;
    }
  }
  return result;
}