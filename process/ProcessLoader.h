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
#ifndef _PROCESS_LOADER_H_
#define _PROCESS_LOADER_H_

#include <Global.h>
#include <ElfSectionHeader.h>
#include <BufferedReader.h>

#define __PROCESS_DLL_FILE		".dll"

uint64_t ProcessLoader_GetCeilAlignedAddress(uint64_t uiAddress, unsigned uiAlign) ;
unsigned ProcessLoader_GetFloorAlignedAddress(unsigned uiAddress, unsigned uiAlign) ;

class ProcessLoader
{
  private:
    ProcessLoader();
  public:
    static ProcessLoader& Instance()
    {
      static ProcessLoader instance;
      return instance;
    }
    byte* LoadDLLInitSection(unsigned& uiSectionSize);
  private:
    byte* LoadInitSection(unsigned& uiSectionSize, const upan::string& szSectionName);

    const upan::string PROCESS_DLL_FILE;
};

#endif
