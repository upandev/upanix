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
#ifndef _DEVICE_DRIVE_H_
#define _DEVICE_DRIVE_H_

#include <Global.h>
#include <mutex.h>
#include <FileSystem.h>
#include <ResourceMutex.h>
#include <DiskCache.h>
#include <ustring.h>
#include <drive.h>
#include <result.h>
#include <rwlock.h>
#include <map.h>

#define DeviceDrive_SUCCESS						0
#define DeviceDrive_ERR_INVALID_DRIVE_NAME		2
#define	DeviceDrive_ERR_NOTFOUND				9
#define DeviceDrive_FAILURE						10

class RawDiskDrive;

class DiskDrive
{
  private:
    DiskDrive(int id,
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
      unsigned uiMaxSectorsInFreePoolCache);

  public:
    void Mount();
    void UnMount();
    void Format();
    void Read(unsigned uiStartSector, unsigned uiNoOfSectors, byte* bDataBuffer);
    void xRead(byte* bDataBuffer, unsigned uiSector, unsigned uiNoOfSectors);
    void Write(unsigned uiStartSector, unsigned uiNoOfSectors, byte* bDataBuffer);
    void xWrite(byte* bDataBuffer, unsigned uiSector, unsigned uiNoOfSectors);
    byte FlushDirtyCacheSectors(int iCount = -1);
    void ReleaseCache();

    const upan::string& DriveName() const { return _driveName; }
    DEVICE_TYPE DeviceType() const { return _deviceType; }
    FS_TYPE FSType() const { return _fsType; }
    DRIVE_NO DriveNumber() const { return _driveNumber; }
    unsigned LBAStartSector() const { return _uiLBAStartSector; }
    unsigned SizeInSectors() const { return _uiSizeInSectors; }
    unsigned SectorsPerTrack() const { return _uiSectorsPerTrack; }
    unsigned TracksPerHead() const { return _uiTracksPerHead; }
    unsigned NoOfHeads() const { return _uiNoOfHeads; }
    unsigned MaxSectorsInFreePoolCache() const { return _uiMaxSectorsInFreePoolCache; }
    bool StopReleaseCacheTask() const { return _bStopReleaseCacheTask; }

    int Id() const { return _id; }
    bool Mounted() const { return _mounted; }
    RawDiskDrive* RawDisk() const { return _rawDisk; }
    void* Device() const { return _device; }
    DiskCache& Cache() { return _mCache; }

    void StopReleaseCacheTask(bool value) { _bStopReleaseCacheTask = value; }
    void FSType(FS_TYPE t) { _fsType = t; }

    upan::rwlock& GetFileLock(const upan::string& nodeId);

  private:
    void RawRead(unsigned uiStartSector, unsigned uiNoOfSectors, byte* bDataBuffer);
    void RawWrite(unsigned uiStartSector, unsigned uiNoOfSectors, byte* bDataBuffer);
    bool FlushSector(unsigned uiSectorID, const byte* pBuffer);
    void StartReleaseCacheTask();
    void ReadRootDirectory();

    int          _id;
    upan::string _driveName;
    DEVICE_TYPE  _deviceType;
    DRIVE_NO     _driveNumber;
    unsigned     _uiLBAStartSector;
    unsigned     _uiSizeInSectors;
    unsigned     _uiSectorsPerTrack;
    unsigned     _uiTracksPerHead;
    unsigned     _uiNoOfHeads;
    bool		     _bEnableDiskCache;
	  void*			    _device;
    RawDiskDrive* _rawDisk;
    unsigned      _uiMaxSectorsInFreePoolCache;

    FS_TYPE       _fsType;
    bool          _mounted;
    upan::mutex   _driveMutex;
    DiskCache		  _mCache;
		bool          _bStopReleaseCacheTask;

    typedef upan::map<upan::string, upan::rwlock*> FileLocks;
    FileLocks _fileLocks;

    friend class DiskDriveManager;
public:
  // FileSystem Mount Info
  FileSystem	_fileSystem ;
};

typedef enum
{
	ATA_HARD_DISK = 100,
	USB_SCSI_DISK,
	FLOPPY_DISK,
} RAW_DISK_TYPES ;

class PartitionTable;
class RawDiskDrive
{
  private:
    RawDiskDrive(const upan::string& name, RAW_DISK_TYPES type, void* device);
  public:
    void Read(unsigned uiStartSector, unsigned uiNoOfSectors, byte* pDataBuffer);
    void Write(unsigned uiStartSector, unsigned uiNoOfSectors, byte* pDataBuffer);
    void UpdateSystemIndicator(unsigned uiLBAStartSector, unsigned uiSystemIndicator);

    const upan::string& Name() const { return _name; }
    RAW_DISK_TYPES Type() const { return _type; }
    unsigned SectorSize() const { return _sectorSize; }
    unsigned SizeInSectors() const { return _sizeInSectors; }
    void* Device() { return _device; }
  private:
    upan::string _name;
    RAW_DISK_TYPES _type;
    unsigned _sectorSize;
    unsigned _sizeInSectors;
    void* _device;
    upan::mutex _diskMutex;

    friend class DiskDriveManager;
};

class DriveRemoveClause
{
	public:
		virtual bool operator()(const DiskDrive* pDiskDrive) const = 0 ;
} ;

class DiskDriveManager
{
  private:
    DiskDriveManager();

  public:
    static DiskDriveManager& Instance()
    {
      static DiskDriveManager instance;
      return instance;
    }

    void Create(const upan::string& driveName, 
      DEVICE_TYPE deviceType, DRIVE_NO driveNumber,
      unsigned uiLBAStartSector, unsigned uiSizeInSectors,
      unsigned uiSectorsPerTrack, unsigned uiTracksPerHead, unsigned uiNoOfHeads,
      void* device, RawDiskDrive* rawDisk,
      unsigned uiMaxSectorsInFreePoolCache);
    void RemoveEntryByCondition(const DriveRemoveClause& removeClause);
    upan::result<DiskDrive*> GetByDriveName(const upan::string& szDriveName, bool bCheckMount);
    upan::result<DiskDrive*> GetByID(int iID, bool bCheckMount);
    void DisplayList();
    byte Change(const upan::string& szDriveName);
    byte GetList(DriveStat** pDriveList, int* iListSize);
    void MountDrive(const upan::string& szDriveName);
    void UnMountDrive(const upan::string& szDriveName);
    void FormatDrive(const upan::string& szDriveName);
    void GetCurrentDriveStat(DriveStat* pDriveStat);

    RawDiskDrive* CreateRawDisk(const upan::string& name, RAW_DISK_TYPES iType, void* pDevice);
    byte RemoveRawDiskEntry(const upan::string& name);
    RawDiskDrive* GetRawDiskByName(const upan::string& name);

    RESOURCE_KEYS GetResourceType(DEVICE_TYPE deviceType);
    RESOURCE_KEYS GetResourceType(RAW_DISK_TYPES diskType);

    const upan::list<DiskDrive*>& DiskDriveList() const { return _driveList; }
    const upan::list<RawDiskDrive*>& RawDiskDriveList() const { return _rawDiskList; }
  private:
    upan::mutex _driveListMutex;
    upan::list<DiskDrive*> _driveList;
    upan::list<RawDiskDrive*> _rawDiskList;
    int _idSequence;
};

#endif
