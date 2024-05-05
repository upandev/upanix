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
#ifndef _FLOPPY_H_
#define _FLOPPY_H_

#include <Global.h>
#include <StorageDrive.h>

#define MAX_RESULT_PHASE_REPLIES 7

typedef enum
{
	FD_HEAD0 = 0x00,
	FD_HEAD1 = 0x01
} Floppy_HEAD_NO ;

typedef enum
{
	FD_READ,
	FD_WRITE
} Floppy_MODE ;

void Floppy_Initialize() ;
bool Floppy_GetInitStatus() ;
void Floppy_Handler() ;
bool Floppy_IsEnhancedController() ;
void Floppy_Read(const StorageDrive* pDiskDrive, unsigned uiStartSectorNo, unsigned uiEndSectorNo, byte* bSectorBuffer) ;
void Floppy_Write(const StorageDrive* pDiskDrive, unsigned uiStartSectorNo, unsigned uiEndSectorNo, byte* bSectorBuffer) ;
void Floppy_Format(const StorageDrive* pDiskDrive) ;

#endif
