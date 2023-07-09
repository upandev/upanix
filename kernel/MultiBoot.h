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
#include <stdio.h>

/**** These Multi boot structures are taken from GRUB multiboot2.h ****/

#define MULTIBOOT2_BOOTLOADER_MAGIC 0x36D76289

#define MULTIBOOT_TAG_TYPE_END 0
#define MULTIBOOT_TAG_TYPE_BOOTDEV 5
#define MULTIBOOT_TAG_TYPE_MMAP 6
#define MULTIBOOT_TAG_TYPE_FRAMEBUFFER 8

extern uintptr_t MULTIBOOT2_INFO_ADDR;
extern uint32_t MULTIBOOT2_BOOTLOADER_MAGIC_VAL;

typedef struct {
  uint32_t type;
  uint32_t size;
} PACKED multiboot_tag;

typedef struct {
  uint32_t type;
  uint32_t size;
  uint32_t biosdev;
  uint32_t slice;
  uint32_t part;
} PACKED multiboot_tag_bootdev;

//mmap
typedef struct {
  uint64_t addr;
  uint64_t length;
#define MULTIBOOT_MEMORY_AVAILABLE              1
#define MULTIBOOT_MEMORY_RESERVED               2
#define MULTIBOOT_MEMORY_ACPI_RECLAIMABLE       3
#define MULTIBOOT_MEMORY_NVS                    4
#define MULTIBOOT_MEMORY_BADRAM                 5
  uint32_t type;
  uint32_t zero;
} PACKED multiboot_mmap_entry;

typedef struct {
  uint32_t type;
  uint32_t size;
  uint32_t entry_size;
  uint32_t entry_version;
  multiboot_mmap_entry entries[0];
} PACKED multiboot_tag_mmap;

typedef struct {
  uint8_t red;
  uint8_t green;
  uint8_t blue;
} PACKED multiboot_color;

typedef struct {
  uint32_t type;
  uint32_t size;

  uint64_t framebuffer_addr;
  uint32_t framebuffer_pitch;
  uint32_t framebuffer_width;
  uint32_t framebuffer_height;
  uint8_t framebuffer_bpp;
#define MULTIBOOT_FRAMEBUFFER_TYPE_INDEXED 0
#define MULTIBOOT_FRAMEBUFFER_TYPE_RGB 1
#define MULTIBOOT_FRAMEBUFFER_TYPE_EGA_TEXT 2
  uint8_t framebuffer_type;
  uint16_t reserved;
} PACKED multiboot_tag_framebuffer_common;

typedef struct {
  multiboot_tag_framebuffer_common common;

  union {
    struct {
      uint16_t framebuffer_palette_num_colors;
      multiboot_color framebuffer_palette[0];
    } PACKED;
    struct {
      uint8_t framebuffer_red_field_position;
      uint8_t framebuffer_red_mask_size;
      uint8_t framebuffer_green_field_position;
      uint8_t framebuffer_green_mask_size;
      uint8_t framebuffer_blue_field_position;
      uint8_t framebuffer_blue_mask_size;
    } PACKED;
  };
} PACKED multiboot_tag_framebuffer;

class MultiBoot {
	private:
		MultiBoot();
	public:
		static MultiBoot& Instance() {
			static MultiBoot instance;
			return instance;
		}

		uint64_t GetRamSize() const { return _ramSize; }
		uint32_t GetBootDeviceID() const { return _bootDevId; }
    uint32_t GetBootPartitionID() const { return _bootPartitionId; }
    const FrameBufferInfo* VideoFrameBufferInfo() const {
		  return _hasFrameBufferInfo ? &_framebufferInfo : nullptr;
		}
    const multiboot_mmap_entry* GetACPIInfoMemMap() const {
      return _acpi_mmap;
    }
    void Print();
	private:
    void InitializeGraphicsPageMap();
    static const int MAX_MMAP_ENTRIES = 64;

    uint32_t _bootDevId;
    uint32_t _bootPartitionId;
    uint64_t _ramSize;
    multiboot_mmap_entry* _acpi_mmap;
    multiboot_mmap_entry _mmap[MAX_MMAP_ENTRIES];
    uint32_t _mmap_size;
    bool _hasFrameBufferInfo;
    FrameBufferInfo _framebufferInfo;
};
