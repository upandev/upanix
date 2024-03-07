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
#include <SCSIHandler.h>
#include <stdio.h>
#include <StringUtil.h>
#include <string.h>
#include <DMM.h>
#include <ProcessManager.h>

static const char SCSIHandler_commandSize[ 8 ] = {
	6, 10, 10, 12,
	16, 12, 10, 10
};

static const char* SCSIHandler_szDeviceTypes[ MAX_SCSI_DEV_TYPES ] = {
	"Direct-Access",
	"Sequential-Access",
	"Printer",
	"Processor",
	"WORM",
	"CD-ROM",
	"Scanner",
	"OpticalDevice",
	"MediumChanger",
	"Communications",
	"Unknown",
	"Unknown",
	"Unknown",
	"Enclosure",
} ;

/********************************** Static ****************************/

static int SCSIHandler_GetCommandSize(int iCode)
{
	return SCSIHandler_commandSize[ (iCode >> 5) & 0x7 ] ;
}

static unsigned SCSIHandler_Rev32ToCPU(unsigned uiVal)
{
	return ( ( uiVal & 0xFF000000 ) >> 24 | ( uiVal & 0x00FF0000 ) >> 8 |
				( uiVal & 0x0000FF00 ) << 8 | ( uiVal & 0x000000FF ) << 24 ) ;
}

SCSICommand::SCSICommand(const SCSIDevice& device) :
  pHost(device.pHost), iChannel(device.iChannel), iDevice(device.iDevice), iLun(device.iLun),
  iDirection(SCSI_DATA_NONE), iCmdLen(0), pRequestBuffer(nullptr),
  iRequestLen(0), iResult(0)
{
  Init();
}

SCSICommand::SCSICommand(SCSIHost* host, int channel, int device, int lun) :
  pHost(host), iChannel(channel), iDevice(device), iLun(lun),
  iDirection(SCSI_DATA_NONE), iCmdLen(0), pRequestBuffer(nullptr),
  iRequestLen(0), iResult(0)
{
  Init();
}

void SCSICommand::Init()
{
  memset(bCommand, 0, sizeof(bCommand));
  memset(&u, 0, sizeof(u));
}

__attribute__((unused)) static int SCSIHandler_UnitNotReady(SCSIDevice* pDevice, SCSISense* pSense)
{
	int iStatus ;

	/* Find out why the unit is not ready */
	switch(pSense->Asc)
	{
		case SCSI_NO_ASC_DATA:
			printf("\n With additional code SCSI_NO_ASC_DATA" ) ;

			/* Probably worth trying again */
			iStatus = SENSE_RETRY ;
			break ;

		case SCSI_LOGICAL_UNIT_NOT_READY:
			printf("\n With additional code SCSI_LOGICAL_UNIT_NOT_READY " ) ;

			/* Find out why it isn't ready */
			switch(pSense->Ascq)
			{
				case SCSI_NOT_REPORTABLE:
					printf("and a qualifier of SCSI_NOT_REPORTABLE" ) ;

					/* We don't know what this error is, may as well try again */
					iStatus = SENSE_RETRY ;
					break ;

				case SCSI_BECOMING_READY:
					printf("and a qualifier of SCSI_BECOMING_READY" ) ;

					/* Wait for the drive to become ready. Delay a little to give the drive a chance to spin up */
					ProcessManager::Instance().Sleep(1000) ;
					iStatus = SENSE_RETRY ;
					break ;

				case SCSI_MUST_INITIALIZE:
					printf("and a qualifier of SCSI_MUST_INITIALIZE" ) ;
	
					/* Need to send a START/STOP to the drive */
					/* TODO
					if(SCSIHandler_Start(pDevice) < 0)
						iStatus = SENSE_FATAL ;
					else
						iStatus = SENSE_RETRY ;
					*/
					break ;

				case SCSI_MANUAL_INTERVENTION:
					printf("and a qualifier of SCSI_MANUAL_INTERVENTION" ) ;
	
					/* Nothing we can do */
					iStatus = SENSE_FATAL ;
					break ;

				case SCSI_FORMAT_IN_PROGRESS:
					printf("and a qualifier of SCSI_FORMAT_IN_PROGRESS" ) ;

					/* Should never happen */
					iStatus = SENSE_FATAL ;
					break ;

				case SCSI_REBUILD_IN_PROGRESS:
					printf("and a qualifier of SCSI_REBUILD_IN_PROGRESS" ) ;

					/* Should never happen */
					iStatus = SENSE_FATAL ;
					break ;

				case SCSI_RECALC_IN_PROGRESS:
					printf("and a qualifier of SCSI_RECALC_IN_PROGRESS" ) ;

					/* Should never happen */
					iStatus = SENSE_FATAL ;
					break ;

				case SCSI_OP_IN_PROGRESS:
					printf("and a qualifier of SCSI_OP_IN_PROGRESS" ) ;

					/* Should never happen */
					iStatus = SENSE_FATAL ;
					break ;

				case SCSI_LONG_WRITE_IN_PROGRESS:
					printf("and a qualifier of SCSI_LONG_WRITE_IN_PROGRESS" ) ;

					/* Should never happen */
					iStatus = SENSE_FATAL ;
					break ;

				case SCSI_SELF_TEST_IN_PROGRESS:
					printf("and a qualifier of SCSI_SELF_TEST_IN_PROGRESS" ) ;

					/* Should never happen */
					iStatus = SENSE_FATAL ;
					break ;

				case SCSI_ASSYM_ACCESS_STATE_TRANS:
					printf("and a qualifier of SCSI_ASSYM_ACCESS_STATE_TRANS" ) ;

					/* Should never happen */
					iStatus = SENSE_FATAL ;
					break ;

				case SCSI_TARGET_PORT_STANDBY:
					printf("and a qualifier of SCSI_TARGET_PORT_STANDBY") ;

					/* Should never happen */
					iStatus = SENSE_FATAL ;
					break ;

				case SCSI_TARGET_PORT_UNAVAILABLE:
					printf("and a qualifier of SCSI_TARGET_PORT_UNAVAILABLE" ) ;

					/* Should never happen */
					iStatus = SENSE_FATAL ;
					break ;

				case SCSI_AUX_MEM_UNAVAILABLE:
					printf("and a qualifier of SCSI_AUX_MEM_UNAVAILABLE" ) ;

					/* Should never happen */
					iStatus = SENSE_FATAL ;
					break ;

				case SCSI_NOTIFY_REQUIRED:
					printf("and a qualifier of SCSI_NOTIFY_REQUIRED" ) ;

					/* Send notify? */
					iStatus = SENSE_FATAL ;
					break ;

				default:
					printf("and an unknown qualifier 0x%04x", pSense->Ascq) ;

					/* We don't know what this error is */
					iStatus = SENSE_FATAL ;
					break ;
			}
			break ;

		case SCSI_NOT_RESPONDING:
			printf("\n With additional code SCSI_NOT_RESPONDING") ;

			/* Give up */
			iStatus = SENSE_FATAL ;
			break ;

		case SCSI_MEDIUM:
			printf("\n With additional code SCSI_MEDIUM ") ;

			/* It helps if the disk is in the drive. Is it? */
			switch(pSense->Ascq)
			{
				case SCSI_MEDIUM_TRAY_OPEN:
					printf("and a qualifier of SCSI_MEDIUM_TRAY_OPEN" ) ;
	
					/* We have the option of sending a START/STOP to close the drive but that could be evil, so we won't */
					iStatus = SENSE_FATAL ;
					break ;

				case SCSI_MEDIUM_NOT_PRESENT:
					printf("and a qualifier of SCSI_MEDIUM_NOT_PRESENT") ;
	
					/* Put the disk in the drive, dum dum! */
					iStatus = SENSE_FATAL ;
					break ;

				case SCSI_MEDIUM_TRAY_CLOSED:
					printf("and a qualifier of SCSI_MEDIUM_TRAY_CLOSED") ;
	
					/* This is a bit of silly error really; why is it an error if the tray is closed? */
					iStatus = SENSE_FATAL ;
					break ;

				case SCSI_MEDIUM_LOADABLE:
					printf("and a qualifier of SCSI_MEDIUM_LOADABLE" ) ;
	
					/* We can't load it ourselves */
					iStatus = SENSE_FATAL ;
					break ;

				default:
					printf("and an unknown qualifier 0x%0x4x", pSense->Ascq) ;
	
					/* We don't know what this error is */
					iStatus = SENSE_FATAL ;
			}
			break ;

		default:
			printf("\n With an unknown or unsupported additional code 0x%04x", pSense->Asc) ;

			/* The drive did something screwy, possibly a deprecated sense condition, but still.. */
			iStatus = SENSE_FATAL ;
	}

	return iStatus ;
}

upan::string SCSIHandler_ExtractString(byte* szSCSIResult, int iStart, int iEnd)
{
  int len = iEnd - iStart;
  char* buf = new char[len + 1];
  buf[len] = '\0';
  
	for(int si = 0, i = iStart; i < iEnd; ++i, ++si)
	{
		if(szSCSIResult[i] >= 0x20 && i < szSCSIResult[4] + 5)
			buf[si] = szSCSIResult[i] ;
		else
      buf[si] = ' ';
	}
  return buf;
}

static SCSIDevice* SCSIHandler_CreateDisk(SCSIHost* pHost, int iChannel, int iDevice, int iLun, byte* szSCSIResult)
{
  SCSIDevice* pDevice = new SCSIDevice();
	pDevice->szVendor = SCSIHandler_ExtractString(szSCSIResult, 8, 16);
	pDevice->szModel = SCSIHandler_ExtractString(szSCSIResult, 16, 32);
	pDevice->szRevision = SCSIHandler_ExtractString(szSCSIResult, 32, 36);
	pDevice->iDeviceTypeIndex = szSCSIResult[0] & 0x1F ;
	pDevice->pszDeviceType = (pDevice->iDeviceTypeIndex < 14) ? SCSIHandler_szDeviceTypes[pDevice->iDeviceTypeIndex] : "Unknown" ;

	pDevice->iSCSILevel = szSCSIResult[ 2 ] & 0x07 ;
	pDevice->bRemovable = (0x80 & szSCSIResult[ 1 ]) >> 7 ;
	
	if(pDevice->bRemovable)
		printf("Removable Disk.\n") ;

	printf("\nSCSI Vendor: %s", pDevice->szVendor.c_str());
	printf("\nSCSI Model: %s", pDevice->szModel.c_str());
	printf("\nSCSI Revision: %s", pDevice->szRevision.c_str());
	printf("\nType: %s ANSI SCSI revision %02x", pDevice->pszDeviceType, szSCSIResult[2] & 0x07) ;

	pDevice->pHost = pHost ;
	pDevice->iChannel = iChannel ;
	pDevice->iDevice = iDevice ;
	pDevice->iLun = iLun ;

	pDevice->szName = pHost->GetName();

	if(SCSIHandler_GenericOpen(pDevice) != SCSIHandler_SUCCESS)
	{
		printf("\n Failed to get disk capacity attributes") ;
		DMM_DeAllocateForKernel((uintptr_t)pDevice) ;
		return NULL ;
	}
	
	return pDevice ;
}

static byte SCSIHandler_TestReady(SCSIDevice* pDevice)
{
	byte bStatus ;

	while(true)
	{
		/* Try to test the state of the unit */
	  SCSICommand sCommand(*pDevice);

		sCommand.bCommand[0] = SCSI_TEST_UNIT_READY ;

		if(pDevice->iSCSILevel <= SCSI_2)
			sCommand.bCommand[1] = ( pDevice->iLun << 5 ) & 0xE0 ;

		sCommand.iCmdLen = SCSIHandler_GetCommandSize(SCSI_TEST_UNIT_READY) ;

		/* Send command */
		bStatus = pDevice->pHost->QueueCommand(&sCommand) ;

		if(sCommand.u.Sense.SenseKey != SCSI_NO_SENSE || sCommand.iResult != 0)
		{
			int ret = SCSIHandler_CheckSense(&sCommand.u.Sense) ;
			switch(ret)
			{
				case SENSE_OK:
					break;
				case SENSE_RETRY:
					continue ;
				case SENSE_FATAL:
					printf("\n SCSI device reporting fatal error, aborting." ) ;
					return bStatus ;
			}
		}

		break ;
	}

	printf("\n SCSI Device ready") ;

	return SCSIHandler_SUCCESS ;
}

static byte SCSIHandler_ReadCapacity(SCSIDevice* pDevice)
{
	struct {
		unsigned uiLBA ;
		unsigned uiBlockLen ;
	} sCap;

	pDevice->uiSectors = 0 ;
	pDevice->uiSectorSize = 0 ;
	pDevice->ulSize = 0 ;

	while(true)
	{
    SCSICommand sCommand(*pDevice);

		sCommand.iDirection = SCSI_DATA_READ ;
		sCommand.pRequestBuffer = (byte*)&sCap ;
		sCommand.iRequestLen = sizeof(sCap) ;

		sCommand.bCommand[0] = SCSI_READ_CAPACITY ;
		sCommand.bCommand[1] = (pDevice->iLun << 5) & 0xE0 ;

		sCommand.iCmdLen = SCSIHandler_GetCommandSize(SCSI_READ_CAPACITY) ;
		
		/* Send command */
		pDevice->pHost->QueueCommand(&sCommand) ;

		if(sCommand.u.Sense.SenseKey != SCSI_NO_SENSE && sCommand.iResult != 0)
		{
			int ret = SCSIHandler_CheckSense(&sCommand.u.Sense) ;
			switch(ret)
			{
				case SENSE_OK:
					break ;
				case SENSE_RETRY:
					continue ;
				case SENSE_FATAL:
					printf("\n SCSI device reporting fatal error, aborting." ) ;
					return SCSIHandler_FAILURE ;
			}
		}

		pDevice->uiSectorSize = SCSIHandler_Rev32ToCPU(sCap.uiBlockLen) ;

		if(pDevice->uiSectorSize > 2048)
		{
			printf("\n Device Sector Size greater then 2048 - %d! Setting to 2048 instead", pDevice->uiSectorSize) ;
			pDevice->uiSectorSize = 2048 ;
		}

		pDevice->uiSectors = 1 + SCSIHandler_Rev32ToCPU(sCap.uiLBA) ;
		pDevice->ulSize = pDevice->uiSectors * pDevice->uiSectorSize ;

		printf("\n Sector Size: %d\n No. of Sectors: %d\n Total Size: %ld", pDevice->uiSectorSize, pDevice->uiSectors, pDevice->ulSize) ;
		return SCSIHandler_SUCCESS ;
	}

	return SCSIHandler_FAILURE ;
}


/*************************************************************************/

void SCSIHandler_Initialize()
{
  KC::MConsole().LoadMessage("SCSI Driver Initialization", Success) ;
}

byte SCSIHandler_GenericOpen(SCSIDevice* pDevice) 
{
	byte bStatus;

	// TODO: Lock Device Access
	
	bStatus = SCSIHandler_TestReady(pDevice) ;

	if(bStatus == SCSIHandler_SUCCESS)
	{
		ProcessManager::Instance().Sleep(1000) ;

		bStatus = SCSIHandler_ReadCapacity(pDevice) ;

		if(bStatus == SCSIHandler_SUCCESS)
			pDevice->iOpenCount++ ;
		else
			printf("\n Error reading from device. Err Code: %d", bStatus) ;
	}
	else
		printf("\n Error waiting for the device. Err Code: %d", bStatus) ;

	// TODO: Unlock Device Access
	
	return bStatus ;
}

byte SCSIHandler_GenericClose(SCSIDevice* pDevice)
{
	// TODO: Lock Device Access 
	pDevice->iOpenCount-- ;
	// TODO: Unlock Device Access
	
	return SCSIHandler_SUCCESS ;
}

void SCSIHandler_GenericRead(SCSIDevice* pDevice, unsigned uiStartSector, unsigned uiNumOfSectors, byte* pDataBuffer)
{
	unsigned uiBlock ;
	unsigned uiBlockCount ;
	unsigned uiMaxBlock = 8 ;
		
	while(uiNumOfSectors > 0)
	{
		uiBlockCount = (uiNumOfSectors > uiMaxBlock) ? uiMaxBlock : uiNumOfSectors ;
		unsigned uiReadLen = uiBlockCount * pDevice->uiSectorSize ;

		uiBlock = uiStartSector ;

		/* Build SCSI_READ_10 command */
    SCSICommand sCommand(*pDevice);

		sCommand.iDirection = SCSI_DATA_READ ;
		sCommand.pRequestBuffer = pDataBuffer ;
		sCommand.iRequestLen = uiReadLen ;

		sCommand.bCommand[0] = SCSI_READ_10 ;

		if(pDevice->iSCSILevel <= SCSI_2)
			sCommand.bCommand[1] = (pDevice->iLun << 5) & 0xE0 ;
		else
			sCommand.bCommand[1] = 0 ;

		sCommand.bCommand[2] = (byte)(uiBlock >> 24) & 0xFF ;
		sCommand.bCommand[3] = (byte)(uiBlock >> 16) & 0xFF ;
		sCommand.bCommand[4] = (byte)(uiBlock >> 8) & 0xFF ;
		sCommand.bCommand[5] = (byte)(uiBlock & 0xFF) ;
		sCommand.bCommand[6] = sCommand.bCommand[9] = 0 ;
		sCommand.bCommand[7] = (byte)(uiBlockCount >> 8) & 0xFF ;
		sCommand.bCommand[8] = (byte)(uiBlockCount & 0xFF) ;

		sCommand.iCmdLen = SCSIHandler_GetCommandSize(SCSI_READ_10) ;

		/* Send command */
		bool bStatus = pDevice->pHost->QueueCommand(&sCommand) ;

		if(!bStatus || sCommand.iResult != 0)
      throw upan::exception(XLOC, "SCSI read failed with status code:%d, result:%d", bStatus, sCommand.iResult);

		uiStartSector += uiBlockCount ;
		uiNumOfSectors -= uiBlockCount ;
		pDataBuffer += uiReadLen ;
	}
}

void SCSIHandler_GenericWrite(SCSIDevice* pDevice, unsigned uiStartSector, unsigned uiNumOfSectors, byte* pDataBuffer)
{
	unsigned uiBlock ;
	unsigned uiBlockCount ;
	unsigned uiMaxBlock = 16 ;
		
	while(uiNumOfSectors > 0)
	{
		uiBlockCount = (uiNumOfSectors > uiMaxBlock) ? uiMaxBlock : uiNumOfSectors ;
		unsigned uiWriteLen = uiBlockCount * pDevice->uiSectorSize ;

		uiBlock = uiStartSector ;

		/* Build SCSI_WRITE_10 command */
    SCSICommand sCommand(*pDevice);
		sCommand.iDirection = SCSI_DATA_WRITE ;
		sCommand.pRequestBuffer = pDataBuffer ;
		sCommand.iRequestLen = uiWriteLen ;

		sCommand.bCommand[0] = SCSI_WRITE_10 ;

		if(pDevice->iSCSILevel <= SCSI_2)
			sCommand.bCommand[1] = (pDevice->iLun << 5) & 0xE0 ;
		else
			sCommand.bCommand[1] = 0 ;

		sCommand.bCommand[2] = (byte)(uiBlock >> 24) & 0xFF ;
		sCommand.bCommand[3] = (byte)(uiBlock >> 16) & 0xFF ;
		sCommand.bCommand[4] = (byte)(uiBlock >> 8) & 0xFF ;
		sCommand.bCommand[5] = (byte)(uiBlock & 0xFF) ;
		sCommand.bCommand[6] = sCommand.bCommand[9] = 0 ;
		sCommand.bCommand[7] = (byte)(uiBlockCount >> 8) & 0xFF ;
		sCommand.bCommand[8] = (byte)(uiBlockCount & 0xFF) ;
		
		sCommand.iCmdLen = SCSIHandler_GetCommandSize(SCSI_WRITE_10) ;

		/* Send command */
		bool bStatus = pDevice->pHost->QueueCommand(&sCommand) ;

		if(!bStatus || sCommand.iResult != 0)
      throw upan::exception(XLOC, "SCSI read failed with status code:%d, result:%d", bStatus, sCommand.iResult);

		uiStartSector += uiBlockCount ;
		uiNumOfSectors -= uiBlockCount ;
		pDataBuffer += uiWriteLen ;
	}
}

byte SCSIHandler_DoStartStop(SCSIDevice* pDevice, unsigned uiFlags)
{
	/* Create and queue request sense command */
	SCSICommand sCommand(*pDevice);

	sCommand.bCommand[0] = SCSI_START_STOP ;
	sCommand.bCommand[4] = uiFlags ;

	sCommand.iCmdLen = SCSIHandler_GetCommandSize(SCSI_START_STOP) ;

	if(sCommand.iResult < 0)
	{
		printf("\n Error: Could not eject drive. Sense key is %x", sCommand.u.Sense.SenseKey ) ;
		return SCSIHandler_FAILURE ;
	}

	return SCSIHandler_SUCCESS ;
}

byte SCSIHandler_Start(SCSIDevice* pDevice)
{
	return SCSIHandler_DoStartStop(pDevice, 0x01) ;
}

byte SCSIHandler_Eject(SCSIDevice* pDevice)
{
	return SCSIHandler_DoStartStop(pDevice, 0x02) ;
}

void SCSIHandler_ReadPart(void* pPrivate, unsigned uiStartSector, unsigned uiNumOfSectors, byte* pDataBuffer)
{
	SCSIDevice* pDevice = (SCSIDevice*)pPrivate ;
  SCSIHandler_GenericRead(pDevice, uiStartSector, uiNumOfSectors, pDataBuffer) ;
}

//byte SCSIHandler_DecodePartitions(SCSIDevice* pDevice)
//{
//	sint32 nNumPartitions;
//	struct sDeviceGeometry sDiskGeom;
//	struct sPartition asPartitions[ 16 ];
//	struct sScsiDevice *psPartition;
//	struct sScsiDevice **ppsTmp;
//	sint32 nIndex, nError;
//	sint32 nDeviceId = psInode -> nDeviceHandle;
//
//	sDiskGeom.nSectorCount = psInode -> nSectors;
//	sDiskGeom.nCylinderCount = 1;
//	sDiskGeom.nSectorsPerTrack = 1;
//	sDiskGeom.nHeadCount = 1;
//	sDiskGeom.nBytesPerSector = psInode -> nSectorSize;
//	sDiskGeom.nReadOnly = FALSE;
//	sDiskGeom.nRemovable = psInode -> bRemovable;
//
//	DEBUG( KERN_INFO, "Decode partition table for %s", psInode -> anName );
//
//	nNumPartitions = decodeDiskPartitions( &sDiskGeom, asPartitions, 16, psInode, scsiReadPart );
//	if ( nNumPartitions < 0 ) {
//		DEBUG( KERN_WARNING, "Invalid partition table" );
//		return nNumPartitions;
//	}
//
//	for ( nIndex = 0; nIndex < nNumPartitions; nIndex++ ) {
//		if ( asPartitions[ nIndex ].nType != 0 &&
//			asPartitions[ nIndex ].nSize != 0 ) {
//			DEBUG( KERN_INFO, "Partition %d: %10Ld -> %10Ld %s (%Ld)",
//				nIndex, asPartitions[ nIndex ].nStart,
//				asPartitions[ nIndex ].nStart + asPartitions[ nIndex ].nSize - 1,
//				GetFsName( asPartitions[ nIndex ].nType ), asPartitions[ nIndex ].nSize );
//		}
//	}
//
//	nError = 0;
//
//	for ( psPartition = psInode -> psFirstPartition; psPartition != NULL; psPartition = psPartition -> psNext ) {
//		bool bFound = FALSE;
//
//		for ( nIndex = 0; nIndex < nNumPartitions; nIndex++ ) {
//			if ( asPartitions[ nIndex ].nStart == psPartition -> nStart && asPartitions[ nIndex ].nSize == psPartition -> nSize ) {
//				bFound = TRUE;
//				break;
//			}
//		}
//
//		if ( bFound == FALSE && AtomicRead( &psPartition -> aOpenCount ) > 0 ) {
//			DEBUG( KERN_WARNING, "Error: Open partition %s on %s has changed",
//				psPartition -> anName, psInode -> anName );
//			nError = -EBUSY;
//			goto error;
//		}
//	}
//
//	/* Remove deleted partitions ... */
//	for ( ppsTmp = &psInode -> psFirstPartition; *ppsTmp != NULL; ) {
//		bool bFound = FALSE;
//
//		psPartition = *ppsTmp;
//		for ( nIndex = 0; nIndex < nNumPartitions; nIndex++ ) {
//			if ( asPartitions[ nIndex ].nStart == psPartition -> nStart && asPartitions[ nIndex ].nSize == psPartition -> nSize ) {
//				asPartitions[ nIndex ].nSize = 0;
//				psPartition -> nPartitionType = asPartitions[ nIndex ].nType;
//				StringPrint( psPartition -> anName, "%d", nIndex );
//				bFound = TRUE;
//				break;
//			}
//		}
//
//		if ( bFound == FALSE ) {
//			*ppsTmp = psPartition -> psNext;
//			//
//			kFreeMemory( psPartition );
//		} else {
//			ppsTmp = &( *ppsTmp ) -> psNext;
//		}
//	}
//
//	/* Create new nodes for any partitions */
//	for ( nIndex = 0; nIndex < nNumPartitions; nIndex++ ) {
//		sint8 anPath[ 64 ];
//
//		if ( asPartitions[ nIndex ].nType == 0 || asPartitions[ nIndex ].nSize == 0 ) {
//			continue;
//		}
//
//		psPartition = kMemoryAllocation( sizeof( struct sScsiDevice ) );
//		if ( psPartition == NULL ) {
//			DEBUG( KERN_WARNING, "No memory for partition" );
//			break;
//		}
//
//		StringPrint( psPartition -> anName, "%d", nIndex );
//		psPartition -> psRawDevice = psInode;
//		psPartition -> psHost = psInode -> psHost;
//		psPartition -> nLock = psInode -> nLock;
//		psPartition -> nId = psInode -> nId;
//		psPartition -> nDeviceHandle = psInode -> nDeviceHandle;
//		psPartition -> nChannel = psInode -> nChannel;
//		psPartition -> nDevice = psInode -> nDevice;
//		psPartition -> nLun = psInode -> nLun;
//		psPartition -> nScsiLevel = psInode -> nScsiLevel;
//		psPartition -> bRemovable = psInode -> bRemovable;
//		psPartition -> pnDataBuffer = psInode -> pnDataBuffer;
//
//		MemoryCopy( &psPartition -> anVendor, &psInode -> anVendor, 8 );
//		MemoryCopy( &psPartition -> anModel, &psInode -> anModel, 16 );
//		MemoryCopy( &psPartition -> anRev, &psInode -> anRev, 4 );
//
//		psPartition -> nSectorSize = psInode -> nSectorSize;
//		psPartition -> nSectors = psInode -> nSectors;
//
//		psPartition -> nStart = asPartitions[ nIndex ].nStart;
//		psPartition -> nSize = asPartitions[ nIndex ].nSize;
//
//		psPartition -> psNext = psInode -> psFirstPartition;
//		psInode -> psFirstPartition = psPartition;
//
//		StringCopy( anPath, psInode -> anName );
//		StringCat( anPath, psPartition -> anName );
//
//		/* Create device */
//		DeviceFsAdd( anPath, nDeviceId, nIndex + 1, S_IRWXU | S_IFBLK, psPartition );
//		if ( BlockSetBegin( nDeviceId, nIndex + 1,
//			sizeof( sint32 ) * 2 * 64, psPartition -> nStart / 512 ) ) {
//			nError = -EFAIL;
//		}
//	}
//
//error:
//	return nError;
//}
//
//struct sScsiDevice *scsiFindPartition( struct sScsiDevice *psDevice, uint32 nId ) {
//	struct sScsiDevice *psLoop = psDevice;
//	sint32 nIndex = 1;
//
//	if ( nId == 0 ) {
//		return psLoop -> psRawDevice;
//	}
//
//	while ( psLoop != NULL ) {
//		DEBUG( KERN_DEBUG, "loop %x, '%s', want %d", psLoop, psLoop -> anName, nId );
//		if ( nIndex == nId ) {
//			return psLoop;
//		}
//		nIndex++;
//		psLoop = psLoop -> psNext;
//	}
//
//	return NULL;
//}

//byte SCSIHandler_Request(
//sint32 scsiRequest( struct sRequest *psRequest ) {
//	struct sScsiDevice *psDevice = NULL;
//	struct sScsiDevice *psPartition = NULL;
//	sint32 nError = 1;
//	sint32 nNum = 0;
//
//	psDevice = scsiFindDevice( psRequest -> nDevice );
//	if ( psDevice == NULL ) {
//		DEBUG( KERN_WARNING, "Could not find device, %d", psRequest -> nDevice );
//		goto end;
//	}
//
//	psPartition = scsiFindPartition( psDevice -> psFirstPartition, SUBID( psRequest -> nDevice ) );
//	if ( psPartition == NULL ) {
//		DEBUG( KERN_WARNING, "Could not find partition, %d", psRequest -> nDevice );
//		goto end;
//	}
//
//	DEBUG( KERN_DEBUG, "start at %Ld, %d", psPartition -> nStart, psRequest -> nSector );
//	switch ( psRequest -> nCommand ) {
//		case READ:
//			nError = scsiGenericDoRead( psDevice, /*psPartition -> nStart / 512 +*/ psRequest -> nSector,
//				psRequest -> ppData, 64, &nNum );
//			break;
//		case WRITE:
//			//nError = scsiWrite( psDevice, psRequest -> nSector,
//			//	psRequest -> ppData, psRequest -> nNumSectors );
//			//nError = 0;
//			//nNum = psRequest -> nNumSectors;
//			break;
//		default:
//			break;
//	}
//
//end:
//	EndRequest( psRequest -> nDevice, nError );
//
//	return nNum;
//}

//struct sFileOperations sScsiDiskFileOperations = {
//	NULL,
//	NULL,
//	NULL,
//	NULL,
//	NULL,
//	scsiGenericOpen,
//	scsiGenericClose,
//	NULL,
//	NULL
//};
//
//struct sScsiDevice *scsiAddDisk( struct sScsiHost *psHost, sint32 nChannel,
//	sint32 nDevice, sint32 nLun, uint8 anScsiResult[] ) {
//	struct sScsiDevice *psDevice = NULL;
//	sint8 anTemp[ 255 ];
//	sint32 nIndex;
//
//	/* Create device if it is a harddisk */
//	psDevice = kMemoryAllocation( sizeof( struct sScsiDevice ) );
//	MemorySet( psDevice, 0, sizeof( struct sScsiDevice ) );
//
//	psDevice -> nId = scsiGetNextId();
//	AtomicSet( &psDevice -> aOpenCount, 0 );
//	psDevice -> psHost = psHost;
//	psDevice -> psRawDevice = psDevice;
//	psDevice -> nLock = CreateSemaphore( "scsi device lock", 1, SEM_NONE );
//	psDevice -> nChannel = nChannel;
//	psDevice -> nDevice = nDevice;
//	psDevice -> nLun = nLun;
//	psDevice -> nType = ( anScsiResult[ 0 ] & 0x1F );
//	psDevice -> nScsiLevel = ( anScsiResult[ 2 ] & 0x07 );
//	psDevice -> bRemovable = ( 0x80 & anScsiResult[ 1 ] ) >> 7;
//	psDevice -> pnDataBuffer = kMemoryAllocation( 0xFFFF );
//
//	if ( psDevice -> bRemovable ) {
//		DEBUG( KERN_DEBUG, "Removable device" );
//	}
//
//	StringCopy( psDevice -> anVendor, "" );
//	for ( nIndex = 8; nIndex < 16; nIndex++ ) {
//		if ( anScsiResult[ nIndex ] >= 0x20 && nIndex < anScsiResult[ 4 ] + 5 ) {
//			StringPrint( anTemp, "%c", anScsiResult[ nIndex ] );
//			StringCat( psDevice -> anVendor, anTemp );
//		} else {
//			StringCat( psDevice -> anVendor, " " );
//		}
//	}
//
//	StringCopy( psDevice -> anModel, "" );
//	for ( nIndex = 16; nIndex < 32; nIndex++ ) {
//		if ( anScsiResult[ nIndex ] >= 0x20 && nIndex < anScsiResult[ 4 ] + 5 ) {
//			StringPrint( anTemp, "%c", anScsiResult[ nIndex ] );
//			StringCat( psDevice -> anModel, anTemp );
//		} else {
//			StringCat( psDevice -> anModel, " " );
//		}
//	}
//
//	StringCopy( psDevice -> anRev, "" );
//	for ( nIndex = 32; nIndex < 36; nIndex++ ) {
//		if ( anScsiResult[ nIndex ] >= 0x20 && nIndex < anScsiResult[ 4 ] + 5 ) {
//			StringPrint( anTemp, "%c", anScsiResult[ nIndex ] );
//			StringCat( psDevice -> anRev, anTemp );
//		} else {
//			StringCat( psDevice -> anRev, " " );
//		}
//	}
//
//	/* Add device to the list */
//	psDevice -> psNext = psFirstScsiDevice;
//	psFirstScsiDevice = psDevice;
//
//	/* Create device fs entries */
//	StringPrint( psDevice -> anName, "sd%c", 'a' + psDevice -> nId );
//	psDevice -> nDeviceHandle = RegisterDevice( psDevice -> anName,
//		&sScsiDiskFileOperations, scsiRequest, 512, 0 );
//	if ( psDevice -> nDeviceHandle < 0 ) {
//		DEBUG( KERN_WARNING, "Cannot register %s", psDevice -> anName );
//		return NULL;
//	}
//	sScsiDiskFileOperations.Read = &BlockRead;
//	sScsiDiskFileOperations.Write = &BlockWrite;
//
//	DeviceFsAdd( psDevice -> anName, psDevice -> nDeviceHandle,
//		psDevice -> nId, S_IRWXU | S_IFBLK, psDevice );
//	
//	/* open & decode partitions */
//	if ( scsiGenericOpenIntern( psDevice ) == 0 ) {
//		scsiGenericCloseIntern( psDevice );
//		scsiDecodePartitions( psDevice );
//	}
//
//	return psDevice;
//}

unsigned SCSIHandler_GetTransferLen(SCSICommand* pCommand)
{
	bool bDefault = false ;
	unsigned uiLen = 0 ;

	static const char* pLenCodes = 
		"00XLZ6XZBXBBXXXB" "00LBBLG0R0L0GG0X"
		"XXXXT8XXB4B0BBBB" "ZZZ0B00HCSSZTBHH"
		"M0HHB0X000H0HH0X" "XHH0HHXX0TH0H0XX"
		"XXXXXXXXXXXXXXXX" "XXXXXXXXXXXXXXXX"
		"XXXXXXXXXXXXXXXX" "XXXXXXXXXXXXXXXX"
		"X0XXX00XB0BXBXBB" "ZZZ0XUIDU000XHBX"
		"XXXXXXXXXXXXXXXX" "XXXXXXXXXXXXXXXX"
		"XDXXXXXXXXXXXXXX" "XXW00HXXXXXXXXXX";

	if(pCommand->iDirection == SCSI_DATA_WRITE)
		bDefault = true ;
	else
	{
		switch ( pLenCodes[ pCommand->bCommand[0] ] )
		{
			case 'L':
				uiLen = pCommand->bCommand[4] ;
				break;
			case 'M':
				uiLen = pCommand->bCommand[8] ;
				break;
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				uiLen = pLenCodes[ pCommand->bCommand[0] ] - '0' ;
				break;
			case 'G':
				uiLen = (((unsigned)pCommand->bCommand[3]) << 8) | pCommand->bCommand[4] ;
				break;
			case 'H':
				uiLen = (((unsigned)pCommand->bCommand[7]) << 8) | pCommand->bCommand[8] ;
				break;
			case 'I':
				uiLen = (((unsigned)pCommand->bCommand[8]) << 8) | pCommand->bCommand[9] ;
				break;
			case 'R':
				uiLen = (((unsigned)pCommand->bCommand[2]) << 16) | (((unsigned)pCommand->bCommand[3]) << 8) | pCommand->bCommand[4] ;
				break;
			case 'S':
				uiLen = (((unsigned)pCommand->bCommand[3]) << 16) | (((unsigned)pCommand->bCommand[4]) << 8) | pCommand->bCommand[5] ;
				break;
			case 'T':
				uiLen = (((unsigned)pCommand->bCommand[6]) << 16) | (((unsigned)pCommand->bCommand[7]) << 8) | pCommand->bCommand[8] ;
				break;
			case 'U':
				uiLen = (((unsigned)pCommand->bCommand[7]) << 16) | (((unsigned)pCommand->bCommand[8]) << 8) | pCommand->bCommand[9] ;
				break;
			case 'C':
				uiLen = (((unsigned)pCommand->bCommand[2]) << 24) | (((unsigned)pCommand->bCommand[3]) << 16) 
							| (((unsigned)pCommand->bCommand[4]) << 8) | pCommand->bCommand[5] ;
				break;
			case 'D':
				uiLen = (((unsigned)pCommand->bCommand[6]) << 24) | (((unsigned)pCommand->bCommand[7]) << 16) 
							| (((unsigned)pCommand->bCommand[8]) << 8) | pCommand->bCommand[9] ;
				break;
			case 'V':
				uiLen = 20 ;
				break;
			case 'W':
				uiLen = 24 ;
				break;
			case 'B':
			case 'X':
			case 'Z':
			default:
				bDefault = true ;
		}
	}

	if(bDefault)
		uiLen = pCommand->iRequestLen ; 

	return uiLen ;
}

int SCSIHandler_CheckSense(SCSISense* pSense)
{
	printf("\n Key: 0x%04x, Asc 0x%04x Ascq 0x%04x", pSense->SenseKey, pSense->Asc, pSense->Ascq) ;

	int iStatus ;

	switch(pSense->SenseKey)
	{
		case SCSI_NO_SENSE:
			printf("\n SCSI drive has reported SCSI_NO_SENSE") ;

			/* No problem, carry on */
			iStatus = SENSE_OK ;
			break ;

		case SCSI_RECOVERED_ERROR:
			printf("\n SCSI drive has reported SCSI_RECOVERED_ERROR" ) ;

			/* No problem, carry on */
			iStatus = SENSE_OK ;
			break ;

		case SCSI_NOT_READY:
			printf("\n SCSI drive has reported SCSI_NOT_READY" ) ;

			/* Find out why the drive is not ready */
			//TODO iStatus = SCSIHandler_UnitNotReady(pDevice, pSense) ;
			break ;

		case SCSI_MEDIUM_ERROR:
			printf("\n SCSI drive has reported SCSI_MEDIUM_ERROR" ) ;

			/* Medium errors may be transient, so we should at least try again */
			iStatus = SENSE_RETRY ;
			break ;

		case SCSI_HARDWARE_ERROR:
			printf("\n SCSI drive has reported SCSI_HARDWARE_ERROR" ) ;

			/* A hardware error is unlikely to be transient, so give up */
			iStatus = SENSE_FATAL ;
			break ;

		case SCSI_ILLEGAL_REQUEST:
			printf("\n SCSI drive has reported SCSI_ILLEGAL_REQUEST" ) ;

			/* May be caused by a drive error, a CD-DA error, or an unknown or bad raw packet command */
			iStatus = SENSE_FATAL ;
			break ;

		case SCSI_UNIT_ATTENTION:
			printf("\n SCSI drive has reported SCSI_UNIT_ATTENTION" ) ;

			/* Media changed, make a note of it and try again */
			//psDevice -> bMediaChanged = TRUE ;
			//psDevice -> bTocValid = FALSE ;
			iStatus = SENSE_RETRY ;
			break ;

		case SCSI_DATA_PROTECT:
			printf("\n SCSI drive has reported SCSI_DATA_PROTECT" ) ;

			/* Not recoverable */
			iStatus = SENSE_FATAL ;
			break ;

		case SCSI_BLANK_CHECK:
			printf("\n SCSI drive has reported SCSI_BLANK_CHECK" ) ;

			/* Not recoverable, unless this drive is update to support WORM devices */
			iStatus = SENSE_FATAL ;
			break ;

		case SCSI_VENDOR_SPECIFIC:
			printf("\n SCSI drive has reported SCSI_VENDOR_SPECIFIC" ) ;

			/* Not recoverable as we have no way to understand the error */
			iStatus = SENSE_FATAL ;
			break ;

		case SCSI_COPY_ABORTED:
			printf("\n SCSI drive has reported SCSI_COPY_ABORTED" ) ;

			/* Never going to happen */
			iStatus = SENSE_FATAL ;
			break ;

		case SCSI_ABORTED_COMMAND:
			printf("\n SCSI drive has reported SCSI_ABORTED_COMMAND" ) ;

			/* It might work again if we attempt to retry the command */
			iStatus = SENSE_RETRY ;
			break ;

		case SCSI_VOLUME_OVERFLOW:
			printf("\n SCSI drive has reported SCSI_VOLUME_OVERFLOW" ) ;

			/* Currently unhandled. Unlikely to happen. */
			iStatus = SENSE_FATAL ;
			break ;

		case SCSI_MISCOMPARE:
			printf("\n SCSI drive has reported SCSI_MISCOMPARE" ) ;

			/* Never going to happen unless this driver is updated to support WORM devices */
			iStatus = SENSE_FATAL ;
			break ;

		default:
			printf("\n SCSI drive is reporting an undefined error" ) ;

			/* The drive did something screwy, possible a deprecated sense condition, but still... */
			iStatus = SENSE_FATAL ;
	}

	return iStatus ;
}

byte SCSIHandler_RequestSense(SCSIDevice* pDevice, SCSISense* pSense)
{
	if(pSense == NULL)
	{
	  printf("\n pSense param is NULL...!");
		return SCSIHandler_FAILURE ;
	}

	/* Create and queue request sense command */
	SCSICommand sCommand(*pDevice);

	sCommand.iDirection = SCSI_DATA_READ ;
	sCommand.pRequestBuffer = (byte*)pSense ;
	sCommand.iRequestLen = sizeof(SCSISense) ;

	sCommand.bCommand[0] = SCSI_REQUEST_SENSE ;
	sCommand.bCommand[4] = sCommand.iRequestLen ;

	sCommand.iCmdLen = SCSIHandler_GetCommandSize(SCSI_REQUEST_SENSE) ;

	pDevice->pHost->QueueCommand(&sCommand) ;

	if(sCommand.u.Sense.SenseKey != SCSI_NO_SENSE || sCommand.iResult != 0)
	{
		printf("\n Could not get SCSI sense data. Sense key is 0x%04x", sCommand.u.bSense[2] & 0xF0) ;
		return SCSIHandler_FAILURE ;
	}

	return SCSIHandler_SUCCESS ;
}

SCSIDevice* SCSIHandler_ScanDevice(SCSIHost *pHost, int iChannel, int iDevice, int iLun)
{
	byte szSCSIResult[ 256 ] ;

	printf("\n Scanning device - %d:%d:%d...", iChannel, iDevice, iLun) ;

	/* Build SCSI_INQUIRY command */
	SCSICommand sCommand(pHost, iChannel, iDevice, iLun);
	memset(&sCommand, 0, sizeof(SCSICommand)) ;

	sCommand.pHost = pHost ;
	sCommand.iChannel = iChannel ;
	sCommand.iDevice = iDevice ;
	sCommand.iLun = iLun ;

	sCommand.iDirection = SCSI_DATA_READ ;

	sCommand.bCommand[ 0 ] = SCSI_INQUIRY ;

	sCommand.bCommand[ 1 ] = (iLun << 5) & 0xE0 ;

	sCommand.bCommand[ 4 ] = 36;

	sCommand.iCmdLen = SCSIHandler_GetCommandSize(SCSI_INQUIRY) ;

	sCommand.pRequestBuffer = szSCSIResult ;
	sCommand.iRequestLen = 256 ;

	/* Send command */
	bool bStatus = pHost->QueueCommand(&sCommand) ;

	printf("\nSCSI: result = %d", sCommand.iResult) ;

	/* Check result */
	if(!bStatus || sCommand.iResult != 0)
		return NULL ;

	switch( szSCSIResult[ 0 ] & 0x1F )
	{
		case SCSI_TYPE_DISK:
			printf("\nSCSI Dev Type: Disk") ;
			break ;

		case SCSI_TYPE_ROM:
			printf("\nSCSI Dev Type: ROM") ;
		default:
			printf("\nUnsupported SCSI Device: %x", szSCSIResult[ 0 ] & 0x1F) ;
			return NULL ;
	}

	return SCSIHandler_CreateDisk(pHost, iChannel, iDevice, iLun, szSCSIResult) ;
}

