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

class ProcessDLLInfo {
public:
  class ELFInfo {
  public:
    ELFInfo() : _elfSectionHeaders(nullptr), _elfSecStrTable(nullptr) {}
    ~ELFInfo() {
      delete []_elfSectionHeaders;
      delete _elfSectionHeaders;
    }
    ElfSectionHeader::Elf64_Shdr* _elfSectionHeaders;
    char* _elfSecStrTable;
  };

  ProcessDLLInfo(int id, uint64_t loadAddress, uint32_t noOfPages) : _id(id), _loadAddress(loadAddress), _noOfPages(noOfPages) {
  }

  int id() const { return _id; }
  uint64_t loadAddress() const {
    return _loadAddress;
  }
  uint64_t loadAddressForProcess() const {
    return _loadAddress /*- PROCESS_BASE*/;
  }
  ELFInfo& elfInfo() {
    return _elfInfo;
  }
  uint32_t noOfPages() const { return _noOfPages; }

private:
  int _id;
  uint64_t _loadAddress;
  uint32_t _noOfPages;
  ELFInfo _elfInfo;
};