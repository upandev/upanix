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
#include <MemUtil.h>
#include <DMM.h>
#include <StringUtil.h>
#include <DynamicLinkLoader.h>
#include <IODescriptorTable.h>
#include <FileOperations.h>
#include <BufferedReader.h>
#include <uniq_ptr.h>

ELFParser::ELFParser(ELF32Header* pELFHeader, ELF32SectionHeader* pELFSectionHeader, char* pSecHeaderStrTable) :
	m_uiSymTabCount(0),
	m_pBR(NULL),
	m_pHeader(pELFHeader),
	m_pSectionHeader(pELFSectionHeader),
	m_pSecHeaderStrTable(pSecHeaderStrTable),
	m_pProgramHeader(NULL),
	m_pSectionTableMap(NULL),
	m_pSymbolTable(NULL)
{
}

ELFParser::ELFParser(const upan::string& szFileName) :
	m_uiSymTabCount(0),
	m_pBR(NULL),
	m_pHeader(NULL),
	m_pSectionHeader(NULL),
	m_pSecHeaderStrTable(NULL),
	m_pProgramHeader(NULL),
	m_pSectionTableMap(NULL),
	m_pSymbolTable(NULL)
{
  upan::uniq_ptr<BufferedReader> pBR(new BufferedReader(szFileName, 0, .5 * 1024 * 1024));
  m_pBR = pBR.get();
  ReadHeader();
  ReadProgramHeaders();
  ReadSectionHeaders();
  ReadSecHeaderStrTable();
  ReadSymbolTables();
  pBR.release();
}

ELFParser::~ELFParser()
{
  if(m_pBR)
  {
    delete m_pBR ;
    DeAllocateSymbolTable() ;
    delete[] m_pSecHeaderStrTable;
    delete[] m_pSectionHeader;
    delete[] m_pSectionTableMap;
    delete[] m_pProgramHeader;
    delete m_pHeader;
  }
}

void ELFParser::AllocateSymbolTable()
{
	m_uiSymTabCount = 0 ;

	for(int i = 0; i < m_pHeader->e_shnum; i++)
	{
		if(m_pSectionHeader[i].sh_type == ELFSectionHeader::SHT_SYMTAB)
			m_uiSymTabCount++ ;
	}

	m_pSymbolTable = new ELFSymbolTableList[ m_uiSymTabCount ] ;

	unsigned uiSymTabIndex = 0 ;
	for(int i = 0; i < m_pHeader->e_shnum; i++)
	{
		if(m_pSectionHeader[i].sh_type == ELFSectionHeader::SHT_SYMTAB)
		{
			m_pSymbolTable[uiSymTabIndex].uiTableSize = (m_pSectionHeader[i].sh_size / m_pSectionHeader[i].sh_entsize) ;

			m_pSymbolTable[uiSymTabIndex].SymTabEntries = new ELFSymbolTable::ELF32SymbolEntry[ m_pSymbolTable[uiSymTabIndex].uiTableSize ] ;

			uiSymTabIndex++ ;
		}
	}
}

void ELFParser::DeAllocateSymbolTable()
{
	for(unsigned i = 0; i < m_uiSymTabCount; i++)
		delete[] (m_pSymbolTable[i].SymTabEntries) ;
	
	delete[] m_pSymbolTable ;
}

void ELFParser::ReadHeader()
{
  m_pHeader = new ELF32Header;
  m_pBR->Seek(0);
  const unsigned n = m_pBR->Read((char*)m_pHeader, sizeof(ELF32Header));

	if(n < sizeof(ELF32Header))
    throw upan::exception(XLOC, "elf file header size %u is less than ELF32Header size %u", n, sizeof(ELF32Header));

	if(!CheckMagicSignature(m_pHeader))
    throw upan::exception(XLOC, "Invalid ELF 32 magic signature");
}

void ELFParser::ReadProgramHeaders()
{
	/* e_phentsize is not used as this Parser is only for 32 bit elf files.
	   and ElfProgHeader size is 32 bytes */

  m_pProgramHeader = new ELF32ProgramHeader[ m_pHeader->e_phnum ] ;

  m_pBR->Seek(m_pHeader->e_phoff);

  const unsigned n = m_pBR->Read((char*)m_pProgramHeader, sizeof(ELF32ProgramHeader) * m_pHeader->e_phnum);

	if(n < sizeof(ELF32ProgramHeader) * m_pHeader->e_phnum)
    throw upan::exception(XLOC, "Invalid program header size: %u - expected: %u", n, sizeof(ELF32ProgramHeader) * m_pHeader->e_phnum);
}

void ELFParser::ReadSectionHeaders()
{
	/* e_shentsize if not used as this Parser is only for 32 bit elf files.
	   and ElfSectionHeader size is 40 bytes */
	   
  m_pSectionHeader = new ELF32SectionHeader[ m_pHeader->e_shnum ] ;
  m_pSectionTableMap = new int[ m_pHeader->e_shnum ] ;

  for(int i = 0; i < m_pHeader->e_shnum; i++)
    m_pSectionTableMap[i] = -1 ;


  m_pBR->Seek(m_pHeader->e_shoff);

  const unsigned n = m_pBR->Read((char*)m_pSectionHeader, sizeof(ELF32SectionHeader) * m_pHeader->e_shnum);

	if(n < sizeof(ELF32SectionHeader) * m_pHeader->e_shnum)
    upan::exception(XLOC, "Invalid elf section header size %u - expected: %u", n, sizeof(ELF32SectionHeader) * m_pHeader->e_shnum);
}

void ELFParser::ReadSecHeaderStrTable()
{
  const unsigned uiSecSize = m_pSectionHeader[m_pHeader->e_shstrndx].sh_size ;
  const unsigned uiSecOffset = m_pSectionHeader[m_pHeader->e_shstrndx].sh_offset ;

  m_pSecHeaderStrTable = new char[ uiSecSize ] ;

  m_pBR->Seek(uiSecOffset);

  unsigned n = m_pBR->Read((char*)m_pSecHeaderStrTable, uiSecSize);

	if(n < uiSecSize)
    upan::exception(XLOC, "Invalid elf section header string table size: %u - expected: %u", n, uiSecSize);
}

void ELFParser::ReadSymbolTables()
{
	AllocateSymbolTable() ;

	unsigned uiSymTabIndex = 0 ;
  for(uint32_t i = 0; i < m_pHeader->e_shnum; i++)
	{
		if(m_pSectionHeader[i].sh_type == ELFSectionHeader::SHT_SYMTAB)
		{
      m_pBR->Seek(m_pSectionHeader[i].sh_offset);
      const uint32_t n = m_pBR->Read((char*)(m_pSymbolTable[uiSymTabIndex].SymTabEntries), sizeof(ELFSymbolTable::ELF32SymbolEntry) * m_pSymbolTable[uiSymTabIndex].uiTableSize);

			if(n < sizeof(ELFSymbolTable::ELF32SymbolEntry) * m_pSymbolTable[uiSymTabIndex].uiTableSize)
        throw upan::exception(XLOC, "Invalid elf symbol table size: %u - excpected: %u", n, sizeof(ELFSymbolTable::ELF32SymbolEntry) * m_pSymbolTable[uiSymTabIndex].uiTableSize);

			m_pSectionTableMap[i] = uiSymTabIndex ;
      ++uiSymTabIndex;
		}
	}
}

upan::result<uint32_t*> ELFParser::GetAddressBySectionName(byte* bProcessImage, unsigned uiMinMemAddr, const char* szSectionName)
{
	for(int i = 0; i < m_pHeader->e_shnum; i++)
		if(strcmp(ELFSectionHeader::GetSectionName(m_pSecHeaderStrTable, m_pSectionHeader[i].sh_name), szSectionName) == 0)
      return upan::good((uint32_t*)(bProcessImage + m_pSectionHeader[i].sh_addr - uiMinMemAddr));

  return upan::result<uint32_t*>::bad("%s section not found in process image", szSectionName);
}

upan::result<uint32_t> ELFParser::GetNoOfGOTEntriesBySectionName(const char* szSectionName)
{
	for(int i = 0; i < m_pHeader->e_shnum; i++)
		if(strcmp(ELFSectionHeader::GetSectionName(m_pSecHeaderStrTable, m_pSectionHeader[i].sh_name), szSectionName) == 0)
      return upan::good(m_pSectionHeader[i].sh_size / m_pSectionHeader[i].sh_entsize);
  return upan::result<uint32_t>::bad("no GOT entries found for %s section", szSectionName);
}

void ELFParser::GetMemImageSize(unsigned* uiMinMemAddr, unsigned* uiMaxMemAddr) const
{
	*uiMinMemAddr = 0 ;
	*uiMaxMemAddr = 0 ;

	bool bFirstTime = true ;

	for(unsigned i = 0; i < m_pHeader->e_phnum; i++)
	{
		if(m_pProgramHeader[i].p_type == ELFProgramHeader::PT_LOAD)
		{
			if(bFirstTime == true)
			{
				bFirstTime = false ;
				*uiMinMemAddr = m_pProgramHeader[i].p_vaddr ;
			}

			*uiMaxMemAddr = m_pProgramHeader[i].p_vaddr + m_pProgramHeader[i].p_memsz ;
		}
	}
}

unsigned ELFParser::GetProgramStartAddress() 
{
	return m_pHeader->e_entry ;
}

upan::result<uint32_t*> ELFParser::GetGOTAddress(byte* bProcessImage, unsigned uiMinMemAddr)
{
  auto res = GetAddressBySectionName(bProcessImage, uiMinMemAddr, ".got.plt");
  if(res.isBad())
    res = GetAddressBySectionName(bProcessImage, uiMinMemAddr, ".got") ;
  return res;
}

upan::result<uint32_t> ELFParser::GetNoOfGOTEntries()
{
  auto res = GetNoOfGOTEntriesBySectionName(".got.plt");
  if(res.isBad())
    res = GetNoOfGOTEntriesBySectionName(".got");
  return res;
}

upan::result<ELF32SectionHeader*> ELFParser::GetSectionHeaderByType(unsigned uiType)
{
	for(int i = 0; i < m_pHeader->e_shnum; i++)
		if(m_pSectionHeader[i].sh_type == uiType)
      return upan::good(&m_pSectionHeader[i]);
  return upan::result<ELF32SectionHeader*>::bad("Failed to find elf section header for type: %u", uiType);
}

upan::result<ELF32SectionHeader*> ELFParser::GetSectionHeaderByTypeAndName(unsigned uiType, const char* szLikeName)
{
	for(int i = 0; i < m_pHeader->e_shnum; i++)
		if(strstr((m_pSecHeaderStrTable + m_pSectionHeader[i].sh_name), szLikeName) && m_pSectionHeader[i].sh_type == uiType)
      return upan::good(&m_pSectionHeader[i]);
  return upan::result<ELF32SectionHeader*>::bad("Failed to find elf section header for type: %u, name: %s", uiType, szLikeName);
}

upan::result<ELF32SectionHeader*> ELFParser::GetSectionHeaderByIndex(unsigned uiIndex)
{
	if(uiIndex >= m_pHeader->e_shnum)
    return upan::result<ELF32SectionHeader*>::bad("Failed to find elf section header for index: %u", uiIndex);
  return upan::good(&m_pSectionHeader[uiIndex]);
}

void ELFParser::CopyProcessImage(byte* bProcessImage, unsigned uiProcessBase, unsigned uiMaxImageSize) const
{
	for(unsigned i = 0; i < m_pHeader->e_phnum; i++)
	{
		if(m_pProgramHeader[i].p_type == ELFProgramHeader::PT_LOAD)
		{
      m_pBR->Seek(m_pProgramHeader[i].p_offset);
      const uint32_t uiOffset = m_pProgramHeader[i].p_vaddr - uiProcessBase ;

			if(uiOffset >= uiMaxImageSize)
        throw upan::exception(XLOC, "process load virtual address %x is larger than max image size %x", uiOffset, uiMaxImageSize);

      const uint32_t n = m_pBR->Read((char*)bProcessImage + uiOffset, m_pProgramHeader[i].p_filesz);

			if(n < m_pProgramHeader[i].p_filesz)
        throw upan::exception(XLOC, "Invalid elf file size: %u - expected: %u", n, m_pProgramHeader[i].p_filesz);
		}
	}
}

char* ELFParser::CopyELFSecStrTable() {
	uint32_t uiSize = m_pSectionHeader[ m_pHeader->e_shstrndx ].sh_size ;
	auto secStrTable = new char[uiSize];
	memcpy(secStrTable, m_pSecHeaderStrTable, uiSize) ;
	return secStrTable;
}

ELF32SectionHeader* ELFParser::CopyELFSectionHeader() {
	auto sectionHeaders = new ELF32SectionHeader[m_pHeader->e_shnum];
  memcpy(sectionHeaders, m_pSectionHeader, sizeof(ELF32SectionHeader) * m_pHeader->e_shnum);
	return sectionHeaders;
}

bool ELFParser::CheckMagicSignature(const ELF32Header* pELFHeader)
{
	return (pELFHeader->e_ident[ELFHeader::EI_MAG0] == 0x7F 
		&& pELFHeader->e_ident[ELFHeader::EI_MAG1] == 'E'
		&& pELFHeader->e_ident[ELFHeader::EI_MAG2] == 'L'
		&& pELFHeader->e_ident[ELFHeader::EI_MAG3] == 'F'
		&& (pELFHeader->e_type == ELFHeader::ET_EXEC || pELFHeader->e_type == ELFHeader::ET_DYN)) ;
}

/*
static void ElfHacker_KC::MDisplay().e_ident_Field(const Elf_Header* elfHeader)
{
	printf("\n e_ident:") ;
	printf("\n\t\tEI_MAG0 = 0x%X (%d)", elfHeader->e_ident[EI_MAG0], elfHeader->e_ident[EI_MAG0]) ;
	printf("\n\t\tEI_MAG1 = %c", elfHeader->e_ident[EI_MAG1]) ; 
	printf("\n\t\tEI_MAG2 = %c", elfHeader->e_ident[EI_MAG2]) ;
	printf("\n\t\tEI_MAG3 = %c", elfHeader->e_ident[EI_MAG3]) ;
	printf("\n\t\tEI_CLASS = %s", Elf_Classes[elfHeader->e_ident[EI_CLASS]]) ; 
	printf("\n\t\tEI_DATA = %s", Elf_DataEncoding[elfHeader->e_ident[EI_DATA]]) ;
	printf("\n\t\tEI_VERSION = %s", Elf_Versions[elfHeader->e_ident[EI_VERSION]]) ;
	printf("\n\t\tEI_PAD = %d\n", elfHeader->e_ident[EI_PAD]) ;
}
{
	ElfHacker_KC::MDisplay().e_ident_Field(&ElfHacker_elfHeader) ;
	printf("\n\te_tpye = %s", Elf_eType[ElfHacker_elfHeader.e_type]) ;
	printf("\n\te_machine = %s", Elf_eMachine[ElfHacker_elfHeader.e_machine]) ;
	printf("\n\te_version = %d", ElfHacker_elfHeader.e_version) ;
	printf("\n\te_entry = 0x%X (%d)", ElfHacker_elfHeader.e_entry, ElfHacker_elfHeader.e_entry) ;
	printf("\n\te_phoff = %d", ElfHacker_elfHeader.e_phoff) ;
	printf("\n\te_shoff = %d", ElfHacker_elfHeader.e_shoff) ;
	printf("\n\te_flags = %d", ElfHacker_elfHeader.e_flags) ;
	printf("\n\te_ehsize = %d", ElfHacker_elfHeader.e_ehsize) ;
	printf("\n\te_phentsize = %d", ElfHacker_elfHeader.e_phentsize) ;
	printf("\n\te_phnum = %d", ElfHacker_elfHeader.e_phnum) ;
	printf("\n\te_shentsize = %d", ElfHacker_elfHeader.e_shentsize) ;
	printf("\n\te_shnum = %d", ElfHacker_elfHeader.e_shnum) ;
	printf("\n\te_shstrndx = %d", ElfHacker_elfHeader.e_shstrndx) ;

}*/

namespace ELFProgramHeader
{
	const char* GetProgHeaderType(unsigned uiPType)
	{
		if(0 <= uiPType && uiPType < MAX_PROG_TYPES)
			return ProgramHeaderType[uiPType] ;

		return (uiPType <= PT_LOPROC || uiPType >= PT_HIPROC) ? "Processor-Specific" : "Invalid Program Header Type" ;
	}
} ;

namespace ELFSectionHeader
{
	const char* GetSecHeaderType(unsigned uiSType)
	{
		if(0 <= uiSType && uiSType < MAX_SEC_HEADER_TYPES)
			return SectionHeaderType[ uiSType ] ;

		return (uiSType > SHT_LOPROC && uiSType <= SHT_HIPROC) ? "Processor-Specific" : "Invalid Section Header Type" ;
	}

	const char* GetSectionName(const char* szStrTable, int iIndex)
	{
		return szStrTable + iIndex ;
	}
} ;
