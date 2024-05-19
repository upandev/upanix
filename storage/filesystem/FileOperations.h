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

#define FileOperations_SUCCESS					0
#define FileOperations_FAILURE					5

#include <Global.h>
#include <FileSystem.h>
#include <FileDescriptor.h>
#include <mutex.h>
#include <FileNodeRef.h>

#define ATTR_READ	0x4
#define ATTR_WRITE	0x2
#define ATTR_EXE	0x1

#define S_OWNER(perm)		((perm & 0x7) << 6)
#define S_GROUP(perm)		((perm & 0x7) << 3)
#define S_OTHERS(perm)		(perm & 0x7)

#define G_OWNER(perm)		((perm >> 6) & 0x7)
#define G_GROUP(perm)		((perm >> 3) & 0x7)
#define G_OTHERS(perm)		(perm & 0x7)

#define FILE_PERM_MASK	0x1FF
#define FILE_TYPE_MASK	0xF000

#define HAS_READ_PERM(perm)		((perm & 0x7) & ATTR_READ)
#define HAS_WRITE_PERM(perm)	((perm & 0x7) & ATTR_WRITE)
#define HAS_EXE_PERM(perm)		((perm & 0x7) & ATTR_EXE)

#define FILE_PERM(attr)	(attr & FILE_PERM_MASK)
#define FILE_TYPE(attr) (attr & FILE_TYPE_MASK)

#define S_ISDIR(attr) (FILE_TYPE(attr) == ATTR_TYPE_DIRECTORY)

#define FILE_STDOUT "STDOUT"
#define FILE_STDIN  "STDIN"
#define FILE_STDERR "STDERR"

typedef enum
{
	DIR_ACCESS_TIME = 0x01,
	DIR_MODIFIED_TIME = 0x02
} TIME_TYPE ;

typedef enum
{
	USER_OWNER,
	USER_GROUP,
	USER_OTHERS
} FILE_USER_TYPE ;

class IODescriptor;
class Process;

class FileOperations {
public:
  static FileOperations& Instance() {
    static FileOperations instance;
    return instance;
  }

  void create(const upan::string& filePath, uint16_t fileType, uint16_t mode);
  FileDescriptor& open(const char* szFileName, const byte mode);
  bool close(int fd);
  void remove(const char* szFilePath) ;
  bool exists(const char* szFileName, unsigned short usFileType);

private:
  StorageDrive& parseFilePath(const upan::string& fullFilePath, const Process& process,
                              FileNodeRef& cwd, FileTree::NodeTokens& fileTokens);
  void _create(const char* szFile, unsigned short usFileType, unsigned short usMode, Process& pas, int driveId);
  bool _exists(const char* szFile, unsigned short usFileType, Process& pas, int driveId);

private:
  upan::mutex _fileOpMutex;
};

bool FileOperations_ReadLine(int fd, upan::string& line);
void FileOperations_UpdateTime(StorageDrive& diskDrive, const FileSystem::WorkingDirectory& cwd, const char* szFileName, byte bTimeType);
FileNode FileOperations_GetDirEntry(const char* szFileName);
struct stat FileOperations_GetStat(const char* szFileName, int iDriveID) ;
struct stat FileOperations_GetStat(StorageDrive& diskDrive, const FileSystem::WorkingDirectory& cwd, const char* szFileName);
byte FileOperations_GetFileOpenMode(int fd) ;
void FileOperations_SyncPWD() ;
void FileOperations_ChangeDir(const char* szFileName) ;
void FileOperations_GetDirectoryContent(const char* szPathAddress, FileNode** pDirList, int* iListSize) ;
bool FileOperations_FileAccess(const char* szFileName, int iDriveID, int mode) ;
void FileOperations_Dup2(int oldFD, int newFD) ;
void FileOperations_GetCWD(char* szPathBuf, int iBufSize) ;
