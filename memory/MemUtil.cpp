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

# include <MemUtil.h>
# include <PortCom.h>
# include <PIC.h>
# include <AsmUtil.h>

//	__asm__ __volatile__("movw %w0,%%ds" : : "rm" (usSelector));
//
unsigned short
MemUtil_GetSS()
{
	unsigned short usSelector ;
//	__asm__ __volatile__("pushw %ss") ;
//	__asm__ __volatile__("popw %0" : "=g"(usSelector):) ;
	__asm__ __volatile__("movw %%ss, %%ss:%0" : "=m"(usSelector) : );
	return usSelector ;
}

 unsigned short
MemUtil_GetDS()
{
	unsigned short usSelector ;
//	__asm__ __volatile__("pushw %ds") ;
//	__asm__ __volatile__("popw %0" : "=g"(usSelector):) ;
	__asm__ __volatile__("movw %%ds, %%ss:%0" : "=m"(usSelector) : );
	return usSelector ;
}

 unsigned short
MemUtil_GetES()
{
	unsigned short usSelector ;
//	__asm__ __volatile__("pushw %es") ;
//	__asm__ __volatile__("popw %0" : "=g"(usSelector):) ;
	__asm__ __volatile__("movw %%es, %%ss:%0" : "=m"(usSelector) : );
	return usSelector ;
}

 unsigned short
MemUtil_GetFS()
{
	unsigned short usSelector ;
//	__asm__ __volatile__("pushw %fs") ;
//	__asm__ __volatile__("popw %0" : "=g"(usSelector):) ;
	__asm__ __volatile__("movw %%fs, %%ss:%0" : "=m"(usSelector) : );
	return usSelector ;
}

 unsigned short
MemUtil_GetGS()
{
	unsigned short usSelector ;
//	__asm__ __volatile__("pushw %gs") ;
//	__asm__ __volatile__("popw %0" : "=g"(usSelector):) ;
	__asm__ __volatile__("movw %%gs, %%ss:%0" : "=m"(usSelector) : );
	return usSelector ;
}

unsigned MemUtil_GetSP()
{
	unsigned uiSP ;
//	__asm__ __volatile__("pushw %ss") ;
//	__asm__ __volatile__("popw %0" : "=g"(usSelector):) ;
	__asm__ __volatile__("movl %%esp, %%ss:%0" : "=m"(uiSP) : );
	return uiSP ;
}

 void MemUtil_MoveByte(const unsigned short usSelector, const unsigned int uiOffset, const byte uiValue)
{
//	__asm__ __volatile__("push %es") ;
//
//	MemUtil_SetES(usSelector) ;
//	__asm__ __volatile__("movb %0, %%es:(%1)" : : "r"(uiValue), "r"(uiOffset)) ;
//
//	__asm__ __volatile__("pop %es") ;
}

void MemUtil_CopyMemory(__volatile__ unsigned short usSrcSelector, unsigned uiSrcBuffer,
					__volatile__ unsigned short usDestSelector, unsigned uiDestBuffer, unsigned uiNoOfBytes)
{
//	if(uiNoOfBytes == 0)
//		return ;
//
//  IrqGuard g;
//
//	__volatile__ unsigned usDS = MemUtil_GetDS() ;
//	__volatile__ unsigned usES = MemUtil_GetES() ;
//
//	__asm__ __volatile__("movw %%ss:%0, %%ds" : : "m"(usSrcSelector) ) ;
//	__asm__ __volatile__("movw %%ss:%0, %%es" : : "m"(usDestSelector) ) ;
//
//	__asm__	__volatile__
//	(
//		"cld;"
//		"movl %k0,%%ecx;"
//		"movl %k1,%%esi;"
//		"movl %k2,%%edi;"
//		"rep movsb;"
//		:
//		: "rm" (uiNoOfBytes), "rm" (uiSrcBuffer), "rm" (uiDestBuffer)
//		: "%ecx", "%esi", "%edi"
//	) ;
//
//	__asm__ __volatile__("movw %%ss:%0, %%ds" :: "m"(usDS) ) ;
//	__asm__ __volatile__("movw %%ss:%0, %%es" :: "m"(usES) ) ;
}

void
MemUtil_CopyMemoryBack(__volatile__ unsigned short usSrcSelector, unsigned uiSrcBuffer,
					__volatile__ unsigned short usDestSelector, unsigned uiDestBuffer, 
					unsigned uiNoOfBytes)
{
	if(uiNoOfBytes == 0)
		return ;

  IrqGuard g;

	__volatile__ unsigned usDS = MemUtil_GetDS() ;
	__volatile__ unsigned usES = MemUtil_GetES() ;

	__asm__ __volatile__("movw %%ss:%0, %%ds" :: "m"(usSrcSelector) ) ;
	__asm__ __volatile__("movw %%ss:%0, %%es" :: "m"(usDestSelector) ) ;

	__asm__	__volatile__
	(
		"std;"
		"movl %k0,%%ecx;"
		"movl %k1,%%esi;"
		"movl %k2,%%edi;"
		"rep movsb;"
		"cld;"
		:
		: "rm" (uiNoOfBytes), "rm" (uiSrcBuffer + uiNoOfBytes - 1), "rm" (uiDestBuffer + uiNoOfBytes - 1)
		: "%ecx", "%esi", "%edi"
	) ;

	__asm__ __volatile__("movw %%ss:%0, %%ds" :: "m"(usDS) ) ;
	__asm__ __volatile__("movw %%ss:%0, %%es" :: "m"(usES) ) ;
}

unsigned
MemUtil_GetCMOS(const byte bAddr)
{
	PortCom_SendByte(CMOS_COMMAND_PORT, bAddr) ;
	return PortCom_ReceiveByte(CMOS_RW_PORT) ;
}

void
MemUtil_PutCMOS(const byte bAddr, const byte bValue)
{
	PortCom_SendByte(CMOS_COMMAND_PORT, bAddr) ;
	PortCom_SendByte(CMOS_RW_PORT, bValue) ;
}

unsigned
MemUtil_GetExtendedMemorySize()
{
	unsigned uiExtendedMemSize = 0 ;
	
	uiExtendedMemSize = (MemUtil_GetCMOS(0x18) << 8) ;
	uiExtendedMemSize += MemUtil_GetCMOS(0x17) ;

	return uiExtendedMemSize ;
}

void MemUtil_Set(byte* pBuffer, byte bVal, unsigned uiLen)
{
	unsigned i ;
	for(i = 0; i < uiLen; i++)
		pBuffer[i] = bVal ;
}
