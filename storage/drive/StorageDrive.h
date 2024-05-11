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

class RawStorageDrive;

class StorageDrive {
  private:
    StorageDrive(int id,
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
    RawStorageDrive* RawDisk() const { return _rawDisk; }
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
    RawStorageDrive* _rawDisk;
    unsigned      _uiMaxSectorsInFreePoolCache;

    FS_TYPE       _fsType;
    bool          _mounted;
    upan::mutex   _driveMutex;
    DiskCache		  _mCache;
		bool          _bStopReleaseCacheTask;

    typedef upan::map<upan::string, upan::rwlock*> FileLocks;
    FileLocks _fileLocks;

    friend class StorageDriveManager;
public:
  // FileSystem Mount Info
  FileSystem	_fileSystem;
};