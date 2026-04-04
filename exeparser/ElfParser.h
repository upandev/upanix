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

#include <ElfHeader.h>
#include <ElfProgHeader.h>
#include <ElfSectionHeader.h>
#include <ElfSymbolTable.h>
#include <result.h>
#include <uniq_ptr.h>

class BufferedReader;
using ElfHeader::Elf64_Ehdr;
using ElfSectionHeader::Elf64_Shdr;
using ElfSymbolTable::ElfSymTables;
using ElfProgramHeader::Elf64_Phdr;

class ElfParser {
	private:
		unsigned m_uiSymTabCount;

    mutable upan::uniq_ptr<BufferedReader> _bufferedReader;

		Elf64_Ehdr* _header;
		Elf64_Shdr* _sectionHeader;
		char* m_pSecHeaderStrTable;

		Elf64_Phdr* _programHeader;
		int* _sectionTableMap;
		ElfSymTables* m_pSymbolTable;
    int _tlsTotalSize;
    int _tlsInitImageSize;
    upan::uniq_ptr<uint8_t> _tlsInitImage;

	public:
		ElfParser(Elf64_Ehdr* pELFHeader, Elf64_Shdr* pELFSectionHeader, char* pSecHeaderStrTable);
		ElfParser(const upan::string& szFileName);
		~ElfParser();

    void CopyProcessImage(byte* processImage, uint64_t processBase, uint64_t maxImageSize) const;
		upan::pair<char*, size_t> CopyELFSecStrTable();
  upan::pair<Elf64_Shdr*, size_t> CopyELFSectionHeader();

    upan::result<uint64_t*> GetGOTAddress(byte* bProcessImage, unsigned uiMinMemAddr);
    upan::result<uint32_t> GetNoOfGOTEntries();

    void GetMemImageSize(uint64_t& minMemAddr, uint64_t& maxMemAddr) const;
		uint64_t GetProgramStartAddress();

    upan::result<Elf64_Shdr*> GetSectionHeaderByType(unsigned uiType);
    upan::result<Elf64_Shdr*> GetSectionHeaderByTypeAndName(unsigned uiType, const char* szLikeName);
    upan::result<Elf64_Shdr*> GetSectionHeaderByIndex(unsigned uiIndex);

		inline const Elf64_Ehdr* GetHeader() const { return _header; }
		inline Elf64_Ehdr* GetHeader() { return _header; }

		inline const Elf64_Shdr* GetSectionHeader() const { return _sectionHeader; }
		inline Elf64_Shdr* GetSectionHeader() { return _sectionHeader; }

		inline const char* GetSecHeaderStrTable() const { return m_pSecHeaderStrTable; }
		inline char* GetSecHeaderStrTable() { return m_pSecHeaderStrTable; }

    int GetTLSTotalSize() const { return _tlsTotalSize; }
    int GetTLSInitImageSize() const { return _tlsInitImageSize; }
    const uint8_t* GetTLSInitImage() const { return _tlsInitImage.get(); }

	private:
		void AllocateSymbolTable();
		void DeAllocateSymbolTable();

    void ReadHeader();
    void ReadProgramHeaders();
    void ReadSectionHeaders();
    void ReadSecHeaderStrTable();
    void ReadSymbolTables();
    void ReadTLS();

    upan::result<Elf64_Off*> GetAddressBySectionName(byte* bProcessImage, unsigned uiMinMemAddr, const char* szSectionName);
    upan::result<uint32_t> GetNoOfGOTEntriesBySectionName(const char* szSectionName);

		bool CheckMagicSignature(const Elf64_Ehdr* pELFHeader);
};

