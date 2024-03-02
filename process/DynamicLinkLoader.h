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

#include <ProcessManager.h>
#include <KernelService.h>

bool DynamicLinkLoader_GetSymbolOffset(const char* szJustDLLName, const char* szSymName, uint64_t* uiDynSymOffset, Process* processAddressSpace) ;
void DynamicLinkLoader_DoRelocation(Process* processAddressSpace, int64_t iID, uint64_t relocationOffset, uint64_t *dynamicSymAddress) ;

class DynamicLinkLoader {
private:
  DynamicLinkLoader();
public:
  static DynamicLinkLoader& Instance() {
    static DynamicLinkLoader instance;
    return instance;
  }

  uint32_t dllResolverSize() {
    return _dll_resolver_size;
  }

  uint8_t* dllResolverProgBits() {
    return _dll_resolver;
  }

private:
  uint8_t* _dll_resolver;
  uint32_t _dll_resolver_size;
};