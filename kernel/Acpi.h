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

#ifndef _ACPI_H_
#define _ACPI_H_

#include <stdlib.h>
#include <list.h>

class Acpi
{
  private:
    Acpi();
    Acpi(const Acpi&);

    class Rsdp;
    class MadtHeader;
    void ReadRootTable(const Acpi::Rsdp& rsdp, upan::list<uint32_t>& headerAddresses);
    void ReadHeader(uint32_t headerAddress);
    void Parse(const MadtHeader&);
  public:
    static Acpi& Instance()
    {
      static Acpi acpi;
      return acpi;
    }

    class Madt
    {
      private:
        Madt() : _localApicAddr(0), _isPcAtCompatible(false) {}
      public:
        class LocalApic
        {
          private:
            LocalApic() : _processorId(0), _id(0) {}
            LocalApic(uint32_t processorId, uint32_t id) : _processorId(processorId), _id(id) {}
          public:
            uint32_t ProcessorId() const { return _processorId; }
            uint32_t Id() const { return _id; }
          private:
            uint32_t _processorId;
            uint32_t _id;
            friend class Madt;
        };
        typedef upan::list<LocalApic> LocalApics;

        class IoApic
        {
          private:
            IoApic() : _id(0), _addr(0), _interruptBase(0) {}
            IoApic(uint32_t id, uint32_t addr, uint32_t interruptBase) : _id(id), _addr(addr), _interruptBase(interruptBase) {}
          public:
            uint32_t Id() const { return _id; }
            uint32_t Address() const { return _addr; }
            uint32_t InterruptBase() const { return _interruptBase; }
          private:
            uint32_t _id;
            uint32_t _addr;
            uint32_t _interruptBase; // Global int num at wich IO apic's interrupts will begin being mapped
            friend class Madt;
        };
        typedef upan::list<IoApic> IoApics;

        class IntSourceOverride
        {
          private:
            IntSourceOverride() : _picIrq(0), _apicIrq(0), _flags(0) {}
            IntSourceOverride(uint32_t picIrq, uint32_t apicIrq, uint32_t flags) : _picIrq(picIrq), _apicIrq(apicIrq), _flags(flags) {}
          public:
            uint8_t PicIrq() const { return _picIrq; }
            uint8_t ApicMappedIrq() const { return _apicIrq; }
            uint16_t Flags() const { return _flags; }
          private:
            uint32_t _picIrq; // Bus relative interrupt (e.g. no + 32)
            uint32_t _apicIrq; // Int no when mapped on IO-APIC
            uint16_t _flags; // See madt.h
            friend class Madt;
        };
        typedef upan::list<IntSourceOverride> IntSourceOverrides;

        class Nmi
        {
          private:
            Nmi() : _flags(0), _intNo(0) {}
            Nmi(uint32_t flags, uint32_t intNo) : _flags(flags), _intNo(intNo) {}
          public:
            uint32_t Flags() const { return _flags; }
            uint32_t IntNo() const { return _intNo; }
          private:
            uint32_t _flags;
            uint32_t _intNo;
            friend class Madt;
        };
        typedef upan::list<Nmi> Nmis;

        class LocalApicNmi
        {
          private:
            LocalApicNmi() : _processorId(0), _flags(0), _lintPin(0) {}
            LocalApicNmi(uint32_t processorId, uint32_t flags, uint32_t lintPin) : _processorId(processorId), _flags(flags), _lintPin(lintPin) {}
          public:
            uint32_t ProcessorId() const { return _processorId; }
            uint32_t Flags() const { return _flags; }
            uint32_t LintPin() const { return _lintPin; }
          private:
            uint32_t _processorId;
            uint32_t _flags;
            uint32_t _lintPin;
            friend class Madt;
        };
        typedef upan::list<LocalApicNmi> LocalApicNmis;

        uint32_t LocalApicAddress() const { return _localApicAddr; }
        bool IsPcAtCompatible() const { return _isPcAtCompatible; }
        const LocalApics& GetLocalApics() const { return _localApics; }
        const IoApics& GetIoApics() const { return _ioApics; }
        const IntSourceOverrides GetIntSrcOverrides() const { return _intSrcOverrides; }
        const Nmis GetNmis() const { return _nmis; }
        const LocalApicNmis& GetLocalApicNmis() const { return _localApicNmis; }

      private:
        void DebugPrint() const;
        void LocalApicAddr(uint64_t addr) { _localApicAddr = addr; }
        void IsPcAtCompatible(bool v) { _isPcAtCompatible = v; }
        void AddLocalApic(uint32_t processorId, uint32_t id)
        {
          _localApics.push_back(LocalApic(processorId, id));
        }
        void AddIoApic(uint32_t id, uint32_t addr, uint32_t interruptBase)
        {
          _ioApics.push_back(IoApic(id, addr, interruptBase));
        }
        void AddIntSrcOverride(uint32_t picIrq, uint32_t apicIrq, uint32_t flags)
        {
          _intSrcOverrides.push_back(IntSourceOverride(picIrq, apicIrq, flags));
        }
        void AddNmi(uint32_t flags, uint32_t intNo)
        {
          _nmis.push_back(Nmi(flags, intNo));
        }
        void AddLocalApicNmi(uint32_t processorId, uint32_t flags, uint32_t lintPin)
        {
          _localApicNmis.push_back(LocalApicNmi(processorId, flags, lintPin));
        }
      private:
        uint64_t          _localApicAddr;
        bool               _isPcAtCompatible;
        LocalApics         _localApics;
        IoApics            _ioApics;
        IntSourceOverrides _intSrcOverrides;
        Nmis               _nmis;
        LocalApicNmis      _localApicNmis;
        friend class Acpi;
    };
    bool IsAvailable() const { return _isAvailable; }
    const Madt& GetMadt() const { return _madt; }

  private:
    static const int OEMID_LEN = 6;

    class Rsdp
    {
      private:
        Rsdp();
        Rsdp(const Rsdp&);
      public:
        static const Rsdp* Search();
        bool MatchSignature() const;
        uint8_t Revision() const { return _rev; }
        uint32_t RootSystemTableAddr() const { return _rootSystemTableAddr; }
        uint32_t RootSystemTableLen() const { return _rootSystemTableLen; }
      private:
        static const int RSDP_SIG_LEN = 8;

        char      _signature[RSDP_SIG_LEN]; //"RSD PTR "
        uint8_t   _checksum; // Checksum of the bytes 0-19 (defined in ACPI rev 1); bytes must add to 0
        char      _oemID[OEMID_LEN];
        uint8_t   _rev;
        uint32_t  _rootSystemTableAddr;
        // All the fields above are used to calc the checksum
        uint32_t  _rootSystemTableLen;
        uint64_t  _extendedRootSystemTableAddr; // 64 bit phys addr
        uint8_t   _checksumEx; // Checksum of the whole structure including the other checksum field
        uint8_t   _reserved[3];
    } PACKED;

    class Header
    {
      private:
        Header();
        Header(const Header&);
      public:
        bool IsRootTable() const;
        bool IsApicHeader() const;
        uint32_t Length() const { return _len; }
      private:
        static const int TABLE_SIG_LEN = 4;
        static const int OEMTBLID_LEN = 8;

        char      _signature[TABLE_SIG_LEN]; // Table-specific ASCII-Signature (e.g. 'APIC')
        uint32_t  _len; // Size of the whole table including the header
        uint8_t   _rev;
        uint8_t   _checksum; // Whole table must add up to 0 to be valid
        char      _oemID[OEMID_LEN];
        char      _oemTableID[OEMTBLID_LEN];
        uint32_t  _oemRev;
        uint32_t  _creatorId;
        uint32_t  _creatorRev;
    } PACKED;

    class MadtHeader : public Header
    {
      public:
        uint32_t LocalApicAddr() const { return _localApicAddr; }
        bool IsPcAtCompatible() const { return _flags & 0x1 ? true : false; }
      private:
        uint32_t _localApicAddr;
        uint32_t _flags;
    } PACKED;

    class MadtEntry
    {
      private:
        MadtEntry();
        MadtEntry(const MadtEntry&);
      public:
        uint8_t Type() const { return _type; }
        uint8_t Length() const { return _length; }
      private:
        uint8_t _type;
        uint8_t _length;
    } PACKED;

    class MadtLocalApicEntry : public MadtEntry
    {
      public:
        uint8_t ProcessorId() const { return _processorId; }
        uint8_t Id() const { return _id; }
        bool IsEnabled() const { return _flags & 0x1 ? true : false; }
      private:
        uint8_t  _processorId;
        uint8_t  _id;
        uint32_t _flags;
    } PACKED;

    class MadtIoApicEntry : public MadtEntry
    {
      public:
        uint8_t Id() const { return _id; }
        uint32_t Addr() const { return _addr; }
        uint32_t InterruptBase() const { return _interruptBase; }
      private: 
        uint8_t  _id;
        uint8_t  _reserved;
        uint32_t _addr;
        uint32_t _interruptBase; // Global int num at wich IO apic's interrupts will begin being mapped
    } PACKED;

    class MadtIntOverrideEntry : public MadtEntry
    {
      public:
        uint8_t PicIrq() const { return _picIrq; }
        uint8_t ApicMappedIrq() const { return _apicIrq; }
        uint16_t Flags() const { return _flags; }
      private:
        uint8_t  _bus; // Will always be 0 (ISA)
        uint8_t  _picIrq; // Bus relative interrupt (e.g. no + 32)
        uint32_t _apicIrq; // Int no when mapped on IO-APIC
        uint16_t _flags; // See madt.h
    } PACKED;

    class MadtNmiEntry : public MadtEntry
    {
      public:
        uint16_t Flags() const { return _flags; }
        uint32_t IntNo() const { return _intNo; }
      private:
        uint16_t _flags;
        uint32_t _intNo;
    } PACKED;

    class MadtLocalApicNmiEntry : public MadtEntry
    {
      public:
        uint8_t ProcessorId() const { return _processorId; }
        uint16_t Flags() const { return _flags; }
        uint8_t LintPin() const { return _lintPin; }
      private:
        uint8_t  _processorId;
        uint16_t _flags;
        uint8_t  _lintPin;
    } PACKED;

    class MadtLocalApicAddrOverrideEntry : public MadtEntry
    {
      public:
        uint64_t Addr() const { return _addr; }
      private:
        uint16_t _reserved;
        uint64_t _addr;
    } PACKED;

  private:
    bool _isAvailable;
    Madt _madt;
};

#endif
