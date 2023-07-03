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
#ifndef _ASMUTIL_H_
#define _ASMUTIL_H_

#define NO_OF_GPR 8

#include <MemUtil.h>
#include <MemConstants.h>

//unsigned KBDriver_GPRStack[NO_OF_GPR] ;
//unsigned PIT_GPRStack[NO_OF_GPR] ;
//unsigned Floppy_GPRStack[NO_OF_GPR] ;
//unsigned RTC_GPRStack[NO_OF_GPR] ;
//unsigned IDEPrimary_GPRStack[NO_OF_GPR] ;
//unsigned IDESecondary_GPRStack[NO_OF_GPR] ;

/*
#define AsmUtil_STORE_GPR() \
__volatile__ uint32_t __gpr_stack[NO_OF_GPR];  \
__asm__ __volatile__("movl %%eax, %%ss:%0" : "=m"(__gpr_stack[0]): ) ; \
__asm__ __volatile__("movl %%ebx, %%ss:%0" : "=m"(__gpr_stack[1]): ) ; \
__asm__ __volatile__("movl %%ecx, %%ss:%0" : "=m"(__gpr_stack[2]): ) ; \
__asm__ __volatile__("movl %%edx, %%ss:%0" : "=m"(__gpr_stack[3]): ) ; \
__asm__ __volatile__("movl %%esi, %%ss:%0" : "=m"(__gpr_stack[4]): ) ; \
__asm__ __volatile__("movl %%edi, %%ss:%0" : "=m"(__gpr_stack[5]): ) ;
//__asm__ __volatile__("movl %%ebp, %%ss:%0" : "=m"(BufferStack[6]): ) ; 
//__asm__ __volatile__("movl %%esp, %%ss:%0" : "=m"(BufferStack[7]): )

#define AsmUtil_RESTORE_GPR() \
__asm__ __volatile__("movl %%ss:%0, %%eax" : : "m"(__gpr_stack[0])) ; \
__asm__ __volatile__("movl %%ss:%0, %%ebx" : : "m"(__gpr_stack[1])) ; \
__asm__ __volatile__("movl %%ss:%0, %%ecx" : : "m"(__gpr_stack[2])) ; \
__asm__ __volatile__("movl %%ss:%0, %%edx" : : "m"(__gpr_stack[3])) ; \
__asm__ __volatile__("movl %%ss:%0, %%esi" : : "m"(__gpr_stack[4])) ; \
__asm__ __volatile__("movl %%ss:%0, %%edi" : : "m"(__gpr_stack[5])) ;
//__asm__ __volatile__("movl %%ss:%0, %%ebp" : : "m"(BufferStack[6])) ; 
//__asm__ __volatile__("movl %%ss:%0, %%esp" : : "m"(BufferStack[7])) 

#define AsmUtil_UNLOAD_KERNEL_SEGS() \
{ \
__asm__ __volatile__("movw %ss:(0), %ds") ; \
__asm__ __volatile__("movw %ss:(2), %fs") ; \
__asm__ __volatile__("movw %ss:(4), %gs") ; \
__asm__ __volatile__("movw %ss:(6), %es") ; \
}

#define AsmUtil_DECLARE_KERNEL_DS_BACK_VAR \
__volatile__ unsigned short AsmUtil_usDS = MemUtil_GetDS() ; \
__volatile__ unsigned short AsmUtil_usES = MemUtil_GetES() ; \
__volatile__ unsigned short AsmUtil_usFS = MemUtil_GetFS() ; \
__volatile__ unsigned short AsmUtil_usGS = MemUtil_GetGS() ;

#define AsmUtil_SET_KERNEL_DATA_SEGMENTS \
AsmUtil_DECLARE_KERNEL_DS_BACK_VAR \
MemUtil_SetDS(SYS_DATA_SELECTOR_DEFINED) ; \
MemUtil_SetES(SYS_LINEAR_SELECTOR_DEFINED) ; \
MemUtil_SetFS(SYS_DATA_SELECTOR_DEFINED) ; \
MemUtil_SetGS(SYS_DATA_SELECTOR_DEFINED) ;

#define AsmUtil_NODEC_SET_KERNEL_DATA_SEGMENTS \
MemUtil_SetDS(SYS_DATA_SELECTOR_DEFINED) ; \
MemUtil_SetES(SYS_LINEAR_SELECTOR_DEFINED) ; \
MemUtil_SetFS(SYS_DATA_SELECTOR_DEFINED) ; \
MemUtil_SetGS(SYS_DATA_SELECTOR_DEFINED) ;

#define AsmUtil_REVOKE_KERNEL_DATA_SEGMENTS \
MemUtil_SetFS(AsmUtil_usFS) ; \
MemUtil_SetGS(AsmUtil_usGS) ; \
MemUtil_SetES(AsmUtil_usES) ; \
MemUtil_SetDS(AsmUtil_usDS) ;

#define __SET_DS_ES_TO_DATA \
	__volatile__ unsigned __uiDS = MemUtil_GetDS() ; \
	__volatile__ unsigned __uiES = MemUtil_GetES() ; \
	MemUtil_SetDS(SYS_DATA_SELECTOR_DEFINED) ; \
	MemUtil_SetES(SYS_DATA_SELECTOR_DEFINED) ;

#define __RESTOR_DS_ES \
	MemUtil_SetDS(__uiDS) ; \
	MemUtil_SetES(__uiES) ;
*/

#define AsmUtil_STORE_GPR()
#define AsmUtil_RESTORE_GPR()
#define AsmUtil_UNLOAD_KERNEL_SEGS()
#define AsmUtil_DECLARE_KERNEL_DS_BACK_VAR
#define AsmUtil_SET_KERNEL_DATA_SEGMENTS

#define AsmUtil_NODEC_SET_KERNEL_DATA_SEGMENTS
#define AsmUtil_REVOKE_KERNEL_DATA_SEGMENTS
#define __SET_DS_ES_TO_DATA
#define __RESTOR_DS_ES

#endif
