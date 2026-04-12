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

#include <ProcessConstants.h>
#include <ElfSectionHeader.h>
#include <result.h>
#include <option.h>
#include <pair.h>
#include <ElfDynamicSection.h>
#include <ElfSymbolTable.h>
#include <ElfRelocationSection.h>

using namespace ElfDynamicSection;
using namespace ElfSectionHeader;
using namespace ElfSymbolTable;
using namespace ElfRelocSection;

class ELFInfo {
public:
  class Section {
  public:
    Section() : _ptr(0), _sh_link(0), _size(0) {}
    Section(uintptr_t ptr, Elf64_Word sh_link, Elf64_Xword size) : _ptr(ptr), _sh_link(sh_link), _size(size) {}

    template<typename T>
    T* get() const { return reinterpret_cast<T*>(_ptr); }
    Elf64_Word sh_link() const { return _sh_link; }
    Elf64_Xword size() const { return _size; }

  private:
    uintptr_t _ptr;
    Elf64_Word _sh_link;
    Elf64_Xword _size;
  };

  ELFInfo() : _base(0),
  _elfSectionHeaders(nullptr), _elfSectionHeaderSize(0),
  _elfSecStrTable(nullptr), _elfSecStrTableSize(0),
  _dynSection(upan::option<Elf64_Dyn*>::empty()), _dynSectionSize(0),
  _dynSymTable(upan::option<Elf64_Sym*>::empty()), _dynSymTableSize(0),
  _dynRelTable(upan::option<Elf64_Rela*>::empty()), _dynRelTableSize(0),
  _dynRelPltTable(upan::option<Elf64_Rela*>::empty()), _dynRelPltTableSize(0),
  _hashTable(upan::option<Elf64_Word*>::empty()),
  _dynSymStrTable(nullptr) {
  }

  ELFInfo(const ELFInfo& _elfInfo) : ELFInfo() {
    init(_elfInfo);
  }

  ELFInfo& operator=(const ELFInfo& _elfInfo);

  ~ELFInfo() {
    clear();
  }

  void init(uint64_t base, upan::pair<ElfSectionHeader::Elf64_Shdr*, size_t>, upan::pair<char*, size_t>);
  void init(const ELFInfo& elfInfo);

  void adjustBase(uint64_t base);
  template <typename LAMBDA>
  void extractDynSymbols(LAMBDA& consumer);
  void loadInitFini(process_init_fini_t& init_fine);

  uint64_t getBase() const { return _base; }
  ElfSectionHeader::Elf64_Shdr* elfSectionHeaders() const { return _elfSectionHeaders; }
  char* elfSecStrTable() const { return _elfSecStrTable; }

  upan::option<Elf64_Dyn*> getDynSection() const { return _dynSection; }
  Elf64_Xword getDynSectionSize() const { return _dynSectionSize; }

  upan::option<Elf64_Sym*> getDynSymTable() const { return _dynSymTable; }
  Elf64_Xword getDynSymTableSize() const { return _dynSymTableSize; }

  upan::option<Elf64_Rela*> getDynRelTable() const { return _dynRelTable; }
  Elf64_Xword getDynRelTableSize() const { return _dynRelTableSize; }

  upan::option<Elf64_Rela*> getDynRelPltTable() const { return _dynRelPltTable; }
  Elf64_Xword getDynRelPltTableSize() const { return _dynRelPltTableSize; }

  upan::option<Elf64_Word*> getHashTable() const { return _hashTable; }

  const char* getDynSymName(Elf64_Xword index) const;

  upan::option<Section> getGOT();
  upan::result<ELFInfo::Section> getSectionByName(const upan::string& name);
  upan::result<ELFInfo::Section> getSectionByType(int type);
  upan::result<ELFInfo::Section> getSectionByTypeAndName(int type, const upan::string& name);
  upan::result<ELFInfo::Section> getSectionByIndex(Elf64_Word index);

private:
  void clear();

  uint64_t _base;

  ElfSectionHeader::Elf64_Shdr* _elfSectionHeaders;
  int _elfSectionHeaderSize;

  char* _elfSecStrTable;
  int _elfSecStrTableSize;

  upan::option<Elf64_Dyn*> _dynSection;
  Elf64_Xword _dynSectionSize;

  upan::option<Elf64_Sym*> _dynSymTable;
  Elf64_Xword _dynSymTableSize;

  upan::option<Elf64_Rela*> _dynRelTable;
  Elf64_Xword _dynRelTableSize;

  upan::option<Elf64_Rela*> _dynRelPltTable;
  Elf64_Xword _dynRelPltTableSize;

  upan::option<Elf64_Word*> _hashTable;

  const char* _dynSymStrTable;
};

class TLSInfo {
public:
  TLSInfo() : _moduleId(0), _offset(0) {}

  TLSInfo(int moduleId, uint64_t offset) : TLSInfo() {
    set(moduleId, offset);
  }

  void set(int moduleId, uint64_t offset) {
    _moduleId = moduleId;
    _offset = offset;
  }

  int moduleId() const { return _moduleId; }
  uint64_t offset() const { return _offset; }

private:
  int _moduleId;
  uint64_t _offset;
};

template <typename LAMBDA>
void ELFInfo::extractDynSymbols(LAMBDA& consumer) {
  getDynSymTable().ifPresent([&](const Elf64_Sym* dynSymTable) {
    for(Elf64_Xword i = 0; i < getDynSymTableSize(); ++i) {
      const auto& dynSym = dynSymTable[i];
      const auto symType = ELF64_ST_TYPE(dynSym.st_info);
      //At first, only STT_TLS and STT_OBJECT symbol types were added. Now, everything is added
      //this includes symbol type STT_FUNC. This is required because function pointers can be used
      //in executable or other shared libraries, which will then appear in their relocation table
      //as GLOB_DAT entries, which needs to be relocated at program start-up in relocateDLLs()
      if (dynSym.st_shndx != STN_UNDEF) {
        const char* symName = getDynSymName(dynSym.st_name);
        consumer(symName, dynSym.st_value);
      }
    }
  });
}

class DLLInfo {
public:
  DLLInfo(int id, uint64_t virtualLoadAddress, uint32_t noOfPages) : _id(id), _virtualLoadAddress(virtualLoadAddress), _noOfPages(noOfPages) {
  }

  int id() const { return _id; }
  uint64_t virtualLoadAddress() const { return _virtualLoadAddress; }
  uint32_t noOfPages() const { return _noOfPages; }
  ELFInfo& elfInfo() { return _elfInfo; }
  const ELFInfo& elfInfo() const { return _elfInfo; }
  const TLSInfo& tlsInfo() const { return _tlsInfo; }

  void setTLSInfo(int module, uint64_t offset) {
    _tlsInfo.set(module, offset);
  }

private:
  int _id;
  uint64_t _virtualLoadAddress;
  uint32_t _noOfPages;
  ELFInfo _elfInfo;
  TLSInfo _tlsInfo;
};

class IRelocateInfo {
  public:
  virtual int id() const = 0;
  virtual uint64_t value() const = 0;
  virtual uint64_t base() const = 0;
  virtual int tlsModuleId() const = 0;
  virtual uint64_t tlsOffset() const = 0;
};

class DLLRelocateInfo : public IRelocateInfo {
public:
  DLLRelocateInfo(const DLLInfo& dllInfo, uint64_t value) : _dllInfo(dllInfo), _value(value) {}

  int id() const override { return _dllInfo.id(); }
  uint64_t base() const override { return _dllInfo.elfInfo().getBase(); }
  uint64_t value() const override { return _value; }
  int tlsModuleId() const override { return _dllInfo.tlsInfo().moduleId(); }
  uint64_t tlsOffset() const override { return _dllInfo.tlsInfo().offset(); }
  const DLLInfo& dllInfo() const { return _dllInfo; }

private:
  const DLLInfo& _dllInfo;
  const uint64_t _value;
};

class ExeRelocateInfo : public IRelocateInfo {
public:
  ExeRelocateInfo(uint64_t base, uint64_t value) : _base(base), _value(value) {}

  int id() const override { return -999; }
  uint64_t base() const override { return _base; }
  uint64_t value() const override { return _value; }
  int tlsModuleId() const override { throw upan::exception(XLOC, "moduleId: TLS relocation not supported for executable"); }
  uint64_t tlsOffset() const override { throw upan::exception(XLOC, "offset: TLS relocation not supported for executable"); }

private:
  const uint64_t _base;
  const uint64_t _value;
};
