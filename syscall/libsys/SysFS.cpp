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
# include <SysCall.h>
# include <fs.h>

int SysFS_ChangeDirectory(const char* szDirPath)
{
	uint64_t retStatus ;
  SysCallFile_Handle(&retStatus, SYS_CALL_CHANGE_DIR, false, (uint64_t) szDirPath, 2, 3, 4, 5);
	return retStatus ;
}

void SysFS_PWD(char** uiReturnDirPathAddress)
{
  uint64_t retStatus ;
  SysCallFile_Handle(&retStatus, SYS_CALL_PWD, false, (uint64_t) uiReturnDirPathAddress, 2, 3, 4, 5);
}

int SysFS_CreateDirectory(const char* szDirPath, unsigned short usAttribute)
{
  uint64_t retStatus ;
  SysCallFile_Handle(&retStatus, SYS_CALL_MKDIR, false, (uint64_t) szDirPath, (uint64_t) usAttribute, 3, 4, 5);
	return retStatus ;
}

int SysFS_DeleteDirectory(const char* szDirPath)
{
  uint64_t retStatus ;
  SysCallFile_Handle(&retStatus, SYS_CALL_RMDIR, false, (uint64_t) szDirPath, 2, 3, 4, 5);
	return retStatus ;
}

int SysFS_GetDirContent(const char* szDirPath, FileNode** pDirList, int* iListSize)
{
  uint64_t retStatus ;
  SysCallFile_Handle(&retStatus, SYS_CALL_GET_DIR_LIST, false, (uint64_t) szDirPath, (uint64_t) pDirList,
                     (uint64_t) iListSize,
                     4, 5);
	return retStatus ;
}

int SysFS_CreateFile(const char* szDirPath, unsigned short usAttribute)
{
  uint64_t retStatus ;
  SysCallFile_Handle(&retStatus, SYS_CALL_FILE_CREATE, false, (uint64_t) szDirPath, (uint64_t) usAttribute, 3, 4, 5);
	return retStatus ;
}

int SysFS_FileOpen(const char* szFileName, byte bMode)
{
  uint64_t retStatus ;
  SysCallFile_Handle(&retStatus, SYS_CALL_FILE_OPEN, false, (uint64_t) szFileName, (uint64_t) bMode, 3, 4, 5);
	return retStatus ;
}

int SysFS_FileClose(int fd)
{
  uint64_t retStatus ;
  SysCallFile_Handle(&retStatus, SYS_CALL_FILE_CLOSE, false, fd, 2, 3, 4, 5);
	return retStatus ;
}

int SysFS_FileRead(int fd, void* buf, int len)
{
  uint64_t retStatus ;
  SysCallFile_Handle(&retStatus, SYS_CALL_FILE_READ, false, fd, (uint64_t) buf, len, 4, 5);
	return retStatus ;
}

int SysFS_FileWrite(int fd, const void* buf, int len)
{
  uint64_t retStatus ;
  SysCallFile_Handle(&retStatus, SYS_CALL_FILE_WRITE, false, fd, (uint64_t) buf, len, 4, 5);
	return retStatus ;
}

void SysFS_FileSelect(io_descriptor* waitIODescriptors, io_descriptor* readyIODescriptors) {
  uint64_t retStatus ;
  SysCallFile_Handle(&retStatus, SYS_CALL_FILE_SELECT, false, (uint64_t) waitIODescriptors,
                     (uint64_t) readyIODescriptors, 3, 4, 5);
}

int SysFS_FileSeek(int fd, int offSet, int seekType)
{
  uint64_t retStatus ;
  SysCallFile_Handle(&retStatus, SYS_CALL_FILE_SEEK, false, fd, offSet, seekType, 4, 5);
	return retStatus ;
}

int SysFS_FileTell(int fd)
{
  uint64_t retStatus ;
  SysCallFile_Handle(&retStatus, SYS_CALL_FILE_TELL, false, fd, 2, 3, 4, 5);
	return retStatus ;
}

int SysFS_FileOpenMode(int fd)
{
  uint64_t retStatus ;
  SysCallFile_Handle(&retStatus, SYS_CALL_FILE_MODE, false, fd, 2, 3, 4, 5);
	return retStatus ;
}

int SysFS_FileStat(const char* szFileName, struct stat* pFileStat)
{
  uint64_t retStatus ;
  SysCallFile_Handle(&retStatus, SYS_CALL_FILE_STAT, false, (uint64_t) szFileName, (uint64_t) pFileStat, 3, 4, 5);
	return retStatus ;
}

int SysFS_FileStatFD(int iFD, struct stat* pFileStat)
{
  uint64_t retStatus ;
  SysCallFile_Handle(&retStatus, SYS_CALL_FILE_STAT_FD, false, iFD, (uint64_t) pFileStat, 3, 4, 5);
	return retStatus ;
}

int read(int fd, void* buf, int len)
{
	return SysFS_FileRead(fd, buf, len) ;
}

int write(int fd, const void* buf, int len)
{
	return SysFS_FileWrite(fd, buf, len) ;
}

void select(io_descriptor* waitIODescriptors, io_descriptor* readyIODescriptors) {
  return SysFS_FileSelect(waitIODescriptors, readyIODescriptors);
}

int lseek(int fd, int offset, int seekType)
{
	return SysFS_FileSeek(fd, offset, seekType) ;
}

unsigned tell(int fd)
{
	return SysFS_FileTell(fd) ;
}

int getomode(int fd)
{
	return SysFS_FileOpenMode(fd) ;
}

int create(const char* file_path, unsigned short file_attr)
{
	return SysFS_CreateFile(file_path, file_attr) ;
}

int open(const char* file_name, byte mode)
{
	return SysFS_FileOpen(file_name, mode) ;
}

int close(int fd)
{
	return SysFS_FileClose(fd) ;
}

int stat(const char* szFileName, struct stat* pFileStat)
{
	return SysFS_FileStat(szFileName, pFileStat) ;
}

int fstat(int iFD, struct stat* pFileStat)
{
	return SysFS_FileStatFD(iFD, pFileStat) ;
}
