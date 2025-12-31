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
# include <FileOperations.h>
# include <FileDescriptor.h>
# include <FileSystem.h>
# include <ProcessManager.h>
# include <StringUtil.h>
# include <StorageDriveManager.h>
# include <FileNodeRef.h>
# include <StorageDrive.h>
# include <TerminalDescriptor.h>
# include <FSDeviceManager.h>

bool FileOperations_ReadLine(int fd, upan::string& line)
{
  line = "";
  const int CHUNK_SIZE = 64;
  char buffer[CHUNK_SIZE + 1];
  upan::list<upan::string> buffers;
  auto file = ProcessManager::Instance().GetCurrentPAS().iodTable().getRealNonDupped(fd);
  while(true)
  {
    int readLen = file->read(buffer, CHUNK_SIZE);
    
    if(readLen == 0)
      break;

    //find new line
    int i = 0;
    for(i = 0; i < readLen; ++i)
      if(buffer[i] == '\n')
        break;
    buffer[i] = '\0';

    buffers.push_back(buffer);

    int offset = i - readLen + 1;
    if(offset < 0)
    {
      file->seek(SEEK_CUR, offset);
      break;
    }
  }

  if(buffers.empty())
    return false;
  for(auto chunk : buffers)
    line += chunk;
  return true;
}

StorageDrive& FileOperations::parseFilePath(const upan::string& fullFilePath, Process& process,
                                            FileNodeRef& cwd, FileTree::NodeTokens& fileTokens) {
  return parseFilePath(fullFilePath, process.driveID(), process.pwd(), true, cwd, fileTokens);
}

StorageDrive& FileOperations::parseFilePath(const upan::string& fullFilePath,
                                            int driveId, FileNodeRef pwd, bool allowCrossDrive,
                                            FileNodeRef& cwd, FileTree::NodeTokens& fileTokens) {
  fullFilePath.tokenize("/", false, fileTokens);

  if (fileTokens.empty()) {
    throw upan::exception(XLOC, "%s file path tokenization failed", fullFilePath.c_str());
  }

  auto& lastToken = *fileTokens.rbegin();
  if (lastToken.empty()) {
    lastToken = DIR_SPECIAL_CURRENT;
  }

  auto drivePrefix = *fileTokens.begin();
  const bool hasDrivePrefix = !drivePrefix.empty() && drivePrefix[drivePrefix.length() - 1] == '@';
  const bool isAbsolutePath = drivePrefix.empty() || hasDrivePrefix;

  if (hasDrivePrefix) {
    fileTokens.pop_front();
    drivePrefix = drivePrefix.substr(0, drivePrefix.length() - 1);
  }

  for(auto i = fileTokens.begin(); i != fileTokens.end();) {
    if ((*i).empty()) {
      fileTokens.erase(i++);
    } else {
      ++i;
    }
  }

  upan::option<StorageDrive&> storageDrive = { nullptr };
  if (allowCrossDrive) {
    storageDrive = { hasDrivePrefix ?
                         StorageDriveManager::Instance().GetByDriveName(drivePrefix, true).goodValueOrThrow(XLOC)
                                        : StorageDriveManager::Instance().GetByID(driveId, true).goodValueOrThrow(XLOC) };
  } else {
    if (hasDrivePrefix) {
      storageDrive = StorageDriveManager::Instance().GetByDriveName(drivePrefix, true).goodValueOrThrow(XLOC);
      if (storageDrive.value().Id() != driveId) {
        throw upan::exception(XLOC, "can't access file across different storage drive (%d -> %d)", driveId, storageDrive.value().Id());
      }
    } else {
      storageDrive = StorageDriveManager::Instance().GetByID(driveId, true).goodValueOrThrow(XLOC);
    }
  }

  cwd = isAbsolutePath ? storageDrive.value().fileSystem().root() : pwd;
  return storageDrive.value();
}

upan::shared_ptr<IODescriptor> FileOperations::open(const upan::string& filePath, int flags, mode_t mode) {
  auto& process = ProcessManager::Instance().GetCurrentPAS();

  FileNodeRef cwd;
  FileTree::NodeTokens fileTokens;

  auto& storageDrive = parseFilePath(filePath, process, cwd, fileTokens);
  auto fileNodeRef = storageDrive.fileSystem().open(fileTokens, flags, mode, cwd, process);

  if (fileNodeRef.empty()) {
    return {};
  }

  return process.iodTable().allocate([&](int fd) -> IODescriptor* {
    if (fileNodeRef.isRegularFile()) {
      return new FileDescriptor(process.processID(), fd, flags, fileNodeRef, storageDrive, fileNodeRef.startSectorId());
    } else if (fileNodeRef.isChrFile()) {
      const auto& fullPath = storageDrive.fileSystem().fullPath(fileNodeRef);
      auto terminalDevice = FSDeviceManager::Instance().getTerminalDevice(fullPath);
      if (terminalDevice.isEmpty()) {
        throw upan::exception(XLOC, "failed to open terminal device file because no terminal device found for %s", fullPath.c_str());
      }
      return new TerminalDescriptor(process.processID(), fd, terminalDevice);
    } else {
      throw upan::exception(XLOC, "unsupported file type %s", FILE_TYPE(fileNodeRef.attribute()));
    }
  });
}

upan::shared_ptr<IODescriptor> FileOperations::openInMemoryTerminalDevice(const upan::string& filePath) {
  auto& process = ProcessManager::Instance().GetCurrentPAS();

  return process.iodTable().allocate([&](int fd) -> IODescriptor* {
    auto terminalDevice = FSDeviceManager::Instance().getTerminalDevice(filePath);
    if (terminalDevice.isEmpty()) {
      throw upan::exception(XLOC, "failed to open in-memory terminal device file because no terminal device found for %s", filePath.c_str());
    }
    return new TerminalDescriptor(process.processID(), fd, terminalDevice);
  });
}

void FileOperations::create(const upan::string& filePath, mode_t mode) {
  auto& process = ProcessManager::Instance().GetCurrentPAS();

  FileNodeRef cwd;
  FileTree::NodeTokens fileTokens;

  auto& storageDrive = parseFilePath(filePath, process, cwd, fileTokens);
  const upan::string newFileName = fileTokens.back();
  fileTokens.pop_back();
  storageDrive.fileSystem().create(fileTokens, newFileName, mode, cwd, process);
}

void FileOperations::remove(const upan::string& filePath) {
  auto& process = ProcessManager::Instance().GetCurrentPAS();

  FileNodeRef cwd;
  FileTree::NodeTokens fileTokens;

  auto& storageDrive = parseFilePath(filePath, process, cwd, fileTokens);
  storageDrive.fileSystem().remove(fileTokens, cwd, process, false);
}

FileNodeRef FileOperations::exists(const upan::string& filePath) {
  auto& process = ProcessManager::Instance().GetCurrentPAS();

  FileNodeRef cwd;
  FileTree::NodeTokens fileTokens;

  auto& storageDrive = parseFilePath(filePath, process, cwd, fileTokens);
  return storageDrive.fileSystem().exists(fileTokens, cwd);
}

bool FileOperations::fileExists(const upan::string& filePath) {
  auto fileNodeRef = exists(filePath);
  return !fileNodeRef.empty() && fileNodeRef.isFile();
}

bool FileOperations::directoryExists(const upan::string& filePath) {
  auto fileNodeRef = exists(filePath);
  return !fileNodeRef.empty() && fileNodeRef.isDirectory();
}

upan::string FileOperations::getcwd() {
  auto& process = ProcessManager::Instance().GetCurrentPAS();

  if (process.pwd().empty()) {
    return "";
  }

  StorageDrive& diskDrive = StorageDriveManager::Instance().GetByID(process.driveID(), true).goodValueOrThrow(XLOC);
  return diskDrive.fileSystem().fullPath(process.pwd());
}

upan::option<struct stat> FileOperations::stats(const upan::string& filePath) {
  auto& process = ProcessManager::Instance().GetCurrentPAS();

  FileNodeRef cwd;
  FileTree::NodeTokens fileTokens;

  auto& storageDrive = parseFilePath(filePath, process, cwd, fileTokens);
  return storageDrive.fileSystem().stats(fileTokens, cwd);
}

int FileOperations::readLink(const upan::string& link, char* buf, size_t bufSize) {
  auto& process = ProcessManager::Instance().GetCurrentPAS();

  FileNodeRef cwd;
  FileTree::NodeTokens fileTokens;
  auto& storageDrive = parseFilePath(link, process, cwd, fileTokens);

  return storageDrive.fileSystem().readLink(fileTokens, cwd, buf, bufSize);
}

void FileOperations::symLink(const upan::string& target, const upan::string& link) {
  auto& process = ProcessManager::Instance().GetCurrentPAS();

  FileNodeRef targetCWD;
  FileTree::NodeTokens targetFileTokens;
  auto& targetStorageDrive = parseFilePath(target, process, targetCWD, targetFileTokens);

  FileNodeRef linkCWD;
  FileTree::NodeTokens linkFileTokens;
  auto& linkStorageDrive = parseFilePath(link, process, linkCWD, linkFileTokens);

  if (targetStorageDrive.Id() != linkStorageDrive.Id()) {
    throw upan::exception(XLOC, "can't sym link file across different storage drive");
  }

  linkStorageDrive.fileSystem().symLink(linkFileTokens, target, linkCWD, process);
}

void FileOperations::rename(const upan::string& oldPath, const upan::string& newPath) {
  auto& process = ProcessManager::Instance().GetCurrentPAS();

  FileNodeRef srcCWD;
  FileTree::NodeTokens srcFileTokens;
  auto& storageDrive = parseFilePath(oldPath, process, srcCWD, srcFileTokens);

  FileNodeRef destCWD;
  FileTree::NodeTokens destFileTokens;
  auto& destStorageDrive = parseFilePath(newPath, process, destCWD, destFileTokens);

  if (storageDrive.Id() != destStorageDrive.Id()) {
    throw upan::exception(XLOC, "can't renameFile file across different storage drive");
  }

  storageDrive.fileSystem().renameFile(srcFileTokens, destFileTokens, srcCWD, destCWD, process);
}

void FileOperations::changeDir(const upan::string& dirPath, char** retPwd) {
  auto& process = ProcessManager::Instance().GetCurrentPAS();

  FileNodeRef cwd;
  FileTree::NodeTokens fileTokens;

  auto& storageDrive = parseFilePath(dirPath, process, cwd, fileTokens);
  FileNodeRef dirNodeRef = exists(dirPath);
  if (dirNodeRef.empty()) {
    throw upan::exception(XLOC, "no such directory");
  }

  if (dirNodeRef.isFile()) {
    throw upan::exception(XLOC, "not a directory");
  }

  process.setDriveID(storageDrive.Id());
  process.pwd(dirNodeRef);
  upan::string pwd(storageDrive.DriveName() + "@" + storageDrive.fileSystem().fullPath(dirNodeRef));

  if (retPwd) {
    *retPwd = (char*)process.dmm().allocate(pwd.length() + 1);
    strcpy(*retPwd, pwd.c_str());
  } else if (process.isKernelProcess()) {
    setenv("PWD", pwd.c_str(), 1);
  }
}

DIR* FileOperations::opendir(const upan::string& dirPath) {
  auto& process = ProcessManager::Instance().GetCurrentPAS();

  FileNodeRef cwd;
  FileTree::NodeTokens fileTokens;
  auto& storageDrive = parseFilePath(dirPath, process, cwd, fileTokens);
  upan::uniq_ptr<DIR> dirp((DIR*)process.dmm().allocate(sizeof(DIR)));
  auto fileNodeRef = storageDrive.fileSystem().openDir(fileTokens, cwd, process, *dirp);
  if (fileNodeRef.empty()) {
    return nullptr;
  }

  auto ioDescriptor = process.iodTable().allocate([&](int fd) -> IODescriptor* {
      return new FileDescriptor(process.processID(), fd, O_RDONLY, fileNodeRef, storageDrive, fileNodeRef.startSectorId());
  });

  dirp->fd = ioDescriptor->id();
  return dirp.release();
}

void FileOperations::readdir(DIR* dirp) {
  auto& process = ProcessManager::Instance().GetCurrentPAS();
  auto fileDescriptor = process.iodTable().getRealNonDupped(dirp->fd).cast<FileDescriptor>();
  fileDescriptor->diskDrive().fileSystem().readDir(fileDescriptor->fileNodeRef(), *fileDescriptor, process, *dirp);
}

void FileOperations::closedir(DIR* dirp) {
  auto& process = ProcessManager::Instance().GetCurrentPAS();
  if (dirp) {
    process.iodTable().free(dirp->fd);
  }
  process.dmm().free((uintptr_t)dirp->data);
  process.dmm().free((uintptr_t)dirp);
}

bool FileOperations::fileAccess(const upan::string& filePath, int mode) {
  auto& process = ProcessManager::Instance().GetCurrentPAS();

  FileNodeRef cwd;
  FileTree::NodeTokens fileTokens;

  auto& storageDrive = parseFilePath(filePath, process, cwd, fileTokens);
  return storageDrive.fileSystem().hasFilePermission(fileTokens, mode, cwd, process);
}

void FileOperations::dup2(int oldFD, int newFD) {
  ProcessManager::Instance().GetCurrentPAS().iodTable().dup2(oldFD, newFD);
}