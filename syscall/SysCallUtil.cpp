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

bool SysCallUtil_IsPresent(uint64_t sysCallId)
{
	return (sysCallId > SYS_CALL_UTIL_START && sysCallId < SYS_CALL_UTIL_END) ;
}

void
SysCallUtil_Handle(uint64_t *retVal, uint64_t sysCallId, bool doAddrTranslation, uint64_t p1, uint64_t p2, uint64_t p3,
                   uint64_t p4, uint64_t p5)
{
	switch(sysCallId)
	{
		case SYS_CALL_UTIL_DTIME : 
			//P1 => Ret RTC Pointer
			{
				RTCDateTime* pRTCTime = ( RTCDateTime*) p1;

				*retVal = 0 ;
				RTC::GetDateTime((*pRTCTime)) ;
			}
			break ;

    case SYS_CALL_UTIL_BTIME:
      {
        *retVal = SysUtil_GetTimeSinceBoot();
      }
      break;

    case SYS_CALL_UTIL_TOD :
      // P1 => Ret timeval Pointer
      {
        struct timeval* tv = ( struct timeval*) p1;

        *retVal = 0 ;
        try
        {
          tv->tSec = SystemUtil_GetTimeOfDay();
        }
        catch(...)
        {
          *retVal = -1 ;
        }
      }
      break ;

    case SYS_CALL_UTIL_REBOOT :
			{
				*retVal = 0 ;
				SystemUtil_Reboot() ;
			}
			break ;
	}
}
