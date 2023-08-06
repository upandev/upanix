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
# include <DeviceDrive.h>
# include <Floppy.h>
# include <ATADrive.h>
# include <ATADeviceController.h>
# include <PartitionManager.h>
# include <DMM.h>
# include <StringUtil.h>
# include <MemUtil.h>
# include <ProcessManager.h>
# include <SCSIHandler.h>
# include <stdio.h>
# include <MountManager.h>
# include <DiskCache.h>
# include <KernelUtil.h>
# include <FileSystem.h>
# include <try.h>
# include <drive.h>

static unsigned uiTotalFloppyDiskReads = 0;
static unsigned uiTotalATADiskReads = 0;
static unsigned uiTotalUSBDiskReads = 0;

void DiskCache_ShowTotalDiskReads()
{
  printf("\n Total Floppy Disk Reads: %u", uiTotalFloppyDiskReads) ;
  printf("\n Total ATA Disk Reads: %u", uiTotalATADiskReads) ;
  printf("\n Total USB Disk Reads: %u", uiTotalUSBDiskReads) ;
}

static void DiskCache_TaskFlushCache(DiskDrive* pDiskDrive, unsigned uiParam2)
{
	do
	{
	//	if(pDiskDrive->Mounted())
    pDiskDrive->FlushDirtyCacheSectors(10) ;
		ProcessManager::Instance().Sleep(200) ;
	} while(!pDiskDrive->StopReleaseCacheTask());

	ProcessManager_EXIT() ;
}

static void DiskCache_TaskReleaseCache(DiskDrive* pDiskDrive, unsigned uiParam2)
{
	do
	{
    pDiskDrive->ReleaseCache();
		ProcessManager::Instance().Sleep(50) ;
	} while(!pDiskDrive->StopReleaseCacheTask());

	ProcessManager_EXIT() ;
}

DiskDrive::DiskDrive(int id,
  const upan::string& driveName, 
  DEVICE_TYPE deviceType,
  DRIVE_NO driveNumber,
  unsigned uiLBAStartSector,
  unsigned uiSizeInSectors,
  unsigned uiSectorsPerTrack,
  unsigned uiTracksPerHead,
  unsigned uiNoOfHeads,
  void* device,
  RawDiskDrive* rawDisk,
  unsigned uiMaxSectorsInFreePoolCache) : _id(id),
    _driveName(driveName),
    _deviceType(deviceType),
    _driveNumber(driveNumber),
    _uiLBAStartSector(uiLBAStartSector),
    _uiSizeInSectors(uiSizeInSectors),
    _uiSectorsPerTrack(uiSectorsPerTrack),
    _uiTracksPerHead(uiTracksPerHead),
    _uiNoOfHeads(uiNoOfHeads),
    _bEnableDiskCache(true),
    _device(device),
    _rawDisk(rawDisk),
    _uiMaxSectorsInFreePoolCache(uiMaxSectorsInFreePoolCache),
    _fsType(FS_UNKNOWN),
    _mounted(false),
    _fileSystem(*this)
{
  StartReleaseCacheTask();
}

void DiskDrive::Mount()
{
	if(Mounted())
    throw upan::exception(XLOC, "Drive %s is already mounted", _driveName.c_str());

  _fileSystem.AllocateFreePoolQueue(MaxSectorsInFreePoolCache());

  _fileSystem.ReadFSBootBlock();
  _fileSystem.LoadFreeSectors();
  ReadRootDirectory();

  _mounted = true;
}

void DiskDrive::UnMount()
{
	if(!Mounted())
    throw upan::exception(XLOC, "drive %s is not mounted", _driveName.c_str());

  _fileSystem.WriteFSBootBlock();
  _fileSystem.FlushTableCache(MAX_SECTORS_IN_TABLE_CACHE);
	FlushDirtyCacheSectors();
  _fileSystem.UnallocateFreePoolQueue();

  _mounted = false;
}

void DiskDrive::ReadRootDirectory()
{
	byte bDataBuffer[512];

  xRead(bDataBuffer, 0, 1);
	
  _fileSystem.FSpwd.DirEntry = *reinterpret_cast<FileSystem::Node*>(bDataBuffer);
  _fileSystem.FSpwd.uiSectorNo = 0;
  _fileSystem.FSpwd.bSectorEntryPosition = 0;
}

void DiskDrive::Read(unsigned uiStartSector, unsigned uiNoOfSectors, byte* bDataBuffer)
{
  upan::mutex_guard g(_driveMutex);
  uiStartSector += LBAStartSector();

	if(!_bEnableDiskCache)
  {
    RawRead(uiStartSector, uiNoOfSectors, bDataBuffer) ;
    return;
  }

	DiskCacheValue* pCacheValue[ uiNoOfSectors ] ;

	unsigned uiEndSector = uiStartSector + uiNoOfSectors ;
	unsigned uiSectorIndex ;
	unsigned uiFirstBreak, uiLastBreak ;
	uiFirstBreak = uiLastBreak = uiEndSector ;

	for(uiSectorIndex = uiStartSector; uiSectorIndex < uiEndSector; uiSectorIndex++)
	{
		unsigned uiIndex = uiSectorIndex - uiStartSector;
		pCacheValue[ uiIndex ] = _mCache.Find(uiSectorIndex);
		if(!pCacheValue[ uiIndex ])
		{
			if(uiFirstBreak == uiEndSector)
				uiFirstBreak = uiSectorIndex ;

			uiLastBreak = uiSectorIndex ;
		}
	}

	__volatile__ unsigned uiIndex ;
	for(uiSectorIndex = uiStartSector; uiSectorIndex < uiFirstBreak; uiSectorIndex++)
	{
		uiIndex = uiSectorIndex - uiStartSector ;
		pCacheValue[ uiIndex ]->Read(bDataBuffer + (uiIndex * 512)) ;
	}

	for(uiSectorIndex = uiLastBreak + 1; uiSectorIndex < uiEndSector; uiSectorIndex++)
	{
		uiIndex = uiSectorIndex - uiStartSector ;
		pCacheValue[ uiIndex ]->Read(bDataBuffer + (uiIndex * 512)) ;
	}

	if(uiFirstBreak < uiEndSector)
	{
		uiIndex = uiFirstBreak - uiStartSector ;

    RawRead(uiFirstBreak, uiLastBreak - uiFirstBreak + 1, (bDataBuffer + uiIndex * 512));

		for(uiSectorIndex = uiFirstBreak; uiSectorIndex <= uiLastBreak; uiSectorIndex++)
		{
			uiIndex = uiSectorIndex - uiStartSector ;

			if(!pCacheValue[ uiIndex ])
			{
				if(_mCache.Full())
				{
					if(!_mCache.ReplaceCache(uiSectorIndex, bDataBuffer + (uiIndex * 512)))
					{
						printf("\n Cache Error: Disabling Disk Cache") ;
						_bEnableDiskCache = false;
            return;
					}

					continue ;
				}

				if(!_mCache.Add(uiSectorIndex, bDataBuffer + (uiIndex * 512)))
				{
					printf("\n Disk Cache failed Insert for SectorID: %d", uiSectorIndex) ;
					printf("\n Disabling Disk Cache!!! %s:%d", __FILE__, __LINE__) ;
          _bEnableDiskCache = false;
          return;
				}
			}
			else
			{
				pCacheValue[ uiIndex ]->Read(bDataBuffer + (uiIndex * 512)) ;
			}
		}
	}
}

void DiskDrive::xRead(byte* bDataBuffer, unsigned uiSector, unsigned uiNoOfSectors)
{
  Read(_fileSystem.GetRealSectorNumber(uiSector), uiNoOfSectors, bDataBuffer);
}

void DiskDrive::RawRead(unsigned uiStartSector, unsigned uiNoOfSectors, byte* bDataBuffer)
{
	switch(DeviceType())
	{
    case DEV_FLOPPY:
      uiTotalFloppyDiskReads++ ;
      Floppy_Read(this, uiStartSector, uiStartSector + uiNoOfSectors, bDataBuffer) ;
      break;

    case DEV_ATA_IDE:
      uiTotalATADiskReads++ ;
      ATADrive_Read((ATAPort*)Device(), uiStartSector, bDataBuffer, uiNoOfSectors) ;
      break;

    case DEV_SCSI_USB_DISK:
      uiTotalUSBDiskReads++ ;
      SCSIHandler_GenericRead((SCSIDevice*)Device(), uiStartSector, uiNoOfSectors, bDataBuffer) ;
      break;

    default:
      throw upan::exception(XLOC, "RawRead failed - invalid device type: %d", DeviceType());
	}
}

void DiskDrive::Write(unsigned uiStartSector, unsigned uiNoOfSectors, byte* bDataBuffer)
{
  upan::mutex_guard g(_driveMutex);
	uiStartSector += LBAStartSector();
	
	if(!_bEnableDiskCache)
	{
    RawWrite(uiStartSector, uiNoOfSectors, bDataBuffer) ;
    return;
	}

	DiskCacheValue* pCacheValue[ uiNoOfSectors ] ;

	unsigned uiEndSector = uiStartSector + uiNoOfSectors ;
	unsigned uiSectorIndex ;
	unsigned uiFirstBreak, uiLastBreak ;
	uiFirstBreak = uiLastBreak = uiEndSector ;

	for(uiSectorIndex = uiStartSector; uiSectorIndex < uiEndSector; uiSectorIndex++)
	{
		unsigned uiIndex = uiSectorIndex - uiStartSector ;
		pCacheValue[ uiIndex ] = _mCache.Find(uiSectorIndex);

		if(!pCacheValue[ uiIndex ])
		{
			if(uiFirstBreak == uiEndSector)
				uiFirstBreak = uiSectorIndex ;

			uiLastBreak = uiSectorIndex ;
		}
	}

	__volatile__ unsigned uiIndex ;
	for(uiSectorIndex = uiStartSector; uiSectorIndex < uiFirstBreak; uiSectorIndex++)
	{
		uiIndex = uiSectorIndex - uiStartSector ;
		pCacheValue[ uiIndex ]->Write(bDataBuffer + (uiIndex * 512)) ;
		_mCache.InsertToDirtyList(DiskCache::SecKeyCacheValue(uiSectorIndex, pCacheValue[ uiIndex ]->GetSectorBuffer())) ;
	}

	for(uiSectorIndex = uiLastBreak + 1; uiSectorIndex < uiEndSector; uiSectorIndex++)
	{
		uiIndex = uiSectorIndex - uiStartSector ;
		pCacheValue[ uiIndex ]->Write(bDataBuffer + (uiIndex * 512)) ;
		_mCache.InsertToDirtyList(DiskCache::SecKeyCacheValue(uiSectorIndex, pCacheValue[ uiIndex ]->GetSectorBuffer())) ;
	}

	if(uiFirstBreak < uiEndSector)
	{
		uiIndex = uiFirstBreak - uiStartSector ;

		//byte bStatus ;
		//RETURN_IF_NOT(bStatus, RawWrite(uiFirstBreak, uiLastBreak - uiFirstBreak + 1, (bDataBuffer + uiIndex * 512)), DiskCache_SUCCESS) ;

		for(uiSectorIndex = uiFirstBreak; uiSectorIndex <= uiLastBreak; uiSectorIndex++)
		{
			uiIndex = uiSectorIndex - uiStartSector ;

			if(!pCacheValue[ uiIndex ])
			{
				if(_mCache.Full())
				{
          RawWrite(uiSectorIndex, 1, (bDataBuffer + uiIndex * 512));

					if(!_mCache.ReplaceCache(uiSectorIndex, bDataBuffer + (uiIndex * 512)))
					{
						printf("\n Cache Error: Disabling Disk Cache") ;
						_bEnableDiskCache = false;
            RawWrite(uiFirstBreak, uiLastBreak - uiFirstBreak + 1, (bDataBuffer + uiIndex * 512)) ;
            return;
					}
					continue ;
				}
			
        DiskCacheValue* pVal = _mCache.Add(uiSectorIndex, bDataBuffer + (uiIndex * 512));
        if(!pVal)
				{
					printf("\n Disk Cache failed Insert. Disabling Disk Cache!!! %s:%d", __FILE__, __LINE__) ;
          _bEnableDiskCache = false;
          return;
				}

				_mCache.InsertToDirtyList(DiskCache::SecKeyCacheValue(uiSectorIndex, pVal->GetSectorBuffer())) ;
			}
			else
			{
				pCacheValue[ uiIndex ]->Write(bDataBuffer + (uiIndex * 512)) ;
				_mCache.InsertToDirtyList(DiskCache::SecKeyCacheValue(uiSectorIndex, pCacheValue[ uiIndex ]->GetSectorBuffer())) ;
			}
		}
	}
}

void DiskDrive::xWrite(byte* bDataBuffer, unsigned uiSector, unsigned uiNoOfSectors)
{
  Write(_fileSystem.GetRealSectorNumber(uiSector), uiNoOfSectors, bDataBuffer);
}

void DiskDrive::RawWrite(unsigned uiStartSector, unsigned uiNoOfSectors, byte* bDataBuffer)
{
	switch(DeviceType())
	{
	case DEV_FLOPPY:
    Floppy_Write(this, uiStartSector, uiStartSector + uiNoOfSectors, bDataBuffer) ;
    break;

	case DEV_ATA_IDE:
    ATADrive_Write((ATAPort*)Device(), uiStartSector, bDataBuffer, uiNoOfSectors) ;
    break;

	case DEV_SCSI_USB_DISK:
    SCSIHandler_GenericWrite((SCSIDevice*)Device(), uiStartSector, uiNoOfSectors, bDataBuffer) ;
    break;

  default:
    throw upan::exception(XLOC, "RawWrite failed - invalid device type: %d", DeviceType());
	}
}

void DiskDrive::Format()
{
  if(DeviceType() == DEV_FLOPPY)
  {
;//		RETURN_IF_NOT(bStatus, Floppy_Format(pDiskDrive->driveNo), Floppy_SUCCESS) ;
  }
  _fileSystem.Format();
  _mounted = false;
  FlushDirtyCacheSectors();
}

byte DiskDrive::FlushDirtyCacheSectors(int iCount)
{
	if(!_bEnableDiskCache)
		return DiskCache_SUCCESS ;

  upan::mutex_guard g(_driveMutex);

  while(iCount != 0)
  {
	  DiskCache::SecKeyCacheValue v;
    if(!_mCache.Get(v))
      break;
    if(!FlushSector(v.m_uiSectorID, v.m_pSectorBuffer))
    {
      printf("\n Flushing Sector %u to Drive %s failed !!", v.m_uiSectorID, DriveName().c_str()) ;
      return DiskCache_FAILURE ;
    }
    --iCount ;
	}
	return DiskCache_SUCCESS ;
}

bool DiskDrive::FlushSector(unsigned uiSectorID, const byte* pBuffer)
{
	if(!pBuffer)
		return false;
  return upan::trycall([&]() { RawWrite(uiSectorID, 1, (byte*)pBuffer); }).isGood();
}

void DiskDrive::StartReleaseCacheTask()
{
  StopReleaseCacheTask(false);

	const upan::string dcfName = upan::string("dcf-") + DriveName();
	upan::vector<uintptr_t> params;
	params.push_back((uintptr_t)this);
  ProcessManager::Instance().CreateKernelProcess(dcfName, (uintptr_t) &DiskCache_TaskFlushCache,
                                                           ProcessManager::Instance().GetCurProcId(), false, params);

  const upan::string dcrName = upan::string("dcr-") + DriveName();
  ProcessManager::Instance().CreateKernelProcess(dcrName, (uintptr_t) &DiskCache_TaskReleaseCache,
                                                 ProcessManager::Instance().GetCurProcId(), false, params);
}

void DiskDrive::ReleaseCache()
{
  if(Mounted())
    _mCache.LFUCacheCleanUp();
}

RawDiskDrive::RawDiskDrive(const upan::string& name, RAW_DISK_TYPES type, void* device)
  : _name(name), _type(type), _device(device)
{
  switch(_type)
  {
    case ATA_HARD_DISK:
      _sectorSize = 512;
      _sizeInSectors = ATADeviceController_GetDeviceSectorLimit((ATAPort*)_device);
      break ;

    case USB_SCSI_DISK:
      _sectorSize = ((SCSIDevice*)_device)->uiSectorSize;
      _sizeInSectors = ((SCSIDevice*)_device)->uiSectors;
      break ;

    case FLOPPY_DISK:
      _sectorSize = 512;
      _sizeInSectors = 2880;
      break;

    default:
      throw upan::exception(XLOC, "\n Unsupported Raw Disk Type: %d", type);
  }
}

DiskDriveManager::DiskDriveManager() : _idSequence(0)
{
}

void DiskDriveManager::Create(const upan::string& driveName, 
  DEVICE_TYPE deviceType, DRIVE_NO driveNumber,
  unsigned uiLBAStartSector, unsigned uiSizeInSectors,
  unsigned uiSectorsPerTrack, unsigned uiTracksPerHead, unsigned uiNoOfHeads,
  void* device, RawDiskDrive* rawDisk,
  unsigned uiMaxSectorsInFreePoolCache)
{
  upan::mutex_guard g(_driveListMutex);
  DiskDrive* pDiskDrive = new DiskDrive(_idSequence++, driveName,
    deviceType, driveNumber,
    uiLBAStartSector, uiSizeInSectors,
    uiSectorsPerTrack, uiTracksPerHead, uiNoOfHeads,
    device, rawDisk,
    uiMaxSectorsInFreePoolCache);
  _driveList.push_back(pDiskDrive);
}

RawDiskDrive* DiskDriveManager::CreateRawDisk(const upan::string& name, RAW_DISK_TYPES iType, void* pDevice)
{
  for(auto d : _rawDiskList)
  {
		if(d->Name() == name)
      throw upan::exception(XLOC, "\n Raw Drive '%s' already in the list", name.c_str());
	}
  RawDiskDrive* pDisk = new RawDiskDrive(name, iType, pDevice);
  _rawDiskList.push_back(pDisk);
	return pDisk;
}

byte DiskDriveManager::RemoveRawDiskEntry(const upan::string& name)
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

RawDiskDrive* DiskDriveManager::GetRawDiskByName(const upan::string& name)
{
  auto it = upan::find_if(_rawDiskList.begin(), _rawDiskList.end(), [&name](const RawDiskDrive* d) { return d->Name() == name; });
  if(it == _rawDiskList.end())
    return nullptr;
  return *it;
}

void DiskDriveManager::RemoveEntryByCondition(const DriveRemoveClause& removeClause)
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

upan::result<DiskDrive*> DiskDriveManager::GetByDriveName(const upan::string& driveName, bool bCheckMount)
{
  upan::mutex_guard g(_driveListMutex);
  auto it = upan::find_if(_driveList.begin(), _driveList.end(), [&driveName, bCheckMount](const DiskDrive* d)
    {
      if(d->DriveName() == driveName)
        return d->Mounted() || !bCheckMount;
      return false;
    });
  if(it == _driveList.end())
    return upan::result<DiskDrive*>::bad("failed to find drive %s (mounted: %d)", driveName.c_str(), bCheckMount);
  return upan::good(*it);
}

upan::result<DiskDrive*> DiskDriveManager::GetByID(int iID, bool bCheckMount)
{	
  upan::mutex_guard g(_driveListMutex);
	if(iID == ROOT_DRIVE)
		iID = ROOT_DRIVE_ID;
	else if(iID == CURRENT_DRIVE)
		iID = ProcessManager::Instance().GetCurrentPAS().driveID();
  auto it = upan::find_if(_driveList.begin(), _driveList.end(), [iID, bCheckMount](const DiskDrive* d)
    {
      if(d->Id() == iID)
        return d->Mounted() || !bCheckMount;
      return false;
    });
  if(it == _driveList.end())
    return upan::result<DiskDrive*>::bad("failed to find drive id %d (mounted: %d)", iID, bCheckMount);
  return upan::good(*it);
}

void DiskDriveManager::DisplayList()
{	
  for(auto d : _driveList)
  {
		DDWORD i = (DDWORD)d->SizeInSectors() * (DDWORD)512;
    printf("\n %s (%u - %llu - %x)", d->DriveName().c_str(), d->SizeInSectors(), i, i);
	}
}

byte DiskDriveManager::Change(const upan::string& szDriveName)
{
  DiskDrive* pDiskDrive = GetByDriveName(szDriveName, false).goodValueOrElse(nullptr);
	if(pDiskDrive == NULL)
		return DeviceDrive_ERR_INVALID_DRIVE_NAME ;

	auto& pas = ProcessManager::Instance().GetCurrentPAS();
	pas.setDriveID(pDiskDrive->Id());
  memcpy(&pas.processPWD(), &(pDiskDrive->_fileSystem.FSpwd), sizeof(FileSystem::PresentWorkingDirectory));
  pas.setEnv("PWD", (const char*)pDiskDrive->_fileSystem.FSpwd.DirEntry.Name()) ;

	return DeviceDrive_SUCCESS ;
}

byte DiskDriveManager::GetList(DriveStat** pDriveList, int* iListSize)
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
		*pDriveList = (DriveStat*)DMM_AllocateForKernel(sizeof(DriveStat) * _driveList.size());
		pAddress = *pDriveList ;
	}
	else
	{
		*pDriveList = (DriveStat*)DMM_Allocate(pAddrSpace, sizeof(DriveStat) * _driveList.size());
		pAddress = (DriveStat*)PROCESS_REAL_ALLOCATED_ADDRESS(*pDriveList);
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
      pAddress[i].ulTotalSize = d->_fileSystem.TotalSize();
      pAddress[i].ulUsedSize = d->_fileSystem.UsedSize();
    }
		
		++i;
	}

	return DeviceDrive_SUCCESS ;
}

void DiskDriveManager::MountDrive(const upan::string& szDriveName)
{
  GetByDriveName(szDriveName, false).goodValueOrThrow(XLOC)->Mount();
}

void DiskDriveManager::UnMountDrive(const upan::string& szDriveName)
{
  DiskDrive* pDiskDrive = GetByDriveName(szDriveName, false).goodValueOrThrow(XLOC);

	bool bKernel = IS_KERNEL() ? true : IS_KERNEL_PROCESS(ProcessManager::GetCurrentProcessID()) ;
	if(!bKernel)
	{
		if(pDiskDrive->Id() == ProcessManager::Instance().GetCurrentPAS().driveID())
      throw upan::exception(XLOC, "can't unmount current drive: %s", szDriveName.c_str());
	}

  pDiskDrive->UnMount();
}

void DiskDriveManager::FormatDrive(const upan::string& szDriveName)
{
  DiskDrive* pDiskDrive = GetByDriveName(szDriveName, false).goodValueOrThrow(XLOC);

  pDiskDrive->Format();

	switch((int)pDiskDrive->DeviceType())
	{
		case DEV_ATA_IDE:
		case DEV_SCSI_USB_DISK:
			pDiskDrive->RawDisk()->UpdateSystemIndicator(pDiskDrive->LBAStartSector(), 0x93);
			break ;
	}
}

void DiskDriveManager::GetCurrentDriveStat(DriveStat* pDriveStat)
{
  DiskDrive* pDiskDrive = GetByID(ProcessManager::Instance().GetCurrentPAS().driveID(), true).goodValueOrThrow(XLOC);

  strncpy(pDriveStat->driveName, pDiskDrive->DriveName().c_str(), 32);
  pDriveStat->bMounted = pDiskDrive->Mounted();
  pDriveStat->uiSizeInSectors = pDiskDrive->SizeInSectors();
  pDriveStat->ulTotalSize = 0;
  pDriveStat->ulUsedSize = 0;
}

RESOURCE_KEYS DiskDriveManager::GetResourceType(DEVICE_TYPE deviceType)
{
	switch(deviceType)
	{
		case DEV_FLOPPY: return RESOURCE_FDD;
		case DEV_ATA_IDE: return RESOURCE_HDD;
		case DEV_SCSI_USB_DISK: return RESOURCE_USD;
    default: return RESOURCE_GENERIC_DISK;
	}
}

RESOURCE_KEYS DiskDriveManager::GetResourceType(RAW_DISK_TYPES diskType)
{
	switch(diskType)
	{
		case ATA_HARD_DISK: return RESOURCE_HDD;
		case USB_SCSI_DISK: return RESOURCE_USD;
	//TODO: FLOPPY falls under this category which should be changed to particular disk type
    default: return RESOURCE_GENERIC_DISK;
	}

}

void RawDiskDrive::Read(unsigned uiStartSector, unsigned uiNoOfSectors, byte* pDataBuffer)
{
  upan::mutex_guard g(_diskMutex);
	switch(_type)
	{
		case ATA_HARD_DISK:
      ATADrive_Read((ATAPort*)Device(), uiStartSector, pDataBuffer, uiNoOfSectors);
       break;
		case USB_SCSI_DISK:
      SCSIHandler_GenericRead((SCSIDevice*)Device(), uiStartSector, uiNoOfSectors, pDataBuffer);
      break;
    default:
      throw upan::exception(XLOC, "error reading - unknown device type: %d", _type);
	}
}

void RawDiskDrive::Write(unsigned uiStartSector, unsigned uiNoOfSectors, byte* pDataBuffer)
{
	upan::mutex_guard g(_diskMutex);
	switch(_type)
	{
		case ATA_HARD_DISK:
      ATADrive_Write((ATAPort*)Device(), uiStartSector, pDataBuffer, uiNoOfSectors);
      break;
		case USB_SCSI_DISK:
      SCSIHandler_GenericWrite((SCSIDevice*)Device(), uiStartSector, uiNoOfSectors, pDataBuffer);
      break;
    default:
      throw upan::exception(XLOC, "error reading - unknown device type: %d", _type);
	}
}

void RawDiskDrive::UpdateSystemIndicator(unsigned uiLBAStartSector, unsigned uiSystemIndicator)
{
	byte bBootSectorBuffer[512] ;

	Read(0, 1, bBootSectorBuffer);

	MBRPartitionInfo* pPartitionTable = ((MBRPartitionInfo*)(bBootSectorBuffer + 0x1BE)) ;
	
  const unsigned NO_OF_PARTITIONS = 4;
	for(unsigned i = 0; i < NO_OF_PARTITIONS; i++)
	{
		if(pPartitionTable[i].LBAStartSector == uiLBAStartSector)
		{
			pPartitionTable[i].SystemIndicator = uiSystemIndicator ;
			break ;
		}
	}

	Write(0, 1, bBootSectorBuffer);
}
