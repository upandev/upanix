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
#ifndef _MEM_MANAGER_H_
#define _MEM_MANAGER_H_

#include <Global.h>
#include <MemConstants.h>
#include <ProcessConstants.h>
#include <ReturnHandler.h>

#define KERNEL_PROCESS_PDE_ID	1022

extern "C"
{
	extern unsigned MEM_PDBR ;
	void Mem_EnablePaging() ;
	void Mem_FlushTLB() ;
	void Mem_FlushTLBPage(unsigned uiPageNumber) ;
}

class MemManager
{
	private:
		MemManager();
	public:
		static MemManager& Instance()
		{
			static MemManager instance;
			return instance;
		}
		static void RawMapGraphicsLFB();
    void PrintInitStatus() const;
		ReturnCode MarkPageAsAllocated(unsigned uiPageNumber, ReturnCode prevRetCode) ;
		unsigned AllocatePhysicalPage();
		void DeAllocatePhysicalPage(const unsigned uiPageNumber) ;
		unsigned AllocatePageForKernel();
		void DeAllocatePageForKernel(unsigned uiPageNumber);

    //uint32_t AllocatePhysicalPage(const uint32_t noOfPages = 1);
    //void DeAllocatePhysicalPage(uint32_t pageNo, const uint32_t noOfPages = 1);
		ReturnCode AllocatePage(int iProcessID, unsigned uiFaultyAddress) ;
		ReturnCode DeAllocatePage(const unsigned uiAddress) ;
		void DisplayNoOfFreePages() ;
		unsigned GetFlatAddress(unsigned uiVirtualAddress) ;
		void DisplayNoOfAllocPages() ;

		static void InitPage(unsigned uiPage) ;
		static void PageFaultHandlerTaskGate() ;

		inline unsigned GetKernelHeapStartAddr() { return m_uiKernelHeapStartAddress ; }
		inline unsigned GetRamSize() { return RAM_SIZE; }

		int AllocateKernelStack();
		void DeAllocateKernelStack(int stackBlockId);
    inline uint32_t GetKernelStackAddress(int stackBlockId) {
      return KERNEL_PROCESS_PDE_ID * PAGE_TABLE_ENTRIES * PAGE_SIZE + stackBlockId * PROCESS_KERNEL_STACK_PAGES * PAGE_SIZE;
		}

		static inline unsigned GetProcessSizeInPages(unsigned uiSizeInBytes)
		{
			return ((uiSizeInBytes - 1) / PAGE_SIZE) + 1 ;
		}

		static inline unsigned GetPTESizeInPages(unsigned uiSizeInPages)
		{
			return ((uiSizeInPages - 1) / PAGE_TABLE_ENTRIES) + 1 ;
		}

		inline uintptr_t& GetKernelAUTAddress()
		{
			return m_uiKernelAUTAddress ;
		}
    void MemMapGraphicsLFB(uint32_t memTypeFlag);

	private:
		bool BuildRawPageMap() ;
		bool BuildPagePoolMap();
		bool BuildPageTable() ;
    bool MarkACPIInfoRegionAsAllocated();
    int GetFreeKernelProcessStackBlockID() ;

	private:
		unsigned m_uiNoOfPages ;
		unsigned m_uiNoOfResvPages ;
		unsigned m_uiKernelHeapSize ;
		unsigned m_uiKernelHeapStartAddress ;

    uintptr_t* m_uiPageMap ;
		unsigned m_uiPageMapSize ;

    uintptr_t* m_uiKernelPagePoolMap;
		unsigned m_uiKernelPagePoolMapSize;
		unsigned m_uiKernelPagePoolStartPage;

		unsigned m_uiResvSize ;

    uintptr_t* m_uiPDEBase ;
    uintptr_t* m_uiPTEBase ;
    uintptr_t* m_uipKernelProcessStackPTEBase ;
    uintptr_t m_uiKernelAUTAddress ;

		int m_iNoOfKernelProcessStackBlocks ;
		bool m_bAllocationMapForKernelProcessStackBlock[PAGE_TABLE_ENTRIES / PROCESS_KERNEL_STACK_PAGES] ;

		const unsigned RAM_SIZE ;
};

#endif
