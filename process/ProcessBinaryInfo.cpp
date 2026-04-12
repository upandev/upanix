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

#include <ProcessBinaryInfo.h>
#include <ElfSectionHeader.h>

#define REL_DYN_SUB_NAME  ".rela.dyn"
#define REL_PLT_SUB_NAME	".rela.plt"

void ELFInfo::init(uint64_t base,
                   upan::pair<ElfSectionHeader::Elf64_Shdr*, size_t> elfSectionHeaders,
                   upan::pair<char*, size_t> elfSecStrTable) {
  _base = base;

  _elfSectionHeaders = elfSectionHeaders.first;
  _elfSectionHeaderSize = elfSectionHeaders.second;

  _elfSecStrTable = elfSecStrTable.first;
  _elfSecStrTableSize = elfSecStrTable.second;

  getSectionByType(SHT_DYNAMIC).onGood([&](Section& section) {
    _dynSection = upan::option<Elf64_Dyn*>(section.get<Elf64_Dyn>());
    _dynSectionSize = section.size();

    getSectionByIndex(section.sh_link()).onGood([&](Section& section1) {
      _dynSymStrTable = section1.get<const char>();
    });
  });

  getSectionByType(SHT_DYNSYM).onGood([&](Section& section) {
    _dynSymTable = upan::option<Elf64_Sym*>(section.get<Elf64_Sym>());
    _dynSymTableSize = section.size();
  });

  getSectionByTypeAndName(SHT_RELA, REL_DYN_SUB_NAME).onGood([&](Section& section) {
    _dynRelTable = upan::option<Elf64_Rela*>(section.get<Elf64_Rela>());
    _dynRelTableSize = section.size();
  });

  getSectionByTypeAndName(SHT_RELA, REL_PLT_SUB_NAME).onGood([&](Section& section) {
    _dynRelPltTable = upan::option<Elf64_Rela*>(section.get<Elf64_Rela>());
    _dynRelPltTableSize = section.size();
  });

  getSectionByType(SHT_HASH).onGood([&](Section& section) {
    _hashTable = upan::option<Elf64_Word*>(section.get<Elf64_Word>());
  });
}

void ELFInfo::init(const ELFInfo& elfInfo) {
  _base = elfInfo._base;

  _elfSectionHeaders = nullptr;
  if (_elfSectionHeaderSize) {
    _elfSectionHeaderSize = elfInfo._elfSectionHeaderSize;
    _elfSectionHeaders = new ElfSectionHeader::Elf64_Shdr[_elfSectionHeaderSize];
    memcpy(_elfSectionHeaders, elfInfo._elfSectionHeaders, sizeof(ElfSectionHeader::Elf64_Shdr) * _elfSectionHeaderSize);
  }

  _elfSecStrTable = nullptr;
  if (_elfSecStrTableSize) {
    _elfSecStrTableSize = elfInfo._elfSecStrTableSize;
    _elfSecStrTable = new char[_elfSecStrTableSize];
    memcpy(_elfSecStrTable, elfInfo._elfSecStrTable, _elfSecStrTableSize);
  }

  //the child process will have same address value for sections as the parent because their address space mapping is similar
  _dynSection = elfInfo._dynSection;
  _dynSectionSize = elfInfo._dynSectionSize;
  _dynSymStrTable = elfInfo._dynSymStrTable;

  _dynSymTable = elfInfo._dynSymTable;
  _dynSymTableSize = elfInfo._dynSymTableSize;

  _dynRelTable = elfInfo._dynRelTable;
  _dynRelTableSize = elfInfo._dynRelTableSize;

  _dynRelPltTable = elfInfo._dynRelPltTable;
  _dynRelPltTableSize = elfInfo._dynRelPltTableSize;

  _hashTable = elfInfo._hashTable;
}

void ELFInfo::clear() {
  delete [] _elfSectionHeaders;
  delete [] _elfSecStrTable;

  _base = 0;
  _elfSectionHeaders = nullptr;
  _elfSectionHeaderSize = 0;
  _elfSecStrTable = nullptr;
  _elfSecStrTableSize = 0;
  _dynSection = upan::option<Elf64_Dyn*>::empty();
  _dynSectionSize = 0;
  _dynSymTable = upan::option<Elf64_Sym*>::empty();
  _dynSymTableSize = 0;
  _dynRelTable = upan::option<Elf64_Rela*>::empty();
  _dynRelTableSize = 0;
  _dynRelPltTable = upan::option<Elf64_Rela*>::empty();
  _dynRelPltTableSize = 0;
  _hashTable = upan::option<Elf64_Word*>::empty();
  _dynSymStrTable = nullptr;
}

ELFInfo& ELFInfo::operator=(const ELFInfo& elfInfo) {
  if (this == &elfInfo) {
    return *this;
  }
  clear();
  init(elfInfo);
  return *this;
}

void ELFInfo::adjustBase(uint64_t base) {
  const uint64_t adjust = base - _base;
  _base = base;

  _dynSection.ifPresent([&](Elf64_Dyn* s) {
    _dynSection = upan::option<Elf64_Dyn*>(reinterpret_cast<Elf64_Dyn*>((uint64_t)s + adjust));
  });

  if (_dynSymStrTable) {
    _dynSymStrTable = reinterpret_cast<const char*>((uint64_t)_dynSymStrTable + adjust);
  }

  _dynSymTable.ifPresent([&](Elf64_Sym* s) {
    _dynSymTable = upan::option<Elf64_Sym*>(reinterpret_cast<Elf64_Sym*>((uint64_t)s + adjust));
  });

  _dynRelTable.ifPresent([&](Elf64_Rela* s) {
    _dynRelTable = upan::option<Elf64_Rela*>(reinterpret_cast<Elf64_Rela*>((uint64_t)s + adjust));
  });

  _dynRelPltTable.ifPresent([&](Elf64_Rela* s) {
    _dynRelPltTable = upan::option<Elf64_Rela*>(reinterpret_cast<Elf64_Rela*>((uint64_t)s + adjust));
  });

  _hashTable.ifPresent([&](Elf64_Word* s) {
    _hashTable = upan::option<Elf64_Word*>(reinterpret_cast<Elf64_Word*>((uint64_t)s + adjust));
  });
}

const char* ELFInfo::getDynSymName(Elf64_Xword index) const {
  return (const char*)&_dynSymStrTable[index];
}

upan::option<ELFInfo::Section> ELFInfo::getGOT() {
  auto res = getSectionByName(".got.plt");
  if (res.isBad()) {
    res = getSectionByName(".got");
  }

  if (res.isBad()) {
    return upan::option<Section>::empty();
  }
  return upan::option<Section>(res.goodValue());
}

upan::result<ELFInfo::Section> ELFInfo::getSectionByName(const upan::string& name) {
  for(int i = 0; i < _elfSectionHeaderSize; ++i) {
    if (strcmp(_elfSecStrTable + _elfSectionHeaders[i].sh_name, name.c_str()) == 0) {
      return upan::good(Section(
              _base + _elfSectionHeaders[i].sh_addr,
              _elfSectionHeaders[i].sh_link,
              _elfSectionHeaders[i].sh_size / _elfSectionHeaders[i].sh_entsize));
    }
  }
  return upan::result<Section>::bad("%s section not found in process image", name.c_str());
}

upan::result<ELFInfo::Section> ELFInfo::getSectionByType(int type) {
  for(int i = 0; i < _elfSectionHeaderSize; ++i) {
    if (_elfSectionHeaders[i].sh_type == type) {
      auto entSize = _elfSectionHeaders[i].sh_entsize == 0 ? 1 : _elfSectionHeaders[i].sh_entsize;
      return upan::good(Section(
              _base + _elfSectionHeaders[i].sh_addr,
              _elfSectionHeaders[i].sh_link,
              _elfSectionHeaders[i].sh_size / entSize));
    }
  }
  return upan::result<Section>::bad("failed to find elf section header for type: %d", type);
}

upan::result<ELFInfo::Section> ELFInfo::getSectionByTypeAndName(int type, const upan::string& name) {
  for(int i = 0; i < _elfSectionHeaderSize; ++i) {
    if (strcmp(_elfSecStrTable + _elfSectionHeaders[i].sh_name, name.c_str()) == 0 && _elfSectionHeaders[i].sh_type == type) {
      auto entSize = _elfSectionHeaders[i].sh_entsize == 0 ? 1 : _elfSectionHeaders[i].sh_entsize;
      return upan::good(Section(
              _base + _elfSectionHeaders[i].sh_addr,
              _elfSectionHeaders[i].sh_link,
              _elfSectionHeaders[i].sh_size / entSize));
    }
  }
  return upan::result<Section>::bad("failed to find elf section header for type: %d, name: %s", type, name.c_str());
}

upan::result<ELFInfo::Section> ELFInfo::getSectionByIndex(Elf64_Word index) {
  if (index >= _elfSectionHeaderSize)
    return upan::result<Section>::bad("failed to find elf section header for index: %d", index);
  auto entSize = _elfSectionHeaders[index].sh_entsize == 0 ? 1 : _elfSectionHeaders[index].sh_entsize;
  return upan::good(Section(
          _base + _elfSectionHeaders[index].sh_addr,
          _elfSectionHeaders[index].sh_link,
          _elfSectionHeaders[index].sh_size / entSize));
}

void ELFInfo::loadInitFini(process_init_fini_t& init_fine) {
  const auto dllDynSection = getDynSection().valueOrThrow(XLOC, "no dynamic section found");
  for(Elf64_Xword i = 0; i < getDynSectionSize(); ++i) {
    if(dllDynSection[i].d_tag == DT_INIT) {
      init_fine._init = (void(*)())(getBase() + dllDynSection[i].d_un.d_ptr);
    } else if(dllDynSection[i].d_tag == DT_FINI) {
      init_fine._fini = (void(*)())(getBase() + dllDynSection[i].d_un.d_ptr);
    }
  }
}
