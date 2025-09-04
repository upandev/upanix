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
#include <DiskCache.h>
#include <ustring.h>
#include <drive.h>
#include <result.h>
#include <rwlock.h>
#include <map.h>
#include <RawStorageDrive.h>

class DriveRemoveClause {
public:
  virtual bool operator()(const StorageDrive* pDiskDrive) const = 0 ;
};

class StorageDriveManager {
private:
  StorageDriveManager();

public:
  static StorageDriveManager& Instance()
  {
    static StorageDriveManager instance;
    return instance;
  }

  void Create(const upan::string& driveName,
              DEVICE_TYPE deviceType, DRIVE_NO driveNumber,
              unsigned uiLBAStartSector, unsigned uiSizeInSectors,
              unsigned uiSectorsPerTrack, unsigned uiTracksPerHead, unsigned uiNoOfHeads,
              void* device, RawStorageDrive* rawDisk,
              unsigned uiMaxSectorsInFreePoolCache);
  void RemoveEntryByCondition(const DriveRemoveClause& removeClause);
  upan::result<StorageDrive&> GetByDriveName(const upan::string& driveName, bool bCheckMount);
  upan::result<StorageDrive&> GetByID(int iID, bool bCheckMount);
  void DisplayList();
  byte Change(const upan::string& szDriveName, char** retPwd);
  byte GetList(DriveStat** pDriveList, int* iListSize);
  void MountDrive(const upan::string& szDriveName);
  void UnMountDrive(const upan::string& szDriveName);
  void FormatDrive(const upan::string& szDriveName);
  void GetCurrentDriveStat(DriveStat* pDriveStat);

  RawStorageDrive* CreateRawDisk(const upan::string& name, RawStorageDrive::Types iType, void* pDevice);
  byte RemoveRawDiskEntry(const upan::string& name);
  RawStorageDrive* GetRawDiskByName(const upan::string& name);

  upan::option<StorageDrive&> GetRootDrive() { return _rootDrive; }
  void SetRootDrive(const upan::string& driveName);

  const upan::list<RawStorageDrive*>& RawDiskDriveList() const { return _rawDiskList; }
  void Close();

private:
  upan::mutex _driveListMutex;
  upan::list<StorageDrive*> _driveList;
  upan::list<RawStorageDrive*> _rawDiskList;
  int _idSequence;
  upan::option<StorageDrive&> _rootDrive;
};
