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
#include <MemManager.h>
#include <ProcessManager.h>
#include <DMM.h>
#include <KernelService.h>
#include <MultiBoot.h>
#include <MemUtil.h>
#include <AsmUtil.h>
#include <IDT.h>
#include <mutex.h>
#include <exception.h>
#include <GraphicsVideo.h>
#include <RootGUIConsole.h>

extern "C" { 
	unsigned MEM_PDBR ;
}

#define PML4_INDEX(ADDR) (((ADDR) >> 39) & 0x1FF)
#define PDP_INDEX(ADDR) (((ADDR) >> 30) & 0x1FF)
#define PD_INDEX(ADDR) (((ADDR) >> 21) & 0x1FF)
#define PT_INDEX(ADDR) (((ADDR) >> 12) & 0x1FF)
#define PAGE_INDEX(ADDR) ((ADDR) & 0xFFF)

void MemManager::PageFaultHandlerTaskGate()
{
	AsmUtil_STORE_GPR() ;
	
	__volatile__ unsigned short usDS = MemUtil_GetDS() ; 
	__volatile__ unsigned short usES = MemUtil_GetES() ; 
	__volatile__ unsigned short usFS = MemUtil_GetFS() ; 
	__volatile__ unsigned short usGS = MemUtil_GetGS() ;

	__asm__ __volatile__("pushw %0" : : "i"(SYS_DATA_SELECTOR_DEFINED)) ; 
	__asm__ __volatile__("pushw %0" : : "i"(SYS_DATA_SELECTOR_DEFINED)) ; 
	__asm__ __volatile__("pushw %0" : : "i"(SYS_DATA_SELECTOR_DEFINED)) ; 
//	__asm__ __volatile__("popw %ds") ;
//	__asm__ __volatile__("popw %fs") ;
//	__asm__ __volatile__("popw %gs") ;

	__asm__ __volatile__("pushw %0" : : "i"(SYS_DATA_SELECTOR_DEFINED)) ; 
//	__asm__ __volatile__("popw %es") ;

//	__volatile__ unsigned MM_errCode ;
//	__volatile__ unsigned CS ;
//	__volatile__ unsigned IP ;
//
	__volatile__ unsigned uiFaultyAddress ;
//	__asm__ __volatile__("mov %%cr2, %0" : "=r"(uiFaultyAddress) : ) ;

	if (IS_KERNEL()) {
    printf("\n Page Fault in Kernel! FIX THIS !!! @ %u", uiFaultyAddress);
    while(1);
  }
	if(!KC::MKernelService().RequestPageFault(uiFaultyAddress))
	{
//	__asm__ __volatile__("leave") ;
//	__asm__ __volatile__("popl %0" : "=m"(MM_errCode) : ) ;
//	__asm__ __volatile__("popl %0" : "=m"(IP) : ) ;
//	__asm__ __volatile__("popl %0" : "=m"(CS) : ) ;
//
//	KC::MDisplay().Address("\nMM_errCode = ", MM_errCode) ;
//	KC::MDisplay().Address("\nIP = ", IP) ;
//	KC::MDisplay().Address("\nCS = ", CS) ;

		ProcessManager_EXIT() ;
	}

	__asm__ __volatile__("movw %%ss:%0, %%ds" :: "m"(usDS) ) ;
	__asm__ __volatile__("movw %%ss:%0, %%es" :: "m"(usES) ) ;
	__asm__ __volatile__("movw %%ss:%0, %%fs" :: "m"(usFS) ) ;
	__asm__ __volatile__("movw %%ss:%0, %%gs" :: "m"(usGS) ) ;

	AsmUtil_RESTORE_GPR() ;

//	AsmUtil_UNLOAD_KERNEL_SEGS_ON_STACK() ;
//	__asm__ __volatile__("popf") ;
//	__asm__ __volatile__("popa") ;

	__asm__ __volatile__("leave") ;
//	__asm__ __volatile__("popl %%ss:%0" : "=m"(MM_errCode) : ) ;
//	TODO: Make this POP more meaningful.
//	__asm__ __volatile__("popl %ecx") ;
	__asm__ __volatile__("addl $0x4, %esp");
	
	__asm__ __volatile__("iret") ;
}

MemManager::MemManager() : 
	m_uiKernelAUTAddress(NULL),
	RAM_SIZE(MultiBoot::Instance().GetRamSize())
{
  if(BuildRawPageMap()) {
    if(BuildPageTable()) {
        if(BuildPagePoolMap()) {
          if (MarkACPIInfoRegionAsAllocated()) {
            MemMapGraphicsLFB(0x0);
            Mem_FlushTLB();

            KC::MConsole().LoadMessage("Memory Manager Initialization", Success);
            return;
          }
        }
    }
  }

  KC::MConsole().Message("\n *********** KERNEL PANIC ************ \n", '$') ;
  while(1) ;
}

void MemManager::PrintInitStatus() const {
  printf("\n\tRAM SIZE = %d", RAM_SIZE) ;
  printf("\n\tNo. of Pages = %d", m_uiNoOfPages) ;
  printf("\n\tNo. of Resv Pages = %d", m_uiNoOfResvPages) ;
}

bool MemManager::MarkACPIInfoRegionAsAllocated() {
  auto acpiMmap = MultiBoot::Instance().GetACPIInfoMemMap();
  if (!acpiMmap) {
    return true;
  }

  const uint32_t noOfPages = ((acpiMmap->length + PAGE_SIZE) / PAGE_SIZE) - 1;
  uint32_t addr = acpiMmap->addr;

  ReturnCode markPageRetCode = Success;
  for(int i = 0; i < noOfPages; ++i) {
    markPageRetCode = MarkPageAsAllocated(addr / PAGE_SIZE, markPageRetCode);
    if (markPageRetCode != Success) {
      return false;
    }
    addr += PAGE_SIZE;
  }

  return true;
}

void MemManager::MemMapGraphicsLFB(uint32_t memTypeFlag)
{
  unsigned noOfPages = (GraphicsVideo::Instance().LFBSize() / PAGE_SIZE) + 1;
  unsigned availablePages = MEM_GRAPHICS_VIDEO_MAP_SIZE / PAGE_SIZE;
  if(noOfPages > availablePages)
  {
    printf("\n Insufficient graphics video buffer. Required pages: %u", noOfPages);
    while(1);
  }
  unsigned lfbaddress = GraphicsVideo::Instance().FlatLFBAddress();
  unsigned mapAddress = MEM_GRAPHICS_VIDEO_MAP_START;
  ReturnCode markPageRetCode = Success;
  for(unsigned i = 0; i < noOfPages; ++i) {
    unsigned addr = lfbaddress + PAGE_SIZE * i;
    unsigned uiPDEIndex = ((mapAddress >> 22) & 0x3FF);
    unsigned uiPTEAddress = (((unsigned*)(MEM_PDBR - GLOBAL_DATA_SEGMENT_BASE))[uiPDEIndex]) & 0xFFFFF000;
    unsigned uiPTEIndex = ((mapAddress >> 12) & 0x3FF);
    // This page is a Read Only area for user process. 0x5 => 101 => User Domain, Read Only, Present Bit
    ((unsigned*)(uiPTEAddress - GLOBAL_DATA_SEGMENT_BASE))[uiPTEIndex] = (addr & 0xFFFFF000) | 0x5 | (memTypeFlag & 0xFF);
    markPageRetCode = MarkPageAsAllocated(addr / PAGE_SIZE, markPageRetCode);
    mapAddress += PAGE_SIZE;
  }
  GraphicsVideo::Instance().MappedLFBAddress(MEM_GRAPHICS_VIDEO_MAP_START);
  RootGUIConsole::Instance().resetFrameBuffer(MEM_GRAPHICS_VIDEO_MAP_START);
}

void MemManager::RawMapGraphicsLFB() {
  auto f = MultiBoot::Instance().VideoFrameBufferInfo();
  const uint32_t lfbSize = f->_width * f->_height * f->_bpp / 8;
  const uint32_t noOfPages = (lfbSize / PAGE_SIZE) + 1;
  const uint32_t availablePages = MEM_GRAPHICS_VIDEO_MAP_SIZE / PAGE_SIZE;
  if(noOfPages > availablePages) {
    //PANIC
    while(true);
  }
  uint64_t lfbaddress = (uint64_t)f->_frameBuffer;
  uint64_t mapAddress = MEM_GRAPHICS_VIDEO_MAP_START;

  for(unsigned i = 0; i < noOfPages; ++i) {
    const uint64_t addr = lfbaddress + PAGE_SIZE * i;

    auto pdpEntry = (uint64_t*)(PML4_TABLE[PML4_INDEX(mapAddress)] & ~0xFFF);
    auto pdEntry = (uint64_t*)(pdpEntry[PDP_INDEX(mapAddress)] & ~0xFFF);
    auto ptEntry = (uint64_t*)(pdEntry[PD_INDEX(mapAddress)] & ~0xFFF);
    // This page is a Read Only area for user process. 0x3 => 011 => Supervisor, Read/Write, Present Bit
    ptEntry[PT_INDEX(mapAddress)] = (addr & ~0xFFF) | 0x3;
    mapAddress += PAGE_SIZE;
  }
  Mem_FlushTLB();
}

void MemManager::InitPage(unsigned uiPage)
{
	uiPage = uiPage * PAGE_SIZE;
  memset((void*)(uiPage - GLOBAL_DATA_SEGMENT_BASE), 0, PAGE_SIZE);
}

bool MemManager::BuildRawPageMap()
{
  m_uiPageMap = (uintptr_t*)MEM_PAGE_MAP_START ;
  m_uiPageMapSize = (((RAM_SIZE / PAGE_SIZE) / 8) / 4) ;
  m_uiResvSize = (((MEM_KERNEL_RESV_SIZE / PAGE_SIZE) / 8) / 4) ;
  m_uiNoOfResvPages = MEM_KERNEL_RESV_SIZE / PAGE_SIZE;
  m_uiKernelHeapSize = (MEM_KERNEL_HEAP_SIZE / PAGE_SIZE) / 8 / 4 ;
  m_uiKernelHeapStartAddress = MEM_KERNEL_HEAP_START - GLOBAL_DATA_SEGMENT_BASE;

  if((m_uiPageMapSize * 4) > (MEM_PAGE_MAP_END - MEM_PAGE_MAP_START))
  {
    KC::MConsole().Message("\n Mem Page Map Size InSufficient\n", 'A') ;
    return false ;
  }

  for(uint32_t i = 0; i < m_uiPageMapSize; i++)
    m_uiPageMap[i] &= 0x0 ;

  for(uint32_t i = 0; i < m_uiResvSize; i++)
    m_uiPageMap[i] |= 0xFFFFFFFF ;

  return true ;
}	

bool MemManager::BuildPagePoolMap()
{
  m_uiKernelPagePoolMap = (uintptr_t*)(MEM_KERNEL_PAGE_POOL_MAP_START - GLOBAL_DATA_SEGMENT_BASE);
  m_uiKernelPagePoolMapSize = MEM_KERNEL_PAGE_POOL_SIZE / PAGE_SIZE / 8 / 4;
  m_uiKernelPagePoolStartPage = (MEM_KERNEL_HEAP_START + MEM_KERNEL_HEAP_SIZE) / PAGE_SIZE;

  if((m_uiKernelPagePoolMapSize * 4) > (MEM_KERNEL_PAGE_POOL_MAP_END - MEM_KERNEL_PAGE_POOL_MAP_START))
  {
    KC::MConsole().Message("\n Mem Page Pool Map Size InSufficient\n", 'A') ;
    return false ;
  }

  for(uint32_t i = 0; i < m_uiKernelPagePoolMapSize; i++)
    m_uiKernelPagePoolMap[i] &= 0x0 ;

  return true;
}

bool MemManager::BuildPageTable()
{
	// Expecting a complete 4 GB Page Table setup
	// i.e, PTE of size 4 MB and PDE of size 4 KB
	MEM_PDBR = MEM_PDE_START + GLOBAL_DATA_SEGMENT_BASE ;

	unsigned uiNoOfPDEEntries ;
	unsigned uiPageAddress ;
	unsigned uiPageTableAddress ;
	
	m_uiNoOfPages = RAM_SIZE / PAGE_SIZE ;
	uiNoOfPDEEntries = m_uiNoOfPages / PAGE_TABLE_ENTRIES ;

	m_uiPDEBase = (uintptr_t*)MEM_PDE_START ;
	m_uiPTEBase = (uintptr_t*)MEM_PTE_START ;
	
	if((uiNoOfPDEEntries * 4) > (MEM_PDE_END - MEM_PDE_START))
	{
    KC::MConsole().Message("\n PDE Size InSufficient\n", 'A') ;
		return false ;
	}

	if((m_uiNoOfPages * 4) > (MEM_PTE_END - MEM_PTE_START))
	{
    KC::MConsole().Message("\n PTE Size InSufficient\n", 'A') ;
		return false ;
	}

	memset((void*)MEM_PDE_START, 0, (MEM_PDE_END-MEM_PDE_START));
	memset((void*)MEM_PTE_START, 0, (MEM_PTE_END-MEM_PTE_START));
	
	uiPageTableAddress = MEM_PTE_START + GLOBAL_DATA_SEGMENT_BASE ;
	
	//Map all PTEs into PDE. Pages in PTE will be mapped based 
	//on RAM SIZE.
	for(uint32_t uiPDE = 0; uiPDE < PAGE_TABLE_ENTRIES; uiPDE++)
	{
		m_uiPDEBase[uiPDE] = (uiPageTableAddress & 0xFFFFF000) | 0x3 ;
		uiPageTableAddress += PAGE_TABLE_SIZE ;
	}

  m_uiPTEBase[0] = 0;
	uiPageAddress = PAGE_SIZE;
	for(uint32_t uiPTE = 1; uiPTE < m_uiNoOfPages; uiPTE++)
	{
		m_uiPTEBase[uiPTE] = (uiPageAddress & 0xFFFFF000) | 0x3 ;
		uiPageAddress += PAGE_SIZE ;
	}

	/***** Initialize Kernel Processes Stack Pages Table Entries *****/
	m_uipKernelProcessStackPTEBase = (uintptr_t*)(MEM_PTE_START + KERNEL_PROCESS_PDE_ID * PAGE_TABLE_SIZE);
	m_iNoOfKernelProcessStackBlocks = PAGE_TABLE_ENTRIES / PROCESS_KERNEL_STACK_PAGES ;
	
	for(int i = 0; i < m_iNoOfKernelProcessStackBlocks; i++)
		m_bAllocationMapForKernelProcessStackBlock[i] = false ;

	return true ;
}

int MemManager::GetFreeKernelProcessStackBlockID() {
  ProcessSwitchLock pLock;
	for(int i = 0; i < m_iNoOfKernelProcessStackBlocks; i++) {
		if(!m_bAllocationMapForKernelProcessStackBlock[i]) {
			m_bAllocationMapForKernelProcessStackBlock[i] = true ;
			return i;
		}
	}
	return -1 ;
}

int MemManager::AllocateKernelStack() {
  ProcessSwitchLock pLock;
  const int iStackBlockID = MemManager::Instance().GetFreeKernelProcessStackBlockID();

  if(iStackBlockID < 0)
    throw upan::exception(XLOC, "No free stack blocks available for kernel process");

  for(int i = 0; i < PROCESS_KERNEL_STACK_PAGES; i++) {
    uint32_t uiFreePageNo = MemManager::Instance().AllocatePhysicalPage();
    m_uipKernelProcessStackPTEBase[iStackBlockID * PROCESS_KERNEL_STACK_PAGES + i] = ((uiFreePageNo * PAGE_SIZE) & 0xFFFFF000) | 0x3 ;
  }
  Mem_FlushTLB();

  return iStackBlockID;
}

void MemManager::DeAllocateKernelStack(int stackBlockId) {
  ProcessSwitchLock pLock;

  if(stackBlockId < 0 || stackBlockId >= m_iNoOfKernelProcessStackBlocks)
    return ;

  for(int i = 0; i < PROCESS_KERNEL_STACK_PAGES; i++) {
    DeAllocatePhysicalPage((m_uipKernelProcessStackPTEBase[stackBlockId * PROCESS_KERNEL_STACK_PAGES + i] & 0xFFFFF000) / PAGE_SIZE);
  }
  Mem_FlushTLB();

  m_bAllocationMapForKernelProcessStackBlock[stackBlockId] = false ;
}

ReturnCode MemManager::MarkPageAsAllocated(unsigned uiPageNumber, ReturnCode prevRetCode) {
    ProcessSwitchLock pLock;
    unsigned uiPageMapIndex = uiPageNumber / 32 ;
    //Sometimes MEM IO addresses can fall beyond actual ram size (?) - no need to mark those pages as allocated then
    if(uiPageMapIndex >= m_uiPageMapSize)
        return Success;
    unsigned uiPageBitIndex = uiPageNumber % 32 ;
    unsigned uiCurVal = (m_uiPageMap[ uiPageMapIndex ] >> uiPageBitIndex) & 0x1 ;
    if(uiCurVal) {
        if (prevRetCode == Success) {
            printf("\n Error: Page: %u is already marked as allocated", uiPageNumber) ;
        }
        return Failure;
    }
    m_uiPageMap[ uiPageMapIndex ] |= (0x1 << uiPageBitIndex) ;
    return Success ;
}

unsigned MemManager::AllocatePhysicalPage()
{	
	ProcessSwitchLock lock;

	unsigned uiPageMapPosition ;
	unsigned uiPageOffset ;
	unsigned uiPageMapEntry ;
	
	for(uiPageMapPosition = m_uiResvSize; uiPageMapPosition < m_uiPageMapSize; uiPageMapPosition++)
	{
		if((m_uiPageMap[uiPageMapPosition] & 0xFFFFFFFF) != 0xFFFFFFFF)
		{
			uiPageMapEntry = m_uiPageMap[uiPageMapPosition] ;
			for(uiPageOffset = 0; uiPageOffset < 32; uiPageOffset++)
			{
				if((uiPageMapEntry & 0x1) == 0x0)
				{
					m_uiPageMap[uiPageMapPosition] |= (0x1	<< uiPageOffset) ;
          return (uiPageMapPosition * 4 * 8) + uiPageOffset;
				}
				uiPageMapEntry >>= 1 ;
			}
		}
	}
  throw upan::exception(XLOC, "Out of memory pages!");
}

void MemManager::DeAllocatePhysicalPage(const unsigned uiPageNumber)
{
  ProcessSwitchLock pLock;

	unsigned uiPageMapPosition ;
	unsigned uiPageOffset ;

	uiPageOffset = uiPageNumber % (8 * 4) ;
	uiPageMapPosition = uiPageNumber / (8 * 4) ;
	
	m_uiPageMap[uiPageMapPosition] = m_uiPageMap[uiPageMapPosition] & ~(0x1 << uiPageOffset) ;
}

unsigned MemManager::AllocatePageForKernel()
{
  ProcessSwitchLock lock;

  for(uint32_t i = 0; i < m_uiKernelPagePoolMapSize; ++i)
  {
    if((m_uiKernelPagePoolMap[i] & 0xFFFFFFFF) != 0xFFFFFFFF)
    {
      uint32_t uiPageMapEntry = m_uiKernelPagePoolMap[i] ;
      for(uint32_t uiPageOffset = 0; uiPageOffset < 32; ++uiPageOffset)
      {
        if((uiPageMapEntry & 0x1) == 0x0)
        {
          m_uiKernelPagePoolMap[i] |= (0x1 << uiPageOffset) ;
          return (i * 4 * 8) + uiPageOffset + m_uiKernelPagePoolStartPage;
        }
        uiPageMapEntry >>= 1 ;
      }
    }
  }
  throw upan::exception(XLOC, "Out of memory pages in kernel pool!");
}

void MemManager::DeAllocatePageForKernel(unsigned uiPageNumber)
{
  ProcessSwitchLock lock;

  uiPageNumber -= m_uiKernelPagePoolStartPage;
  const unsigned uiPageMapPosition = uiPageNumber / (8 * 4);
  const unsigned uiPageOffset = uiPageNumber % (8 * 4) ;

  m_uiKernelPagePoolMap[uiPageMapPosition] = m_uiKernelPagePoolMap[uiPageMapPosition] & ~(0x1 << uiPageOffset) ;
}

//uint32_t MemManager::AllocatePhysicalPage(const uint32_t noOfPages)
//{	
//	ProcessSwitchLock lock;
//  if(noOfPages == 0)
//    throw upan::exception(XLOC, "NoOfPages to allocate must be > 0");
//  struct { 
//    uint32_t index;
//    uint32_t offset;
//  } startPage, endPage;
//  auto finishAllocation = [this, &startPage, &endPage]() {
//      for(uint32_t mapIndex = startPage.index; mapIndex <= endPage.index; ++mapIndex)
//      {
//        const uint32_t sOffset = mapIndex == startPage.index ? startPage.offset : 0;
//        const uint32_t eOffset = mapIndex == endPage.index ? endPage.offset : 32;
//        for(uint32_t offset = sOffset; offset <= eOffset; ++offset)
//          m_uiPageMap[mapIndex] |= (1 << offset);
//      }
//  };
//  uint32_t count = noOfPages;
//	for(uint32_t uiPageMapPosition = m_uiResvSize; uiPageMapPosition < m_uiPageMapSize; uiPageMapPosition++)
//	{
//	  uint32_t uiPageMapEntry = m_uiPageMap[uiPageMapPosition];
//		if((uiPageMapEntry & 0xFFFFFFFF) != 0xFFFFFFFF)
//		{
//			for(uint32_t uiPageOffset = 0; uiPageOffset < 32; uiPageOffset++)
//			{
//				if((uiPageMapEntry & 0x1) == 0x0)
//				{
//          if(count == noOfPages)
//          {
//            startPage.index = uiPageMapPosition;
//            startPage.offset = uiPageOffset;
//          }
//          --count;
//          if(count == 0)
//          {
//            endPage.index = uiPageMapPosition;
//            endPage.offset = uiPageOffset;
//            finishAllocation();
//            return (startPage.index * 4 * 8) + startPage.offset;
//          }
//				}
//        else
//          count = noOfPages;
//				uiPageMapEntry >>= 1;
//			}
//		}
//	}
//  throw upan::exception(XLOC, "Out of memory pages!");
//}
//
//void MemManager::DeAllocatePhysicalPage(uint32_t pageNo, const uint32_t noOfPages)
//{
//  ProcessSwitchLock pLock;
//  for(uint32_t count = 0; count < noOfPages; ++count, ++pageNo)
//  {
//    const uint32_t index = pageNo / (8 * 4);
//    const uint32_t offset = pageNo % (8 * 4);
//    m_uiPageMap[index] = m_uiPageMap[index] & ~(0x1 << offset) ;
//  }
//}

extern __volatile__ int SYS_CALL_ID;
extern __volatile__ int KERNEL_DMM_ON;

ReturnCode MemManager::AllocatePage(int iProcessID, unsigned uiFaultyAddress) {
  upan::mutex_guard g(ProcessManager::Instance().GetSchedulableProcess(iProcessID).value().pageAllocMutex().value());

  unsigned uiFreePageNo, uiVirtualPageNo ;
	unsigned uiPDEAddress, uiPTEAddress, uiPTEFreePage ;

	//KC::MDisplay().Address("\n Addr: ", uiFaultyAddress) ; 

	uiVirtualPageNo = uiFaultyAddress / PAGE_SIZE ;

	if(ProcessManager::Instance().IsKernelProcess(iProcessID))
	{
		printf("\n Page Fault in Kernel! FIX THIS !!!") ;
		printf("\n Page Fault Address/Page: %x / %u", uiFaultyAddress, uiVirtualPageNo);
		__asm__ __volatile__ ("HLT");
		while(1);
		m_uiPTEBase[uiVirtualPageNo] = ((uiFreePageNo * PAGE_SIZE) & 0xFFFFF000) | 0x3 ;
	}
	else
	{
		//For User Process a Page Fault can occur only while accessing the HEAP AREA which starts at Virtual Address
		//2 GB = 0x80000000
		
		/* Not accessing Heap and Not the startUp Address access (20MB) in proc_init */
		const uint32_t pdeIndex = ((uiFaultyAddress >> 22) & 0x3FF);
		if (pdeIndex != PROCESS_STACK_PDE_ID || pdeIndex != PROCESS_GUI_FRAMEBUFFER_PDE_ID || ProcessManager::Instance().IsDMMOn(iProcessID)) {
      if ((uiFaultyAddress < PROCESS_HEAP_START_ADDRESS)
          || (uiFaultyAddress >= PROCESS_HEAP_START_ADDRESS && !ProcessManager::Instance().IsDMMOn(iProcessID))
          //This space is for process Stack - page fault here should be only while expanding stack and not for Heap
          || (pdeIndex == PROCESS_STACK_PDE_ID && ProcessManager::Instance().IsDMMOn(iProcessID))
          //This space is for process gui framebuffer - this space must be pre-allocated
          || (pdeIndex == PROCESS_GUI_FRAMEBUFFER_PDE_ID)) {
        printf("\n Segmentation Fault @ Address: 0x%x", uiFaultyAddress);
        printf("\n Sys Call Id: %d", SYS_CALL_ID);
        printf("\n PID: %d, DMM Flag: %d, PDE Index: %d", iProcessID, ProcessManager::Instance().IsDMMOn(iProcessID),
               pdeIndex);
        return Failure;
      }
    }

		uiPDEAddress = ProcessManager::Instance().GetSchedulableProcess(iProcessID).value().taskState().CR3_PDBR ;

		uiPTEAddress = (((unsigned*)(uiPDEAddress - GLOBAL_DATA_SEGMENT_BASE))[ ((uiFaultyAddress >> 22) & 0x3FF) ]) ;

		if((uiPTEAddress & 0x1) == 0x0) {
			uiPTEFreePage = AllocatePhysicalPage();

			((unsigned*)(uiPDEAddress - GLOBAL_DATA_SEGMENT_BASE))[ ((uiFaultyAddress >> 22) & 0x3FF)] = 
				((uiPTEFreePage * PAGE_SIZE) & 0xFFFFF000) | 0x7 ;

			uiPTEAddress = uiPTEFreePage * PAGE_SIZE ;
			InitPage(uiPTEFreePage) ;
		} else if((uiPTEAddress & 0x7) == 0x7) {
			uiPTEAddress = uiPTEAddress & 0xFFFFF000;
		} else {
      /* Crash the Process..... With SegFault Or OutOfMemeory Error*/
      printf("\n Segmentation/Permission Fault @ Address: %x, PDE Index: %u", uiFaultyAddress, pdeIndex);
      return Failure;
    }

		unsigned uiPageAdress = ((unsigned*)(uiPTEAddress - GLOBAL_DATA_SEGMENT_BASE))[((uiFaultyAddress >> 12) & 0x3FF)];

		if((uiPageAdress & 0x01) == 0x00)	{
			uiFreePageNo = AllocatePhysicalPage();

			((unsigned*)(uiPTEAddress - GLOBAL_DATA_SEGMENT_BASE))[ ((uiFaultyAddress >> 12) & 0x3FF) ] = 
					((uiFreePageNo * PAGE_SIZE) & 0xFFFFF000) | 0x7 ;

			InitPage(uiFreePageNo) ;
		} else if ((uiPageAdress & 0x7) == 0x7) {
      // we are good - page is already allocated - possibly because of a page fault on same address/page area from another thread.
		} else {
			/* Crash the Process..... With SegFault Or OutOfMemeory Error*/
			printf("\n Segmentation/Permission Fault @ Address: %x, PDE Index: %u", uiFaultyAddress, pdeIndex);
			return Failure;
		}
	}

	//Mem_FlushTLB();
//	KC::MDisplay().Address("\n Alloc Done: ", uiFaultyAddress) ;
	return Success;
}

ReturnCode MemManager::DeAllocatePage(const unsigned uiAddress) {
	unsigned uiFreePageNo, uiVirtualPageNo ;

	uiVirtualPageNo = uiAddress / PAGE_SIZE ;

	if((m_uiPTEBase[uiVirtualPageNo] & 0x1) == 0x0)
		return DupDealloc;
	
	uiFreePageNo = (m_uiPTEBase[uiVirtualPageNo] & 0xFFFFF000) / PAGE_SIZE ;
	m_uiPTEBase[uiVirtualPageNo] &= 0x2 ;
	DeAllocatePhysicalPage(uiFreePageNo) ;

//	KC::MDisplay().Address("\n DeAllocate Virtual: Address = ", uiAddress) ;
//	KC::MDisplay().Address(" : Page = ", uiVirtualPageNo) ;
//	KC::MDisplay().Address("\n DeAllocated Real Page = ", uiFreePageNo) ;

	return Success;
}

//void MemManager::PageFaultHandlerTask()
//{
//	/*	What is This...... ;) while(1) in a Exception Handler... 
//		This Exception Handler is Called Via Task Gate in IDT... So After the First Call
//		the EIP will be pointing to the Next Instruction of IRET..... U Got it,,, right
//		Yes.... No one there to update the TSS of this Task Gate.... Hence a tweak which works... */
//	
//	while(1) 
//	{
//		SPECIAL_TASK = true ;
//
//		__volatile__ unsigned uiFaultyAddress ;
//		__asm__ __volatile__("mov %%cr2, %0" : "=r"(uiFaultyAddress) : ) ;
//		
//		if(AllocatePage(ProcessManager_iCurrentProcessID, uiFaultyAddress) == Failure)
//		{
//			//TODO:
//			ProcessManager::Instance().GetCurrentPAS().status = TERMINATE ;
//			__asm__ __volatile__("HLT") ;
//		}
//		SPECIAL_TASK = false ;
//		__asm__ __volatile__("IRET") ;
//	}
//}

void MemManager::DisplayNoOfFreePages()
{	
	unsigned uiPageMapPosition ;
	unsigned uiPageOffset ;
	unsigned uiPageMapEntry ;
	unsigned uiFreePageCount = 0 ;
	
	for(uiPageMapPosition = m_uiResvSize; uiPageMapPosition < m_uiPageMapSize; uiPageMapPosition++)
	{
		if((m_uiPageMap[uiPageMapPosition] & 0xFFFFFFFF) != 0xFFFFFFFF)
		{
			uiPageMapEntry = m_uiPageMap[uiPageMapPosition] ;
			for(uiPageOffset = 0; uiPageOffset < 32; uiPageOffset++)
			{
				if((uiPageMapEntry & 0x1) == 0x0)
					uiFreePageCount++ ;
				uiPageMapEntry >>= 1 ;
			}
		}
	}

	printf("\n Free Page Count = %d", uiFreePageCount);
}

unsigned MemManager::GetFlatAddress(unsigned uiVirtualAddress)
{
	unsigned uiPDEAddress = ProcessManager::Instance().GetCurrentPAS().pdbr();

	unsigned uiPTEAddress = (((unsigned*)(uiPDEAddress - GLOBAL_DATA_SEGMENT_BASE))[((uiVirtualAddress >> 22) & 0x3FF)]) ;

	if((uiPTEAddress & 0x1) == 0x0)
		return NULL ;

	uiPTEAddress = uiPTEAddress & 0xFFFFF000 ;
		
	unsigned uiPageAddress = ((unsigned*)(uiPTEAddress - GLOBAL_DATA_SEGMENT_BASE))[((uiVirtualAddress >> 12) & 0x3FF)] ;

	if((uiPageAddress & 0x1) == 0x0)
		return NULL ;	

	uiPageAddress = uiPageAddress & 0xFFFFF000 ;

	return uiPageAddress + (uiVirtualAddress & 0xFFF) ;
}

void MemManager::DisplayNoOfAllocPages()
{	
	unsigned uiPageMapPosition ;
	unsigned uiPageOffset ;
	unsigned uiPageMapEntry ;
	unsigned uiAllocPageCount = 0 ;

  KC::MConsole().Message("\n\n", ' ');
	for(uiPageMapPosition = m_uiResvSize + m_uiKernelHeapSize; uiPageMapPosition < m_uiPageMapSize; uiPageMapPosition++)
	{
		//if((m_uiPageMap[uiPageMapPosition] & 0xFFFFFFFF) != 0xFFFFFFFF)
		{
			uiPageMapEntry = m_uiPageMap[uiPageMapPosition] ;
			for(uiPageOffset = 0; uiPageOffset < 32; uiPageOffset++)
			{
				if((uiPageMapEntry & 0x1) == 0x1)
				{
					uiAllocPageCount++ ;
					printf(",%u", (uiPageMapPosition * 4 * 8) + uiPageOffset);
				}
				uiPageMapEntry >>= 1 ;
			}
		}
	}
	
	printf("\n Alloc Page Count = %u\n", uiAllocPageCount) ;
}

