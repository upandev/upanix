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
#include <ElfParser.h>
#include <ElfConstants.h>
#include <ElfHeader.h>
#include <ElfProgHeader.h>
#include <ElfSectionHeader.h>
#include <ElfSymbolTable.h>
#include <StringUtil.h>
#include <BufferedReader.h>
#include <uniq_ptr.h>

ElfParser::ElfParser(Elf64_Ehdr* pELFHeader, Elf64_Shdr* pELFSectionHeader, char* pSecHeaderStrTable) :
        m_uiSymTabCount(0),
        _bufferedReader(nullptr),
        _header(pELFHeader),
        _sectionHeader(pELFSectionHeader),
        m_pSecHeaderStrTable(pSecHeaderStrTable),
        _programHeader(nullptr),
        _sectionTableMap(nullptr),
        m_pSymbolTable(nullptr),
        _tlsTotalSize(0),
        _tlsInitImageSize(0),
        _tlsInitImage(nullptr) {
}

ElfParser::ElfParser(const upan::string& szFileName) :
        m_uiSymTabCount(0),
        _bufferedReader(new BufferedReader(szFileName, 0, .5 * 1024 * 1024)),
        _header(nullptr),
        _sectionHeader(nullptr),
        m_pSecHeaderStrTable(nullptr),
        _programHeader(nullptr),
        _sectionTableMap(nullptr),
        m_pSymbolTable(nullptr),
        _tlsTotalSize(0),
        _tlsInitImageSize(0),
        _tlsInitImage(nullptr) {
  ReadHeader();
  ReadProgramHeaders();
  ReadSectionHeaders();
  ReadSecHeaderStrTable();
  ReadSymbolTables();
  ReadTLS();
}

ElfParser::~ElfParser() {
  if(_bufferedReader.get()) {
    DeAllocateSymbolTable();
    delete[] m_pSecHeaderStrTable;
    delete[] _sectionHeader;
    delete[] _sectionTableMap;
    delete[] _programHeader;
    delete _header;
  }
}

void ElfParser::AllocateSymbolTable() {
	m_uiSymTabCount = 0;

	for(int i = 0; i < _header->e_shnum; i++) {
		if(_sectionHeader[i].sh_type == ElfSectionHeader::SHT_SYMTAB)
			m_uiSymTabCount++;
	}

	m_pSymbolTable = new ElfSymTables[ m_uiSymTabCount ];

	unsigned uiSymTabIndex = 0;
	for(int i = 0; i < _header->e_shnum; i++) {
		if(_sectionHeader[i].sh_type == ElfSectionHeader::SHT_SYMTAB) {
			m_pSymbolTable[uiSymTabIndex].table_size = (_sectionHeader[i].sh_size / _sectionHeader[i].sh_entsize);
			m_pSymbolTable[uiSymTabIndex].symTabEntries = new ElfSymbolTable::Elf64_Sym[ m_pSymbolTable[uiSymTabIndex].table_size ];
			uiSymTabIndex++;
		}
	}
}

void ElfParser::DeAllocateSymbolTable() {
	for(unsigned i = 0; i < m_uiSymTabCount; i++)
		delete[] (m_pSymbolTable[i].symTabEntries);
	delete[] m_pSymbolTable;
}

void ElfParser::ReadHeader() {
  _header = new Elf64_Ehdr;
  _bufferedReader->Seek(0);
  const unsigned n = _bufferedReader->Read((uint8_t*)_header, sizeof(Elf64_Ehdr));

	if(n < sizeof(Elf64_Ehdr))
    throw upan::exception(XLOC, "elf file header size %u is less than Elf64_Ehdr size %u", n, sizeof(Elf64_Ehdr));

	if(!CheckMagicSignature(_header)) {
	  //printf("\n %x %c %c %c", _header->e_ident[0], _header->e_ident[1], _header->e_ident[2], _header->e_ident[3]);
    throw upan::exception(XLOC, "Invalid ELF 64 magic signature");
  }
}

void ElfParser::ReadProgramHeaders() {
	/* e_phentsize is not used as this Parser is only for 32 bit elf files.
	   and ElfProgHeader size is 32 bytes */

  _programHeader = new Elf64_Phdr[ _header->e_phnum ];

  _bufferedReader->Seek(_header->e_phoff);

  const unsigned n = _bufferedReader->Read((uint8_t *)_programHeader, sizeof(Elf64_Phdr) * _header->e_phnum);

	if(n < sizeof(Elf64_Phdr) * _header->e_phnum)
    throw upan::exception(XLOC, "Invalid program header size: %u - expected: %u", n, sizeof(Elf64_Phdr) * _header->e_phnum);
}

void ElfParser::ReadSectionHeaders() {
	/* e_shentsize if not used as this Parser is only for 32 bit elf files.
	   and ElfSectionHeader size is 40 bytes */
	   
  _sectionHeader = new Elf64_Shdr[ _header->e_shnum ];
  _sectionTableMap = new int[ _header->e_shnum ];

  for(int i = 0; i < _header->e_shnum; i++)
    _sectionTableMap[i] = -1;

  _bufferedReader->Seek(_header->e_shoff);

  const auto n = _bufferedReader->Read((uint8_t*)_sectionHeader, sizeof(Elf64_Shdr) * _header->e_shnum);

  if (n < sizeof(Elf64_Shdr) * _header->e_shnum) {
    throw upan::exception(XLOC, "Invalid elf section header size %u - expected: %u", n, sizeof(Elf64_Shdr) * _header->e_shnum);
  }
}

void ElfParser::ReadSecHeaderStrTable() {
  const auto uiSecSize = _sectionHeader[_header->e_shstrndx].sh_size;
  const auto uiSecOffset = _sectionHeader[_header->e_shstrndx].sh_offset;

  m_pSecHeaderStrTable = new char[ uiSecSize ];

  _bufferedReader->Seek(uiSecOffset);

  auto n = _bufferedReader->Read((uint8_t*)m_pSecHeaderStrTable, uiSecSize);

	if(n < uiSecSize) {
    throw upan::exception(XLOC, "Invalid elf section header string table size: %u - expected: %u", n, uiSecSize);
  }
}

void ElfParser::ReadSymbolTables() {
	AllocateSymbolTable();

	unsigned uiSymTabIndex = 0;
  for(uint32_t i = 0; i < _header->e_shnum; i++) {
		if(_sectionHeader[i].sh_type == ElfSectionHeader::SHT_SYMTAB) {
      _bufferedReader->Seek(_sectionHeader[i].sh_offset);
      const auto n = _bufferedReader->Read((uint8_t*)(m_pSymbolTable[uiSymTabIndex].symTabEntries), sizeof(ElfSymbolTable::Elf64_Sym) * m_pSymbolTable[uiSymTabIndex].table_size);

			if(n < sizeof(ElfSymbolTable::Elf64_Sym) * m_pSymbolTable[uiSymTabIndex].table_size)
        throw upan::exception(XLOC, "Invalid elf symbol table size: %u - expected: %u", n, sizeof(ElfSymbolTable::Elf64_Sym) * m_pSymbolTable[uiSymTabIndex].table_size);

      _sectionTableMap[i] = uiSymTabIndex;
      ++uiSymTabIndex;
		}
	}
}

void ElfParser::ReadTLS() {
  for(auto i = 0; i < _header->e_phnum; i++) {
    if(_programHeader[i].p_type == ElfProgramHeader::PT_TLS) {
      _tlsTotalSize = upan::align_up(_programHeader[i].p_memsz, _programHeader[i].p_align);
      _tlsInitImageSize = _programHeader[i].p_filesz;
      _tlsInitImage.reset(new uint8_t[_tlsInitImageSize]);
      _bufferedReader->Seek(_programHeader[i].p_offset);
      _bufferedReader->Read(_tlsInitImage.get(), _tlsInitImageSize);
      break;
    }
  }
}

upan::result<uint64_t*> ElfParser::GetAddressBySectionName(byte* bProcessImage, unsigned uiMinMemAddr, const char* szSectionName) {
	for(int i = 0; i < _header->e_shnum; i++) {
    if (strcmp(ElfSectionHeader::GetSectionName(m_pSecHeaderStrTable, _sectionHeader[i].sh_name), szSectionName) == 0) {
      return upan::good((Elf64_Off *) (bProcessImage + _sectionHeader[i].sh_addr - uiMinMemAddr));
    }
  }
  return upan::result<Elf64_Off*>::bad("%s section not found in process image", szSectionName);
}

upan::result<uint32_t> ElfParser::GetNoOfGOTEntriesBySectionName(const char* szSectionName) {
	for(int i = 0; i < _header->e_shnum; i++) {
    if (strcmp(ElfSectionHeader::GetSectionName(m_pSecHeaderStrTable, _sectionHeader[i].sh_name), szSectionName) == 0) {
      return upan::good((uint32_t) (_sectionHeader[i].sh_size / _sectionHeader[i].sh_entsize));
    }
  }
  return upan::result<uint32_t>::bad("no GOT entries found for %s section", szSectionName);
}

void ElfParser::GetMemImageSize(uint64_t& minMemAddr, uint64_t& maxMemAddr) const {
	minMemAddr = 0;
	maxMemAddr = 0;

	bool bFirstTime = true;

	for(auto i = 0; i < _header->e_phnum; i++) {
		if(_programHeader[i].p_type == ElfProgramHeader::PT_LOAD) {
			if(bFirstTime == true) {
				bFirstTime = false;
				minMemAddr = _programHeader[i].p_vaddr;
			}
			maxMemAddr = _programHeader[i].p_vaddr + _programHeader[i].p_memsz;
		}
	}
}

uint64_t ElfParser::GetProgramStartAddress() {
	return _header->e_entry;
}

upan::result<uint64_t*> ElfParser::GetGOTAddress(byte* bProcessImage, unsigned uiMinMemAddr) {
  auto res = GetAddressBySectionName(bProcessImage, uiMinMemAddr, ".got.plt");
  if(res.isBad())
    res = GetAddressBySectionName(bProcessImage, uiMinMemAddr, ".got");
  return res;
}

upan::result<uint32_t> ElfParser::GetNoOfGOTEntries() {
  auto res = GetNoOfGOTEntriesBySectionName(".got.plt");
  if(res.isBad())
    res = GetNoOfGOTEntriesBySectionName(".got");
  return res;
}

upan::result<Elf64_Shdr*> ElfParser::GetSectionHeaderByType(unsigned uiType) {
	for(int i = 0; i < _header->e_shnum; i++) {
    if (_sectionHeader[i].sh_type == uiType) {
      return upan::good(&_sectionHeader[i]);
    }
  }
  return upan::result<Elf64_Shdr*>::bad("Failed to find elf section header for type: %u", uiType);
}

upan::result<Elf64_Shdr*> ElfParser::GetSectionHeaderByTypeAndName(unsigned uiType, const char* szLikeName) {
	for(int i = 0; i < _header->e_shnum; i++) {
    if (strstr((m_pSecHeaderStrTable + _sectionHeader[i].sh_name), szLikeName) && _sectionHeader[i].sh_type == uiType) {
      return upan::good(&_sectionHeader[i]);
    }
  }
  return upan::result<Elf64_Shdr*>::bad("Failed to find elf section header for type: %u, name: %s", uiType, szLikeName);
}

upan::result<Elf64_Shdr*> ElfParser::GetSectionHeaderByIndex(unsigned uiIndex) {
	if(uiIndex >= _header->e_shnum)
    return upan::result<Elf64_Shdr*>::bad("Failed to find elf section header for index: %u", uiIndex);
  return upan::good(&_sectionHeader[uiIndex]);
}

void ElfParser::CopyProcessImage(byte* processImage, uint64_t processBase, uint64_t maxImageSize) const {
	for(unsigned i = 0; i < _header->e_phnum; i++) {
		if(_programHeader[i].p_type == ElfProgramHeader::PT_LOAD) {
      _bufferedReader->Seek(_programHeader[i].p_offset);
      const auto offset = _programHeader[i].p_vaddr - processBase;

			if(offset >= maxImageSize)
        throw upan::exception(XLOC, "process load virtual address %x is larger than max image size %x", offset, maxImageSize);

      const auto n = _bufferedReader->Read((uint8_t*)processImage + offset, _programHeader[i].p_filesz);

			if(n < _programHeader[i].p_filesz)
        throw upan::exception(XLOC, "Invalid elf file size: %u - expected: %u", n, _programHeader[i].p_filesz);
		}
	}
}

upan::pair<char*, size_t> ElfParser::CopyELFSecStrTable() {
	auto size = _sectionHeader[ _header->e_shstrndx ].sh_size;
	auto secStrTable = new char[size];
	memcpy(secStrTable, m_pSecHeaderStrTable, size);
	return { secStrTable, size };
}

upan::pair<Elf64_Shdr*, size_t> ElfParser::CopyELFSectionHeader() {
  auto sectionHeaders = new Elf64_Shdr[_header->e_shnum];
  memcpy(sectionHeaders, _sectionHeader, sizeof(Elf64_Shdr) * _header->e_shnum);
	return { sectionHeaders, _header->e_shnum };
}

bool ElfParser::CheckMagicSignature(const Elf64_Ehdr* pELFHeader) {
	return (pELFHeader->e_ident[ElfHeader::EI_MAG0] == 0x7F
		&& pELFHeader->e_ident[ElfHeader::EI_MAG1] == 'E'
		&& pELFHeader->e_ident[ElfHeader::EI_MAG2] == 'L'
		&& pELFHeader->e_ident[ElfHeader::EI_MAG3] == 'F'
		&& (pELFHeader->e_type == ElfHeader::ET_EXEC || pELFHeader->e_type == ElfHeader::ET_DYN));
}

namespace ElfProgramHeader {
	const char* GetProgHeaderType(unsigned uiPType) {
		if(0 <= uiPType && uiPType < MAX_PROG_TYPES)
			return ProgramHeaderType[uiPType];
		return (uiPType <= PT_LOPROC || uiPType >= PT_HIPROC) ? "Processor-Specific" : "Invalid Program Header Type";
	}
};

namespace ElfSectionHeader {
	const char* GetSecHeaderType(unsigned uiSType) {
		if(0 <= uiSType && uiSType < MAX_SEC_HEADER_TYPES)
			return SectionHeaderType[ uiSType ];

		return (uiSType > SHT_LOPROC && uiSType <= SHT_HIPROC) ? "Processor-Specific" : "Invalid Section Header Type";
	}

	const char* GetSectionName(const char* szStrTable, int iIndex) {
		return szStrTable + iIndex;
	}
};
