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

#ifndef _CPU_H_
#define _CPU_H_

#include <stdlib.h>

enum CPU_REGISTER
{ // Uses bits larger than 5 because the bitnumber inside the registers can be 31 at maximum which needs the first 5 bits
  CR_EAX = 0x20,
  CR_EBX = 0x40,
  CR_ECX = 0x80,
  CR_EDX = 0x100
};

enum CPU_FEATURE
{ // In this enum the last five bits contain the bit in the register, the other bits are the register (see above)
  CF_CPUID        = 0,

  CF_FPU          = CR_EDX|0,
  CF_VME86        = CR_EDX|1,
  CF_IOBREAKPOINT = CR_EDX|2,
  CF_PAGES4MB     = CR_EDX|3,
  CF_RDTSC        = CR_EDX|4,
  CF_MSR          = CR_EDX|5, // Model Specific Registers
  CF_PAE          = CR_EDX|6,
  CF_MCE          = CR_EDX|7,
  CF_X8           = CR_EDX|8,
  CF_APIC         = CR_EDX|9,
  CF_SYSENTEREXIT = CR_EDX|11,
  CF_MTRR         = CR_EDX|12, // Memory-Type Range Registers
  CF_PGE          = CR_EDX|13,
  CF_MCA          = CR_EDX|14,
  CF_CMOV         = CR_EDX|15,
  CF_PAT          = CR_EDX|16,
  CF_PSE36        = CR_EDX|17,
  CF_CLFLUSH      = CR_EDX|19,
  CF_MMX          = CR_EDX|23,
  CF_FXSR         = CR_EDX|24,
  CF_SSE          = CR_EDX|25,
  CF_SSE2         = CR_EDX|26,
  CF_HTT          = CR_EDX|28,

  CF_SSE3         = CR_ECX|0,
  CF_PCLMULQDQ    = CR_ECX|1,
  CF_MONITOR      = CR_ECX|3,
  CF_SSSE3        = CR_ECX|9,
  CF_FMA          = CR_ECX|12,
  CF_CMPXCHG16B   = CR_ECX|13,
  CF_SSE41        = CR_ECX|19,
  CF_SSE42        = CR_ECX|20,
  CF_MOVBE        = CR_ECX|22,
  CF_POPCNT       = CR_ECX|23,
  CF_AESNI        = CR_ECX|25,
  CF_XSAVE        = CR_ECX|26,
  CF_OSXSAVE      = CR_ECX|27,
  CF_AVX          = CR_ECX|28,
  CF_F16C         = CR_ECX|29,
  CF_RDRAND       = CR_ECX|30,
};

#define MSR_FS_BASE 0xC0000100
#define IA32_EFER 0xC0000080
#define IA32_STAR 0xC0000081
#define IA32_LSTAR 0xC0000082
#define IA32_FMASK 0xC0000084

class Cpu {
  private:
    Cpu();
    Cpu(const Cpu&);

  public:
    static Cpu& Instance() {
      static Cpu cpu;
      return cpu;
    }
    bool HasSupport(CPU_FEATURE feature);
    uint64_t MSRread(uint32_t msr);
    void MSRwrite(uint32_t msr, uint64_t value);

    typedef enum {
      UNCACHEABLE = 0,
      WRITE_COMBINING = 1,
      WRITE_THROUGH = 4,
      WRITE_PROTECTED = 5,
      WRITE_BACK = 6,
      UNCACHED = 7
    } MEM_TYPE;

    typedef enum {
      CR0, CR1, CR2, CR3, CR4
    } Register;

    static const char* memTypeToStr(MEM_TYPE memType);
    static uint64_t GetRegValue(Cpu::Register reg);
    static void SetRegValue(Cpu::Register reg, uint64_t val);

  private:
    void EnableSSE();
    uint32_t GetIdRegister(uint32_t function, CPU_REGISTER reg);

    bool _cpuIdAvailable;
};

#endif
