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
# include <DMM.h>
# include <Directory.h>
# include <FileOperations.h>
# include <MultiBoot.h>
# include <MountManager.h>
# include <try.h>
# include <drive.h>
# include <StorageDriveManager.h>

static char MountManager_szRootDriveName[33] = "" ;
static int MountManager_iRootDriveID = CURRENT_DRIVE ;
static bool MountManager_bInitStatus = false ;

/*************************** static *****************************************/
static void MountManager_GetBootMountDrive(char* szBootDriveName)
{
	byte bBootDevice = MultiBoot::Instance().GetBootDeviceID() ;
	byte bBootPartitionID = MultiBoot::Instance().GetBootPartitionID() ;

	switch(bBootDevice)
	{
	case DEV_FLOPPY:
		strcpy(szBootDriveName, "floppya") ;
		break ;
	
	case DEV_ATA_IDE:
		strcpy(szBootDriveName, "hdda") ;
		szBootDriveName[3] += bBootPartitionID ;	
		break ;	

	case DEV_SCSI_USB_DISK:
		strcpy(szBootDriveName, "usda") ;
		szBootDriveName[3] += bBootPartitionID ;
		break ;

	default:
		strcpy(szBootDriveName, "floppya") ;
		break ;
	}
}

static bool MountManager_GetHomeMountDrive(char* szHomeDriveName, unsigned uiSize)
{
  auto result = upan::tryreturn([&]() {
    auto& fd = FileOperations::Instance().open("ROOT@/.mount.lst", O_RDONLY);
    return fd.read(szHomeDriveName, uiSize);
  });

  if(result.isBad())
    return false;

  int bytesRead = result.goodValue();
	
  szHomeDriveName[bytesRead - 1] = '\0' ; /* Junk Fix... Use ctype and trim functions
	from UPANIXApps library... port it to kernel using kernel coding conventions */

	return true ;
}

static void MountManager_MountDrive(char* szDriveName)
{
  printf("\n Mounting Drive: %s ...", szDriveName);

	// Find Drive
  StorageDrive* pDiskDrive = StorageDriveManager::Instance().GetByDriveName(szDriveName, false).goodValueOrThrow(XLOC);

	// Mount Drive
  pDiskDrive->Mount();

	// Set Process Drive
  auto& pas = ProcessManager::Instance().GetCurrentPAS();
  pas.setDriveID(pDiskDrive->Id());
  pas.processPWD() = pDiskDrive->_fileSystem.pwd();

	// Change To Root Directory
  FileOperations_ChangeDir(FS_ROOT_DIR);
}
/****************************************************************************/

void MountManager_Initialize()
{
	MountManager_bInitStatus = false ;

	MountManager_GetBootMountDrive(MountManager_szRootDriveName) ;
	printf("\n\tBoot Mount Drive: %s", MountManager_szRootDriveName);
	
  StorageDrive* pDiskDrive = StorageDriveManager::Instance().GetByDriveName(MountManager_szRootDriveName, false).goodValueOrElse(nullptr);
	
	if(pDiskDrive == NULL)
	{
		MountManager_bInitStatus = false ;
		MountManager_iRootDriveID = CURRENT_DRIVE ;
	}

  KC::MConsole().LoadMessage("Mount Manager Initialization", MountManager_bInitStatus ? Success : Failure);
}

bool MountManager_GetInitStatus()
{
	return MountManager_bInitStatus ;
}

void MountManager_MountDrives()
{
	if(MountManager_bInitStatus == false)
	{
    KC::MConsole().Message("\n\tMount Manager Init Failed. Not mounting any drive", '@') ;
		return ;
	}
	
	MountManager_MountDrive(MountManager_szRootDriveName) ;
	char szHomeDriveName[33] ;
	if(MountManager_GetHomeMountDrive(szHomeDriveName, 32))
	{
		if(strcmp(MountManager_szRootDriveName, szHomeDriveName) != 0)
		{
			MountManager_MountDrive(szHomeDriveName) ;
		}	
	}
}

const char* MountManager_GetRootDriveName()
{
	return MountManager_szRootDriveName ;
}

int MountManager_GetRootDriveID()
{
	if(MountManager_iRootDriveID == CURRENT_DRIVE)
	{
    StorageDrive* pDiskDrive = StorageDriveManager::Instance().GetByDriveName(MountManager_szRootDriveName, false).goodValueOrElse(nullptr);
		
		if(pDiskDrive == NULL)
		{
			if(ProcessManager::GetCurrentProcessID() != NO_PROCESS_ID)
				return ProcessManager::Instance().GetCurrentPAS().driveID() ;

			return CURRENT_DRIVE ;
		}

		MountManager_iRootDriveID = pDiskDrive->Id();
	}
	
	return MountManager_iRootDriveID ;
}

void MountManager_SetRootDrive(StorageDrive* pDiskDrive)
{
	strcpy(MountManager_szRootDriveName, pDiskDrive->DriveName().c_str());
	MountManager_iRootDriveID = pDiskDrive->Id();
}
