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

# include <RawStorageDrive.h>
# include <drivers/ide/ATADrive.h>
# include <drivers/ide/ATADeviceController.h>
# include <PartitionManager.h>
# include <drivers/bus/SCSIHandler.h>

RawStorageDrive::RawStorageDrive(const upan::string& name, RawStorageDrive::Types type, void* device)
  : _name(name), _type(type), _device(device) {
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

void RawStorageDrive::Read(unsigned uiStartSector, unsigned uiNoOfSectors, byte* pDataBuffer)
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

void RawStorageDrive::Write(unsigned uiStartSector, unsigned uiNoOfSectors, byte* pDataBuffer)
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

void RawStorageDrive::UpdateSystemIndicator(unsigned uiLBAStartSector, unsigned uiSystemIndicator)
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
