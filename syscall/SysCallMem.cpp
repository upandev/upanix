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

byte SysCallMem_IsPresent(unsigned uiSysCallID)
{
	return (uiSysCallID > SYS_CALL_MEM_START && uiSysCallID < SYS_CALL_MEM_END) ;
}

void SysCallMem_Handle(
__volatile__ int* piRetVal,
__volatile__ unsigned uiSysCallID, 
__volatile__ bool bDoAddrTranslation,
__volatile__ unsigned uiP1, 
__volatile__ unsigned uiP2, 
__volatile__ unsigned uiP3, 
__volatile__ unsigned uiP4, 
__volatile__ unsigned uiP5, 
__volatile__ unsigned uiP6, 
__volatile__ unsigned uiP7, 
__volatile__ unsigned uiP8, 
__volatile__ unsigned uiP9)
{
	switch(uiSysCallID)
	{
		case SYS_CALL_ALLOC : //Allocate Mem.. Ment only for User Process
			//P1 => Return Alloc Address
			//P2 => Size in Bytes
			{
				void** addr = KERNEL_ADDR(bDoAddrTranslation, void**, uiP1) ;
				// ProcessManager_DisableTaskSwitch() ;

				*addr = (void*)DMM_Allocate(&ProcessManager::Instance().GetCurrentPAS(), uiP2) ;

				// ProcessManager_EnableTaskSwitch() ;
			}
			break ;

		case SYS_CALL_FREE : //Free Mem.. Ment only for User Process
			//P1 => Address
			//P2 => Status
			{
				// ProcessManager_DisableTaskSwitch() ;

				*piRetVal = 0 ;
				
				if(DMM_DeAllocate(&ProcessManager::Instance().GetCurrentPAS(), uiP1) != DMM_SUCCESS)
					*piRetVal = -1 ;

				// ProcessManager_EnableTaskSwitch() ;
			}
			break ;

		case SYS_CALL_GET_ALLOC_SIZE : //Get Allocated Mem Size 
			//P1 => Address
			//P2 => Ret Size
			{
        ProcessSwitchLock pLock;
				auto pRetAllocSize = KERNEL_ADDR(bDoAddrTranslation, size_t*, uiP2) ;
				*piRetVal = 0 ;
				if(!DMM_GetAllocSize(uiP1, pRetAllocSize)) {
          *piRetVal = -1;
        }
			}
			break ;
	}
}


