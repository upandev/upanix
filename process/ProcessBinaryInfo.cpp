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
#include <result.h>

#define REL_DYN_SUB_NAME  ".rela.dyn"

void ELFInfo::init(uint64_t base, int elfSectionHeaderSize, ElfSectionHeader::Elf64_Shdr* elfSectionHeaders, char* elfSecStrTable) {
  _base = base;
  _elfSectionHeaderSize = elfSectionHeaderSize;
  _elfSectionHeaders = elfSectionHeaders;
  _elfSecStrTable = elfSecStrTable;


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
}

void ELFInfo::adjustBase(uint64_t base) {
  uint64_t adjust = base - _base;

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
}

const char* ELFInfo::getDynSymName(Elf64_Xword index) {
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