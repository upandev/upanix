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

class IODescriptor;
class Process;

class FileOperations {
public:
  static FileOperations& Instance() {
    static FileOperations instance;
    return instance;
  }

  void create(const upan::string& filePath, uint16_t fileType, uint16_t mode);
  upan::option<FileDescriptor&> open(const upan::string& filePath, const uint8_t mode);
  bool close(int fd);
  void remove(const upan::string& filePath) ;
  bool fileExists(const upan::string& filePath);
  bool directoryExists(const upan::string& filePath);
  struct stat stats(const upan::string& filePath);
  void getpwd(char** pwd);
  upan::string getcwd();
  void changeDir(const upan::string& dirPath);
  void listDir(const upan::string& filePath, FileStats& fileStats);
  void listDir(const upan::string& filePath, struct stat_ex** fileStatsArray, int* size);
  bool fileAccess(const upan::string& filePath, uint8_t mode);
  void dup2(int oldFD, int newFD);

private:
  StorageDrive& parseFilePath(const upan::string& fullFilePath, const Process& process,
                              FileNodeRef& cwd, FileTree::NodeTokens& fileTokens);
  FileNodeRef exists(const upan::string& filePath);
};

bool FileOperations_ReadLine(int fd, upan::string& line);
