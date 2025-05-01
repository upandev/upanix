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
# include "StorageDriveManager.h"
# include "drivers/floppy/Floppy.h"
# include "drivers/ide/ATADrive.h"
# include "drivers/ide/ATADeviceController.h"
# include "PartitionManager.h"
# include "memory/DMM.h"
# include "util/StringUtil.h"
# include "process/ProcessManager.h"
# include "drivers/bus/SCSIHandler.h"
# include "stdio.h"
# include "kernel/MountManager.h"
# include "DiskCache.h"
# include "util/KernelUtil.h"
# include "storage/filesystem/FileSystem.h"
# include "try.h"
# include "drive.h"

StorageDriveManager::StorageDriveManager() : _idSequence(0) {
}

void StorageDriveManager::Create(const upan::string& driveName,
                                 DEVICE_TYPE deviceType, DRIVE_NO driveNumber,
                                 unsigned uiLBAStartSector, unsigned uiSizeInSectors,
                                 unsigned uiSectorsPerTrack, unsigned uiTracksPerHead, unsigned uiNoOfHeads,
                                 void* device, RawStorageDrive* rawDisk,
                                 unsigned uiMaxSectorsInFreePoolCache)
{
  upan::mutex_guard g(_driveListMutex);
  StorageDrive* pDiskDrive = new StorageDrive(_idSequence++, driveName,
                                              deviceType, driveNumber,
                                              uiLBAStartSector, uiSizeInSectors,
                                              uiSectorsPerTrack, uiTracksPerHead, uiNoOfHeads,
                                              device, rawDisk,
                                              uiMaxSectorsInFreePoolCache);
  _driveList.push_back(pDiskDrive);
}

RawStorageDrive* StorageDriveManager::CreateRawDisk(const upan::string& name, RawStorageDrive::Types iType, void* pDevice)
{
  for(auto d : _rawDiskList)
  {
		if(d->Name() == name)
      throw upan::exception(XLOC, "\n Raw Drive '%s' already in the list", name.c_str());
	}
  RawStorageDrive* pDisk = new RawStorageDrive(name, iType, pDevice);
  _rawDiskList.push_back(pDisk);
	return pDisk;
}

byte StorageDriveManager::RemoveRawDiskEntry(const upan::string& name)
{
  for(auto it = _rawDiskList.begin(); it != _rawDiskList.end(); ++it)
  {
		if((*it)->Name() == name)
    {
      _rawDiskList.erase(it);
			return DeviceDrive_SUCCESS;
		}
  }
	return DeviceDrive_ERR_NOTFOUND;
}

RawStorageDrive* StorageDriveManager::GetRawDiskByName(const upan::string& name)
{
  auto it = upan::find_if(_rawDiskList.begin(), _rawDiskList.end(), [&name](const RawStorageDrive* d) { return d->Name() == name; });
  if(it == _rawDiskList.end())
    return nullptr;
  return *it;
}

void StorageDriveManager::RemoveEntryByCondition(const DriveRemoveClause& removeClause)
{
  upan::mutex_guard g(_driveListMutex);

  for(auto it = _driveList.begin(); it != _driveList.end();)
  {
		if(removeClause(*it))
		{
			if((*it)->Mounted())
        (*it)->UnMount();
      delete *it;
      _driveList.erase(it++);
		}
    else
      ++it;
	}
}

upan::result<StorageDrive&> StorageDriveManager::GetByDriveName(const upan::string& driveName, bool bCheckMount) {
  upan::mutex_guard g(_driveListMutex);
  if (driveName == ROOT_DRIVE_SYN) {
    return GetByID(ROOT_DRIVE_ID, bCheckMount);
  }
  auto it = upan::find_if(_driveList.begin(), _driveList.end(), [&driveName, bCheckMount](const StorageDrive* d)
    {
      if(d->DriveName() == driveName)
        return d->Mounted() || !bCheckMount;
      return false;
    });
  if(it == _driveList.end())
    return upan::result<StorageDrive&>::bad("failed to find drive %s (mounted: %d)", driveName.c_str(), bCheckMount);
  return upan::result<StorageDrive&>(**it);
}

upan::result<StorageDrive&> StorageDriveManager::GetByID(int iID, bool bCheckMount)
{	
  upan::mutex_guard g(_driveListMutex);
	if(iID == ROOT_DRIVE)
		iID = ROOT_DRIVE_ID;
	else if(iID == CURRENT_DRIVE)
		iID = ProcessManager::Instance().GetCurrentPAS().driveID();
  auto it = upan::find_if(_driveList.begin(), _driveList.end(), [iID, bCheckMount](const StorageDrive* d)
    {
      if(d->Id() == iID)
        return d->Mounted() || !bCheckMount;
      return false;
    });
  if(it == _driveList.end())
    return upan::result<StorageDrive&>::bad("failed to find drive id %d (mounted: %d)", iID, bCheckMount);
  return upan::result<StorageDrive&>(**it);
}

void StorageDriveManager::DisplayList()
{	
  for(auto d : _driveList)
  {
		DDWORD i = (DDWORD)d->SizeInSectors() * (DDWORD)512;
    printf("\n %s (%u - %llu - %x)", d->DriveName().c_str(), d->SizeInSectors(), i, i);
	}
}

byte StorageDriveManager::Change(const upan::string& szDriveName) {
  auto r = GetByDriveName(szDriveName, false);
	if(r.isBad()) {
    return DeviceDrive_ERR_INVALID_DRIVE_NAME;
  }

  auto& storageDrive = r.goodValue();
	auto& pas = ProcessManager::Instance().GetCurrentPAS();
	pas.setDriveID(storageDrive.Id());
  pas.pwd(storageDrive.fileSystem().root());

  pas.setEnv("PWD", storageDrive.DriveName() + "@" + storageDrive.fileSystem().fullPath(storageDrive.fileSystem().root()));

	return DeviceDrive_SUCCESS ;
}

byte StorageDriveManager::GetList(DriveStat** pDriveList, int* iListSize)
{	
  upan::mutex_guard g(_driveListMutex);

	*pDriveList = NULL ;
	*iListSize = _driveList.size();
	
	if(_driveList.empty())
		return DeviceDrive_SUCCESS ;

	Process* pAddrSpace = &ProcessManager::Instance().GetCurrentPAS() ;
	DriveStat* pAddress = NULL ;
	
	if(pAddrSpace->isKernelProcess())
	{
		*pDriveList = (DriveStat*)KernelDMM::Instance().allocate(sizeof(DriveStat) * _driveList.size());
		pAddress = *pDriveList ;
	}
	else
	{
		*pDriveList = (DriveStat*)pAddrSpace->dmm().allocate(sizeof(DriveStat) * _driveList.size());
		pAddress = (DriveStat*)(*pDriveList);
	}

	if(pAddress == NULL)
		return DeviceDrive_FAILURE ;

	int i = 0;
  for(auto d : _driveList)
	{
    strncpy(pAddress[i].driveName, d->DriveName().c_str(), 32);
    pAddress[i].bMounted = d->Mounted();
    pAddress[i].uiSizeInSectors = d->SizeInSectors();
		pAddress[i].ulTotalSize = 0;
		pAddress[i].ulUsedSize = 0;

		if(d->Mounted())
    {
      pAddress[i].ulTotalSize = d->fileSystem().totalSize();
      pAddress[i].ulUsedSize = d->fileSystem().usedSize();
    }
		
		++i;
	}

	return DeviceDrive_SUCCESS ;
}

void StorageDriveManager::MountDrive(const upan::string& szDriveName)
{
  GetByDriveName(szDriveName, false).goodValueOrThrow(XLOC).Mount();
}

void StorageDriveManager::UnMountDrive(const upan::string& szDriveName) {
  StorageDrive& storageDrive = GetByDriveName(szDriveName, false).goodValueOrThrow(XLOC);

	bool bKernel = IsKernel() ? true : IsKernelProcess(ProcessManager::GetCurrentProcessID()) ;
	if(!bKernel) {
		if(storageDrive.Id() == ProcessManager::Instance().GetCurrentPAS().driveID()) {
      throw upan::exception(XLOC, "can't unmount current drive: %s", szDriveName.c_str());
    }
	}

  storageDrive.UnMount();
}

void StorageDriveManager::FormatDrive(const upan::string& szDriveName)
{
  auto& storageDrive = GetByDriveName(szDriveName, false).goodValueOrThrow(XLOC);

  storageDrive.Format();

	switch((int)storageDrive.DeviceType())
	{
		case DEV_ATA_IDE:
		case DEV_SCSI_USB_DISK:
      storageDrive.RawDisk()->UpdateSystemIndicator(storageDrive.LBAStartSector(), 0x93);
			break ;
	}
}

void StorageDriveManager::GetCurrentDriveStat(DriveStat* pDriveStat)
{
  StorageDrive& diskDrive = GetByID(ProcessManager::Instance().GetCurrentPAS().driveID(), true).goodValueOrThrow(XLOC);

  strncpy(pDriveStat->driveName, diskDrive.DriveName().c_str(), 32);
  pDriveStat->bMounted = diskDrive.Mounted();
  pDriveStat->uiSizeInSectors = diskDrive.SizeInSectors();
  pDriveStat->ulTotalSize = 0;
  pDriveStat->ulUsedSize = 0;
}

RESOURCE_KEYS StorageDriveManager::GetResourceType(DEVICE_TYPE deviceType)
{
	switch(deviceType)
	{
		case DEV_FLOPPY: return RESOURCE_FDD;
		case DEV_ATA_IDE: return RESOURCE_HDD;
		case DEV_SCSI_USB_DISK: return RESOURCE_USD;
    default: return RESOURCE_GENERIC_DISK;
	}
}

RESOURCE_KEYS StorageDriveManager::GetResourceType(RawStorageDrive::Types diskType)
{
	switch(diskType)
	{
		case RawStorageDrive::ATA_HARD_DISK: return RESOURCE_HDD;
		case RawStorageDrive::USB_SCSI_DISK: return RESOURCE_USD;
	//TODO: FLOPPY falls under this category which should be changed to particular disk type
    default: return RESOURCE_GENERIC_DISK;
	}

}