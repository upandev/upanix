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
# include <StorageDrive.h>
# include <Floppy.h>
# include <ATADrive.h>
# include <ATADeviceController.h>
# include <PartitionManager.h>
# include <DMM.h>
# include <StringUtil.h>
# include <ProcessManager.h>
# include <SCSIHandler.h>
# include <stdio.h>
# include <MountManager.h>
# include <DiskCache.h>
# include <KernelUtil.h>
# include <FileSystem.h>
# include <try.h>
# include <drive.h>
# include <logger.h>
# include <KernelRootProcess.h>

static unsigned uiTotalFloppyDiskReads = 0;
static unsigned uiTotalATADiskReads = 0;
static unsigned uiTotalUSBDiskReads = 0;

void DiskCache_ShowTotalDiskReads()
{
  printf("\n Total Floppy Disk Reads: %u", uiTotalFloppyDiskReads) ;
  printf("\n Total ATA Disk Reads: %u", uiTotalATADiskReads) ;
  printf("\n Total USB Disk Reads: %u", uiTotalUSBDiskReads) ;
}

static void DiskCache_TaskFlushCache(StorageDrive* pDiskDrive, unsigned uiParam2)
{
	do
	{
    pDiskDrive->FlushDirtyCacheSectors(10) ;
		ProcessManager::Instance().Sleep(200) ;
	} while(!pDiskDrive->StopReleaseCacheTask());

	ProcessManager_Exit() ;
}

static void DiskCache_TaskReleaseCache(StorageDrive* pDiskDrive, unsigned uiParam2)
{
	do
	{
    pDiskDrive->ReleaseCache();
		ProcessManager::Instance().Sleep(50) ;
	} while(!pDiskDrive->StopReleaseCacheTask());

	ProcessManager_Exit() ;
}

StorageDrive::StorageDrive(int id,
                           const upan::string& driveName,
                           DEVICE_TYPE deviceType,
                           DRIVE_NO driveNumber,
                           unsigned uiLBAStartSector,
                           unsigned uiSizeInSectors,
                           unsigned uiSectorsPerTrack,
                           unsigned uiTracksPerHead,
                           unsigned uiNoOfHeads,
                           void* device,
                           RawStorageDrive* rawDisk,
                           uint32_t uiMaxSectorsInFreePoolCache) : _id(id),
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
    _fsType(FS_UNKNOWN),
    _mounted(false),
    _fileSystem(*this, uiMaxSectorsInFreePoolCache)
{
  StartReleaseCacheTask();
}

void StorageDrive::Format() {
  if(DeviceType() == DEV_FLOPPY)
  {
    ;//		RETURN_IF_NOT(bStatus, Floppy_Format(pDiskDrive->driveNo), Floppy_SUCCESS) ;
  }
  fileSystem().format();
  _mounted = false;
}

void StorageDrive::Mount() {
	if(Mounted()) {
    throw upan::exception(XLOC, "Drive %s is already mounted", _driveName.c_str());
  }
  fileSystem().mount();
  _mounted = true;
  KernelRootProcess::Instance().openSysLoggerFile(_driveName);
}

void StorageDrive::UnMount() {
	if(!Mounted()) {
    throw upan::exception(XLOC, "drive %s is not mounted", _driveName.c_str());
  }
  KernelRootProcess::Instance().closeSysLoggerFile();
  fileSystem().unmount();
  _mounted = false;
}

void StorageDrive::Read(unsigned uiStartSector, unsigned uiNoOfSectors, byte* bDataBuffer)
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

void StorageDrive::xRead(byte* bDataBuffer, unsigned uiSector, unsigned uiNoOfSectors)
{
  Read(fileSystem().getRealSectorNumber(uiSector), uiNoOfSectors, bDataBuffer);
}

void StorageDrive::RawRead(unsigned uiStartSector, unsigned uiNoOfSectors, byte* bDataBuffer)
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

void StorageDrive::Write(unsigned uiStartSector, unsigned uiNoOfSectors, byte* bDataBuffer)
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

void StorageDrive::xWrite(byte* bDataBuffer, unsigned uiSector, unsigned uiNoOfSectors)
{
  Write(fileSystem().getRealSectorNumber(uiSector), uiNoOfSectors, bDataBuffer);
}

void StorageDrive::RawWrite(unsigned uiStartSector, unsigned uiNoOfSectors, byte* bDataBuffer)
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

bool StorageDrive::FlushDirtyCacheSectors(int count) {
	if(!_bEnableDiskCache) {
    return true;
  }

	upan::mutex_guard g(_driveMutex);

  while(count != 0) {
	  DiskCache::SecKeyCacheValue v;
    if(!_mCache.Get(v))
      break;
    if(!FlushSector(v.m_uiSectorID, v.m_pSectorBuffer)) {
      printf("\n Flushing Sector %u to Drive %s failed !!", v.m_uiSectorID, DriveName().c_str()) ;
      return false;
    }
    --count ;
	}
	return true;
}

bool StorageDrive::FlushSector(unsigned uiSectorID, const byte* pBuffer) {
	if(!pBuffer)
		return false;
	return upan::trycall([&]() { RawWrite(uiSectorID, 1, (byte*)pBuffer); }).isGood();
}

void StorageDrive::StartReleaseCacheTask()
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

void StorageDrive::ReleaseCache()
{
  if(Mounted())
    _mCache.LFUCacheCleanUp();
}