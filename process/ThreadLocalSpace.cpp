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
#include <ThreadLocalSpace.h>

ThreadLocalSpace::~ThreadLocalSpace() {
  for(auto& i : _dtv) {
    delete i.init_image;
  }
}

TLSInfo ThreadLocalSpace::add(int totalLen, int initLen, const uint8_t* initImage) {
  dtv_entry dtvEntry;
  dtvEntry.total_len = totalLen;
  dtvEntry.init_len = initLen;
  dtvEntry.init_image = nullptr;

  if (dtvEntry.init_len) {
    dtvEntry.init_image = new uint8_t[dtvEntry.init_len];
    memcpy(dtvEntry.init_image, initImage, dtvEntry.init_len);
  }

  _dtv.push_back(dtvEntry);

  uint64_t offset = 0;
  for (const auto& dtv : _dtv) {
    offset += dtv.total_len;
  }

  return { _dtv.size(), offset };
}