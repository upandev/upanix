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
# include <SysCallDrive.h>
# include <DeviceDrive.h>
# include <try.h>

byte SysCallDrive_IsPresent(uint64_t sysCallId)
{
	return (sysCallId > SYS_CALL_DRIVE_START && sysCallId < SYS_CALL_DRIVE_END) ;
}

void
SysCallDrive_Handle(uint64_t *retVal, uint64_t sysCallId, bool doAddrTranslation, uint64_t p1, uint64_t p2, uint64_t p3,
                    uint64_t p4, uint64_t p5)
{
	switch(sysCallId)
	{
		case SYS_CALL_CHANGE_DRIVE : //Change Drive
			//P1 => Drive Name
			{
				char* szDriveName = ( char*) p1;

				*retVal = 0 ;
				if(DiskDriveManager::Instance().Change(szDriveName) != DeviceDrive_SUCCESS)
					*retVal = -1 ;
			}
			break ;

		case SYS_CALL_SHOW_DRIVES :
			// P1 => Ret Drive Stat List
			// P2 => Ret Drive Stat List Size
			{
				DriveStat** pDriveList = ( DriveStat**) p1;
				int* iListSize = ( int*) p2;

				*retVal = 0 ;
				if(DiskDriveManager::Instance().GetList(pDriveList, iListSize) != DeviceDrive_SUCCESS)
					*retVal = -1 ;
			}
			break ;

		case SYS_CALL_MOUNT_DRIVE : //Mount Drive
			//P1 => Drive Name
			{
				char* szDriveName = ( char*) p1;
        *retVal = 0;
        try
        {
          DiskDriveManager::Instance().MountDrive(szDriveName);
        }
        catch(...)
        {
          *retVal = -1;
        }
			}
			break ;

		case SYS_CALL_UNMOUNT_DRIVE : //UnMount Drive
			//P1 => Drive Name
			{
				char* szDriveName = ( char*) p1;
        *retVal = 0;
        try
        {
          DiskDriveManager::Instance().UnMountDrive(szDriveName);
        }
        catch(...)
        {
          *retVal = -1;
        }
			}
			break ;

		case SYS_CALL_FORMAT_DRIVE : //Format Drive
			//P1 => Drive Name
			{
				char* szDriveName = ( char*) p1;
        *retVal = 0;
        try
        {
          DiskDriveManager::Instance().FormatDrive(szDriveName);
        }
        catch(...)
        {
          *retVal = -1;
        }
			}
			break ;

		case SYS_CALL_CURRENT_DRIVE_STAT : //Current Drive
			//P1 => Ret Drive
			{
        *retVal = 0;
        try
        {
          DriveStat* pDriveStat = ( DriveStat*) p1;
          DiskDriveManager::Instance().GetCurrentDriveStat(pDriveStat);
        }
        catch(...)
        {
          *retVal = -1;
        }
			}
			break ;
	}
}
