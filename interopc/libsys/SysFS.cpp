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

int SysFS_ChangeDirectory(const char* szDirPath, char** retPwd)
{
	uint64_t retStatus ;
  SysCallFile_Handle(&retStatus, SYS_CALL_CHANGE_DIR, false, (uint64_t)szDirPath, (uint64_t)retPwd, 3, 4, 5);
	return retStatus ;
}

int SysFS_CWD(char* uiReturnDirPathAddress, int len) {
  uint64_t retStatus ;
  SysCallFile_Handle(&retStatus, SYS_CALL_CWD, false, (uint64_t) uiReturnDirPathAddress, len, 3, 4, 5);
  return retStatus;
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

int SysFS_CreateFile(const char* szDirPath, unsigned short usAttribute)
{
  uint64_t retStatus ;
  SysCallFile_Handle(&retStatus, SYS_CALL_FILE_CREATE, false, (uint64_t) szDirPath, (uint64_t) usAttribute, 3, 4, 5);
	return retStatus ;
}

int SysFS_FileOpen(const char* szFileName, uint32_t mode)
{
  uint64_t retStatus ;
  SysCallFile_Handle(&retStatus, SYS_CALL_FILE_OPEN, false, (uint64_t) szFileName, (uint64_t) mode, 3, 4, 5);
	return retStatus ;
}

int SysFS_FileOpenStream(uint32_t mode)
{
  uint64_t retStatus ;
  SysCallFile_Handle(&retStatus, SYS_CALL_FILE_OPEN_STREAM, false, (uint64_t) mode, 2, 3, 4, 5);
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

int SysFS_FileSelect(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout) {
  uint64_t retStatus ;
  SysCallFile_Handle(&retStatus, SYS_CALL_FILE_SELECT, false, (uint64_t)nfds, (uint64_t)readfds, (uint64_t)writefds, (uint64_t)exceptfds, (uint64_t)timeout);
  return (int)retStatus;
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

int SysFS_FileAccess(const char* szFileName, int mode) {
  uint64_t retStatus;
  SysCallFile_Handle(&retStatus, SYS_CALL_FILE_ACCESS, false, (uint64_t)szFileName, (uint64_t)mode, 3, 4, 5);
  return retStatus;
}

int SysFS_Dup2(int oldFD, int newFD) {
  uint64_t retStatus;
  SysCallFile_Handle(&retStatus, SYS_CALL_FILE_DUP2, false, (uint64_t)oldFD, (uint64_t)newFD, 3, 4, 5);
  return retStatus;
}

DIR* SysFS_OpenDir(const char* szDirPath) {
  uint64_t retStatus;
  SysCallFile_Handle(&retStatus, SYS_CALL_FILE_OPEN_DIR, false, (uint64_t)szDirPath, 2, 3, 4, 5);
  return reinterpret_cast<DIR*>(retStatus);
}

struct dirent* SysFS_ReadDir(DIR* dirp) {
  uint64_t retStatus;
  SysCallFile_Handle(&retStatus, SYS_CALL_FILE_READ_DIR, false, (uint64_t)dirp, 2, 3, 4, 5);
  return reinterpret_cast<struct dirent*>(retStatus);
}

int SysFS_CloseDir(DIR* dirp) {
  uint64_t retStatus;
  SysCallFile_Handle(&retStatus, SYS_CALL_FILE_CLOSE_DIR, false, (uint64_t)dirp, 2, 3, 4, 5);
  return (int)retStatus;
}
