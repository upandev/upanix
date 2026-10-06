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
# include <ATACommandManager.h>
# include <ATAPortManager.h>
# include <ATAPortOperation.h>
# include <PortCom.h>
# include <KernelUtil.h>
# include <TscClock.h>

void ATACommandManager_ExecuteATACommand(ATACommand* pCommand)
{
	byte bWrite = false ;
	byte bDMA = false ;

	byte* pBuffer = pCommand->pTransferBuffer ;
	unsigned uiLength = pCommand->uiTransferLength ;

	ATAPort* pPort = pCommand->pPort ;

	byte bCommand ;
	UNUSED byte bControl ;
	byte bStatus ;
	int iRetry ;

	while(true)
	{
		pCommand->iStatus = -1 ;
		pCommand->iError = ATACommandManager_ERR_IO ;

		//Change DMA and WRITE Flag if required
		if(pCommand->bCommand[ATA_REG_COMMAND] == ATA_CMD_WRITE_PIO)
			bWrite = true ;

		if(pPort->uiCurrentSpeed >= ATA_SPEED_DMA)
			bDMA = true ;

		// Wait
		if(ATAPortManager_IOWait(pPort, ATA_STATUS_BUSY, 0) != ATAPortManager_SUCCESS)
		{
			ATA_READ_REG(pPort, ATA_REG_ERROR, pCommand->iError) ;
			printf("\n\tFailed to Execute Command %u", pCommand->bCommand[ATA_REG_COMMAND]);
			return ;
		}

		// Select Port
		pPort->portOperation.Select(pPort, pCommand->bCommand[ATA_REG_DEVICE]) ;

		// Write Control Register
		if(bDMA)
		{
			ATA_WRITE_REG(pPort, ATA_REG_CONTROL, pCommand->bCommand[ATA_REG_CONTROL]) ;
		}
		else
		{
			ATA_WRITE_REG(pPort, ATA_REG_CONTROL, pCommand->bCommand[ATA_REG_CONTROL] | ATA_CONTROL_INTDISABLE) ;
		}

		if(pPort->bLBA48Bit)
		{
			//Write 48 bit registers
			ATA_WRITE_REG(pPort, ATA_REG_FEATURE, pCommand->bCommand[9]) ;
			ATA_WRITE_REG(pPort, ATA_REG_COUNT, pCommand->bCommand[10]) ;
			ATA_WRITE_REG(pPort, ATA_REG_LBA_LOW, pCommand->bCommand[11]) ;
			ATA_WRITE_REG(pPort, ATA_REG_LBA_MID, pCommand->bCommand[12]) ;
			ATA_WRITE_REG(pPort, ATA_REG_LBA_HIGH, pCommand->bCommand[13]) ;
		}
		//TODO: Shouldn't this Write be in Else Part
		//Write Standard Registers
		ATA_WRITE_REG(pPort, ATA_REG_FEATURE, pCommand->bCommand[ATA_REG_FEATURE]) ;
		ATA_WRITE_REG(pPort, ATA_REG_COUNT, pCommand->bCommand[ATA_REG_COUNT]) ;
		ATA_WRITE_REG(pPort, ATA_REG_LBA_LOW, pCommand->bCommand[ATA_REG_LBA_LOW]) ;
		ATA_WRITE_REG(pPort, ATA_REG_LBA_MID, pCommand->bCommand[ATA_REG_LBA_MID]) ;
		ATA_WRITE_REG(pPort, ATA_REG_LBA_HIGH, pCommand->bCommand[ATA_REG_LBA_HIGH]) ;

		// Wait
		if(ATAPortManager_IOWait(pPort, ATA_STATUS_BUSY, 0) != ATAPortManager_SUCCESS)
		{
			ATA_READ_REG(pPort, ATA_REG_ERROR, pCommand->iError) ;
			printf("\n\tFailed to Execute Command %u", pCommand->bCommand[ATA_REG_COMMAND]);
			return ;
		}

		// Select Command
		if(bDMA)
		{
			if(bWrite)
				bCommand = (pPort->bLBA48Bit) ? ATA_CMD_WRITE_DMA_48 : ATA_CMD_WRITE_DMA ;
			else
				bCommand = (pPort->bLBA48Bit) ? ATA_CMD_READ_DMA_48 : ATA_CMD_READ_DMA ;
		}
		else
		{
			if(bWrite)
				bCommand = (pPort->bLBA48Bit) ? ATA_CMD_WRITE_PIO_48 : ATA_CMD_WRITE_PIO ;
			else
				bCommand = (pPort->bLBA48Bit) ? ATA_CMD_READ_PIO_48 : ATA_CMD_READ_PIO ;
		}

		if(bDMA)
		{
			// initialize DMA Transfer
	
			for(iRetry = 0; iRetry < 3; iRetry++)
			{
				bStatus = (bWrite) ? pPort->portOperation.PrepareDMAWrite(pPort, uiLength) :
									 pPort->portOperation.PrepareDMARead(pPort, uiLength) ;

				if(bStatus == ATAPortOperation_SUCCESS)
					break ;
			}

			if(iRetry == 3)
			{
			  printf("\n\t Failed to Prepare the DMA Table");
			  printf("\n\t Trying to Use PIO Mode");

				pPort->uiCurrentSpeed = ATA_SPEED_PIO ;
				bDMA = false ;
				continue ;
			}
				
			// Put Command 
			ATA_WRITE_REG(pPort, ATA_REG_COMMAND, bCommand) ;
			ATA_READ_REG(pPort, ATA_REG_COMMAND, bControl) ;

			if(pPort->portOperation.FlushRegs)
				pPort->portOperation.FlushRegs(pPort) ;

			if(bWrite)
				ATAPortOperation_CopyToKernelBufferFromUserBuffer(pPort, pBuffer, uiLength) ;
				
			// Start DMA Transfer
			for(iRetry = 0; iRetry < 3; iRetry++)
			{
				bStatus = pPort->portOperation.StartDMA(pPort) ;

				if(bStatus == ATAPortOperation_SUCCESS)
					break ;
			}

			if(!bWrite)
				ATAPortOperation_CopyToUserBufferFromKernelBuffer(pPort, pBuffer, uiLength) ;

			if(iRetry == 3)
			{
			  printf("\n\t Failed to Perform DMA Tranfer");
			  printf("\n\t Trying to Use PIO Mode");

				pPort->uiCurrentSpeed = ATA_SPEED_PIO ;
				bDMA = false ;
				continue ;
			}

			pCommand->iStatus = pCommand->iError = ATACommandManager_SUCCESS ;
			return ;
		}
		else // PIO Transfer
		{
			// Put Command
			ATA_WRITE_REG(pPort, ATA_REG_COMMAND, bCommand) ;
			ATA_READ_REG(pPort, ATA_REG_CONTROL, bControl) ;

      TscClock::instance().busyWait(ATA_CMD_DELAY);

			if(pPort->portOperation.FlushRegs)
				pPort->portOperation.FlushRegs(pPort) ;

			iRetry = 0 ;
			unsigned uiTransfered = 0 ;
			while(uiTransfered < uiLength)
			{
				for(iRetry = 0; iRetry < 3; iRetry++)
				{
					bStatus = (bWrite) ? ATAPortManager_IOWrite(pPort, pBuffer, 512) :
										 ATAPortManager_IORead(pPort, pBuffer, 512) ;
				
					if(bStatus == ATAPortManager_SUCCESS)
						break ;
				}

				if(iRetry == 3)
				{
					printf("\n\t Failed to execute Command %u", pCommand->bCommand[ATA_REG_COMMAND]) ;
					return ;
				}

				pBuffer += 512 ;
				uiTransfered += 512 ;
			}

			pCommand->iStatus = pCommand->iError = ATACommandManager_SUCCESS ;
			return ;
		}	
	}
}

void ATACommandManager_ExecuteATAPICommand(ATACommand* pCommand) 
{
	byte bDMA = false ;

	ATAPort* pPort = pCommand->pPort ;
	byte *pBuffer = pCommand->pTransferBuffer ;
	unsigned uiLength = pCommand->uiTransferLength ;

	unsigned short* pCmd = (unsigned short*)&(pCommand->bCommand[0]) ;

	UNUSED byte bControl ;
	byte bStatus ;
	int iRetry ;
	unsigned i ;

	while(true)
	{
		pCommand->iStatus = -1 ;
		pCommand->iError = ATACommandManager_ERR_IO ;

		// Change DMA Flag of required
		if(pPort->uiCurrentSpeed >= ATA_SPEED_DMA && pCommand->bCanDMA)
			bDMA = true ;

		// Select Port
		pPort->portOperation.Select(pPort, 0) ;

		//Wait
		if(ATAPortManager_IOWait(pPort, ATA_STATUS_BUSY, 0) != ATAPortManager_SUCCESS)
			return ;
		
		//Write ATAPI Command Registers
		ATA_WRITE_REG(pPort, ATA_REG_FEATURE, bDMA ? ATAPI_FEATURE_DMA : 0) ;
		ATA_WRITE_REG(pPort, ATAPI_REG_IRR, 0) ;
		ATA_WRITE_REG(pPort, ATAPI_REG_SAMTAG, 0) ;
		ATA_WRITE_REG(pPort, ATAPI_REG_COUNT_LOW, uiLength & 0xFF) ;
		ATA_WRITE_REG(pPort, ATAPI_REG_COUNT_HIGH, (uiLength >> 8) & 0xFF) ;

		// Write Control Registers
		if(bDMA)
		{
			ATA_WRITE_REG(pPort, ATA_REG_CONTROL, ATA_CONTROL_DEFAULT) ;
		}
		else
		{
			ATA_WRITE_REG(pPort, ATA_REG_CONTROL, ATA_CONTROL_DEFAULT | ATA_CONTROL_INTDISABLE) ;
		}

		// Prepare DMA Transfer
		if(bDMA)
		{
			for(iRetry = 0; iRetry < 3; iRetry++)
			{
				bStatus = pPort->portOperation.PrepareDMARead(pPort, uiLength) ;
				if(bStatus == ATAPortOperation_SUCCESS)
					break ;
			}

			if(iRetry == 3)
			{
			  printf("\n\t Failed to Prepare the DMA Table");
			  printf("\n\t Trying to Use PIO Mode");

				pPort->uiCurrentSpeed = ATA_SPEED_PIO ;
				bDMA = false ;
				continue ;
			}
		}

		//Put Command
		ATA_WRITE_REG(pPort, ATA_REG_COMMAND, ATAPI_CMD_PACKET) ;
		ATA_READ_REG(pPort, ATA_REG_CONTROL, bControl) ;

		if(pPort->portOperation.FlushRegs)
			pPort->portOperation.FlushRegs(pPort) ;

		// Wait
		if(ATAPortManager_IOWait(pPort, ATA_STATUS_BUSY, 0) != ATAPortManager_SUCCESS)
			return ;

		ATA_READ_REG(pPort, ATA_REG_STATUS, bStatus) ;

		if(bStatus & ATA_STATUS_ERROR)
		{
			ATA_READ_REG(pPort, ATA_REG_ERROR, pCommand->iError) ;
			pCommand->atapiSense.bSenseKey = pCommand->iError >> 4 ;
			return ;
		}

		// Wait
		if(ATAPortManager_IOWait(pPort, ATA_STATUS_DRQ, ATA_STATUS_DRQ) != ATAPortManager_SUCCESS)
			return ;

		if(bStatus & ATA_STATUS_DRQ)
		{
			// Transfer Command
			if(pPort->bPIO32Bit)
			{
				unsigned* pCmd32 = (unsigned*)pCmd ;

				for(i = 0; i < 3; i++, pCmd32++)
				{
					ATA_WRITE_REG32(pPort, ATA_REG_DATA, *pCmd32) ;
				}
			}
			else
			{
				for(i = 0; i < 6; i++, pCmd++)
				{
					ATA_WRITE_REG16(pPort, ATA_REG_DATA, *pCmd) ;
				}
			}

			// Transfer Data
			if(uiLength > 0)
			{
				if(bDMA)
				{
					//Start Transfer
					if(pPort->portOperation.StartDMA(pPort) != ATAPortManager_SUCCESS)
					{
					  printf("\n\t Failed to Prepare the DMA Table");
					  printf("\n\t Trying to Use PIO Mode");

						pPort->uiCurrentSpeed = ATA_SPEED_PIO ;
						bDMA = false ;
						continue ;
					}

					ATAPortOperation_CopyToUserBufferFromKernelBuffer(pPort, pBuffer, uiLength) ;

					pCommand->iStatus = pCommand->iError = ATACommandManager_SUCCESS ;
					return ;
				}

				byte bTimedOut = true ;

				for(i = 0; i < 2; i++)
				{
					ATA_READ_REG(pPort, ATA_REG_STATUS, bStatus) ;

					// Wait for BUSY to clear, DRQ or ERR to be Set
					if(!(bStatus & ATA_STATUS_BUSY) && ((bStatus & ATA_STATUS_DRQ) || (bStatus & ATA_STATUS_ERROR)))
					{
						bTimedOut = false ;
						break ;
					}

          TscClock::instance().busyWait(ATA_CMD_TIMEOUT);
				}

				if(bTimedOut)
				{
					if(uiLength > 0)
						printf("\n\tData Transfer Request Timed Out. Err Status = %u", bStatus) ;
					return ;
				}

				if(bStatus & ATA_STATUS_ERROR)
				{
				  printf("\n\tData Transfer Failed");
					ATA_READ_REG(pPort, ATA_REG_ERROR, pCommand->iError) ;
					pCommand->atapiSense.bSenseKey = pCommand->iError >> 4 ;
					return ;
				}

				unsigned uiTransfered = 0 ;
				byte bLow, bHigh ;
				unsigned uiCurrent ;

				while(uiTransfered < uiLength) 
				{
					// Wait
					if(ATAPortManager_IOWaitAlt(pPort, ATA_STATUS_DRQ, ATA_STATUS_DRQ) != ATAPortManager_SUCCESS)
						return ;

					// Read Position
					ATA_READ_REG(pPort, ATAPI_REG_COUNT_LOW, bLow) ;
					ATA_READ_REG(pPort, ATAPI_REG_COUNT_HIGH, bHigh) ;

					uiCurrent = bLow + ( bHigh * 256 ) ;	

					if(uiCurrent == 0)
					{
					  printf("\n\tDRQ is high but the Drive Reports 0 bytes Available");
						ATA_READ_REG(pPort, ATA_REG_ERROR, pCommand->iError) ;
						pCommand->atapiSense.bSenseKey = pCommand->iError >> 4 ;
						return ;
					}

					if(uiCurrent > uiLength)
						uiCurrent = uiLength ;

					// Perform 32 Bit PIO Trasnfer only if Drive Supports PIO 32 Bit and Data Size is even multiple
					if(pPort->bPIO32Bit && (uiCurrent % 4 == 0))
					{
						unsigned* pBuffer32 = (unsigned*)pBuffer ;
						for(i = 0; i < uiCurrent / 4; i++, pBuffer32++)
						{
							ATA_READ_REG32(pPort, ATA_REG_DATA, *pBuffer32) ;
							pBuffer += uiCurrent / 2 ;
						}
					}
					else
					{
						for(i = 0; i < uiCurrent / 2; i++, pBuffer++)
						{
							ATA_READ_REG16(pPort, ATA_REG_DATA, *pBuffer) ;
						}
					}

					uiTransfered += uiCurrent ;
			
					if(uiTransfered < uiLength)
					{
						// Wait
						if(ATAPortManager_IOWait(pPort, ATA_STATUS_DRQ, ATA_STATUS_DRQ) != ATAPortManager_SUCCESS)
						{
              printf("\n\tPartial Transfer. DRQ is Low, Bytes Left = %u", uiLength - uiTransfered) ;
							break ;	
						}
					}
				}

				// Wait for Command Completion; BUSY & DRQ Low and DRDY High
				bTimedOut = true ;

				byte bStatus = 0 ;
				byte i ;

				for(i = 0; i < 200; i++) {
          TscClock::instance().busyWait(ATA_CMD_TIMEOUT);

					ATA_READ_REG(pPort, ATA_REG_STATUS, bStatus) ;

					if(!(bStatus & ATA_STATUS_BUSY) && !(bStatus & ATA_STATUS_DRQ) && (bStatus & ATA_STATUS_DRDY))
					{
						bTimedOut = false ;
						break ;
					}
				}

				if(bTimedOut)
				{
				  printf("\n\tTimed out waiting for Command to Complete");
					return ;
				}
			}
			else
			{
				// Command Without Data Transfer
				if(ATAPortManager_IOWaitAlt(pPort, ATA_STATUS_BUSY, 0) != ATAPortManager_SUCCESS)
					return ;

				ATA_READ_REG(pPort, ATA_REG_ERROR, pCommand->iError) ;
				pCommand->atapiSense.bSenseKey = pCommand->iError >> 4 ;
				pCommand->iStatus = ATACommandManager_SUCCESS ;

        printf("\n\tNo Data to Transfer. Sense Key = %u", pCommand->atapiSense.bSenseKey) ;
			}
		}
	}
}

void ATACommandManager_InitCommand(ATACommand* pCommand, ATAPort* pPort)
{
  memset((byte*)pCommand, 0, sizeof(ATACommand));
	pCommand->pPort = pPort ;
}

void ATACommandManager_Queue(ATACommand* pCommand)
{
	// Direct Mode
	if(pCommand->pPort->uiDevice == ATA_DEV_ATAPI)
		ATACommandManager_ExecuteATAPICommand(pCommand) ;
	else
		ATACommandManager_ExecuteATACommand(pCommand) ;
}

