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

#include <Mtrr.h>
#include <stdio.h>
#include <Pat.h>
#include "PortCom.h"
#include "ConsoleCommands.h"

// Memory Type Range Registers
Mtrr::Mtrr() {
  _isSupported = Cpu::Instance().HasSupport(CF_MSR) && Cpu::Instance().HasSupport(CF_MTRR);
  if (_isSupported) {
    printf("\nMTRR is supported");
    _capReg = static_cast<CapReg>(Cpu::Instance().MSRread(IA32_MTRR_CAP));
    _capReg.Print();

    _defType = static_cast<DefType>(Cpu::Instance().MSRread(IA32_MTRR_DEF_TYPE));
    _defType.Print();

    if(_defType.isMTRREnabled()) {
      if (_defType.isFixedRangeMTRREnabled()) {
        _fixedR._fixed64 = Cpu::Instance().MSRread(IA32_MTRR_FIXED64K);
        _fixedR._fixed16[0] = Cpu::Instance().MSRread(IA32_MTRR_FIXED16K);
        _fixedR._fixed16[1] = Cpu::Instance().MSRread(IA32_MTRR_FIXED16K + 1);
        for (int i = 0; i < 8; ++i) {
          _fixedR._fixed4[i] = Cpu::Instance().MSRread(IA32_MTRR_FIXED4K + i);
        }
      }

      for (uint32_t i = 0; i < _capReg.noOfVariableSizeRangeRegisters(); ++i) {
        const auto base = Cpu::Instance().MSRread(IA32_MTRR_PHY_BASEn + i * 2);
        const auto mask = Cpu::Instance().MSRread(IA32_MTRR_PHY_MASKn + i * 2);
        VarRR varRR(base, mask);
        varRR.Print();
        _varRRs.push_back(varRR);
      }
    }

    if (_capReg.isWriteCombiningMemTypeSupported() && Pat::Instance().isSupported()) {
      printf("\n Updating PAT7 to Write-Combining");
      Pat::Instance().set(7, Cpu::MEM_TYPE::WRITE_COMBINING);
      Pat::Instance().print();
    }
  }
}

void Mtrr::CapReg::Print() {
  printf("\n VCNT: %d, FIX: %d, WC: %d, SMRR: %d",
         noOfVariableSizeRangeRegisters(),
         isFixRangeRegistersSupported(),
         isWriteCombiningMemTypeSupported(),
         isSysManagementRangeRegistersSupported());
}

void Mtrr::DefType::Print() {
  printf("\n Def MemType: %s, FE: %d, E: %d",
         Cpu::memTypeToStr(defaultMemoryType()),
         isFixedRangeMTRREnabled(),
         isMTRREnabled());
}

void Mtrr::VarRR::Print() {
  printf("\n MemType: %s, Base: %x, Mask: %x, Valid: %d",
         Cpu::memTypeToStr(memType()),
         base(),
         mask(),
         isValid());
}