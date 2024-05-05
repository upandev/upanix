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

#include "util/Global.h"
#include "mutex.h"
#include "storage/filesystem/FileSystem.h"
#include "kernel/ResourceMutex.h"
#include "DiskCache.h"
#include "ustring.h"
#include "drive.h"
#include "result.h"
#include "rwlock.h"
#include "map.h"

class RawDiskDrive {
public:
  enum RawDiskDriveTypes {
    ATA_HARD_DISK = 100,
    USB_SCSI_DISK,
    FLOPPY_DISK,
  };

private:
  RawDiskDrive(const upan::string& name, RawDiskDriveTypes type, void* device);

public:
  void Read(unsigned uiStartSector, unsigned uiNoOfSectors, byte* pDataBuffer);
  void Write(unsigned uiStartSector, unsigned uiNoOfSectors, byte* pDataBuffer);
  void UpdateSystemIndicator(unsigned uiLBAStartSector, unsigned uiSystemIndicator);

  const upan::string& Name() const { return _name; }
  RawDiskDriveTypes Type() const { return _type; }
  unsigned SectorSize() const { return _sectorSize; }
  unsigned SizeInSectors() const { return _sizeInSectors; }
  void* Device() { return _device; }
private:
  upan::string _name;
  RawDiskDriveTypes _type;
  unsigned _sectorSize;
  unsigned _sizeInSectors;
  void* _device;
  upan::mutex _diskMutex;

  friend class DiskDriveManager;
};
