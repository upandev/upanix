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

FSDeviceManager::FSDeviceManager() : _rootPrefix(upan::string(ROOT_DRIVE_SYN) + "@"), _nextTerminalId(0) {
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

void FSDeviceManager::createSocketDevice(const upan::string& path) {
  if (_devices.exists(path)) {
    throw upan::exception(XLOC, "socket device already exists for path: %s", path.c_str());
  }

  const auto& fileStat = FileOperations::Instance().stats(_rootPrefix + path);
  if (fileStat.isEmpty()) {
    FileOperations::Instance().create(_rootPrefix + path, S_IFSOCK, 0744);
  } else {
    if (S_ISSOCK(fileStat.value().st_mode) == false) {
      throw upan::exception(XLOC, "a non socket file already exists at path: %s", path.c_str());
    }
    if (!FileOperations::Instance().fileAccess(_rootPrefix + path, O_RDWR)) {
      throw upan::exception(XLOC, "permission denied to access socket device at path: %s", path.c_str());
    }
  }

  _devices[path].reset(new FSSocketDevice(path));
}

int FSDeviceManager::createTerminalDevice(int flags) {
  upan::string path("/dev/pts");
  path += upan::string::to_string(_nextTerminalId.inc());

  if (_devices.exists(path)) {
    throw upan::exception(XLOC, "tty device already exists for path: %s", path.c_str());
  }

  const auto& fileStat = FileOperations::Instance().stats(_rootPrefix + path);
  if (fileStat.isEmpty()) {
    FileOperations::Instance().create(_rootPrefix + path, S_IFCHR, 0620);
  } else {
    if (S_ISCHR(fileStat.value().st_mode) == false) {
      throw upan::exception(XLOC, "a non tty file already exists at path: %s", path.c_str());
    }
    if (!FileOperations::Instance().fileAccess(_rootPrefix + path, flags)) {
      throw upan::exception(XLOC, "permission denied to access tty device at path: %s", path.c_str());
    }
  }

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

void FSDeviceManager::removeDevice(const upan::string& path) {
  auto i = _devices.find(path);
  if (i == _devices.end()) {
    return;
  }
  _devices.erase(i);
}