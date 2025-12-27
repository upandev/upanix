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
#pragma once

#include <Global.h>
#include <FileSystem.h>
#include <FileDescriptor.h>
#include <mutex.h>
#include <FileNodeRef.h>
#include <dirent.h>

class IODescriptor;
class Process;

class FileOperations {
public:
  static FileOperations& Instance() {
    static FileOperations instance;
    return instance;
  }

  void create(const upan::string& filePath, mode_t mode);
  upan::shared_ptr<IODescriptor> open(const upan::string& filePath, int flags, mode_t mode);
  upan::shared_ptr<IODescriptor> openInMemoryTerminalDevice(const upan::string& filePath);
  void remove(const upan::string& filePath) ;
  bool fileExists(const upan::string& filePath);
  bool directoryExists(const upan::string& filePath);
  upan::option<struct stat> stats(const upan::string& filePath);
  void getpwd(char** pwd);
  upan::string getcwd();
  void rename(const upan::string& oldPath, const upan::string& newPath);
  void changeDir(const upan::string& dirPath, char** retPwd);
  DIR* opendir(const upan::string& dirPath);
  void readdir(DIR* dirp);
  void closedir(DIR* dirp);
  bool fileAccess(const upan::string& filePath, int mode);
  void dup2(int oldFD, int newFD);

private:
  StorageDrive& parseFilePath(const upan::string& fullFilePath, const Process& process,
                              FileNodeRef& cwd, FileTree::NodeTokens& fileTokens);
  FileNodeRef exists(const upan::string& filePath);
};

bool FileOperations_ReadLine(int fd, upan::string& line);
