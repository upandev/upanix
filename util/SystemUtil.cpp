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
# include <SystemUtil.h>
# include <RTC.h>
# include <mdate.h>
# include <ProcessManager.h>
# include <StorageDrive.h>
# include <PS2Controller.h>
# include <StorageDriveManager.h>

void SystemUtil_Reboot() {
  ProcessManager::Instance().stopUserProcesses();
  StorageDriveManager::Instance().Close();
  ProcessManager::Instance().stopKernelProcesses();
	ProcessManager::Instance().Sleep(2000000);
  PS2Controller::Instance().Reboot();
}

time_t SystemUtil_GetTimeOfDay()
{
	RTCDateTime rtcTime ;
	RTC::GetDateTime(rtcTime) ;

	mdate dt;
	mdate_Set(&dt, rtcTime._dayOfMonth, rtcTime._month, rtcTime._century * 100 + rtcTime._year) ;
	
	int days ;	
	if(!mdate_SeedDateDifference(&dt, &days))
    throw upan::exception(XLOC, "invalid time");

  return days * 24 * 60 * 60 + rtcTime._hour * 60 * 60 + rtcTime._minute * 60 + rtcTime._second ;
	//tv->uimSec = tv->tSec * 1000 ;
}

void SystemUtil_GetRTCTimeFromTime(RTCDateTime* rtcDateTime, const struct timeval* tv)
{
	mdate d1 ;
	mdate_GetSeedDate(&d1) ;
	
	time_t days = tv->tv_sec / (HRS_IN_DAY) ;
	mdate_AddDays(&d1, days) ;

  rtcDateTime->_dayOfMonth = d1.dayOfMonth ;
  rtcDateTime->_month = d1.month ;
  rtcDateTime->_year = d1.year % 100 ;
  rtcDateTime->_century = d1.year / 100 ;

  time_t resi = tv->tv_sec % HRS_IN_DAY ;
  rtcDateTime->_hour = resi / 3600 ;

  time_t resh = resi % 3600 ;
  rtcDateTime->_minute = resh / 60 ;
  rtcDateTime->_second = resh % 60 ;
}

