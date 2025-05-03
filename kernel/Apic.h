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

#ifndef _APIC_H_
#define _APIC_H_

#include <stdlib.h>
#include <IrqManager.h>

// APIC registers (as indices of uint32_t*)
enum APIC_REG_INDEX {
  APIC_APICID             = 0x20  / 4,
  APIC_APICVERSION        = 0x30  / 4,
  APIC_TASKPRIORITY       = 0x80  / 4,
  APIC_EOI                = 0xB0  / 4, // End of Interrupt
  APIC_LDR                = 0xD0  / 4, // Logical Destination
  APIC_DFR                = 0xE0  / 4, // Destination Format
  APIC_SPURIOUSINTERRUPT  = 0xF0  / 4,
  APIC_ESR                = 0x280 / 4, // Error Status
  APIC_ICRL               = 0x300 / 4, // Interrupt Command Lo
  APIC_ICRH               = 0x310 / 4, // Interrupt Command Hi

  APIC_TIMER              = 0x320 / 4, // LVT Timer Register, cf. 10.5.1 Local Vector Table
  APIC_THERMALSENSOR      = 0x330 / 4, // LVT Thermal Monitor Register, cf. 10.5.1 Local Vector Table
  APIC_PERFORMANCECOUNTER = 0x340 / 4, // LVT Performance Counter Register, cf. 10.5.1 Local Vector Table
  APIC_LINT0              = 0x350 / 4, // LVT LINT0 Register, cf. 10.5.1 Local Vector Table
  APIC_LINT1              = 0x360 / 4, // LVT LINT1 Register, cf. 10.5.1 Local Vector Table
  APIC_ERROR              = 0x370 / 4, // LVT Error Register, cf. 10.5.1 Local Vector Table

  APIC_TIMER_INITCOUNT    = 0x380 / 4,
  APIC_TIMER_CURRENTCOUNT = 0x390 / 4,
  APIC_TIMER_DIVIDECONFIG = 0x3E0 / 4,
};

class Apic : public IrqManager
{
  private:
    Apic();
    Apic(const Apic&);
    void Initialize();

  public:
    static bool IsAvailable();
    uint8_t GetLocalApicID() const;
    uint8_t GetIOApicID();
    uint32_t PhyApicBase() const { return _phyApicBase; }

  private:
    uint32_t* MmapBase(uint64_t vAddr, uint64_t pAddr);
    uint32_t IoApicRead(uint8_t index);
    void IoApicWrite(uint8_t index, uint32_t val);
    void RemapVector(uint8_t vector, uint32_t mapped, bool level /*f:edge t:level*/, bool low /*f:high t:low*/, bool disabled);

    void SendEOI(const IRQ&) override;
    void EnableIRQ(const IRQ&) override;
    void DisableIRQ(const IRQ&) override;
    bool IsIRQEnabled(const IRQ&) override;

  private:
    uint64_t _phyApicBase;
    __volatile__ uint32_t* _apicBase;
    __volatile__ uint32_t* _ioApicBase;

    friend class IrqManager;
};

//#define APIC_SW_ENABLE              BIT(8)
//#define APIC_CPUFOCUS               BIT(9)
//#define APIC_NMI                    BIT(10)
//
//#define TMR_PERIODIC                BIT(17)
//
//#define APIC_EXT_INT                (BIT(8) | BIT(9) | BIT(10))

#endif
