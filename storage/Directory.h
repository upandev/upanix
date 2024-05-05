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
#ifndef _DIRECTORY_H_
#define _DIRECTORY_H_

#include <ProcessManager.h>
#include <FileSystem.h>
#include <IODescriptorTable.h>
#include <DeviceDrive.h>

#define Directory_SUCCESS					0
#define Directory_ERR_UNKNOWN_DEVICE		1
#define Directory_ERR_FS_TABLE_CORRUPTED	2
#define Directory_ERR_EXISTS				3
#define Directory_ERR_NOT_EXISTS			4
#define Directory_ERR_ZERO_WRITE_SIZE		5
#define Directory_ERR_NOT_EMPTY				6
#define Directory_ERR_INVALID_OFFSET		7
#define Directory_ERR_IS_DIRECTORY			8
#define Directory_ERR_EOF					9
#define Directory_ERR_IS_NOT_DIRECTORY		10
#define Directory_ERR_NOT_DIR				11
#define Directory_ERR_SPECIAL_ENTRIES		12
#define Directory_ERR_INVALID_NAME			14
#define Directory_ERR_INVALID_DRIVE			15
#define Directory_FAILURE					16

#define DIR_SPECIAL_CURRENT		"."
#define DIR_SPECIAL_PARENT		".."

class FileDescriptor;

void Directory_Create(Process* processAddressSpace, int iDriveID, byte* bParentDirectoryBuffer, const FileSystem::CWD* pCWD,
                      char* szDirName, unsigned short usDirAttribute) ;
void Directory_Delete(Process* processAddressSpace, int iDriveID, byte* bParentDirectoryBuffer, const FileSystem::CWD* pCWD,
                      const char* szDirName) ;
void Directory_GetDirEntryForCreateDelete(const Process*, int iDriveID, const char* szDirPath, char* szDirName, unsigned& uiSectorNo, byte& bSectorPos, byte* bDirectoryBuffer) ;
bool Directory_FindDirectory(DiskDrive&, const FileSystem::CWD& cwd, const char* szDirName, unsigned& uiSectorNo, byte& bSectorPos, byte* bDestSectorBuffer);
void Directory_GetDirectoryContent(const char* szFileName, Process* processAddressSpace, int iDriveID, FileSystem::Node** pDirList, int* iListSize) ;
void Directory_FileWrite(DiskDrive* pDiskDrive, FileSystem::CWD* pCWD, FileDescriptor& fdEntry, byte* bDataBuffer, unsigned uiDataSize) ;
void Directory_ActualFileWrite(DiskDrive* pDiskDrive, byte* bDataBuffer, FileDescriptor& fdEntry, unsigned uiDataSize, FileSystem::Node* dirFile) ;
int Directory_FileRead(DiskDrive* pDiskDrive, FileSystem::CWD* pCWD, FileDescriptor& fdEntry, byte* bDataBuffer, unsigned uiDataSize);
void Directory_ReadDirEntryInfo(DiskDrive&, const FileSystem::CWD&, const char* szFileName, unsigned& uiSectorNo, byte& bSectorPos, byte* bDirectoryBuffer) ;
void Directory_Change(const char* szFileName, int iDriveID, Process* processAddressSpace) ;
void Directory_PresentWorkingDirectory(Process* processAddressSpace, char** uiReturnDirPathAddress) ;
const FileSystem::Node Directory_GetDirEntry(const char* szFileName, Process* processAddressSpace, int iDriveID) ;
void Directory_SyncPWD(Process* processAddressSpace) ;

#endif
