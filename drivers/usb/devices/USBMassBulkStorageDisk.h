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
#ifndef _USB_MASS_BULK_STORAGE_DISK_H_
#define _USB_MASS_BULK_STORAGE_DISK_H_

#include <Global.h>
#include <SCSIHandler.h>
#include <USBStructures.h>

#define USBMassBulkStorageDisk_SUCCESS 0
#define USBMassBulkStorageDisk_FAILURE 1
#define USBMassBulkStorageDisk_ERROR 2
#define USBMassBulkStorageDisk_SHORT_DATA 3

#define USB_SC_RBC		0x01
#define USB_SC_8020		0x02
#define USB_SC_QIC		0x03
#define USB_SC_UFI		0x04
#define USB_SC_8070		0x05
#define USB_SC_SCSI		0x06
#define USB_SC_ISD200	0x07

#define USB_PR_CBI	0x00
#define USB_PR_CB	0x01
#define USB_PR_BULK	0x50
#define USB_PR_DPCM_USB	0xF0
#define USB_PR_JUMPSHOT	0xF3

#define USBDISK_BULK_CBW_SIGNATURE 0x43425355
#define USBDISK_BULK_CSW_SIGNATURE 0x53425355

#define USB_CLASS_MASS_STORAGE 8

// Host To Device
#define USBDISK_CBW_DATA_OUT 0x00 
// Device To Host
#define USBDISK_CBW_DATA_IN 0x80

#define USB_CBW_SIZE 31

#define US_BULK_STAT_OK		0
#define US_BULK_STAT_FAIL	1
#define US_BULK_STAT_PHASE	2

#define US_BULK_MAX_TRANSFER_SIZE 32768 //32 KB

class USBDevice;
class RawStorageDrive;

typedef enum
{
	CBW_CMD_SUCCESS,
	CBW_CMD_FAILURE,
	CBW_CMD_PHERROR
} USB_CSW_Status ;

typedef struct
{
	unsigned uiSignature ;
	unsigned uiTag ;
	unsigned uiDataTransferLen ;

	byte bFlags ;
	byte bLUN ; // MSB 4 bits reserved (0)
	byte bCBLen ; // MSB 3 bits reserved (0)

	byte bCommandBlock[ 16 ] ;
} PACKED USBulkCBW ;

typedef struct
{
	unsigned uiSignature ;
	unsigned uiTag ;
	unsigned uiDataResidue ;
	byte bStatus ;
} PACKED USBulkCSW ;

class USBDiskDriver final : public USBDriver
{
public:
  USBDiskDriver(const upan::string& name) : USBDriver(name) {}
  static void Register();
  static int NextDeviceId() { return ++_deviceId; }

private:
  bool AddDevice(USBDevice*);
  void RemoveDevice(USBDevice*);
  int MatchingInterfaceIndex(USBDevice*);

private:
  static int _deviceId;
};

class USBMassBulkStorageDisk : public SCSIHost
{
  public:
    USBMassBulkStorageDisk(USBulkDisk* disk) : _disk(disk) {}
    upan::string GetName();
    bool QueueCommand(SCSICommand* pCommand);
    byte DoReset();
    void AddDeviceDrive(RawStorageDrive* pDisk);
  private:
    byte SendCommand(SCSICommand* pCommand);
    byte ReadData(void* pDataBuf, unsigned uiLen);
    byte WriteData(void* pDataBuf, unsigned uiLen);
    USBulkDisk* _disk;
};

#endif
