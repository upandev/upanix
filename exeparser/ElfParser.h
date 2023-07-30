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
#ifndef _ELF_PARSER_H_
#define _ELF_PARSER_H_

#include <ElfHeader.h>
#include <ElfProgHeader.h>
#include <ElfSectionHeader.h>
#include <ElfSymbolTable.h>
#include <result.h>

class BufferedReader ;
using ELFHeader::ELF32Header ;
using ELFSectionHeader::ELF32SectionHeader ;
using ELFSymbolTable::ELFSymbolTableList ;
using ELFProgramHeader::ELF32ProgramHeader ;

class ELFParser
{
	private:
		unsigned m_uiSymTabCount ;

    BufferedReader* m_pBR ;

		ELF32Header* m_pHeader ;
		ELF32SectionHeader* m_pSectionHeader ;
		char* m_pSecHeaderStrTable ;

		ELF32ProgramHeader* m_pProgramHeader ;
		int* m_pSectionTableMap ;
		ELFSymbolTableList* m_pSymbolTable ;

	public:
		ELFParser(ELF32Header* pELFHeader, ELF32SectionHeader* pELFSectionHeader, char* pSecHeaderStrTable) ;
		ELFParser(const upan::string& szFileName) ;
		~ELFParser() ;

    void CopyProcessImage(byte* bProcessImage, unsigned uiProcessBase, unsigned uiMaxImageSize) const;
		char* CopyELFSecStrTable();
    ELF32SectionHeader* CopyELFSectionHeader();

    upan::result<uint32_t*> GetGOTAddress(byte* bProcessImage, unsigned uiMinMemAddr);
    upan::result<uint32_t> GetNoOfGOTEntries();

    void GetMemImageSize(unsigned* uiMinMemAddr, unsigned* uiMaxMemAddr) const;
		unsigned GetProgramStartAddress() ;

    upan::result<ELF32SectionHeader*> GetSectionHeaderByType(unsigned uiType);
    upan::result<ELF32SectionHeader*> GetSectionHeaderByTypeAndName(unsigned uiType, const char* szLikeName);
    upan::result<ELF32SectionHeader*> GetSectionHeaderByIndex(unsigned uiIndex);

		inline const ELF32Header* GetHeader() const { return m_pHeader ; }
		inline ELF32Header* GetHeader() { return m_pHeader ; }

		inline const ELF32SectionHeader* GetSectionHeader() const { return m_pSectionHeader ; }
		inline ELF32SectionHeader* GetSectionHeader() { return m_pSectionHeader ; }

		inline const char* GetSecHeaderStrTable() const { return m_pSecHeaderStrTable ; }
		inline char* GetSecHeaderStrTable() { return m_pSecHeaderStrTable ; }

	private:
		void AllocateSymbolTable() ;
		void DeAllocateSymbolTable() ;

    void ReadHeader() ;
    void ReadProgramHeaders() ;
    void ReadSectionHeaders() ;
    void ReadSecHeaderStrTable() ;
    void ReadSymbolTables() ;

    upan::result<uint32_t*> GetAddressBySectionName(byte* bProcessImage, unsigned uiMinMemAddr, const char* szSectionName);
    upan::result<uint32_t> GetNoOfGOTEntriesBySectionName(const char* szSectionName);

		bool CheckMagicSignature(const ELF32Header* pELFHeader) ;
} ;

#endif 
