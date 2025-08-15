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
# include <drive.h>

int SysDrive_ChangeDrive(const char* szDriveName, char** retPwd)
{
  uint64_t retStatus ;
  SysCallDrive_Handle(&retStatus, SYS_CALL_CHANGE_DRIVE, false, (uintptr_t) szDriveName, (uintptr_t)retPwd, 3, 4, 5);
	return retStatus ;
}

int SysDrive_ShowDrives(DriveStat** pDriveList, int* iListSize)
{
  uint64_t retStatus ;
  SysCallDrive_Handle(&retStatus, SYS_CALL_SHOW_DRIVES, false, (uintptr_t) pDriveList, (uintptr_t) iListSize, 3, 4, 5);
	return retStatus ;
}

int SysDrive_Mount(const char* szDriveName)
{
  uint64_t retStatus ;
  SysCallDrive_Handle(&retStatus, SYS_CALL_MOUNT_DRIVE, false, (uintptr_t) szDriveName, 2, 3, 4, 5);
	return retStatus ;
}

int SysDrive_UnMount(const char* szDriveName)
{
  uint64_t retStatus ;
  SysCallDrive_Handle(&retStatus, SYS_CALL_UNMOUNT_DRIVE, false, (uintptr_t) szDriveName, 2, 3, 4, 5);
	return retStatus ;
}

int SysDrive_Format(const char* szDriveName)
{
  uint64_t retStatus ;
  SysCallDrive_Handle(&retStatus, SYS_CALL_FORMAT_DRIVE, false, (uintptr_t) szDriveName, 2, 3, 4, 5);
	return retStatus ;
}

int SysDrive_GetCurrentDriveStat(DriveStat* pDriveStat)
{
  uint64_t retStatus ;
  SysCallDrive_Handle(&retStatus, SYS_CALL_CURRENT_DRIVE_STAT, false, (uintptr_t) pDriveStat, 2, 3, 4, 5);
	return retStatus ;
}
