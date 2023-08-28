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
# include <SysCall.h>
# include <SysCallMem.h>

byte SysCallMem_IsPresent(uint32_t sysCallID)
{
	return (sysCallID > SYS_CALL_MEM_START && sysCallID < SYS_CALL_MEM_END) ;
}

void SysCallMem_Handle(
        __volatile__ int* piRetVal,
        __volatile__ uint32_t sysCallID,
        __volatile__ bool bDoAddrTranslation,
        volatile uint64_t P1,
        volatile uint64_t P2,
        volatile uint64_t P3,
        volatile uint64_t P4,
        volatile uint64_t P5,
        volatile uint64_t P6,
        volatile uint64_t P7,
        volatile uint64_t P8,
        volatile uint64_t P9)
{
	switch(sysCallID)
	{
		case SYS_CALL_ALLOC : //Allocate Mem.. Ment only for User Process
			//P1 => Return Alloc Address
			//P2 => Size in Bytes
			{
				void** addr = KERNEL_ADDR(bDoAddrTranslation, void**, P1) ;
				// ProcessManager_DisableTaskSwitch() ;

				*addr = (void*)DMM_Allocate(&ProcessManager::Instance().GetCurrentPAS(), P2) ;

				// ProcessManager_EnableTaskSwitch() ;
			}
			break ;

		case SYS_CALL_FREE : //Free Mem.. Ment only for User Process
			//P1 => Address
			//P2 => Status
			{
				// ProcessManager_DisableTaskSwitch() ;

				*piRetVal = 0 ;
				
				if(DMM_DeAllocate(&ProcessManager::Instance().GetCurrentPAS(), P1) != DMM_SUCCESS)
					*piRetVal = -1 ;

				// ProcessManager_EnableTaskSwitch() ;
			}
			break ;

		case SYS_CALL_GET_ALLOC_SIZE : //Get Allocated Mem Size 
			//P1 => Address
			//P2 => Ret Size
			{
        ProcessSwitchLock pLock;
				auto pRetAllocSize = KERNEL_ADDR(bDoAddrTranslation, size_t*, P2) ;
				*piRetVal = 0 ;
				if(!DMM_GetAllocSize(P1, pRetAllocSize)) {
          *piRetVal = -1;
        }
			}
			break ;
	}
}


