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
#ifndef _ATA_TIMING_MANAGER_H_
#define _ATA_TIMING_MANAGER_H_

# include <ATADeviceController.h>

#define ATATimingManager_SUCCESS		0
#define ATATimingManager_FAILURE		1

#define ATA_TIMING_SETUP	0x01
#define ATA_TIMING_ACT8B	0x02
#define ATA_TIMING_REC8B	0x04
#define ATA_TIMING_CYC8B	0x08
#define ATA_TIMING_8BIT		0x0e
#define ATA_TIMING_ACTIVE	0x10
#define ATA_TIMING_RECOVER	0x20
#define ATA_TIMING_CYCLE	0x40
#define ATA_TIMING_UDMA		0x80
#define ATA_TIMING_ALL		0xff

#define FIT( v, min, max ) MAX( MIN( v, max ), min )
#define ENOUGH( v, unit ) ( ( ( v ) - 1 ) / ( unit ) + 1 )
#define EZ( v, unit ) ( ( v != 0 ) ? ENOUGH( v, unit ) : 0 )

typedef struct
{
	int iMode ;
	unsigned short usSetup ;
	unsigned short usAct8b ;
	unsigned short usRec8b ;
	unsigned short usCyc8b ;
	unsigned short usAct ;
	unsigned short usRec ;
	unsigned short usCyc ;
	unsigned short usUDMA ;
} ATATiming ;

void ATATimingManager_Merge(ATATiming* pFirst, ATATiming* pSecond, ATATiming* pResult, unsigned uiWhat) ;
void ATATimingManager_Compute(ATAPort* pPort, int iSpeed, ATATiming* pATATiming, unsigned uiTiming, unsigned uiUDMATiming) ;

#endif 
