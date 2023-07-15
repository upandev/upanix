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
#ifndef _DMA_H_
#define _DMA_H_

#define DMA_SUCCESS 0
#define DMA_ERR_CHANNEL_BUSY 1
#define DMA_ERR_INVALID_MEM_ADDRESS 2

#include <Global.h>
#include <PortCom.h>

typedef enum
{
	DMA_CH0,
	DMA_CH1,
	DMA_CH2,
	DMA_CH3/*,
	DMA_CH4,
	DMA_CH5,
	DMA_CH6,
	DMA_CH7*/
} DMA_CHANNEL ;

typedef enum 
{
	DMA_MODE_WRITE = 0x44, // I/O To Memory, No AutoInit, Addr Increment, Single Mode -- Channel bit(0 & 1) Set Later
	DMA_MODE_READ = 0x48 // Memory To I/O, No AutoInit, Addr Increment, Single Mode -- Channel bit(0 & 1) Set Later
} DMA_MODE ;

typedef enum
{
	DMA_DEVICE_ID_FLOPPY = 2
} DMA_DEVICE_ID ;

typedef struct DMA_CHANNEL_AVAILABILITY_TABLE
{
	byte bLock ;
	byte bDeviceID ;
} DMA_TABLE ;

typedef struct DMA_Request
{
	DMA_CHANNEL DMAChannelNo ;
	byte bDeviceID ;
	uint32_t uiPhysicalAddress ;
	uint32_t uiWordCount ;
	DMA_MODE DMAMode ;
} DMA_Request ;

void DMA_Initialize() ;
byte DMA_RequestChannel(DMA_Request* DMARequest) ; 
void DMA_ReleaseChannel(const DMA_CHANNEL DMAChannelNo) ;
#endif

