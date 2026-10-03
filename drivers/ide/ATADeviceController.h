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
#ifndef _ATA_DEVICE_CONTROLLER_H_
#define _ATA_DEVICE_CONTROLLER_H_

#include <PCIBusHandler.h>
#include <PIC.h>

#define ATADeviceController_SUCCESS					0
#define ATADeviceController_ERR_ENABLE_CONTROLLER	1
#define ATADeviceController_ERR_SWITCH_NATIVE_MODE	2
#define ATADeviceController_FAILURE					3

#define ATA_CMD_READ_PIO		0x20
#define ATA_CMD_READ_PIO_48		0x24
#define ATA_CMD_READ_DMA_48		0x25
#define ATA_CMD_WRITE_PIO		0x30
#define ATA_CMD_WRITE_PIO_48	0x34
#define ATA_CMD_WRITE_DMA_48	0x35
#define ATA_CMD_READ_DMA		0xC8
#define ATA_CMD_WRITE_DMA		0xCA

#define ATA_DEVICE_SLAVE	0x10
#define ATA_DEVICE_LBA		0x40
#define ATA_DEVICE_DEFAULT	0xA0

#define ATA_DMA_STATUS_ERROR	0x02
#define ATA_DMA_STATUS_IRQ		0x04
#define ATA_DMA_CONTROL_READ	0x08
#define ATA_DMA_CONTROL_WRITE	0x00
#define ATA_DMA_CONTROL_START	0x01

#define ATA_DMA_STATUS_0_EN	0x20
#define ATA_DMA_STATUS_1_EN	0x40 

#define ATA_MAX_PORTS	8

//in micro-seconds
#define ATA_CMD_DELAY     20000
#define ATA_CMD_TIMEOUT		18000

#define ATA_CMD_SET_FEATURES	0xEF
#define ATA_CMD_IDENTIFY		0xEC

#define ATA_STATUS_ERROR	0x01
#define ATA_STATUS_DRQ		0x08
#define ATA_STATUS_BUSY		0x80
#define ATA_STATUS_DRDY		0x40

#define ATA_CONTROL_INTDISABLE	0x02
#define ATA_CONTROL_RESET		0x04
#define ATA_CONTROL_DEFAULT		0x08

#define ATA_XFER_PIO_3		0x0B
#define ATA_XFER_MWDMA_0	0x20
#define ATA_XFER_UDMA_0		0x40

#define ATAPI_CMD_IDENTIFY	0xA1
#define ATAPI_CMD_RESET		0x08
#define ATAPI_FEATURE_DMA	0x01
#define ATAPI_CMD_PACKET	0xA0

#define DMA_MEM_BLOCK_SIZE	65536 // 64 KB
#define HDD_DMA_BUFFER_SIZE 0x80000 // 1 MB

enum ATAPIRegs
{
	ATAPI_REG_IRR = 2,
	ATAPI_REG_SAMTAG,
	ATAPI_REG_COUNT_LOW,
	ATAPI_REG_COUNT_HIGH,
	ATAPI_TOTAL_REGS
} ;

enum ATARegs
{
	ATA_REG_DATA,
	ATA_REG_ERROR,
	ATA_REG_COUNT,
	ATA_REG_LBA_LOW,
	ATA_REG_LBA_MID,
	ATA_REG_LBA_HIGH,
	ATA_REG_DEVICE,
	ATA_REG_STATUS,
	ATA_REG_CONTROL,
	ATA_TOTAL_REGS,
	ATA_REG_FEATURE = ATA_REG_ERROR,
	ATA_REG_COMMAND = ATA_REG_STATUS,
	ATA_REG_ALTSTATUS = ATA_REG_CONTROL
} ;

enum ATADMARegs
{
	ATA_REG_DMA_CONTROL,
	ATA_REG_DMA_STATUS,
	ATA_REG_DMA_TABLE,
	ATA_DMA_TOTAL_REGS
} ;

enum ATASpeed
{
	ATA_SPEED_PIO,
	ATA_SPEED_PIO_3,
	ATA_SPEED_PIO_4,
	ATA_SPEED_PIO_5,
	ATA_SPEED_DMA,
	ATA_SPEED_MWDMA_0,
	ATA_SPEED_MWDMA_1,
	ATA_SPEED_MWDMA_2,
	ATA_SPEED_UDMA_0,
	ATA_SPEED_UDMA_1,
	ATA_SPEED_UDMA_2,
	ATA_SPEED_UDMA_3,
	ATA_SPEED_UDMA_4,
	ATA_SPEED_UDMA_5,
	ATA_SPEED_UDMA_6,
	ATA_SPEED_UDMA_7,
	ATA_SPEED_HIGHEST = ATA_SPEED_UDMA_7
} ;

enum ATAType
{
	ATA_PATA,
	ATA_SATA
} ;

enum ATACable
{
	ATA_CABLE_NONE,
	ATA_CABLE_UNKNOWN,
	ATA_CABLE_PATA40,
	ATA_CABLE_PATA80,
	ATA_CABLE_SATA
} ;

enum ATADevice
{
	ATA_DEV_NONE,
	ATA_DEV_UNKNOWN,
	ATA_DEV_ATA,
	ATA_DEV_ATAPI
} ;

typedef struct ATAPort ATAPort ;
typedef struct ATAController ATAController ;
typedef struct ATAPortOperations ATAPortOperations ;
typedef struct ATAIdentifyInfo ATAIdentifyInfo ;
typedef struct ATAPCIDevice ATAPCIDevice ;
typedef struct ATAPRD ATAPRD ;

typedef void VendorSpecificATAController_Initialize(const PCIEntry* pPCIEntry, ATAController* pController) ;

struct ATAPRD
{
	uint64_t uiAddress ; // AD[31:1], Bit 0 is always 0
	unsigned short usLength ; // Bit 0 Not used, AD[15:1] - byte count
	unsigned short usPRDTerminateIndicator ; // AD[16:30] - Reserved, Bit 31 - EOT (1->Terminate, 0->Continue)
} PACKED ;

struct ATAController
{
	struct ATAController* pNextATAController ;

	char szName[32] ;
	unsigned uiChannels ;
	unsigned uiPortsPerChannel ;

	ATAPort* pPort[ATA_MAX_PORTS] ;
} ;

struct ATAPortOperations
{
	byte (*Reset)(ATAPort* pPort) ;
  void (*Configure)(ATAPort* pPort) ;
	void (*Select)(ATAPort* pPort, byte bAddress) ;
	byte (*PrepareDMARead)(ATAPort* pPort, unsigned uiLength) ;
	byte (*PrepareDMAWrite)(ATAPort* pPort, unsigned uiLength) ;
	byte (*StartDMA)(ATAPort* pPort) ;
	void (*FlushRegs)(ATAPort* pPort) ;
} ;

struct ATAIdentifyInfo 
{
	unsigned short usConfiguration ;
	unsigned short usCylinders ;
	unsigned short usReserved01 ;
	unsigned short usHead ;
	unsigned short usTrackBytes ;

	unsigned short usSectorBytes ;
	unsigned short usSectors ;
	unsigned short usReserved03[3] ;

	byte bSerialNumber[20] ;
	unsigned short usBufType ;
	unsigned short usBufSize ;
	unsigned short usECCBytes ;
	byte bRevision[8] ;
	
	char szModelID[40] ;
	byte bSectorsPerRWLong ;
	byte bReserved05 ;
	unsigned short usReserved06 ;
	byte bReserved07 ;

	byte bCapabilities ;
	unsigned short usReserved08 ;
	byte bReserved09 ;
	byte bPIOCycleTime ;
	byte bReserved10 ;

	byte bDMA ;
	unsigned short usValid ;
	unsigned short usCurrentCylinders ;
	unsigned short usCurrentHeads ;
	unsigned short usCurrentSectors ;

	unsigned short usCurrentCapacity0 ;
	unsigned short usCurrentCapacity1 ;
	byte bSectorsPerRWIrq ;
	byte bSectorsPerRWIrqValid ;
	unsigned uiLBASectors ;

	unsigned short usSingleWordDMAInfo ;
	unsigned short usMultiWordDMAInfo ;
	unsigned short usEIDEPIOModes ;
	unsigned short usEIDEDMAMin ;
	unsigned short usEIDEDMATime ;

	unsigned short usEIDEPIO ;
	unsigned short usEIDEPIOOrdy ;
	unsigned short usReserved13[2] ;
	unsigned short usReserved14[4] ;

	unsigned short usCommandQueueDepth ;
	unsigned short usReserved15[4] ;
	unsigned short usMajor ;
	unsigned short usMinor ;
	unsigned short usCommandSet1 ;
	unsigned short usCommandSet2 ;

	unsigned short usCommandSetFeaturesExtensions ;
	unsigned short usCommandSetFeaturesEnable1 ;
	unsigned short usCommandSetFeaturesEnable2 ;
	unsigned short usCommandSetFeaturesDefault ;

	unsigned short usUltraDMAModes ;
	
	unsigned short usReserved16[2] ;
	unsigned short usAdvancedPowerManagement ;

	unsigned short usReserved17 ;
	unsigned short usHardwareConfig ;
	
	unsigned short usAcoustic ;
	unsigned short usReserved25[5] ;
	
	unsigned uiLBACapacity48 ;

	unsigned short usReserved18[22] ;
	unsigned short usLastLUN ;
	unsigned short usReserved19 ;
	unsigned short usDeviceLockFunctions ;
	unsigned short usCurrentSetFeaturesOptions ;

	unsigned short usReserved20[26] ;
	unsigned short usReserved21 ;
	unsigned short usReserved22[3] ;
	unsigned short usReserved23[95] ;
} ;

struct ATAPort
{
	ATAController* pController ;
	unsigned uiChannel ;
	unsigned uiPort ;
	ATAPortOperations portOperation ;
	unsigned uiCable ;
	unsigned uiDevice ;
	byte bConfigured ;
	byte bMMIO ;
	unsigned uiType ;
	unsigned uiID ;
	unsigned uiCurrentSpeed ;
	unsigned uiSupportedPortSpeed ;
	unsigned uiSupportedDriveSpeed ;
	byte bPIO32Bit ;
	byte bLBA48Bit ;
	byte* pDataBuffer ;
	char szDeviceName[32] ;
	uintptr_t uiDMARegs[ ATA_DMA_TOTAL_REGS ] ;
	uintptr_t uiRegs[ ATA_TOTAL_REGS ] ;
	ATAIdentifyInfo id ;
	void* DeviceStruct ;
	void* pVendorSpecInfo ;

	ATAPRD* pPRDTable ;
	uint64_t pDMATransferAddr ;
} ;

struct ATAPCIDevice
{
    unsigned short usVendorID ;
	unsigned short usDeviceID ;
	VendorSpecificATAController_Initialize* InitFunc ;
} ;

void ATADeviceController_Initialize() ;
bool ATADeviceController_GetInitStatus() ;
unsigned ATADeviceController_GetDeviceSectorLimit(ATAPort* pPort) ;
const IRQ& ATADeviceController_GetHDInterruptNo(ATAPort* pPort) ;
void ATADeviceController_PrimaryIRQHandler();
void ATADeviceController_SecondaryIRQHandler();

#define ATA_READ_REG(port, reg, value) \
{ \
	if(port->bMMIO != 0) \
	{ \
		value = *((byte*)port->uiRegs[reg]) ;\
	} \
	else \
	{ \
		value = PortCom_ReceiveByte(port->uiRegs[reg]) ; \
	} \
}

#define ATA_READ_DMA_REG(port, reg, value) \
{ \
	if(port->bMMIO != 0) \
	{ \
		value = *((byte*)port->uiDMARegs[reg]) ; \
	} \
	else \
	{ \
		value = PortCom_ReceiveByte(port->uiDMARegs[reg]) ; \
	} \
}

#define ATA_READ_REG16(port, reg, value) \
{ \
	if(port->bMMIO != 0) \
	{ \
		value = *((unsigned short*)port->uiRegs[reg]) ; \
	} \
	else \
	{ \
		value = PortCom_ReceiveWord(port->uiRegs[reg]) ; \
	} \
}

#define ATA_READ_REG32(port, reg, value) \
{ \
	if(port->bMMIO != 0) \
	{ \
		value = *((unsigned*)port->uiRegs[reg]) ; \
	} \
	else \
	{ \
		value = PortCom_ReceiveDoubleWord(port->uiRegs[reg]) ; \
	} \
}

#define ATA_WRITE_REG(port, reg, value) \
{ \
	if(port->bMMIO != 0) \
	{ \
		*((byte*)port->uiRegs[reg]) = value ; \
	} \
	else \
	{ \
		PortCom_SendByte(port->uiRegs[reg], value) ; \
	} \
}

#define ATA_WRITE_REG16(port, reg, value) \
{ \
	if(port->bMMIO != 0) \
	{ \
		*((unsigned short*)port->uiRegs[reg]) = value ; \
	} \
	else \
	{ \
		PortCom_SendWord(port->uiRegs[reg], value) ; \
	} \
}

#define ATA_WRITE_REG32(port, reg, value) \
{ \
	if(port->bMMIO != 0) \
	{ \
		*((unsigned*)port->uiRegs[reg]) = value ; \
	} \
	else \
	{ \
		PortCom_SendDoubleWord(port->uiRegs[reg], value) ; \
	} \
}

#define ATA_WRITE_DMA_REG(port, reg, value) \
{ \
	if(port->bMMIO != 0) \
	{ \
		*((byte*)port->uiDMARegs[reg]) = value ; \
	} \
	else \
	{ \
		PortCom_SendByte(port->uiDMARegs[reg], value) ; \
	} \
}

#define ATA_WRITE_DMA_REG32(port, reg, value) \
{ \
	if(port->bMMIO != 0) \
	{ \
		*((unsigned*)port->uiDMARegs[reg]) = value ; \
	} \
	else \
	{ \
		PortCom_SendDoubleWord(port->uiDMARegs[reg], value) ; \
	} \
}

#endif
