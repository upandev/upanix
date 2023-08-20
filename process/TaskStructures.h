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
#pragma once
#include <Global.h>
#include <vector.h>

class TaskState
{
public:
  void BuildForUser(uint64_t stackStartAddress, uint64_t pml4Address, uint64_t entryAdddress, unsigned processEntryStackSize);
  void BuildForKernel(const uintptr_t uiTaskAddress, unsigned uiStackTop, const upan::vector<uintptr_t>& params);

  unsigned short	backlink ;
  unsigned short	FILLER1 ;

  unsigned		ESP0 ;
  unsigned short	SS0 ;
  unsigned short	FILLER2 ;

  unsigned		ESP1 ;
  unsigned short	SS1 ;
  unsigned short	FILLER3 ;

  unsigned		ESP2 ;
  unsigned short	SS2 ;
  unsigned short	FILLER4 ;

  uint64_t 		CR3_PDBR ;
  unsigned		EIP ;
  unsigned		EFLAGS ;

  unsigned		EAX ;
  unsigned		ECX ;
  unsigned		EDX ;
  unsigned		EBX ;
  unsigned		ESP ;
  unsigned		EBP ;
  unsigned		ESI ;
  unsigned		EDI ;

  unsigned short	ES ;
  unsigned short	FILLER5 ;

  unsigned short	CS ;
  unsigned short	FILLER6 ;

  unsigned short	SS ;
  unsigned short	FILLER7 ;

  unsigned short	DS ;
  unsigned short	FILLER8 ;

  unsigned short	FS ;
  unsigned short	FILLER9 ;

  unsigned short	GS ;
  unsigned short	FILLER10 ;

  unsigned short	LDT ;
  unsigned short	FILLER11 ;

  unsigned short	DEBUG_T_BIT ;
  unsigned short	IO_MAP_BASE ;
} PACKED;

class Descriptor
{
public:
  void Build(unsigned uiLimit, unsigned uiBase, byte bType, byte bFlag);

private:
  unsigned short	_usLimit15_0;
  unsigned short	_usBase15_0 ;
  byte			_bBase23_16 ;
  byte			_bType ;
  byte			_bLimit19_16_Flags ;
  byte			_bBase31_24 ;
} PACKED;

typedef struct
{
  unsigned short int lowerOffset ;
  unsigned short int selector ;
  byte parameterCount ;
  byte options ; /*	constant:5;	 5 bits
           dpl:2;		 2 bits
           present:1;	 1 bit		*/
  unsigned short int upperOffset ;
} PACKED GateDescriptor ;

class ProcessLDT
{
public:
  void BuildForUser();
  void BuildForKernel();

private:
  Descriptor NullDesc ;
  Descriptor LinearDesc ;
  Descriptor CodeDesc ;
  Descriptor DataDesc ;
  Descriptor StackDesc ;
  Descriptor CallGateStackDesc ;
} PACKED;
