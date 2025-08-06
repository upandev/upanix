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
# include <MountManager.h>
# include <DMM.h>
# include <StringUtil.h>
# include <StorageDriveManager.h>
# include <FileNodeRef.h>

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

StorageDrive& FileOperations::parseFilePath(const upan::string& fullFilePath, const Process& process,
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

  auto& storageDrive = hasDrivePrefix ?
          StorageDriveManager::Instance().GetByDriveName(drivePrefix, true).goodValueOrThrow(XLOC)
          : StorageDriveManager::Instance().GetByID(process.driveID(), true).goodValueOrThrow(XLOC);

  cwd = isAbsolutePath ? storageDrive.fileSystem().root() : process.pwd();

  return storageDrive;
}

upan::shared_ptr<IODescriptor> FileOperations::open(const upan::string& filePath, const uint8_t mode) {
  auto& process = ProcessManager::Instance().GetCurrentPAS();

  FileNodeRef cwd;
  FileTree::NodeTokens fileTokens;

  auto& storageDrive = parseFilePath(filePath, process, cwd, fileTokens);
  auto fileNodeRef = storageDrive.fileSystem().open(fileTokens, mode, cwd, process);

  if (fileNodeRef.empty()) {
    return {};
  }

  return process.iodTable().allocate([&](int fd) {
    return new FileDescriptor(process.processID(), fd, mode, fileNodeRef, storageDrive, fileNodeRef.startSectorId());
  });
}

void FileOperations::create(const upan::string& filePath, uint16_t fileType, uint16_t mode) {
  auto& process = ProcessManager::Instance().GetCurrentPAS();

  FileNodeRef cwd;
  FileTree::NodeTokens fileTokens;

  auto& storageDrive = parseFilePath(filePath, process, cwd, fileTokens);
  const upan::string newFileName = fileTokens.back();
  fileTokens.pop_back();
  storageDrive.fileSystem().create(fileTokens, newFileName, fileType, mode, cwd, process);
}

void FileOperations::remove(const upan::string& filePath) {
  auto& process = ProcessManager::Instance().GetCurrentPAS();

  FileNodeRef cwd;
  FileTree::NodeTokens fileTokens;

  auto& storageDrive = parseFilePath(filePath, process, cwd, fileTokens);
  const upan::string fileName = fileTokens.back();
  fileTokens.pop_back();
  storageDrive.fileSystem().remove(fileTokens, fileName, cwd, process);
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

struct stat FileOperations::stats(const upan::string& filePath) {
  auto& process = ProcessManager::Instance().GetCurrentPAS();

  FileNodeRef cwd;
  FileTree::NodeTokens fileTokens;

  auto& storageDrive = parseFilePath(filePath, process, cwd, fileTokens);
  return storageDrive.fileSystem().stats(fileTokens, cwd);
}

void FileOperations::changeDir(const upan::string& dirPath) {
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
  process.setEnv("PWD", storageDrive.DriveName() + "@" + storageDrive.fileSystem().fullPath(dirNodeRef));
}

void FileOperations::listDir(const upan::string& filePath, FileStats& fileStats) {
  auto& process = ProcessManager::Instance().GetCurrentPAS();

  FileNodeRef cwd;
  FileTree::NodeTokens fileTokens;

  auto& storageDrive = parseFilePath(filePath, process, cwd, fileTokens);
  storageDrive.fileSystem().listDir(fileTokens, cwd, process, fileStats);
}

void FileOperations::listDir(const upan::string& filePath, struct stat_ex** fileStatsArray, int* size) {
  FileStats fileStats;
  listDir(filePath, fileStats);

  auto& process = ProcessManager::Instance().GetCurrentPAS();
  *fileStatsArray = (struct stat_ex*)process.dmm().allocate(fileStats.size() * sizeof(struct stat_ex));

  for (int i = 0; i < fileStats.size(); ++i) {
    (*fileStatsArray)[i] = fileStats[i];
  }
  *size = fileStats.size();
}

bool FileOperations::fileAccess(const upan::string& filePath, uint8_t mode) {
  auto& process = ProcessManager::Instance().GetCurrentPAS();

  FileNodeRef cwd;
  FileTree::NodeTokens fileTokens;

  auto& storageDrive = parseFilePath(filePath, process, cwd, fileTokens);
  return storageDrive.fileSystem().hasFilePermission(fileTokens, mode, cwd, process);
}

void FileOperations::dup2(int oldFD, int newFD) {
  ProcessManager::Instance().GetCurrentPAS().iodTable().dup2(oldFD, newFD);
}