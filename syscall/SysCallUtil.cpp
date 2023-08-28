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
# include <SysCallUtil.h>
# include <SystemUtil.h>
# include <RTC.h>

byte SysCallUtil_IsPresent(uint32_t sysCallID)
{
	return (sysCallID > SYS_CALL_UTIL_START && sysCallID < SYS_CALL_UTIL_END) ;
}

void SysCallUtil_Handle(
        __volatile__ int* piRetVal,
        __volatile__ unsigned uiSysCallID,
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
	switch(uiSysCallID)
	{
		case SYS_CALL_UTIL_DTIME : 
			//P1 => Ret RTC Pointer
			{
				RTCDateTime* pRTCTime = KERNEL_ADDR(bDoAddrTranslation, RTCDateTime*, P1) ;

				*piRetVal = 0 ;
				RTC::GetDateTime((*pRTCTime)) ;
			}
			break ;

    case SYS_CALL_UTIL_BTIME:
      {
        *piRetVal = SysUtil_GetTimeSinceBoot();
      }
      break;

    case SYS_CALL_UTIL_TOD :
      // P1 => Ret timeval Pointer
      {
        struct timeval* tv = KERNEL_ADDR(bDoAddrTranslation, struct timeval*, P1) ;

        *piRetVal = 0 ;
        try
        {
          tv->tSec = SystemUtil_GetTimeOfDay();
        }
        catch(...)
        {
          *piRetVal = -1 ;
        }
      }
      break ;

    case SYS_CALL_UTIL_REBOOT :
			{
				*piRetVal = 0 ;
				SystemUtil_Reboot() ;
			}
			break ;
	}
}
