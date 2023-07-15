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

#include <mutex.h>
#include <Apic.h>
#include <Cpu.h>
#include <Acpi.h>
#include <PIC.h>
#include <MemConstants.h>
#include <MemManager.h>
#include <Bit.h>
#include <PCIBusHandler.h>
#include <PortCom.h>
#include <PIT.h>
#include <atomicop.h>

#define IA32_APIC_BASE_MSR          0x1B
#define IA32_APIC_BASE_BSP          0x100
#define IA32_APIC_BASE_MSR_ENABLE   0x800

// IO APIC
/* Offset der memory-mapped Register */
#define IOAPIC_IOREGSEL      0
#define IOAPIC_IOWIN         4

#define IOAPIC_ID            0x00
#define IOAPIC_VERSION       0x01
#define IOAPIC_ARBITRATIONID 0x02

static const uint32_t APIC_INTERRUPTDISABLED = 0x10000;
static const uint32_t APIC_LEVEL = (1U << 13);
static const uint32_t APIC_LOW = (1U << 15);
static const uint32_t TMR_PERIODIC = (1U << 17);
static const uint32_t APIC_SW_ENABLE = (1U << 8);
static const uint32_t APIC_NMI = (1U << 10);
static const uint32_t APIC_EXT_INT = (1U << 8) | (1U << 9) | (1U << 10);

bool Apic::IsAvailable()
{
  if(Cpu::Instance().HasSupport(CF_MSR) && Cpu::Instance().HasSupport(CF_APIC)) // We need MSR (to initialize APIC) and (obviously) APIC
  {
    // Ensure that I/O-APIC is available - this is the case if its address was found in the ACPI tables
    return Acpi::Instance().GetMadt().GetIoApics().size() > 0;
  }
  return false;
}

Apic::Apic() : _apicBase(nullptr), _ioApicBase(nullptr)
{
}

void Apic::Initialize()
{
  // Local APIC, cf. Intel manual 3A, chapter 10
  _phyApicBase = (Cpu::Instance().MSRread(IA32_APIC_BASE_MSR) & ~0xFFFU); // read APIC base address (ignore bit0-11)
  Cpu::Instance().MSRwrite(IA32_APIC_BASE_MSR, ((_phyApicBase & ~0xFFFU) | IA32_APIC_BASE_BSP | IA32_APIC_BASE_MSR_ENABLE)); // enable APIC, Bootstrap Processor
  _apicBase = MmapBase(MMAP_APIC_BASE, _phyApicBase);

  uint64_t phyIoApicBase = (*Acpi::Instance().GetMadt().GetIoApics().begin()).Address();
  _ioApicBase = MmapBase(MMAP_IOAPIC_BASE, phyIoApicBase);

  printf("\n APIC Base: %x, IO APIC Base: %x", _phyApicBase, phyIoApicBase);

  IrqGuard g;

  uint8_t ioApicMaxIndexRedirTab = Bit::Byte3(IoApicRead(IOAPIC_VERSION)); // bit16-23 // Maximum Redirection Entry  ReadOnly
  for(int i = 0; i < 16; ++i) {
    // Remap ISA IRQs (edge/hi)
    RemapVector(i, i + IRQ_BASE, false /*edge*/, false /*high*/, true/*enabled, except #2*/);
  }

  auto& irqABCD = PCIBusHandler::Instance().IrqABCD();
  if (ioApicMaxIndexRedirTab >= 19 && !irqABCD.empty())
  {
    auto mappedIrq = irqABCD.begin();
    for(uint8_t i = 16; i < 20 && i < ioApicMaxIndexRedirTab && mappedIrq != irqABCD.end(); ++i, ++mappedIrq)
    {
      const auto& irq = *mappedIrq;
      // 0x80 is default value in P2I bridge device; 0, 1, 2, 13 are not allowed.
      bool disabled = (irq == 0x80 || irq == 0 || irq == 1 || irq == 2 || irq == 13);
      disabled = true;

      /// TODO: what is correct? level/low or edge/high
      /*
      // Remap PCI lines A#, B#, C#, D# (level/low)
      ioapic_remapVector(i, mapped + IRQ_BASE, true, true, disabled); // bochs: PIRQA# set to 0x0b, PIRQC# set to 0x0b
      */

      // Remap PCI lines A#, B#, C#, D# (edge/high)
      RemapVector(i, irq + IRQ_BASE, false, false, disabled); // bochs: PIRQA# set to 0x0b, PIRQC# set to 0x0b
    }
  }

  auto& irqEFGH = PCIBusHandler::Instance().IrqEFGH();
  if (ioApicMaxIndexRedirTab >= 23 && !irqEFGH.empty())
  {
    auto mappedIrq = irqEFGH.begin();
    for (uint8_t i = 20; i < 24 && mappedIrq != irqEFGH.end(); ++i, ++mappedIrq)
    {
      const auto& irq = *mappedIrq;
      // Remap motherboard interrupts 20/21, general purpose interrupt 22, INTIN23 or SMI# (dependant on mask bit)
      // Could also be E#, F#, G#, H# from PCI
      // 0x80 is default value in P2I bridge device; 0, 1, 2, 13 are not allowed.
      bool disabled = (irq == 0x80 || irq == 0 || irq == 1 || irq == 2 || irq == 13);
      disabled = true;

      /// TODO: what is correct? level/low or edge/high
      // Remap PCI lines E#, F#, G#, H# (level/low)
      RemapVector(i, irq + IRQ_BASE, true, true, disabled);
    }
  }

  /////////////////////////////////////////////////
  // Local APIC, cf. Intel manual 3A, chapter 10

  _apicBase[APIC_DFR] =  0xFFFFFFFF;             // Put the APIC into flat delivery mode

  _apicBase[APIC_LDR] &= 0x00FFFFFF;             // LDR mask
  uint8_t logicalApicID = 0;                     // Logical APIC ID
  _apicBase[APIC_LDR] |= logicalApicID<<24;      // LDR

  _apicBase[APIC_TASKPRIORITY] = 0;

  _apicBase[APIC_LINT0] = APIC_EXT_INT | (1U << 15); // Enable normal external Interrupts // binary 1000 0111 0000 0000
  _apicBase[APIC_LINT1] = APIC_NMI;               // Enable NMI Processing
  _apicBase[APIC_ERROR] = APIC_INTERRUPTDISABLED; // Disable Error Interrupts
  _apicBase[APIC_THERMALSENSOR] = APIC_INTERRUPTDISABLED; // Disable thermal sensor interrupts
  _apicBase[APIC_PERFORMANCECOUNTER] = APIC_NMI;  // Enable NMI Processing

  // Local APIC timer
  _apicBase[APIC_TIMER_DIVIDECONFIG] = 0x03; // Set up divide value to 16
  _apicBase[APIC_TIMER] = TMR_PERIODIC | (IRQ_BASE + StdIRQ::Instance().TIMER_IRQ.GetIRQNo()); // Enable timer interrupts, 32, IRQ0
  _apicBase[APIC_TIMER_INITCOUNT] = 0xFFFFFFFF; // Set initial counter -> starts timer

  _apicBase[APIC_SPURIOUSINTERRUPT] = APIC_SW_ENABLE | (IRQ_BASE + 7); // Enable APIC. Spurious: 39, IRQ7 (bit0-7)

  // repeat
  _apicBase[APIC_LINT0] = APIC_EXT_INT | (1U << 15); // Enable normal external Interrupts // binär 1000 0111 0000 0000
  _apicBase[APIC_LINT1] = APIC_NMI; // Enable NMI Processing

  // we use PIT (counter 2) for determining correct APIC timer init counter value
  uint16_t partOfSecond = 20; // 50 ms
  PortCom_SendByte(PIT_COUNTER_2_CTRLPORT, (PortCom_ReceiveByte(PIT_COUNTER_2_CTRLPORT) & 0xFD) | 1);
  PortCom_SendByte(PIT_MODE_PORT, PIT_COUNTER_2 | PIT_RW_LO_HI_MODE | PIT_MODE0 | SIXTEEN_BIT_BINARY); // Mode 0 is important!
  uint32_t val = TIMECOUNTER_i8254_FREQU / partOfSecond;
  PortCom_SendByte(PIT_COUNTER_2_PORT, Bit::Byte1(val)); //LSB
  PortCom_SendByte(PIT_COUNTER_2_PORT, Bit::Byte2(val)); //MSB

  // start PIT counting and APIC timer
  val = PortCom_ReceiveByte(PIT_COUNTER_2_CTRLPORT) & 0xFE;
  PortCom_SendByte(PIT_COUNTER_2_CTRLPORT, val);
  PortCom_SendByte(PIT_COUNTER_2_CTRLPORT, val | 1);
  _apicBase[APIC_TIMER_INITCOUNT] = 0xFFFFFFFF;

  // PIT timer at zero?
  while (!(PortCom_ReceiveByte(PIT_COUNTER_2_CTRLPORT) & (1U << 5))) 
  {
    //if no, do nothing
  };
  _apicBase[APIC_TIMER] = APIC_INTERRUPTDISABLED; // if yes, stop APIC timer

  // calculate value for APIC timer
  val = (((0xFFFFFFFF - _apicBase[APIC_TIMER_CURRENTCOUNT]) + 1) / INT_PER_SEC) * partOfSecond;

  // setting divide value register again and set timer
  _apicBase[APIC_TIMER_DIVIDECONFIG] = 0x03;
  _apicBase[APIC_TIMER] = TMR_PERIODIC | (IRQ_BASE + StdIRQ::Instance().TIMER_IRQ.GetIRQNo());

  // set APIC timer init value
  _apicBase[APIC_TIMER_INITCOUNT] = (val < 16 ? 16 : val);
}

uint32_t* Apic::MmapBase(uint64_t vAddr, uint64_t pAddr) {
  MemManager::Mmap(vAddr, pAddr, 0x3);
	Mem_FlushTLB();
  return (uint32_t*)(vAddr + (pAddr % PAGE_SIZE));
}

// read IO APIC register
uint32_t Apic::IoApicRead(uint8_t index)
{
  _ioApicBase[IOAPIC_IOREGSEL] = index;
  return _ioApicBase[IOAPIC_IOWIN];
}

// write IO APIC register
void Apic::IoApicWrite(uint8_t index, uint32_t val)
{
  _ioApicBase[IOAPIC_IOREGSEL] = index;
  _ioApicBase[IOAPIC_IOWIN] = val;
}

void Apic::RemapVector(uint8_t vector, uint32_t mapped, bool level /*f:edge t:level*/, bool low /*f:high t:low*/, bool disabled)
{
  // INTEL 82093AA I/O ADVANCED PROGRAMMABLE INTERRUPT CONTROLLER (IOAPIC)
  // 3.2.4. IOREDTBL[23:0]I/O REDIRECTION TABLE REGISTERS

  // 63:56 Destination FieldR/W:
  // If the Destination Mode of this entry is Physical Mode (bit 11=0), bits [59:56] contain an APIC ID
  // If Logical Mode is selected (bit 11=1), the Destination Field potentially defines a set of processors.
  // Bits [63:56] of the Destination Field specify the logical destination address.
  // 55:17 Reserved.
  // 16 Interrupt MaskR/W: 1: masked
  // 15 Trigger ModeR/W: 1: Level sensitive, 0: Edge sensitive
  // 14 Remote IRRRO.
  // 13 Interrupt Input Pin Polarity (INTPOL)R/W: 0: High active, 1: Low active
  // 12 Delivery Status (DELIVS)RO.
  // 11 Destination Mode (DESTMOD)R/W: 0: Physical Mode, 1: Logical Mode
  // 10:8 Delivery Mode (DELMOD)R/W: 000: Fixed, 010: SMI, 100: NMI, 101: INIT, 111: ExtINT
  // 7:0 Interrupt Vector (INTVEC)R/W: Vector values range from 10h to FEh.

  if(disabled)
    mapped |= APIC_INTERRUPTDISABLED;

  if(level)
    mapped |= APIC_LEVEL;

  if(low)
    mapped |= APIC_LOW;

  IoApicWrite(0x10 + vector * 2, mapped);
  IoApicWrite(0x11 + vector * 2, _apicBase[APIC_APICID] << (56 - 32));
}

uint8_t Apic::GetLocalApicID() const
{
  return _apicBase[APIC_APICID];
}

uint8_t Apic::GetIOApicID()
{
  return ((IoApicRead(IOAPIC_ID) >> 24) & 0xF);
}

void Apic::SendEOI(const IRQ&)
{
//  _apicBase[APIC_EOI] = 0;
  upan::atomic::op::swap(_apicBase[APIC_EOI], 0);
}

void Apic::EnableIRQ(const IRQ& irq)
{
  IrqGuard g;
  uint32_t entry = 0x10 + irq.GetIRQNo() * 2;
  uint32_t ivalue = IoApicRead(entry);
  ivalue &= ~(APIC_INTERRUPTDISABLED);
  IoApicWrite(entry, ivalue);
}

void Apic::DisableIRQ(const IRQ& irq)
{
  IrqGuard g;
  uint32_t entry = 0x10 + irq.GetIRQNo() * 2;
  uint32_t ivalue = IoApicRead(entry);
  ivalue |= APIC_INTERRUPTDISABLED;
  IoApicWrite(entry, ivalue);
}
