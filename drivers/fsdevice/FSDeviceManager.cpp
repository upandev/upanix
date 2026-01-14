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
#include <FSDeviceManager.h>
#include <StorageDriveManager.h>
#include <StorageDrive.h>
#include <FileOperations.h>
#include <FSSocketDevice.h>
#include <FSTerminalDevice.h>
#include <ProcessManager.h>
#include <TerminalMasterDescriptor.h>
#include <RedirectDescriptor.h>
#include <FSPipeDevice.h>

FSDeviceManager::FSDeviceManager() : _nextTerminalId(0), _nextPipeId(0) {
}

//function to update file node ref across all devices when file system is mounted - which means creating the file node references that are missing
//function to remove file node ref across all devices when file system is umounted

upan::shared_ptr<FSDevice> FSDeviceManager::getDevice(const upan::string& path) {
  auto i = _devices.find(path);
  if (i == _devices.end()) {
    return {};
  }
  return i->second;
}

upan::shared_ptr<FSSocketDevice> FSDeviceManager::getSocketDevice(const upan::string& path) {
  return getDevice(path).cast<FSSocketDevice>();
}

upan::shared_ptr<FSTerminalDevice> FSDeviceManager::getTerminalDevice(const upan::string& path) {
  return getDevice(path).cast<FSTerminalDevice>();
}

upan::shared_ptr<FSPipeDevice> FSDeviceManager::getPipeDevice(const upan::string& path) {
  return getDevice(path).cast<FSPipeDevice>();
}

void FSDeviceManager::createSocketDevice(const upan::string& path) {
  if (_devices.exists(path)) {
    throw upan::exception(XLOC, "socket device already exists for path: %s", path.c_str());
  }
  _devices[path].reset(new FSSocketDevice(path));
}

int FSDeviceManager::createTerminalDevice(int flags) {
  upan::string path("/dev/pts");
  path += upan::string::to_string(_nextTerminalId.inc());

  if (_devices.exists(path)) {
    throw upan::exception(XLOC, "tty device already exists for path: %s", path.c_str());
  }

  const upan::string rootPrefix(ROOT_DRIVE_PREFIX);
  const auto& fileStat = FileOperations::Instance().stats(rootPrefix + path);
  if (fileStat.isEmpty()) {
    FileOperations::Instance().create(rootPrefix + path, S_IFCHR | 0620);
  } else {
    if (S_ISCHR(fileStat.value().st_mode) == false) {
      throw upan::exception(XLOC, "a non tty file already exists at path: %s", path.c_str());
    }
    if (!FileOperations::Instance().fileAccess(rootPrefix + path, W_OK)) {
      throw upan::exception(XLOC, "permission denied to access tty device at path: %s", path.c_str());
    }
  }

  return createTerminalDevice(flags, path);
}

int FSDeviceManager::createKernelRootInMemoryTerminalDevice(const upan::string& path) {
  if (_devices.exists(path)) {
    throw upan::exception(XLOC, "tty device already exists for path: %s", path.c_str());
  }
  return createTerminalDevice(O_RDWR, path);
}

int FSDeviceManager::createTerminalDevice(int flags, const upan::string& path) {
  auto& process = ProcessManager::Instance().GetCurrentPAS();

  _devices[path].reset(new FSTerminalDevice(process, path, 4096, 4096));
  upan::shared_ptr<FSTerminalDevice> terminalDevice = _devices[path].cast<FSTerminalDevice>();

  auto masterDesc = process.iodTable().allocate([&](int fd) {
    return new TerminalMasterDescriptor(process.processID(), fd, terminalDevice);
  });

  if (process.ownerControllingTerminal().isEmpty()) {
    if (!(flags & O_NOCTTY)) {
      process.setControllingTerminal(terminalDevice);
      process.iodTable().get(IODescriptorTable::TERMINAL_MASTER).cast<RedirectDescriptor>()->changeRedirection(masterDesc);
    }
  }

  return masterDesc->id();
}

upan::string FSDeviceManager::createPipeDevice() {
  upan::string path = "/dev/upipe";
  path += upan::string::to_string(_nextPipeId.inc());
  _devices[path].reset(new FSPipeDevice(path, 4096, false));
  return path;
}

void FSDeviceManager::createPipeDevice(const upan::string& path) {
  if (_devices.exists(path)) {
    throw upan::exception(XLOC, "device already exists for path: %s", path.c_str());
  }
  _devices[path].reset(new FSPipeDevice(path, 4096, true));
}

void FSDeviceManager::removeDevice(const upan::string& path) {
  auto i = _devices.find(path);
  if (i == _devices.end()) {
    return;
  }
  _devices.erase(i);
}

upan::shared_ptr<FSPipeDevice> FSDeviceManager::registerPipeDescriptor(int fd, const upan::string& path) {
  auto pipeDevice = getPipeDevice(path);
  if (pipeDevice.isEmpty()) {
    throw upan::exception(XLOC, "pipe device not found for path: %s", path.c_str());
  }
  pipeDevice->addFD(fd);
  return pipeDevice;
}

void FSDeviceManager::unregisterPipeDescriptor(int fd, const upan::string& path) {
  auto pipeDevice = getPipeDevice(path);
  if (pipeDevice.isEmpty()) {
    throw upan::exception(XLOC, "pipe device not found for path: %s", path.c_str());
  }
  pipeDevice->removeFD(fd);
  if (pipeDevice->fdCount() == 0) {
    removeDevice(path);
  }
}