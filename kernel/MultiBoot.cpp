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
#include <MultiBoot.h>
#include <PortCom.h>
#include <MemManager.h>
#include <GraphicsVideo.h>
#include <RootGUIConsole.h>

MultiBoot::MultiBoot() : _bootDevId(0), _bootPartitionId(0), _ramSize(0),
                        _acpi_mmap(nullptr), _mmap_size(0), _hasFrameBufferInfo(false), _rawFrameBufferAddress(0) {
  if (MULTIBOOT2_BOOTLOADER_MAGIC_VAL != MULTIBOOT2_BOOTLOADER_MAGIC) {
    // Unsupported boot loader -> Only multiboot2 complaint boot loading is supported.
    while (true);
  }

  const uint32_t size = ((uint32_t *) MULTIBOOT2_INFO_ADDR)[0];
  for (auto tag = (multiboot_tag *) (MULTIBOOT2_INFO_ADDR + 8);
       tag->type != MULTIBOOT_TAG_TYPE_END;
       tag = (multiboot_tag *) ((uint8_t *) tag + ((tag->size + 7) & ~7))) {
    switch (tag->type) {
      case MULTIBOOT_TAG_TYPE_BOOTDEV: {
        _bootDevId = ((multiboot_tag_bootdev *) tag)->biosdev;
        _bootPartitionId = ((multiboot_tag_bootdev *) tag)->part;
      }
      break;

      case MULTIBOOT_TAG_TYPE_MMAP: {
        int i = 0;
        auto mmap_tag = (multiboot_tag_mmap *) tag;
        for (auto mmap = mmap_tag->entries; (uint8_t *) mmap < (uint8_t *) tag + tag->size;
             mmap = (multiboot_mmap_entry *) ((uintptr_t) mmap + mmap_tag->entry_size)) {
          if (i >= MAX_MMAP_ENTRIES) {
            //PANIC
            while (true);
          }
          _mmap[i].addr = mmap->addr;
          _mmap[i].length = mmap->length;
          _mmap[i].type = mmap->type;
          _ramSize += mmap->length;
          if (mmap->type == MULTIBOOT_MEMORY_ACPI_RECLAIMABLE) {
            _acpi_mmap = &_mmap[i];
          }
          ++i;
        }
        _mmap_size = i;
      }
      break;

      case MULTIBOOT_TAG_TYPE_FRAMEBUFFER: {
        auto fb_tag = (multiboot_tag_framebuffer *) tag;
        _rawFrameBufferAddress = fb_tag->common.framebuffer_addr;
        _framebufferInfo._frameBuffer = (uint32_t *)_rawFrameBufferAddress;
        _framebufferInfo._pitch = fb_tag->common.framebuffer_pitch;
        _framebufferInfo._width = fb_tag->common.framebuffer_width;
        _framebufferInfo._height = fb_tag->common.framebuffer_height;
        _framebufferInfo._bpp = fb_tag->common.framebuffer_bpp;
        _hasFrameBufferInfo = true;
      }
      break;
    }
  }

  if (_hasFrameBufferInfo) {
    InitializeGraphicsPageMap(-1);
  }
}

void MultiBoot::InitializeGraphicsPageMap(int memTypeFlag) {
  const uint32_t lfbSize = _framebufferInfo._width * _framebufferInfo._height * _framebufferInfo._bpp / 8;
  const uint32_t noOfPages = ((lfbSize - 1) / PAGE_SIZE) + 1;
  const uint32_t availablePages = MEM_GRAPHICS_VIDEO_MAP_SIZE / PAGE_SIZE;
  if (noOfPages > availablePages) {
    if (memTypeFlag >= 0) {
      printf("\n Insufficient graphics video buffer. Required pages: %u", noOfPages);
    }
    //PANIC
    while (true);
  }
  uint64_t lfbaddress = _rawFrameBufferAddress;
  uint64_t mapAddress = MEM_GRAPHICS_VIDEO_MAP_START;

  const uint16_t wcFlag = memTypeFlag >= 0 ? memTypeFlag & 0xFF : 0;
  const uint32_t pageFlag = 0x3 | (wcFlag & 0xFF);

  for (unsigned i = 0; i < noOfPages; ++i) {
    const uint64_t addr = lfbaddress + PAGE_SIZE * i;
    MemManager::KernelPageTableMmap(mapAddress, addr, pageFlag);
    mapAddress += PAGE_SIZE;
  }
  Mem_FlushTLB();

  if (memTypeFlag >= 0) {
    GraphicsVideo::Instance().MappedLFBAddress(MEM_GRAPHICS_VIDEO_MAP_START);
    RootGUIConsole::Instance().resetFrameBuffer(MEM_GRAPHICS_VIDEO_MAP_START);
  } else {
    _framebufferInfo._frameBuffer = (uint32_t*)MEM_GRAPHICS_VIDEO_MAP_START;
  }
}

void MultiBoot::Print() {
  char buffer[256];
  upan::string msg;

  for (int i = 0; i < _mmap_size; ++i) {
    sprintf(buffer, "\n%d) Address: %llu, Length: %llu, Type: %u", i + 1, _mmap[i].addr, _mmap[i].length, _mmap[i].type);
    msg += buffer;
  }
  sprintf(buffer, "\n Total RAM SIZE: %llu", _ramSize);
  msg += buffer;

  sprintf(buffer, "\n FRAMEBUFFER_ADDR: 0x%x", _framebufferInfo._frameBuffer);
  msg += buffer;
  sprintf(buffer, "\n FRAMEBUFFER_PITCH: %u", _framebufferInfo._pitch);
  msg += buffer;
  sprintf(buffer, "\n FRAMEBUFFER_WIDTH: %u", _framebufferInfo._width);
  msg += buffer;
  sprintf(buffer, "\n FRAMEBUFFER_HEIGHT: %u", _framebufferInfo._height);
  msg += buffer;
  sprintf(buffer, "\n FRAMEBUFFER_BPP: %u", _framebufferInfo._bpp);
  msg += buffer;

  COM1::Instance().Write(msg);
  printf("%s", msg.c_str());
}
