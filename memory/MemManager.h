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

#include <Global.h>
#include <MemConstants.h>
#include <ProcessConstants.h>
#include <ReturnHandler.h>
#include "DMM.h"

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
		uint64_t AllocatePhysicalPage();
		void DeAllocatePhysicalPage(uint64_t uiPageNumber) ;
		unsigned AllocatePageForKernel();
		void DeAllocatePageForKernel(unsigned pageNumber);

		ReturnCode AllocatePage(int iProcessID, uintptr_t faultyAddress) ;
    uintptr_t GetFlatAddress(uint64_t* pml4Table, uintptr_t virtualAddress) ;
    uintptr_t GetFlatAddressFromPD(uint64_t* pdTable, uintptr_t virtualAddress);
		void DisplayNoOfFreePages() ;
		void DisplayNoOfAllocPages() ;

    static void KernelPageTableMmap(const uint64_t vAddr, const uint64_t pAddr, const uint32_t pageFlag);
    uint64_t* GetPTTable(uint64_t* pml4Table, uintptr_t virtualAddress);
    uint64_t* GetPTTableFromPD(uint64_t* pdTable, uintptr_t virtualAddress);

    void AllocateAddressSpace(uint64_t* pml4Table, uint32_t pageConfig, uintptr_t virtualAddress, uintptr_t size);
    void AllocatePDAddressSpace(uint64_t* pdTable, uint32_t pageConfig, uintptr_t virtualAddress, uintptr_t size);
    void DeallocateAddressSpace(uint64_t* pml4Table);
    void DeallocatePDAddressSpace(uint64_t* pdTable);

    void MapAddressSpace(uint64_t* pml4Table, uint32_t pageConfig, uintptr_t virtualAddress, uintptr_t realAddress, uintptr_t size);
    void UnMapAddressSpace(uint64_t* pml4Table, uintptr_t virtualAddress, uintptr_t size);

		static void InitPage(uint64_t uiPage) ;
		static void PageFaultHandlerTaskGate(uint64_t errorCode) ;

		inline uint64_t GetRamSize() { return RAM_SIZE; }

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

    uintptr_t* m_uipKernelProcessStackPTEBase ;

		bool _allocMapForKernelProcessStackBlock[NO_OF_KERNEL_STACK_BLOCKS];

		const uint64_t RAM_SIZE ;
};
