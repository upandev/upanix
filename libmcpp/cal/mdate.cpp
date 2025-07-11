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
#include <mdate.h>

static const mdate seedDate = { 1, 1, 1970, THURSDAY } ;
//static const mdate seedDate = { 31, 12, 1999, FRIDAY } ;

/* Day of Month is the only one which is 1 based index */

//No. of days in Months - Non Leap Year
static const int mdate_NoOfDaysInMonth_NLY[MONTHS_PER_YEAR_SIZE] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 } ;

//No. of days in Months - Leap Year
static const int mdate_NoOfDaysInMonth_LY[MONTHS_PER_YEAR_SIZE] = { 0, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 } ;

static const char mdate_szMonths[MONTHS_PER_YEAR_SIZE][12] = { 
	"NA",
	"January",
	"February",
	"March",
	"April",
	"May",
	"June",
	"July",
	"August",
	"September",
	"October",
	"November",
	"December"
} ;

static const char mdate_szDays[DAYS_PER_WEEK + 1][12] = {
	"Monday",
	"Tuesday",
	"Wednesday",
	"Thursday",
	"Friday",
	"Saturday",
	"Sunday",
	"NA"
} ;

bool mdate_Set(mdate* date, int dayOfMonth, int month, int year)
{
	date->dayOfMonth = dayOfMonth ;
	date->month = month ;
	date->year = year ;

	return mdate_IsValid(date) ;
}

void mdate_Copy(mdate* d1, const mdate* d2)
{
	d1->dayOfMonth = d2->dayOfMonth ;
	d1->month = d2->month ;
	d1->year = d2->year ;
	d1->dayOfWeek = d2->dayOfWeek ;
}

void mdate_GetSeedDate(mdate* d1)
{
	mdate_Copy(d1, &seedDate) ;
}

void mdate_Normalize(const mdate* src, mdate* dest)
{
	dest->dayOfMonth = src->dayOfMonth ;
	dest->month = src->month - 1 ;
	dest->year = src->year ;
}

int mdate_GetDaysInMonth(const mdate* date)
{
	if(IS_LEAP_YEAR(date->year))
		return mdate_NoOfDaysInMonth_LY[date->month] ;
	else
		return mdate_NoOfDaysInMonth_NLY[date->month] ;
}

int mdate_GetDaysInMonth_1(int month, int year)
{
	if(IS_LEAP_YEAR(year))
		return mdate_NoOfDaysInMonth_LY[month] ;
	else
		return mdate_NoOfDaysInMonth_NLY[month] ;
}

bool mdate_IsValidMonth(int month)
{
	return (month >= JANUARY && month <= DECEMBER) ;
}

bool mdate_IsValidYear(int year)
{
	return (year > 0 && year < 10000) ;
}

bool mdate_IsValidDayOfMonth(const mdate* date)
{
	if(date->dayOfMonth < 1)
		return false ;
		
	if(date->dayOfMonth > mdate_GetDaysInMonth(date))
		return false ;

	return true ;
}

bool mdate_IsValidDayOfWeek(int dayOfWeek)
{
	return (dayOfWeek >= MONDAY && dayOfWeek <= SUNDAY) ;
}

bool mdate_IsValid(const mdate* date)
{
	if(mdate_IsValidYear(date->year))
		if(mdate_IsValidMonth(date->month))
			if(mdate_IsValidDayOfMonth(date))
				return true ;
	
	return false ;
}

/* Date Add Functions */
bool mdate_AddDays(mdate* date, int days)
{
	if(!mdate_IsValid(date))
		return false ;

	if(days < 0)
		return false ;

	if(days == 0)
		return true ;

//	int dayOfWeek = (date->dayOfWeek + days) % DAYS_PER_WEEK ;
	int dayOfMonth = date->dayOfMonth ;
	int month = date->month ;
	int year = date->year ;

	for( ; days > 0; )
	{
		int daysInMonth = mdate_GetDaysInMonth_1(month, year) ;

		if( dayOfMonth + days > daysInMonth )
		{
			days -= ( daysInMonth - dayOfMonth ) ;

			dayOfMonth = 0 ;

			month++ ;
			if( month > DECEMBER )
			{
				month = JANUARY ;
				year++ ;
			}
		}
		else
		{
			dayOfMonth += days ;
			break ;
		}
	}

	date->dayOfMonth = dayOfMonth ;
	date->month = month ;
	date->year = year ;

	return true ;
}

bool mdate_AddWeeks(mdate* date, int weeks)
{
	int days = weeks * DAYS_PER_WEEK ;
	return mdate_AddDays(date, days) ;
}

bool mdate_AddMonths(mdate* date, int months)
{
	if(!mdate_IsValid(date))
		return false ;

	if(months < 0)
		return false ;

	if(months == 0)
		return true ;

	int dayOfMonth = date->dayOfMonth ;
	int month = date->month ;
	int year = date->year ;

	year = year + (month + months - 1) / MONTHS_PER_YEAR ;
	month = ((month + months - 1) % MONTHS_PER_YEAR) ;
	month += 1 ;

	int daysInMonth = mdate_GetDaysInMonth_1(month, year) ;
	if(dayOfMonth > daysInMonth)
		dayOfMonth = daysInMonth ;

	date->dayOfMonth = dayOfMonth ;
	date->month = month ;
	date->year = year ;

	return true ;
}

bool mdate_AddYears(mdate* date, int years)
{
	if(!mdate_IsValid(date))
		return false ;

	if(years < 0)
		return false ;

	if(years == 0)
		return true ;
	
	int year = date->year + years ;

	if(date->month == FEBRUARY)
	{
		int daysInMonth = mdate_GetDaysInMonth_1(date->month, year) ;
		if(date->dayOfMonth > daysInMonth)
			date->dayOfMonth = daysInMonth ;		
	}

	date->year = year ;

	return true ;
}
/* End of Date Add Functions */


/* Date Subtract Functions */
bool mdate_SubtractDays(mdate* date, int days)
{
	if(!mdate_IsValid(date))
		return false ;

	if(days < 0)
		return false ;

	if(days == 0)
		return true ;

//	int dayOfWeek = (date->dayOfWeek + days) % DAYS_PER_WEEK ;
	int dayOfMonth = date->dayOfMonth ;
	int month = date->month ;
	int year = date->year ;

	for( ; days > 0; )
	{

		if( days >= dayOfMonth )
		{
			days -= dayOfMonth ;

			month-- ;
			if( month < JANUARY )
			{
				month = DECEMBER ;
				year-- ;
			}

			dayOfMonth = mdate_GetDaysInMonth_1(month, year) ;
		}
		else
		{
			dayOfMonth -= days ;
			break ;
		}
	}

	date->dayOfMonth = dayOfMonth ;
	date->month = month ;
	date->year = year ;

	return true ;
}

bool mdate_SubtractWeeks(mdate* date, int weeks)
{
	int days = weeks * DAYS_PER_WEEK ;
	return mdate_SubtractDays(date, days) ;
}

bool mdate_SubtractMonths(mdate* date, int months)
{
	if(!mdate_IsValid(date))
		return false ;

	if(months < 0)
		return false ;

	if(months == 0)
		return true ;

	int dayOfMonth = date->dayOfMonth ;
	int month = date->month ;
	int year = date->year ;

	int norMonths = months % MONTHS_PER_YEAR ;

	month = 12 - norMonths + month ;
	if(month == 0)
		month = DECEMBER ;

	year = year - (months / MONTHS_PER_YEAR) ;
	if((month - norMonths) < 1)
		year-- ;

	int daysInMonth = mdate_GetDaysInMonth_1(month, year) ;
	if(dayOfMonth > daysInMonth)
		dayOfMonth = daysInMonth ;

	date->dayOfMonth = dayOfMonth ;
	date->month = month ;
	date->year = year ;

	return true ;
}

bool mdate_SubtractYears(mdate* date, int years)
{
	if(!mdate_IsValid(date))
		return false ;

	if(years < 0)
		return false ;

	if(years == 0)
		return true ;
	
	int year = date->year - years ;

	if(year <= 0)
		return false ;

	if(date->month == FEBRUARY)
	{
		int daysInMonth = mdate_GetDaysInMonth_1(date->month, year) ;
		if(date->dayOfMonth > daysInMonth)
			date->dayOfMonth = daysInMonth ;		
	}

	date->year = year ;

	return true ;
}
/* End of Date Subtract Functions */

/* Logical Comparison on Dates */
bool mdate_IsGreater(const mdate* a, const mdate* b)
{
	if(a->year > b->year)
		return true ;
	else if(a->year < b->year)
		return false ;
	else
	{
		if(a->month > b->month)
			return true ;
		else if(a->month < b->month)
			return false ;
		else
		{
			if(a->dayOfMonth > b->dayOfMonth)
				return true ;
			return false ;
		}
	}

	return false ;
}

bool mdate_IsLesser(const mdate* a, const mdate* b)
{
	if(a->year < b->year)
		return true ;
	else if(a->year > b->year)
		return false ;
	else
	{
		if(a->month < b->month)
			return true ;
		else if(a->month > b->month)
			return false ;
		else
		{
			if(a->dayOfMonth < b->dayOfMonth)
				return true ;
			return false ;
		}
	}

	return false ;
}

bool mdate_IsEqual(const mdate* a, const mdate* b)
{
	return (a->year == b->year) && (a->month == b->month) && (a->dayOfMonth == b->dayOfMonth) ;
}

bool mdate_IsNotEqual(const mdate* a, const mdate* b)
{
	return (a->year != b->year) || (a->month != b->month) || (a->dayOfMonth != b->dayOfMonth) ;
}
/* End of Logical Comparison on Dates */

// A should be greater than B
int mdate_SameYearDateDifference(const mdate* a, const mdate* b)
{
	if(mdate_IsEqual(a, b))
		return 0 ;

	if(a->year != b->year)
		return -1 ; // error
	
	if( a->month == b->month )
		return ( a->dayOfMonth - b->dayOfMonth ) ;
	
	int res = (mdate_GetDaysInMonth(b) - b->dayOfMonth) ;
	
	int m ;
	for(m = b->month + 1; m != a->month; m++ )
	{
		res += mdate_GetDaysInMonth_1(m, b->year) ;
	}
	
	res += a->dayOfMonth ;
	return res ;
}

int mdate_DateDifference(const mdate* a, const mdate* b)
{
	if(mdate_IsEqual(a, b))
		return 0 ;
	
	const mdate* date1 = a ;
	const mdate* date2 = b ;

	int switchSign = 1 ;
	if(mdate_IsLesser(a, b))
	{
		switchSign = -1 ;
		date1 = b ;
		date2 = a ;
	}

	int res = 0 ;

	// date1 - date2
	
	if(date1->year == date2->year)
	{
		if(date1->month == date2->month)
		{
			res = date1->dayOfMonth - date2->dayOfMonth ;
		}
		else
		{
			res = mdate_SameYearDateDifference(date1, date2) ;
		}
	}
	else
	{
		int y ;

		for(y = date2->year + 1; y != date1->year; y++)
		{
			if(IS_LEAP_YEAR(y))
				res += 366 ;
			else
				res += 365 ;
		}

		mdate d ;
		
		mdate_Set(&d, 31, 12, date2->year) ;
		res += mdate_SameYearDateDifference(&d, date2) ;
		
		mdate_Set(&d, 1, 1, date1->year) ;
		res += mdate_SameYearDateDifference(date1, &d) ;
		res++ ;
	}

	return res * switchSign ;
}

bool mdate_SetDayOfWeek(mdate* date)
{
	if(!mdate_IsValid(date))
		return false ;

	if(mdate_IsEqual(date, &seedDate))
	{
		date->dayOfWeek = seedDate.dayOfWeek ;
		return true ;
	}

	int days = mdate_DateDifference(date, &seedDate) ;
	int res ;
	
	if(days >= 0)
	{
		res = (seedDate.dayOfWeek + days) % DAYS_PER_WEEK ;
	}
	else
	{
		days *= -1 ;
		res = days % DAYS_PER_WEEK ;
		if(res <= seedDate.dayOfWeek)
			res = seedDate.dayOfWeek - res ;
		else
			res = DAYS_PER_WEEK - (res - seedDate.dayOfWeek) ;
	}

	date->dayOfWeek = res ;
	return true ;
}

bool mdate_SeedDateDifference(const mdate* dt, int* days)
{
	*days = 0 ;

	if(!mdate_IsValid(dt))
		return false ;

	if(mdate_IsEqual(dt, &seedDate))
		return true ;

	*days = mdate_DateDifference(dt, &seedDate) ;
	return true ;
}

const char* mdate_GetMonthName(const mdate* date)
{
	if(date->month >= JANUARY && date->month <= DECEMBER)
		return mdate_szMonths[date->month] ;
	return mdate_szMonths[0] ;
}

const char* mdate_GetDayName(const mdate* date)
{
	if(date->dayOfWeek >= MONDAY && date->dayOfWeek <= SUNDAY)
		return mdate_szDays[date->dayOfWeek] ;
	return mdate_szDays[DAYS_PER_WEEK] ;
}

