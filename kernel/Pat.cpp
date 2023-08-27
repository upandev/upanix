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

#include <Pat.h>
#include <Cpu.h>
#include <stdio.h>
#include <exception.h>

Pat::Pat() {
  _isSupported = Cpu::Instance().HasSupport(CF_MSR) && Cpu::Instance().HasSupport(CF_PAT);
  if (_isSupported) {
    printf("\nPAT is supported");
    _patReg = Cpu::Instance().MSRread(IA32_PAT);
    print();
  }
}

void Pat::print() {
  for(int i = 0; i < NO_OF_PAT_ENTRIES; ++i) {
    printf("\nPAT%d: %s", i, Cpu::memTypeToStr(get(i)));
  }
}

int Pat::pageTableFlag(Cpu::MEM_TYPE memType) {
  if (_isSupported) {
    for (int i = 0; i < NO_OF_PAT_ENTRIES; ++i) {
      if (get(i) == memType) {
        return ((i >> 2) & 0x1) << 7 | ((i >> 1) & 0x1) << 4 | (i & 0x1) << 3;
      }
    }
  }
  return -1;
}

Cpu::MEM_TYPE Pat::get(int i) {
  if (i < 0 || i > NO_OF_PAT_ENTRIES) {
    throw upan::exception(XLOC,"Invalid PAT index: %d", i);
  }
  return (Cpu::MEM_TYPE)((_patReg >> (i * 8)) & 0x7);
}

void Pat::set(int i, Cpu::MEM_TYPE memType) {
  if (i < 0 || i > NO_OF_PAT_ENTRIES) {
    throw upan::exception(XLOC,"Invalid PAT index: %d", i);
  }

  const uint64_t mask = ~(((uint64_t)0x7) << (i * 8));
  const uint64_t value = ((uint64_t)memType) << (i * 8);
  _patReg = (_patReg & mask) | value;

  Cpu::Instance().MSRwrite(IA32_PAT, _patReg);
}