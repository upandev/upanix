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

extern "C" {
	extern uint64_t MEM_PML4 ;
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
    void PrintInitStatus() const;
		ReturnCode MarkPageAsAllocated(unsigned uiPageNumber, ReturnCode prevRetCode) ;
		unsigned AllocatePhysicalPage();
		void DeAllocatePhysicalPage(const unsigned uiPageNumber) ;
		unsigned AllocatePageForKernel();
		void DeAllocatePageForKernel(unsigned pageNumber);

    //uint32_t AllocatePhysicalPage(const uint32_t noOfPages = 1);
    //void DeAllocatePhysicalPage(uint32_t pageNo, const uint32_t noOfPages = 1);
		ReturnCode AllocatePage(int iProcessID, unsigned uiFaultyAddress) ;
		ReturnCode DeAllocatePage(const unsigned uiAddress) ;
		void DisplayNoOfFreePages() ;
		unsigned GetFlatAddress(unsigned uiVirtualAddress) ;
		void DisplayNoOfAllocPages() ;

    static void Mmap(uint64_t vAddr, uint64_t pAddr, uint32_t pageFlag);
		static void InitPage(unsigned uiPage) ;
		static void PageFaultHandlerTaskGate() ;

		inline unsigned GetKernelHeapStartAddr() { return MEM_KERNEL_HEAP_START; }
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
			return _kernelAUTAddress ;
		}

	private:
		bool BuildRawPageMap() ;
		bool BuildPagePoolMap();
		bool BuildPageTable() ;
    bool MarkACPIInfoRegionAsAllocated();

	private:
    uint32_t _noOfPages ;
    uint32_t _kernelReservedPages ;
    uint32_t _kernelHeapMapSize ;

    uint64_t* _pageMap ;
		uint32_t _pageMapSize ;

    uint64_t* _kernelPagePoolMap;
    uint32_t _kernelPagePoolMapSize;
    uint32_t _kernelPagePoolStartPage;

    uint32_t _kernelReservedMapSize ;

    uintptr_t* m_uiPTEBase ;
    uintptr_t* m_uipKernelProcessStackPTEBase ;
    uintptr_t _kernelAUTAddress ;

		bool _allocMapForKernelProcessStackBlock[NO_OF_KERNEL_STACK_BLOCKS];

		const uint64_t RAM_SIZE ;
};

#endif
