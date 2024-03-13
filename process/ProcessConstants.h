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

#include <MemConstants.h>

#define MAX_NO_PROCESS 3500

//4MB stack starting (backwards) at 512 GB
constexpr uint64_t PROCESS_STACK_TOP_ADDRESS = 512 GB;
constexpr uint64_t PROCESS_SYSCALL_STACK_SIZE = 8 * PAGE_SIZE;
constexpr uint64_t PROCESS_INIT_STACK_SIZE = PAGE_SIZE;
constexpr uint64_t PROCESS_STACK_SIZE = 4 MB;

//The first PDP entry - which is of size 1GB is reserved for Kernel.
//The user process is expected to be loaded at address 1GB or higher
constexpr uint64_t USER_PROCESS_MIN_LOAD_ADDRESS = 1 GB;

//Each process gets a max of 4MB for graphics UI framebuffer
constexpr uintptr_t PROCESS_GUI_FRAMEBUFFER_ADDRESS = 510 GB;
constexpr uint64_t PROCESS_GUI_FRAMEBUFFER_SIZE = 4 MB;

//Each process gets a max of 2GB heap
constexpr uintptr_t PROCESS_HEAP_START_ADDRESS = 508 GB;
constexpr uint64_t PROCESS_HEAP_SIZE = 2 GB;

constexpr uintptr_t PROCESS_DLL_START_ADDRESS = 8 GB;

constexpr uintptr_t PROCESS_KERNEL_STACK_BASE = 511 GB;
constexpr uint32_t PROCESS_KERNEL_STACK_SIZE = 32 KB;
constexpr uint32_t MEM_KERNEL_STACK_POOL_SIZE = 4 MB;
constexpr uint32_t NO_OF_KERNEL_STACK_BLOCKS = (MEM_KERNEL_STACK_POOL_SIZE) / (PROCESS_KERNEL_STACK_SIZE);

constexpr uint32_t PROCESS_ARGUMENTS_ON_REGS_X86_64 = 6;

#define NO_PROCESS_ID -1
