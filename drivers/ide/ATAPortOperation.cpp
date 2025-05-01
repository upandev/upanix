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
# include <ATAPortOperation.h>
# include <ATAPortManager.h>
# include <PIC.h>
# include <PortCom.h>
# include <KernelUtil.h>
# include <MemConstants.h>
# include <ProcessManager.h>

#define NO_OF_PRD			256
#define PRD_TERMINATE		0x8000

byte ATAPortOperation_PreparePortDMATable(ATAPort* pPort, unsigned uiLength)
{
	auto uiTransferAddress = pPort->pDMATransferAddr;
	if(uiLength % 2)
		uiLength++ ;
	
	int iRemainingBytes = uiLength ;
	unsigned uiPRDIndex ;

	// Check if Length is too big
	if(uiLength > 0xFFFFF * 255)
		return ATAPortOperation_ERR_SIZE_TOO_BIG ;

	for(uiPRDIndex = 0; uiPRDIndex < NO_OF_PRD; uiPRDIndex++)
	{
		if(iRemainingBytes > 0)
		{
			pPort->pPRDTable[uiPRDIndex].uiAddress = uiTransferAddress ;

			if(iRemainingBytes < DMA_MEM_BLOCK_SIZE)
			{
				pPort->pPRDTable[uiPRDIndex].usLength = iRemainingBytes ;
				pPort->pPRDTable[uiPRDIndex].usPRDTerminateIndicator = PRD_TERMINATE ;
				break ;
			}

			pPort->pPRDTable[uiPRDIndex].usLength = 0x0 ;
			pPort->pPRDTable[uiPRDIndex].usPRDTerminateIndicator = 0 ;

			iRemainingBytes -= DMA_MEM_BLOCK_SIZE ;
			uiTransferAddress += DMA_MEM_BLOCK_SIZE ;
		}
		else
			break ;
	}

	if(uiPRDIndex == NO_OF_PRD)
		return ATAPortOperation_FAILURE ;

	return ATAPortOperation_SUCCESS ;
}

void ATAPortOperation_PortSelect(ATAPort* pPort, byte bAddress)
{
	UNUSED byte bControl ;
	
	if(pPort->uiPort == 0)
	{
		ATA_WRITE_REG(pPort, ATA_REG_DEVICE, ATA_DEVICE_DEFAULT | bAddress) ;
	}
	else
	{
		ATA_WRITE_REG(pPort, ATA_REG_DEVICE, ATA_DEVICE_DEFAULT | ATA_DEVICE_SLAVE | bAddress) ;
	}

	ATA_READ_REG(pPort, ATA_REG_CONTROL, bControl) ;
	// Removed Wait as it looks unwanted... 
//	KernelUtil::Wait(ATA_CMD_DELAY) ;
}

void ATAPortOperation_PortConfigure(ATAPort* pPort)
{
}

byte ATAPortOperation_PortPrepareDMARead(ATAPort* pPort, unsigned uiLength)
{
	byte bStatus ;

	if(uiLength > HDD_DMA_BUFFER_SIZE)
		return ATAPortOperation_ERR_MAX_LEN ;

	// Prepare DMA Table
	RETURN_IF_NOT(bStatus, ATAPortOperation_PreparePortDMATable(pPort, uiLength), ATAPortOperation_SUCCESS) ;

	// Write Registers 
	ATA_WRITE_DMA_REG32(pPort, ATA_REG_DMA_TABLE, (uintptr_t)pPort->pPRDTable) ;
	ATA_WRITE_DMA_REG(pPort, ATA_REG_DMA_CONTROL, ATA_DMA_CONTROL_READ) ;
	ATA_READ_DMA_REG(pPort, ATA_REG_DMA_STATUS, bStatus) ;
	ATA_WRITE_DMA_REG(pPort, ATA_REG_DMA_STATUS, bStatus | ATA_DMA_STATUS_IRQ | ATA_DMA_STATUS_ERROR) ;

	return ATAPortOperation_SUCCESS ;
}

byte ATAPortOperation_PortPrepareDMAWrite(ATAPort* pPort, unsigned uiLength)
{
	byte bStatus ;

	if(uiLength > HDD_DMA_BUFFER_SIZE)
		return ATAPortOperation_ERR_MAX_LEN ;

	// Prepare DMA Table
	RETURN_IF_NOT(bStatus, ATAPortOperation_PreparePortDMATable(pPort, uiLength), ATAPortOperation_SUCCESS) ;

	// Write Registers 
	ATA_WRITE_DMA_REG32(pPort, ATA_REG_DMA_TABLE, (uint64_t)pPort->pPRDTable)
	ATA_WRITE_DMA_REG(pPort, ATA_REG_DMA_CONTROL, ATA_DMA_CONTROL_WRITE) ;
	ATA_READ_DMA_REG(pPort, ATA_REG_DMA_STATUS, bStatus) ;
	ATA_WRITE_DMA_REG(pPort, ATA_REG_DMA_STATUS, bStatus | ATA_DMA_STATUS_IRQ | ATA_DMA_STATUS_ERROR) ;

	return ATAPortOperation_SUCCESS ;
}

byte ATAPortOperation_StartDMA(ATAPort* pPort)
{
	byte bControl ;
	byte bStatus ;

//	ProcessManager_SignalInterruptOccured(HD_IRQ) ;

	// Start Transfer
	ATA_READ_DMA_REG(pPort, ATA_REG_DMA_CONTROL, bControl) ;
	ATA_WRITE_DMA_REG(pPort, ATA_REG_DMA_CONTROL, bControl | ATA_DMA_CONTROL_START) ;

	if(IsKernel())
		KernelUtil::WaitOnInterrupt(ATADeviceController_GetHDInterruptNo(pPort)) ;
	else
		ProcessManager::Instance().WaitOnInterrupt(ATADeviceController_GetHDInterruptNo(pPort)) ;

	bStatus = 0 ;
	
	ATA_READ_DMA_REG(pPort, ATA_REG_DMA_CONTROL, bControl) ;
	ATA_WRITE_DMA_REG(pPort, ATA_REG_DMA_CONTROL, bControl & ~ATA_DMA_CONTROL_START) ;

	ATA_READ_DMA_REG(pPort, ATA_REG_DMA_STATUS, bStatus) ;
	ATA_WRITE_DMA_REG(pPort, ATA_REG_DMA_STATUS, bStatus | ATA_DMA_STATUS_IRQ | ATA_DMA_STATUS_ERROR) ;

	ATA_READ_REG(pPort, ATA_REG_CONTROL, bControl) ;

	return ((bStatus & (ATA_DMA_STATUS_ERROR | ATA_DMA_STATUS_IRQ))) == ATA_DMA_STATUS_IRQ ?  ATAPortOperation_SUCCESS : ATAPortOperation_FAILURE ;
}

void ATAPortOperation_CopyToUserBufferFromKernelBuffer(ATAPort* pPort, byte* pBuffer, unsigned uiLength)
{
  memcpy(pBuffer, (void*)pPort->pDMATransferAddr, uiLength);
}

void ATAPortOperation_CopyToKernelBufferFromUserBuffer(ATAPort* pPort, byte* pBuffer, unsigned uiLength)
{
  memcpy((void*)pPort->pDMATransferAddr, pBuffer, uiLength);
}

byte ATAPortOperation_PortReset(ATAPort* pPort)
{
	unsigned uiCount, uiLBALow ;

	// Select Drive
	ATA_WRITE_REG(pPort, ATA_REG_CONTROL, ATA_CONTROL_DEFAULT) ;
	if(pPort->uiPort == 0)
		ATA_WRITE_REG(pPort, ATA_REG_DEVICE, ATA_DEVICE_DEFAULT)
	else
		ATA_WRITE_REG(pPort, ATA_REG_DEVICE, ATA_DEVICE_DEFAULT | ATA_DEVICE_SLAVE) ;

	KernelUtil::Wait(ATA_CMD_DELAY) ;

	// Reset
	ATA_WRITE_REG(pPort, ATA_REG_COUNT, 0x55) ;
	ATA_WRITE_REG(pPort, ATA_REG_LBA_LOW, 0xAA) ;
	ATA_WRITE_REG(pPort, ATA_REG_COUNT, 0x55) ;
	ATA_WRITE_REG(pPort, ATA_REG_LBA_LOW, 0xAA) ;
	ATA_WRITE_REG(pPort, ATA_REG_COUNT, 0x55) ;
	ATA_WRITE_REG(pPort, ATA_REG_LBA_LOW, 0xAA) ;
	
	// Check Registers
	ATA_READ_REG(pPort, ATA_REG_COUNT, uiCount) ;
	ATA_READ_REG(pPort, ATA_REG_LBA_LOW, uiLBALow) ;

	if(!(uiCount == 0x55 && uiLBALow == 0xAA))
		return ATAPortOperation_FAILURE ;

	if(pPort->uiPort == 0)
		ATA_WRITE_REG(pPort, ATA_REG_DEVICE, ATA_DEVICE_DEFAULT)
	else
		ATA_WRITE_REG(pPort, ATA_REG_DEVICE, ATA_DEVICE_DEFAULT | ATA_DEVICE_SLAVE)

	KernelUtil::Wait(ATA_CMD_DELAY) ;

	// Drive Reset
	ATA_WRITE_REG(pPort, ATA_REG_CONTROL, ATA_CONTROL_DEFAULT | ATA_CONTROL_RESET) ;
	KernelUtil::Wait(10) ;
	ATA_WRITE_REG(pPort, ATA_REG_CONTROL, ATA_CONTROL_DEFAULT) ;
	KernelUtil::Wait(10) ;
	
	RETURN_X_IF_NOT(ATAPortManager_IOWait(pPort, ATA_STATUS_BUSY, 0), ATAPortManager_SUCCESS, ATAPortOperation_FAILURE) ;

	// Select Again
	if(pPort->uiPort == 0)
		ATA_WRITE_REG(pPort, ATA_REG_DEVICE, ATA_DEVICE_DEFAULT) 
	else
		ATA_WRITE_REG(pPort, ATA_REG_DEVICE, ATA_DEVICE_DEFAULT | ATA_DEVICE_SLAVE) 

	KernelUtil::Wait(ATA_CMD_DELAY) ;

	ATA_READ_REG(pPort, ATA_REG_COUNT, uiCount) ;
	ATA_READ_REG(pPort, ATA_REG_LBA_LOW, uiLBALow) ;

	if(!(uiCount == 0x01 && uiLBALow == 0x01))
		return ATAPortOperation_FAILURE ;

	KernelUtil::Wait(50) ;

	return ATAPortOperation_SUCCESS ;
}

