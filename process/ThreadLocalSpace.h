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

#include <vector.h>
#include <pair.h>

class ThreadLocalSpace {
public:
  ThreadLocalSpace() = default;
  ~ThreadLocalSpace();

  uint64_t add(int totalLen, int initLen, const uint8_t* initImage);

  typedef struct {
    int total_len;
    int init_len;
    uint8_t* init_image;
  } dtv_entry;

  typedef upan::vector<dtv_entry> DTV_LIST;
  const DTV_LIST& getDTV() const { return _dtv; }

private:
  DTV_LIST _dtv;
};