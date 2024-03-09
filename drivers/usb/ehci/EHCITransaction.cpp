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
#include <MemManager.h>
#include <DMM.h>
#include <ProcessManager.h>
#include <EHCIController.h>
#include <EHCIDataHandler.h>
#include <EHCITransaction.h>

EHCITransaction::EHCITransaction(EHCIQueueHead* qh, EHCIQTransferDesc* tdStart)
  : _qh(qh), _tdStart(tdStart)
{
	EHCIQTransferDesc* tdCur = _tdStart;

	for(; (uintptr_t)tdCur != 1; tdCur = (EHCIQTransferDesc*)(KERNEL_VIRTUAL_ADDRESS(tdCur->uiNextpTDPointer)))
	{
    _dStorageList.push_back(KERNEL_VIRTUAL_ADDRESS(tdCur->uiBufferPointer[0]));
    _dStorageList.push_back((uintptr_t)tdCur);
	}

	_qh->uiNextpTDPointer = KERNEL_REAL_ADDRESS(_tdStart);
}

bool EHCITransaction::PollWait()
{
	const unsigned uiSleepTime = 10 ; // 10 ms
	int iMaxLimit = 10000 ; // 10 Sec

	EHCIQTransferDesc* tdCur;

	while(iMaxLimit > 10)
	{
		int poll = 10000;
		while(poll > 0)
		{
			if(_qh->uiNextpTDPointer == 1)
			{
				for(tdCur = _tdStart; (uintptr_t)tdCur != 1; )
				{
					if((tdCur->uipTDToken & 0xFE))
						break ;

					tdCur = (EHCIQTransferDesc*)(KERNEL_VIRTUAL_ADDRESS(tdCur->uiNextpTDPointer)) ;
				}

				if((uintptr_t)tdCur == 1)
					return true ;
			}
			--poll ;
		}
		iMaxLimit -= uiSleepTime ;
		if(iMaxLimit == 20)
			ProcessManager::Instance().Sleep(uiSleepTime) ;
	}

	return false ;
}

void EHCITransaction::Clear()
{
	EHCIDataHandler_CleanAysncQueueHead(_qh);
	
  for(auto i : _dStorageList)
	{
		if(i != NULL)
			KernelDMM::Instance().free(i);
	}
  _dStorageList.clear();
}
