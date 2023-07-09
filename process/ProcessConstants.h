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

#define MAX_NO_PROCESS 3500

#define PROCESS_KERNEL_STACK_PAGES 8
#define PROCESS_CG_STACK_PAGES 8
#define NO_OF_PAGES_FOR_STARTUP_ARGS 1

#define PROCESS_SPACE_FOR_OS (MEM_KERNEL_RESV_SIZE / 4 / 1024 / 1024)
#define PROCESS_BASE (PROCESS_SPACE_FOR_OS * PAGE_TABLE_ENTRIES * PAGE_SIZE)

//Each process gets a max of 4MB (i.e. one PDE entry a.k.a one whole PTE) for graphics UI framebuffer
#define PROCESS_GUI_FRAMEBUFFER_PDE_ID 1021u
#define PROCESS_GUI_FRAMEBUFFER_ADDRESS (PROCESS_GUI_FRAMEBUFFER_PDE_ID * PAGE_TABLE_ENTRIES * PAGE_SIZE)

//Each process gets 4MB (i.e. one PDE entry a.k.a one whole PTE) of stack that includes PROCESS_CG_STACK_PAGES
#define PROCESS_STACK_PDE_ID 1022u
#define PROCESS_STACK_TOP_ADDRESS ((PROCESS_STACK_PDE_ID + 1) * PAGE_SIZE * PAGE_TABLE_ENTRIES)

#define NO_PROCESS_ID -1
