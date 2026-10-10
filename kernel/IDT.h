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

class IDT {
  public:
    static constexpr int MAX_IDT_ENTRIES = 256;

	private:
		explicit IDT();

	public:
		static IDT& Instance() {
			static IDT instance;
			return instance;
		}

	private:
		void LoadHandlers() ;
		void LoadEntry(uint32_t idtNo, uintptr_t offset, uint16_t selector, uint8_t options);

		typedef struct {
			uint16_t _lowerOffset;
			uint16_t _selector;

			uint8_t _ist:3;
      uint8_t _reserved1:5;
      uint8_t _options; //present(1 bit), dpl(2 bits), 0, gate-type(4 bits)

      uint16_t _midOffset;
      uint32_t _higherOffset;
      uint32_t _reserved;
		} PACKED IDTEntry;

    friend class IrqManager;
    friend class Processor;
};