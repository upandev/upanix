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

#include <mosstd.h>

extern "C" {
  extern uint16_t SYS_CODE_SELECTOR;
  extern uint16_t SYS_DATA_SELECTOR;
  extern uint16_t USER_CODE_SELECTOR;
  extern uint16_t USER_DATA_SELECTOR;
  extern uint32_t SYS_TSS_SELECTOR;

  extern uintptr_t IDT_BASE_ADDR;
  extern uintptr_t TSS_BASE_ADDR;
}

#define PAGE_SIZE 4096u // 4 KB
#define ENTRIES_PER_PAGE_TABLE 512

constexpr uint64_t MAX_PROCESS_SPACE_SIZE = 8 MB;

#define MEM_DMA_START			0x20000
#define MEM_DMA_FLOPPY_START	0x20000
#define MEM_DMA_FLOPPY_END		0x30000 // 65536 -> 64 KB
#define MEM_DMA_END				0x30000 

#define MEM_VIDEO_START		0xB8000 // 753664
#define MEM_VIDEO_END		0xB9400 // 0xB8000 + 0x1400 -> 5120 -> 5 KB

constexpr uint64_t MMAP_APIC_BASE = 72 MB;
constexpr uint64_t MMAP_IOAPIC_BASE = MMAP_APIC_BASE + PAGE_SIZE;

constexpr uint64_t EHCI_MMIO_BASE_ADDR = 80 MB;
constexpr uint64_t EHCI_MMIO_BASE_END = EHCI_MMIO_BASE_ADDR + 48 * PAGE_SIZE;

constexpr uint64_t XHCI_MMIO_BASE_ADDR = EHCI_MMIO_BASE_END;
constexpr uint64_t XHCI_MMIO_BASE_ADDR_END = XHCI_MMIO_BASE_ADDR + 64 * PAGE_SIZE;

constexpr uint64_t NET_ATH9K_MMIO_BASE_ADDR = 81 MB;
constexpr uint64_t NET_ATH9K_MMIO_BASE_ADDR_END = NET_ATH9K_MMIO_BASE_ADDR + 64 * PAGE_SIZE;
constexpr uint64_t NET_E1000_MMIO_BASE_ADDR = NET_ATH9K_MMIO_BASE_ADDR_END;
constexpr uint64_t NET_E1000_MMIO_BASE_ADDR_END = NET_ATH9K_MMIO_BASE_ADDR_END + 64 * PAGE_SIZE;

constexpr uintptr_t MEM_KERNEL_STACK_TOP = 32 MB; //this should match the one defined in startup .asm file KERNEL_STACK_TOP

extern uint64_t* const MEM_PML4_TABLE;
extern const uint32_t MEM_PML4_SIZE;

extern uint64_t* const MEM_PDP_TABLE;
extern const uint32_t MEM_PDP_SIZE;

extern uint64_t* const MEM_PD_TABLE;
extern const uint32_t MEM_PD_SIZE;

extern uint64_t* const MEM_PT_TABLE;
extern const uint32_t MEM_PT_SIZE;

extern const uint32_t MEM_INIT_PAGE_MAP_SIZE;

constexpr uint64_t PAGE_TABLE_END = 0x4012000; // (uint64_t)MEM_PT_TABLE + MEM_PT_SIZE * sizeof(uint64_t);

constexpr uint64_t MEM_PAGE_MAP_START = PAGE_TABLE_END;
constexpr uint64_t MEM_PAGE_MAP_END = MEM_PAGE_MAP_START + 128 KB;

/* kernel page heap/pool */
constexpr uint64_t MEM_KERNEL_PAGE_POOL_MAP_START = MEM_PAGE_MAP_END;
constexpr uint64_t MEM_KERNEL_PAGE_POOL_MAP_END =  MEM_KERNEL_PAGE_POOL_MAP_START + 8 KB;

constexpr uint32_t MEM_GRAPHICS_VIDEO_MAP_SIZE = 16 MB;

constexpr uint64_t MEM_GRAPHICS_TEXT_BUFFER_START = MEM_KERNEL_PAGE_POOL_MAP_END;
constexpr uint64_t MEM_GRAPHICS_TEXT_BUFFER_END = MEM_GRAPHICS_TEXT_BUFFER_START + 50 KB;

constexpr uint64_t MEM_GRAPHICS_VIDEO_MAP_START = 96 MB;
constexpr uint64_t MEM_GRAPHICS_Z_BUFFER_START = MEM_GRAPHICS_VIDEO_MAP_START + MEM_GRAPHICS_VIDEO_MAP_SIZE;

constexpr uint64_t MEM_KERNEL_HEAP_START = MEM_GRAPHICS_Z_BUFFER_START + MEM_GRAPHICS_VIDEO_MAP_SIZE;
constexpr uint32_t MEM_KERNEL_HEAP_SIZE = 48 MB;

constexpr int MAX_PROCESSOR_COUNT = 64; // if you increase this, then the reserved space below for GDT, TSS and STACK must be expanded accordingly

//512 bytes per TSS per CPU = 32 KB
constexpr int MAX_KERNEL_TSS_SIZE = 512;
constexpr int MAX_KERNEL_TSS_BLOCK_SIZE = MAX_KERNEL_TSS_SIZE * MAX_PROCESSOR_COUNT;

//32 KB per stack region per CPU = 2 MB per stack region
constexpr int MAX_KERNEL_STACK_SIZE = 32 KB;
constexpr int MAX_KERNEL_STACK_BLOCK_SIZE = MAX_KERNEL_STACK_SIZE * MAX_PROCESSOR_COUNT;

constexpr uint64_t MEM_KERNEL_TSS_START = MEM_KERNEL_HEAP_START + MEM_KERNEL_HEAP_SIZE;
constexpr uint64_t MEM_KERNEL_RING0_STACK_TOP = MEM_KERNEL_TSS_START + MAX_KERNEL_TSS_BLOCK_SIZE + MAX_KERNEL_STACK_BLOCK_SIZE;
constexpr uint64_t MEM_KERNEL_IST1_TIMER_STACK_TOP = MEM_KERNEL_RING0_STACK_TOP + MAX_KERNEL_STACK_BLOCK_SIZE;
constexpr uint64_t MEM_KERNEL_IST2_PAGE_FAULT_STACK_TOP = MEM_KERNEL_IST1_TIMER_STACK_TOP + MAX_KERNEL_STACK_BLOCK_SIZE;
constexpr uint64_t MEM_KERNEL_IST3_XHCI_STACK_TOP = MEM_KERNEL_IST2_PAGE_FAULT_STACK_TOP + MAX_KERNEL_STACK_BLOCK_SIZE;
constexpr uint64_t MEM_KERNEL_IST4_COMMON_STACK_TOP = MEM_KERNEL_IST3_XHCI_STACK_TOP + MAX_KERNEL_STACK_BLOCK_SIZE;
constexpr uint64_t MEM_KERNEL_PAGE_POOL_START = MEM_KERNEL_IST4_COMMON_STACK_TOP;
constexpr uint32_t MEM_KERNEL_PAGE_POOL_SIZE = 16 MB;
constexpr uint32_t MEM_KERNEL_RESV_SIZE = MEM_KERNEL_PAGE_POOL_START + MEM_KERNEL_PAGE_POOL_SIZE;

constexpr uint64_t PAGE_CONFIG_MASK = 0x1FF;
constexpr uint64_t PAGE_MASK = ~PAGE_CONFIG_MASK;

#define PML4_INDEX(ADDR) (((ADDR) >> 39) & PAGE_CONFIG_MASK)
#define PDP_INDEX(ADDR) (((ADDR) >> 30) & PAGE_CONFIG_MASK)
#define PD_INDEX(ADDR) (((ADDR) >> 21) & PAGE_CONFIG_MASK)
#define PT_INDEX(ADDR) (((ADDR) >> 12) & PAGE_CONFIG_MASK)
#define PAGE_INDEX(ADDR) ((ADDR) & 0xFFF)
#define PAGE_OFFSET(ADDR) ((ADDR) % PAGE_SIZE)

#define PAGE_IS_PRESENT(TABLE, INDEX) ((TABLE)[INDEX] & 0x1)
#define PAGE_ADDRESS(TABLE, INDEX) ((TABLE)[INDEX] & PAGE_MASK)
#define PAGE_NUMBER(TABLE, INDEX) (((TABLE)[INDEX] & PAGE_MASK) / PAGE_SIZE)
#define PAGE_TABLE(TABLE, INDEX) (uint64_t*)PAGE_ADDRESS(TABLE, INDEX)

