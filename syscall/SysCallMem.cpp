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

byte SysCallMem_IsPresent(uint64_t sysCallId)
{
	return (sysCallId > SYS_CALL_MEM_START && sysCallId < SYS_CALL_MEM_END) ;
}

void
SysCallMem_Handle(uint64_t *retVal, uint64_t sysCallId, bool doAddrTranslation, uint64_t p1, uint64_t p2,
                  uint64_t p3, uint64_t p4, uint64_t p5)
{
	switch(sysCallId)
	{
		case SYS_CALL_ALIGNED_ALLOC : //Allocate Memory, meant only for User Process
      // P1 => return allocated address
      // P2 => alignment
      // P3 => size in bytes
			{
				void** addr = ( void**) p1;
				*addr =  (void*)ProcessManager::Instance().GetCurrentPAS().dmm().allocate(p3, p2) ;
			}
			break ;

    case SYS_CALL_FREE : //Free Memory, meant only for User Process
			//P1 => Address
			//P2 => Status
			{
				// ProcessManager_DisableTaskSwitch() ;

				*retVal = 0 ;
				
				if(!ProcessManager::Instance().GetCurrentPAS().dmm().free(p1))
					*retVal = -1 ;

				// ProcessManager_EnableTaskSwitch() ;
			}
			break ;

		case SYS_CALL_GET_ALLOC_SIZE : //Get Allocated Mem Size 
			//P1 => Address
			//P2 => Ret Size
			{
        ProcessSwitchLock pLock;
				auto pRetAllocSize = ( size_t*) p2;
				*retVal = 0 ;
				if(!ProcessManager::Instance().GetCurrentPAS().dmm().getAllocSize(p1, pRetAllocSize)) {
          *retVal = -1;
        }
			}
			break ;
	}
}


