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
# include <dtime.h>

void SysUtil_GetDateTime(RTCDateTime* rtcDateTime)
{
	int iRetStatus ;
	SysCallUtil_Handle(&iRetStatus, SYS_CALL_UTIL_DTIME, false, (uint64_t)rtcDateTime, 2, 3, 4, 5, 6, 7, 8, 9);
}

void SysUtil_Reboot()
{
	int iRetStatus ;
	SysCallUtil_Handle(&iRetStatus, SYS_CALL_UTIL_REBOOT, false, 1, 2, 3, 4, 5, 6, 7, 8, 9);
}

int SysUtil_GetTimeOfDay(struct timeval* pTV)
{
	int iRetStatus ;
	SysCallUtil_Handle(&iRetStatus, SYS_CALL_UTIL_TOD, false, (uint64_t)pTV, 2, 3, 4, 5, 6, 7, 8, 9);
	return iRetStatus ;
}

uint32_t SysUtil_GetTimeSinceBoot() {
  return PIT::Instance().GetClockCount();
}