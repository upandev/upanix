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
#ifndef _SYS_CALL_FILE_H_
#define _SYS_CALL_FILE_H_

# include <Global.h>

byte SysCallFile_IsPresent(unsigned uiSysCallID) ;

void SysCallFile_Handle(
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
        volatile uint64_t P9) ;

#endif
