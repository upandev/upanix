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
extern const uint64_t PROCESS_STACK_TOP_ADDRESS;
constexpr uint64_t PROCESS_CG_STACK_SIZE = 8 * PAGE_SIZE;
constexpr uint64_t PROCESS_INIT_STACK_SIZE = PAGE_SIZE;
constexpr uint64_t PROCESS_STACK_SIZE = 4 MB;

constexpr uint64_t PROCESS_SPACE_FOR_OS = MEM_KERNEL_RESV_SIZE;
constexpr uint64_t PROCESS_BASE = PROCESS_SPACE_FOR_OS;

//Each process gets a max of 4MB for graphics UI framebuffer
constexpr uintptr_t PROCESS_GUI_FRAMEBUFFER_ADDRESS = 510 GB;
constexpr uint64_t PROCESS_GUI_FRAMEBUFFER_SIZE = 4 MB;

//Each process gets a max of 2GB heap
constexpr uintptr_t PROCESS_HEAP_START_ADDRESS = 508 GB;
constexpr uint64_t PROCESS_HEAP_SIZE = 2 GB;

constexpr uintptr_t PROCESS_DLL_START_ADDRESS = 8 GB;

constexpr uintptr_t PROCESS_KERNEL_STACK_BASE = 511 GB;
constexpr uint32_t PROCESS_KERNEL_STACK_SIZE = 32 KB;

#define NO_PROCESS_ID -1
