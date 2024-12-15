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

class ELFInfo {
public:
  ELFInfo() : _elfSectionHeaders(nullptr), _elfSecStrTable(nullptr) {}

  ~ELFInfo() {
    delete []_elfSectionHeaders;
    delete []_elfSecStrTable;
  }

  void set(ElfSectionHeader::Elf64_Shdr* elfSectionHeaders, char* elfSecStrTable) {
    _elfSectionHeaders = elfSectionHeaders;
    _elfSecStrTable = elfSecStrTable;
  }

  ElfSectionHeader::Elf64_Shdr* elfSectionHeaders() const { return _elfSectionHeaders; }
  char* elfSecStrTable() const { return _elfSecStrTable; }

private:
  ElfSectionHeader::Elf64_Shdr* _elfSectionHeaders;
  char* _elfSecStrTable;
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

class DLLInfo {
public:
  DLLInfo(int id, uint64_t virtualLoadAddress, uint32_t noOfPages) : _id(id), _virtualLoadAddress(virtualLoadAddress), _noOfPages(noOfPages) {
  }

  int id() const { return _id; }
  uint64_t virtualLoadAddress() const { return _virtualLoadAddress; }
  uint32_t noOfPages() const { return _noOfPages; }
  const ELFInfo& elfInfo() const { return _elfInfo; }
  const TLSInfo& tlsInfo() const { return _tlsInfo; }

  void setELFInfo(ElfSectionHeader::Elf64_Shdr* elfSectionHeaders, char* elfSecStrTable) {
    _elfInfo.set(elfSectionHeaders, elfSecStrTable);
  }

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

class RelocateInfo {
public:
  RelocateInfo(const DLLInfo& dllInfo, uint64_t value) : _dllInfo(dllInfo), _value(value) {}
  const DLLInfo& dllInfo() const { return _dllInfo; }
  uint64_t value() const { return _value; }
private:
  const DLLInfo& _dllInfo;
  uint64_t _value;
};