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

#include <ProcessManager.h>
#include <FileSystem.h>
#include <IODescriptorTable.h>
#include <StorageDrive.h>

class FileDescriptor;

void Directory_Create(Process* processAddressSpace, StorageDrive &diskDrive, byte* bParentDirectoryBuffer, FileSystem::WorkingDirectory &cwd,
                      char* szDirName, unsigned short usDirAttribute) ;
void Directory_Delete(Process &pas, StorageDrive &diskDrive, byte* bParentDirectoryBuffer, FileSystem::WorkingDirectory &cwd,
                      const char* szDirName) ;
void Directory_GetDirEntryForCreateDelete(Process &pas, StorageDrive &diskDrive, const char* szDirPath, char* szDirName, unsigned& uiSectorNo, byte& bSectorPos, byte* bDirectoryBuffer) ;
bool Directory_FindDirectory(StorageDrive&, const FileSystem::WorkingDirectory& cwd, const char* szDirName, unsigned& uiSectorNo, byte& bSectorPos, byte* bDestSectorBuffer);
void Directory_GetDirectoryContent(const char* szFileName, Process &pas, int iDriveID, FileNode** pDirList, int* iListSize) ;
void Directory_FileWrite(StorageDrive* pDiskDrive, const FileSystem::WorkingDirectory &cwd, FileDescriptor& fdEntry, byte* bDataBuffer, unsigned uiDataSize) ;
void Directory_ActualFileWrite(StorageDrive* pDiskDrive, byte* bDataBuffer, FileDescriptor& fdEntry, unsigned uiDataSize, FileNode* dirFile) ;
int Directory_FileRead(StorageDrive* pDiskDrive, const FileSystem::WorkingDirectory &cwd, FileDescriptor& fdEntry, byte* bDataBuffer, unsigned uiDataSize);
void Directory_ReadDirEntryInfo(StorageDrive&, const FileSystem::WorkingDirectory&, const char* szFileName, unsigned& uiSectorNo, byte& bSectorPos, byte* bDirectoryBuffer) ;
void Directory_Change(const char* szFileName, int iDriveID, Process &pas) ;
void Directory_PresentWorkingDirectory(Process* processAddressSpace, char** uiReturnDirPathAddress) ;
FileNode Directory_GetDirEntry(const char* szFileName, Process &pas, int iDriveID) ;
void Directory_SyncPWD(Process &pas) ;
