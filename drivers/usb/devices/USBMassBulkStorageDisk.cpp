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
#include <USBMassBulkStorageDisk.h>
#include <ProcessManager.h>
#include <DMM.h>
#include <stdio.h>
#include <PortCom.h>
#include <ustring.h>
#include <uniq_ptr.h>
#include <PartitionManager.h>
#include <FileSystem.h>
#include <StringUtil.h>
#include <USBController.h>
#include <EHCIStructures.h>
#include <USBDevice.h>
#include <RawStorageDrive.h>
#include <StorageDriveManager.h>

static int USDDeviceId ;
int USBDiskDriver::_deviceId = 0;

void USBMassBulkStorageDisk::AddDeviceDrive(RawStorageDrive* pDisk)
{
	char driveCh = 'a' ;
	char driveName[5] = { 'u', 's', 'd', '\0', '\0' } ;

	SCSIDevice* pDevice = (SCSIDevice*)pDisk->Device();
	PartitionTable partitionTable(*pDisk);
	if(partitionTable.IsEmpty())
	{
    printf("\n Disk Not Partitioned");
		return ;
	}
	
	/*** Calculate MountSpacePerPartition, FreePoolSize, TableCacheSize *****/
	const unsigned uiSectorsInFreePool = 4096 ;
	
	/*** DONE - Calculating mount stuff ***/
	for(const auto& pe : partitionTable.GetPartitions())
	{
		driveName[3] = driveCh + USDDeviceId++;
    StorageDriveManager::Instance().Create(driveName, DEV_SCSI_USB_DISK, USD_DRIVE0,
                                           pe.LBAStartSector(), pe.LBASize(),
                                           1, 1, 1, pDevice, pDisk, uiSectorsInFreePool);
	}
}

byte USBMassBulkStorageDisk::DoReset()
{
	printf("\n Doing Bulk Reset on USB Bulk Mass Storage Device\n") ;

	USBDevice* pUSBDevice = _disk->_usbDevice ;

	if(pUSBDevice == NULL)
		return USBMassBulkStorageDisk_FAILURE ;

	printf("\n Doing Command Reset");

	if(!pUSBDevice->CommandReset())
	{
		printf("\n Failed to Reset device. Bulk Mass Storage Reset Failed !\n") ;
		return USBMassBulkStorageDisk_FAILURE ;
	}

	printf("\n Command Reset Complete");

	// Long Wait For Reset
	ProcessManager::Instance().Sleep( 200);

	printf("\n Doing IN EndPoint Clear Halt");

	if(!pUSBDevice->ClearHaltEndPoint(_disk, true))
	{
		printf("\n Failed to Clear Halt EndPointIn. Bulk Mass Storage Reset Failed !\n") ;
		return USBMassBulkStorageDisk_FAILURE ;
	}

	printf("\n IN EndPoint Clear Halt Complete");

	printf("\n Doing OUT EndPoint Clear Halt");

	if(!pUSBDevice->ClearHaltEndPoint(_disk, false))
	{
		printf("\n Failed to Clear Halt EndPointOut. Bulk Mass Storage Reset Failed !\n") ;
		return USBMassBulkStorageDisk_FAILURE ;
	}

	printf("\n OUT EndPoint Clear Halt Complete");

	printf("\n Bulk soft Reset Complete\n") ;

	return USBMassBulkStorageDisk_SUCCESS ;
}

byte USBMassBulkStorageDisk::ReadData(void* pDataBuf, unsigned uiLen)
{
	unsigned uiTranLen ;
	while(uiLen > 0)
	{
		uiTranLen = (uiLen > US_BULK_MAX_TRANSFER_SIZE) ? US_BULK_MAX_TRANSFER_SIZE : uiLen ;
		uiLen -= uiTranLen ;

		if(!_disk->_usbDevice->BulkRead(_disk, pDataBuf, uiTranLen))
		{
			printf("\n Bulk Read Data Failed") ;
			return USBMassBulkStorageDisk_FAILURE ;
		}

		pDataBuf = (byte*)pDataBuf + uiTranLen ;
	}

	return USBMassBulkStorageDisk_SUCCESS ;
}

byte USBMassBulkStorageDisk::WriteData(void* pDataBuf, unsigned uiLen)
{
	unsigned uiTranLen ;
	while(uiLen > 0)
	{
		uiTranLen = (uiLen > US_BULK_MAX_TRANSFER_SIZE) ? US_BULK_MAX_TRANSFER_SIZE : uiLen ;
		uiLen -= uiTranLen ;

		if(!_disk->_usbDevice->BulkWrite(_disk, pDataBuf, uiTranLen))
		{
			printf("\n Bulk Write Data Failed") ;
			return USBMassBulkStorageDisk_FAILURE ;
		}

		pDataBuf = (byte*)pDataBuf + uiTranLen ;
	}

	return USBMassBulkStorageDisk_SUCCESS ;
}

byte USBMassBulkStorageDisk::SendCommand(SCSICommand* pCommand)
{
	USBulkCBW oCBW ;
	USBulkCSW oCSW ;

	auto usbDevice = _disk->_usbDevice;

	if(usbDevice == NULL)
		return USBMassBulkStorageDisk_FAILURE ;

	// Setup Command Block Wrapper (CBW)
	oCBW.uiSignature = USBDISK_BULK_CBW_SIGNATURE ;
	oCBW.uiTag = 0 ;
	oCBW.uiDataTransferLen = SCSIHandler_GetTransferLen(pCommand) ;
	oCBW.bFlags = (pCommand->iDirection == SCSI_DATA_READ) ? USBDISK_CBW_DATA_IN : USBDISK_CBW_DATA_OUT ;
	oCBW.bLUN = (pCommand->bCommand[ 1 ] >> 5 ) & 0xF ;

	/*TODO:
	if ( psDisk -> nFlags & US_FL_SCM_MULT_TARG ) {
		sBcb.nLun |= psSrb -> nDevice << 4;
	}*/

	oCBW.bCBLen = pCommand->iCmdLen & 0x1F ;

	/* copy the command payload */
	memset(oCBW.bCommandBlock, 0, sizeof(oCBW.bCommandBlock)) ;
	memcpy(oCBW.bCommandBlock, pCommand->bCommand, oCBW.bCBLen) ;

	/* Send it to out endpoint */
	byte bStatus ;
	RETURN_IF_NOT(bStatus, WriteData(&oCBW, USB_CBW_SIZE), USBMassBulkStorageDisk_SUCCESS) ;

	if(pCommand->iDirection == SCSI_DATA_READ)
	{
		RETURN_IF_NOT(bStatus, ReadData(pCommand->pRequestBuffer, oCBW.uiDataTransferLen), USBMassBulkStorageDisk_SUCCESS) ;
	}
	else
	{
		RETURN_IF_NOT(bStatus, WriteData(pCommand->pRequestBuffer, oCBW.uiDataTransferLen), USBMassBulkStorageDisk_SUCCESS) ;
	}

	RETURN_IF_NOT(bStatus, ReadData(&oCSW, 13), USBMassBulkStorageDisk_SUCCESS) ;

	/* Check bulk status */
	if(oCSW.uiSignature != USBDISK_BULK_CSW_SIGNATURE || oCSW.uiTag != oCBW.uiTag || oCSW.bStatus > US_BULK_STAT_PHASE)
	{
		printf("\nUSB Disk: Command Block Status Failed") ;
		printf(" --- Signature: 0x%x, Tag: %d, Status: %d", oCSW.uiSignature, oCSW.uiTag, oCSW.bStatus) ;
		return USBMassBulkStorageDisk_FAILURE ;
	}

	/* based on the status code, we report good or bad */
	switch(oCSW.bStatus)
	{
		case US_BULK_STAT_OK:
			if(oCSW.uiDataResidue > 0)
			{
				printf("\n USB: Short Data Situation - Unprocessed: %d", oCSW.uiDataResidue) ;
				return USBMassBulkStorageDisk_SHORT_DATA ;
			}

			return USBMassBulkStorageDisk_SUCCESS ;
		case US_BULK_STAT_FAIL:
			printf("Command Failed\n") ;
			return USBMassBulkStorageDisk_FAILURE ;
		case US_BULK_STAT_PHASE:
			printf("Error Processing Command\n") ;
			return USBMassBulkStorageDisk_ERROR ;
	}

	return USBMassBulkStorageDisk_SUCCESS ;
}

upan::string USBMassBulkStorageDisk::GetName()
{
	return "USB Disk" ;
}

bool USBMassBulkStorageDisk::QueueCommand(SCSICommand* pCommand)
{
	if(pCommand->iLun > _disk->MaxLun())
		return false ;

	byte bStatus = SendCommand(pCommand) ;

	if(bStatus == USBMassBulkStorageDisk_ERROR)
	{
		printf("\n Doing Full Reset of Mass Storage Device") ;
		DoReset() ;
		pCommand->iResult = 1 ;
		return false ;
	}

	pCommand->iResult = 0 ;
	bool bNeedSense = (bStatus == USBMassBulkStorageDisk_FAILURE) ;

	/* Also, if we have a short transfer on a command that can't have
	 * a short transfer, we're going to do this.
	 */
	byte bCode = pCommand->bCommand[ 0 ] ;

	if(!bNeedSense && (bStatus == USBMassBulkStorageDisk_SHORT_DATA))
	{
		switch(bCode)
		{
			case SCSI_REQUEST_SENSE:
			case SCSI_INQUIRY:
			case SCSI_MODE_SENSE:
			case SCSI_LOG_SENSE:
			case SCSI_MODE_SENSE_10:
				break ;
			default:
				bNeedSense = true ;
		}
	}

	/* Now, if we need do the auto-sense, let's do it */
	if(bNeedSense)
	{
		byte* pOldReqBuf ;
		int iOldDataLen ;
		int iOldDirection ;
		byte bOldCommand[ SCSI_CMD_SIZE ] ;

		memcpy(bOldCommand, pCommand->bCommand, SCSI_CMD_SIZE) ;

		/* Set the command and the LUN */
		pCommand->bCommand[ 0 ] = SCSI_REQUEST_SENSE;
		pCommand->bCommand[ 1 ] = pCommand->bCommand[ 1 ] & 0xE0;
		pCommand->bCommand[ 2 ] = 0;
		pCommand->bCommand[ 3 ] = 0;
		pCommand->bCommand[ 4 ] = 18;
		pCommand->bCommand[ 5 ] = 0;

		/* set the transfer direction */
		iOldDirection = pCommand->iDirection ;
		pCommand->iDirection = SCSI_DATA_READ ;

		/* use the new buffer we have */
		pOldReqBuf = pCommand->pRequestBuffer ;
		pCommand->pRequestBuffer = pCommand->u.bSense ;

		/* set the buffer length for transfer */
		iOldDataLen = pCommand->iRequestLen ;
		pCommand->iRequestLen = 18 ;

		/* issue the auto-sense command */
		printf("\nAuto Sensing...") ;
		bStatus = SendCommand(pCommand) ;

		if(bStatus == USBMassBulkStorageDisk_ERROR)
		{
			printf("Error processing Request Sense. Trying Complete RESET\n") ;
			DoReset() ;
			pCommand->iResult = 2 ;
			return false ;
		}

		/* we're done here, let's clean up */
		pCommand->pRequestBuffer = pOldReqBuf ;
		pCommand->iRequestLen = iOldDataLen ;
		pCommand->iDirection = iOldDirection ;
		memcpy(pCommand->bCommand, bOldCommand, SCSI_CMD_SIZE) ;

		/* if things are really okay, then let's show that */
		if((pCommand->u.bSense[ 2 ] & 0xF) == 0x00)
		{
			pCommand->iResult = 0 ;
			return false ;
		}
		
		pCommand->iResult = 3 ;
	}

	return (bStatus == USBMassBulkStorageDisk_SUCCESS) ;
}

//sint32 usbDiskAllocateIrq( struct sUsbDisk *psDisk ) {
//	uint32 nPipe;
//	sint32 nMaxP;
//	sint32 nResult = 1;
//
//	kPrint( "Allocating IRQ for CBI transport\n" );
//
//	/* Lock access to the data structure */
//	LOCK( psDisk -> nIrqUrbSem );
//
//	/* Allocate the URB */
//	psDisk -> psIrqUrb = usbAllocPacket( 0 );
//	if ( !psDisk -> psIrqUrb ) {
//		UNLOCK( psDisk -> nIrqUrbSem );
//		kPrint( "[%s]: Couldn't allocate interrupt URB\n", __FUNCTION__ );
//		return 1;
//	}
//
//	/* calculate the pipe and max packet size */
//	nPipe = USB_RCVINTPIPE( psDisk -> psUsbDev, psDisk -> psEpInt -> nEndPointAddress & USB_ENDPOINT_NUMBER_MASK );
//	nMaxP = USB_MAXPACKET( psDisk -> psUsbDev, nPipe, USB_PIPEOUT( nPipe ) );
//	if ( nMaxP > sizeof( psDisk -> anIrqBuf ) ) {
//		nMaxP = sizeof( psDisk -> anIrqBuf );
//	}
//
//	/* fill in the URB with our data */
//	USB_FILL_INT( psDisk -> psIrqUrb, psDisk -> psUsbDev, nPipe,
//		psDisk -> anIrqBuf, nMaxP, usbStorCbiIrq, psDisk, psDisk -> psEpInt -> nInterval );
//
//	/* submit the URB for processing */
//	nResult = usbSubmitPacket( psDisk -> psIrqUrb );
//	if ( nResult ) {
//		usbFreePacket( psDisk -> psIrqUrb );
//		UNLOCK( psDisk -> nIrqUrbSem );
//		return 2;
//	}
//
//	/* unlock the data structure and return success */
//	UNLOCK( psDisk -> nIrqUrbSem );
//
//	return 0;
//}

void USBDiskDriver::Register()
{
	USDDeviceId = 0;
	USBController::Instance().RegisterDriver(new USBDiskDriver("USB Mass Storage Disk"));
}

int USBDiskDriver::MatchingInterfaceIndex(USBDevice* pUSBDevice)
{
  USBStandardConfigDesc* pConfig = &(pUSBDevice->_pArrConfigDesc[ pUSBDevice->_iConfigIndex ]) ;
  for(byte i = 0; i < pConfig->bNumInterfaces; ++i)
  {
    const auto& interface = pConfig->pInterfaces[i];
    if(interface.bInterfaceClass == USB_CLASS_MASS_STORAGE && interface.bInterfaceProtocol == USB_PR_BULK)
      return i;
  }
  return -1;
}

bool USBDiskDriver::AddDevice(USBDevice* pUSBDevice)
{
  int interfaceIndex = MatchingInterfaceIndex(pUSBDevice);
  if(interfaceIndex < 0)
    return false;

  if(pUSBDevice->_deviceDesc.sVendorID == 0x05DC)
    throw upan::exception(XLOC, "Lexar Jumpshot USB CF Reader detected - Not supported yet!");

  const USBStandardInterface& interface = pUSBDevice->_pArrConfigDesc[ pUSBDevice->_iConfigIndex ].pInterfaces[ interfaceIndex ];

  byte bMaxLun = 0;
  /* Set the handler pointers based on the protocol
  * Again, this data is persistant across reattachments
  */
  switch(interface.bInterfaceProtocol)
  {
    case USB_PR_BULK:
      printf("\n Reading MAX LUN for the Bulk Storage Disk\n") ;
      //TODO: Some devices which don't support multiple LUN can stall the command
      //bMaxLun = pUSBDevice->GetMaxLun();
      printf("\n Max LUN of the device: %d", bMaxLun) ;
      break;

    default:
      throw upan::exception(XLOC, "Unsupported USB Disk Device Protocol: %d", interface.bInterfaceProtocol);
  }

  upan::string protoName;
  switch(interface.bInterfaceSubClass)
  {
    case USB_SC_SCSI:
      protoName = "SCSI";
      break;

    default:
      throw upan::exception(XLOC, "Unsupported USB Disk Device SubClass: %d", interface.bInterfaceSubClass);
  }

  /* Create device */
  upan::uniq_ptr<USBulkDisk> pDisk(new USBulkDisk(pUSBDevice, interfaceIndex, bMaxLun, protoName));
  pDisk->Initialize();
  pDisk.release();
  return true;
}

void USBDiskDriver::RemoveDevice(USBDevice* pUSBDevice)
{
	USBulkDisk* pDisk = (USBulkDisk*)pUSBDevice->_pPrivate ;
	SCSIDevice** pSCSIDeviceList = pDisk->pSCSIDeviceList ;

	class USBDriveRemoveClause : public DriveRemoveClause
	{
		public:
			USBDriveRemoveClause(SCSIDevice** p, int iMaxLun) : m_pSCSIDeviceList(p), m_iMaxLun(iMaxLun) { }

			bool operator()(const StorageDrive* pDiskDrive) const
			{
				int iLun ;
				for(iLun = 0; iLun <= m_iMaxLun; iLun++)
				{
					if(m_pSCSIDeviceList[ iLun ] == NULL)
						break ;

					if(pDiskDrive->Device() == m_pSCSIDeviceList[ iLun ])
						return true ;
				}

				return false ;
			}

		private:
			SCSIDevice** m_pSCSIDeviceList ;
			const int m_iMaxLun ;
	} ;

	if(pSCSIDeviceList != NULL)
		StorageDriveManager::Instance().RemoveEntryByCondition(USBDriveRemoveClause(pSCSIDeviceList, pDisk->MaxLun())) ;

  delete pDisk;
}
