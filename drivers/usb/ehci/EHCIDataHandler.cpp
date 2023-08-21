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
#include <DMM.h>
#include <stdio.h>
#include <string.h>
#include <EHCIStructures.h>

void EHCIDataHandler_CleanAysncQueueHead(EHCIQueueHead*  pQH)
{
	pQH->uiCurrrentpTDPointer = 0 ;
	pQH->uiNextpTDPointer = 1 ;
	pQH->uiAltpTDPointer_NAKCnt = 1 ;
	pQH->uipQHToken = 0 ;

	unsigned i ;
	for(i = 0; i < 5; i++)
	{
		pQH->uiBufferPointer[ i ] = 0 ;
		pQH->uiExtendedBufferPointer[ i ] = 0 ;
	}
}

EHCIQueueHead* EHCIDataHandler_CreateAsyncQueueHead()
{
  uintptr_t uiQHAddress = DMM_AllocateForKernel(sizeof(EHCIQueueHead), 32) ;
	
	memset((void*)(uiQHAddress), 0, sizeof(EHCIQueueHead)) ;

	return (EHCIQueueHead*)(uiQHAddress) ;
}

EHCIQTransferDesc* EHCIDataHandler_CreateAsyncQTransferDesc()
{
  uintptr_t uiTDAddress = DMM_AllocateForKernel(sizeof(EHCIQTransferDesc), 32) ;

	memset((void*)(uiTDAddress), 0, sizeof(EHCIQTransferDesc)) ;

	return (EHCIQTransferDesc*)(uiTDAddress) ;
}

void EHCIDataHandler_ReleaseAsyncQueueHead(EHCIQueueHead* pQH)
{
	DMM_DeAllocateForKernel((uintptr_t)pQH) ;
}


