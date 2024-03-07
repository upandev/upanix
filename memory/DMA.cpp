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

// INTEL 8237A DMA CHIP
#include <DMA.h>
#include <MemConstants.h>
#include <IrqManager.h>

#define DMA_CMD_REG_W 0x08
#define DMA_STATUS_REG_R 0x08
#define DMA_REQ_REG_W 0x09
#define DMA_SINGLE_MASK_REG_W 0x0A
#define DMA_MODE_REG_W 0x0B
#define DMA_TEMP_REG_R 0x0D
#define DMA_ALL_MASK_REG_W 0x0F

#define DMA_FFCLEAR_SWCMD 0x0C
#define DMA_MASTER_CLEAR_SWCMD 0x0D
#define DMA_CLEAR_MASK_REG_SWCMD 0x0E

#define DMA_COMMAND 0x10 //DACK sense active low(0), DREQ sense active high(0), Late Write Selection(0),
//Rotating Priority(1), Normal Timing(0), Conroller Enabled(0), X, No Mem-Mem Transfer(0)

#define DMA_MAX_CHANNELS 4

DMA_TABLE DMA_channelAvailabilityList[DMA_MAX_CHANNELS] ;

const byte DMA_PAGE_PORT[] = { 0x87, 0x83, 0x81, 0x82 } ;
const byte DMA_OFFSET_PORT[] = { 0x00, 0x02, 0x04, 0x06 } ;
const byte DMA_COUNT_PORT[] = { 0x01, 0x03, 0x05, 0x07 } ;

static  void
DMA_EnableChannel(DMA_CHANNEL DMAChannelNo)
{
	PortCom_SendByte(DMA_SINGLE_MASK_REG_W, DMAChannelNo & 0x03) ;	
}

static  void
DMA_DisableChannel(DMA_CHANNEL DMAChannelNo)
{
	PortCom_SendByte(DMA_SINGLE_MASK_REG_W, DMAChannelNo | 0x04) ;
}

static  void
DMA_SetMode(DMA_CHANNEL DMAChannelNo, DMA_MODE DMAMode)
{
	PortCom_SendByte(DMA_MODE_REG_W, DMAMode | (DMAChannelNo & 0x03)) ;
}

static  void
DMA_ClearFlipFlop()
{
	PortCom_SendByte(DMA_FFCLEAR_SWCMD, 0x00) ;
}

static  void
DMA_MasterClear()
{
	PortCom_SendByte(DMA_MASTER_CLEAR_SWCMD, 0x00) ;
}

static  void
DMA_ClearMaskReg()
{
	PortCom_SendByte(DMA_CLEAR_MASK_REG_SWCMD, 0x00) ;
}

static  void
DMA_SendCommand(byte bCmd)
{
	PortCom_SendByte(DMA_CMD_REG_W, bCmd) ;
}

static void
DMA_SetTransferParameters(const DMA_Request *DMARequest)
{
	PortCom_SendByte(DMA_PAGE_PORT [ DMARequest->DMAChannelNo ], DMARequest->uiPhysicalAddress >> 16) ;
	
	PortCom_SendByte(DMA_OFFSET_PORT [ DMARequest->DMAChannelNo ], DMARequest->uiPhysicalAddress & 0xFF) ;
	PortCom_SendByte(DMA_OFFSET_PORT [ DMARequest->DMAChannelNo ], (DMARequest->uiPhysicalAddress >> 8) & 0xFF) ;
	
	PortCom_SendByte(DMA_COUNT_PORT [ DMARequest->DMAChannelNo ], DMARequest->uiWordCount & 0xFF) ;
	PortCom_SendByte(DMA_COUNT_PORT [ DMARequest->DMAChannelNo ], (DMARequest->uiWordCount >> 8) & 0xFF) ;
}

void
DMA_Initialize()
{
	byte i ;
	
	DMA_MasterClear() ;
	
	DMA_SendCommand(DMA_COMMAND) ;

	for(i = 0; i < DMA_MAX_CHANNELS; i++)
	{
		DMA_SetMode(static_cast<DMA_CHANNEL>(i), DMA_MODE_READ) ;
		DMA_channelAvailabilityList[i].bLock = 0 ;
		DMA_channelAvailabilityList[i].bDeviceID = 0 ;
	}

	DMA_ClearMaskReg() ;
  KC::MConsole().LoadMessage("DMA Initialization", Success) ;
}

byte
DMA_RequestChannel(DMA_Request* DMARequest)
{
	if(DMA_channelAvailabilityList [ DMARequest->DMAChannelNo ].bLock != 0)
		return DMA_ERR_CHANNEL_BUSY ;

	if(DMARequest->uiPhysicalAddress < MEM_DMA_START || 
			(DMARequest->uiPhysicalAddress + DMARequest->uiWordCount) > MEM_DMA_END)
		return DMA_ERR_INVALID_MEM_ADDRESS ;

  IrqGuard g;

	DMA_channelAvailabilityList [ DMARequest->DMAChannelNo ].bLock = 1 ;
	DMA_channelAvailabilityList [ DMARequest->DMAChannelNo ].bDeviceID = DMARequest->bDeviceID ;
	
	DMA_DisableChannel(DMARequest->DMAChannelNo) ;
	
	DMA_ClearFlipFlop() ;
	
	DMA_SetMode(DMARequest->DMAChannelNo, DMARequest->DMAMode) ;
	
	DMA_SetTransferParameters(DMARequest) ;

	DMA_EnableChannel(DMARequest->DMAChannelNo) ;
	
	return DMA_SUCCESS ;
}

void
DMA_ReleaseChannel(const DMA_CHANNEL DMAChannelNo)
{ 
  IrqGuard g;
	DMA_channelAvailabilityList [ DMAChannelNo ].bLock = 0 ;
	DMA_channelAvailabilityList [ DMAChannelNo ].bDeviceID = 0 ;
}

