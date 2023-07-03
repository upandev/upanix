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

#include <Acpi.h>
#include <MultiBoot.h>
#include <ustring.h>
#include <MemManager.h>

#define MADT_LOCAL_APIC                     0
#define MADT_IO_APIC                        1
#define MADT_INTERRUPT_SOURCE_OVERRIDE      2
#define MADT_NMI                            3
#define MADT_LOCAL_APIC_NMI                 4
#define MADT_LOCAL_APIC_ADDRESS_OVERRIDE    5

Acpi::Acpi() : _isAvailable(false)
{
  auto rsdp = Rsdp::Search();
  if(!rsdp)
    return;
  upan::list<uint32_t> headerAddresses;
  ReadRootTable(*rsdp, headerAddresses);
  for(auto headerAddress : headerAddresses)
    ReadHeader(headerAddress);
  _isAvailable = true;
}

void Acpi::ReadRootTable(const Acpi::Rsdp& rsdp, upan::list<uint32_t>& headerAddresses)
{
  printf("\n ACPI Revision: %d", rsdp.Revision());
  printf("\n ACPI Root SysTable Addr: %x", rsdp.RootSystemTableAddr());
  uint32_t mappedAddr = rsdp.RootSystemTableAddr();
  const Acpi::Header* table = reinterpret_cast<Acpi::Header*>(mappedAddr);
  if(!table->IsRootTable())
    return;
  printf("\n ACPI Table Length: %d", table->Length());
  for(uint32_t* a = (uint32_t*)(mappedAddr + sizeof(Acpi::Header)); a < (uint32_t*)(mappedAddr + table->Length()); ++a)
    headerAddresses.push_back(*a);
}

void Acpi::ReadHeader(uint32_t headerAddress)
{
  printf("\n ACPI Table Header Addr: %x", headerAddress);
  uint32_t mappedAddr = headerAddress;
  const Acpi::Header* header = reinterpret_cast<Acpi::Header*>(mappedAddr);
  if(header->IsApicHeader())
    Parse(static_cast<const MadtHeader&>(*header));
}

void Acpi::Parse(const Acpi::MadtHeader& madtHeader)
{
  _madt.LocalApicAddr(madtHeader.LocalApicAddr());
  _madt.IsPcAtCompatible(madtHeader.IsPcAtCompatible());

  for(uint32_t i = (uint32_t)&madtHeader + sizeof(MadtHeader);
      i < (uint32_t)&madtHeader + madtHeader.Length();
      i += ((MadtEntry*)i)->Length())
  {
    const MadtEntry& e = *((MadtEntry*)i);
    switch(e.Type())
    {
      case MADT_LOCAL_APIC: {
        // ACPI Spec 5.2.12.2 Processor Local APIC Structure
        if(e.Length() != 8)
          throw upan::exception(XLOC, "Local APIC entry found with incorrect length: %d", e.Length());
        auto& se = static_cast<const MadtLocalApicEntry&>(e);
        if(se.IsEnabled())
          _madt.AddLocalApic(se.ProcessorId(), se.Id());
      } 
      break;
      case MADT_IO_APIC: {
        // ACPI Spec 5.2.12.3 I/O APIC Structure
        if(e.Length() != 12)
          throw upan::exception(XLOC, "IO APIC entry found with incorrect length: %d", e.Length());
        auto& se = static_cast<const MadtIoApicEntry&>(e);
        _madt.AddIoApic(se.Id(), se.Addr(), se.InterruptBase());
      }
      break;
      case MADT_INTERRUPT_SOURCE_OVERRIDE: {
        // ACPI Spec 5.2.12.5 Interrupt Source Override Structure
        if(e.Length() != 10)
          throw upan::exception(XLOC, "Interrupt Source Override entry found with incorrect length: %d", e.Length());
        auto& se = static_cast<const MadtIntOverrideEntry&>(e);
        _madt.AddIntSrcOverride(se.PicIrq(), se.ApicMappedIrq(), se.Flags());
      }
      break;
      case MADT_NMI: {
        // ACPI Spec 5.2.12.6 Non-Maskable Interrupt Source Structure
        if(e.Length() != 8)
          throw upan::exception(XLOC, "NMI entry found with incorrect length: %d", e.Length());
        auto& se = static_cast<const MadtNmiEntry&>(e);
        _madt.AddNmi(se.Flags(), se.IntNo());
      }
      break;
      case MADT_LOCAL_APIC_NMI: {
        // ACPI Spec 5.2.12.7 Local APIC NMI Structure
        if(e.Length() != 6)
          throw upan::exception(XLOC, "Local APIC NMI entry found with incorrect length: %d", e.Length());
        auto& se = static_cast<const MadtLocalApicNmiEntry&>(e);
        _madt.AddLocalApicNmi(se.ProcessorId(), se.Flags(), se.LintPin());
      }
      break;
      case MADT_LOCAL_APIC_ADDRESS_OVERRIDE: {
        // ACPI Spec 5.2.12.8 Local APIC Address Override Structure
        if(e.Length() != 12)
          throw upan::exception(XLOC, "Local APIC Addr Override entry found with incorrect length: %d", e.Length());
        auto& se = static_cast<const MadtLocalApicAddrOverrideEntry&>(e);
        _madt.LocalApicAddr(se.Addr());
      }
      break;
    }
  }
  _madt.DebugPrint();
}

bool Acpi::Header::IsRootTable() const
{
  static const char* RSDP_TABSIG = "RSDT";
  return memcmp(_signature, RSDP_TABSIG, TABLE_SIG_LEN) == 0;
}

bool Acpi::Header::IsApicHeader() const
{
  static const char* APIC_HEADER = "APIC";
  return memcmp(_signature, APIC_HEADER, TABLE_SIG_LEN) == 0;
}

const Acpi::Rsdp* Acpi::Rsdp::Search()
{
  auto acpi_mmap = MultiBoot::Instance().GetACPIInfoMemMap();
  if(!acpi_mmap)
  {
    printf("\n GRUB multiboot info doesn't have ACPI memory mapped area");
    return nullptr;
  }

  printf("\n ACPI info base address: 0x%llx, len: %llu", acpi_mmap->addr, acpi_mmap->length);
  for(unsigned i = 0; i < (acpi_mmap->length + RSDP_SIG_LEN - 1); i += 16)
  {
    uint8_t* a = reinterpret_cast<uint8_t*>(acpi_mmap->addr + i);
    Rsdp* rsdp = reinterpret_cast<Rsdp*>(a);
    if(rsdp->MatchSignature())
    {
      uint8_t checksum = 0;

      uint8_t* end = a + 20; // ACPI 1.0 spec size
      for(uint8_t* mem = a; mem != end; ++mem) 
        checksum += *mem; // First checksum

      if(!checksum && rsdp->Revision() == 2) // Test worked && newest ACPI spec
      {
          end = a + sizeof(Rsdp) - 3; // Current RDSP size - 3 reserved bytes
          for (uint8_t* mem = a; mem != end; ++mem) checksum += *mem; // Extended checksum
      }

      if(!checksum) 
        return rsdp;
    }
  }
  return nullptr;
}

bool Acpi::Rsdp::MatchSignature() const
{
  static const char* RSDP_SIGNATURE = "RSD PTR ";
  return memcmp(_signature, RSDP_SIGNATURE, RSDP_SIG_LEN) == 0;
}

void Acpi::Madt::DebugPrint() const
{
  printf("\n Local APIC Address: %x", _localApicAddr);
  printf("\n Is PC/AT Compatible: %d", _isPcAtCompatible);
  
  printf("\n Local APICs:");
  for(const auto& i : _localApics)
    printf("\n\tProcessorId: %d, Id: %d", i.ProcessorId(), i.Id());
  
  printf("\n IO APICs:");
  for(const auto& i : _ioApics)
    printf("\n\tId: %d, Address: %x, InterruptBase: %x", i.Id(), i.Address(), i.InterruptBase());

  printf("\n Interrupt Source Overrides:");
  for(const auto& i : _intSrcOverrides)
    printf("\n\tPIC IRQ: %d, APIC Mapped IRQ: %d, Flags: %x", i.PicIrq(), i.ApicMappedIrq(), i.Flags());

  printf("\n NMIs:");
  for(const auto& i : _nmis)
    printf("\n\tFlags: %x, IntNo: %d", i.Flags(), i.IntNo());

  printf("\n Local APIC NMIs:");
  for(const auto& i : _localApicNmis)
    printf("\n\tProcessorId: %d, Flags: %x, LINT Pin: %d", i.ProcessorId(), i.Flags(), i.LintPin());
}
