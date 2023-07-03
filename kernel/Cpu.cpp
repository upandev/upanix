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

#include <Cpu.h>
#include <stdio.h>
#include <exception.h>

Cpu::Cpu()
{
//  uint32_t result = 0;
//  // Test if the CPU supports the CPUID-Command
//  __asm__ __volatile__("pushfl;"
//                       "pushfl;"
//                       "pop %%eax;"
//                       "mov %%eax, %%ecx;"
//                       "xorl $0x200000, %%eax;"
//                       "push %%eax;"
//                       "popfl;"
//                       "pushfl;"
//                       "pop %%eax;"
//                       "xorl %%ecx, %%eax;"
//                       "shrl $21, %%eax;"
//                       "popfl;" : "=m"(result) : : "eax", "ecx", "memory");
//  _cpuIdAvailable = (result == 0);
//  if(_cpuIdAvailable) {
//    printf("\n CPUID is available");
//    EnableSSE();
//  }
}

bool Cpu::HasSupport(CPU_FEATURE feature)
{
  if(feature == CF_CPUID)
      return _cpuIdAvailable;

  if(!_cpuIdAvailable)
      return false;

  CPU_REGISTER r = (CPU_REGISTER)(feature & ~31);

  return GetIdRegister(0x1, r) & (1 << (feature - r));
}

uint32_t Cpu::GetIdRegister(uint32_t function, CPU_REGISTER reg)
{
  switch (reg)
  {
    case CR_EAX: {
      uint32_t temp;
      __asm__ __volatile__("cpuid" : "=a"(temp) : "a"(function) : "ebx", "ecx", "edx");
      return temp;
    }
    case CR_EBX: {
      uint32_t temp, temp2;
      __asm__ __volatile__("cpuid" : "=b"(temp), "=a"(temp2) : "a"(function) : "ecx", "edx");
      return temp;
    }
    case CR_ECX: {
      uint32_t temp, temp2;
      __asm__ __volatile__("cpuid" : "=c"(temp), "=a"(temp2) : "a"(function) : "ebx", "edx");
      return (temp);
    }
    case CR_EDX: {
      uint32_t temp, temp2;
      __asm__ __volatile__("cpuid" : "=d"(temp), "=a"(temp2) : "a"(function) : "ebx", "ecx");
      return (temp);
    }
  }
  return 0;
}

uint64_t Cpu::MSRread(uint32_t msr)
{
  uint32_t low, high;
  __asm__ __volatile__("rdmsr" : "=a" (low), "=d" (high) : "c" (msr));
  return ((uint64_t)high << 32) | low;
}

void Cpu::MSRwrite(uint32_t msr, uint64_t value)
{
  uint32_t low = value & 0xFFFFFFFF;
  uint32_t high = value >> 32;
  __asm__ __volatile__("wrmsr" :: "a"(low), "c"(msr), "d"(high));
}

void Cpu::EnableSSE() {
//  if (HasSupport(CF_SSE) && HasSupport(CF_SSE2)) {
//    printf("\n SSE/SSE2 is supported.");
//    __asm__ __volatile__("mov %%cr0, %%eax;"
//                         "and $0xFFFB, %%ax;" // clear coprocessor emulation CR0.EM
//                         "or $0x2, %%ax;" // set coprocessor monitoring CR0.MP
//                         "mov %%eax, %%cr0;"
//                         "mov %%cr4, %%eax;"
//                         "or $0x600, %%ax;" // set CR4.OSFXSR and CR4.OSXMMEXCPT
//                         "mov %%eax, %%cr4;" : : : );
//    printf("\n SSE/SSE2 enabled");
//    if(HasSupport(CF_FPU)) {
//      //set NE flag
//      __asm__ __volatile__("mov %%cr0, %%eax;"
//                           "or $0x20, %%ax;"
//                           "mov %%eax, %%cr0;" : : : );
//      printf("\n Builtin FPU found");
//    }
//  } else {
//    printf("\n SSE/SSE2 is not supported!!");
//    while (1);
//  }
}

uint32_t Cpu::GetRegValue(Cpu::Register reg) {
  uint32_t val;
  switch(reg) {
    case Register::CR0:
      //__asm__ __volatile__("mov %%cr0, %0" :  "=r"(val) : );
      break;
    case Register::CR1:
      //__asm__ __volatile__("mov %%cr1, %0" :  "=r"(val) : );
      break;
    case Register::CR2:
      //__asm__ __volatile__("mov %%cr2, %0" :  "=r"(val) : );
      break;
    case Register::CR3:
      //__asm__ __volatile__("mov %%cr3, %0" :  "=r"(val) : );
      break;
    case Register::CR4:
      //__volatile__("mov %%cr4, %0" :  "=r"(val) : );
      break;
    default:
      throw upan::exception(XLOC, "Unrecognized register: %d", reg);
  }
  return val;
}

const char* Cpu::memTypeToStr(MEM_TYPE memType) {
  switch (memType) {
    case UNCACHEABLE: return "Uncacheable";
    case WRITE_COMBINING: return "Write-Combining";
    case WRITE_THROUGH: return "Write-Through";
    case WRITE_PROTECTED: return "Write-Protected";
    case WRITE_BACK: return "WriteBack";
    case UNCACHED: return "Uncached";
  }
  return "undefined";
}