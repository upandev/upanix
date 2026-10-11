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

bool Cpu::HasSupport(CPU_FEATURE feature) {
  if(feature == CF_CPUID) {
      return true;
  }

  CPU_REGISTER r = (CPU_REGISTER)(feature & ~31);
  return GetIdRegister(0x1, r) & (1 << (feature - r));
}

uint32_t Cpu::GetIdRegister(uint32_t function, CPU_REGISTER reg) {
  switch (reg) {
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

uint64_t Cpu::MSRread(uint32_t msr) {
  uint32_t low, high;
  __asm__ __volatile__("rdmsr" : "=a" (low), "=d" (high) : "c" (msr));
  return ((uint64_t)high << 32) | low;
}

void Cpu::MSRwrite(uint32_t msr, uint64_t value) {
  uint32_t low = value & 0xFFFFFFFF;
  uint32_t high = value >> 32;
  __asm__ __volatile__("wrmsr" :: "a"(low), "c"(msr), "d"(high));
}

uint64_t Cpu::GetRegValue(Cpu::Register reg) {
  uint64_t val;
  switch(reg) {
    case Register::CR0:
      __asm__ __volatile__("mov %%cr0, %0" :  "=r"(val) : );
      break;
    case Register::CR1:
      __asm__ __volatile__("mov %%cr1, %0" :  "=r"(val) : );
      break;
    case Register::CR2:
      __asm__ __volatile__("mov %%cr2, %0" :  "=r"(val) : );
      break;
    case Register::CR3:
      __asm__ __volatile__("mov %%cr3, %0" :  "=r"(val) : );
      break;
    case Register::CR4:
      __asm__ __volatile__("mov %%cr4, %0" :  "=r"(val) : );
      break;
    default:
      throw upan::exception(XLOC, "Unrecognized register: %d", reg);
  }
  return val;
}

void Cpu::SetRegValue(Cpu::Register reg, uint64_t val) {
  switch(reg) {
    case Register::CR0:
      __asm__ __volatile__("mov %0, %%cr0" : : "r"(val));
      break;
    case Register::CR1:
      __asm__ __volatile__("mov %0, %%cr1" : : "r"(val));
      break;
    case Register::CR2:
      __asm__ __volatile__("mov %0, %%cr2" : : "r"(val));
      break;
    case Register::CR3:
      __asm__ __volatile__("mov %0, %%cr3" : : "r"(val));
      break;
    case Register::CR4:
      __asm__ __volatile__("mov %0, %%cr4" : : "r"(val));
      break;
    default:
      throw upan::exception(XLOC, "Unrecognized register: %d", reg);
  }
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