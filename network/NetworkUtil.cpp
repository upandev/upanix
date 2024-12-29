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

#include <NetworkUtil.h>

uint16_t NetworkUtil::CalculateChecksum(const uint16_t* buf, uint32_t lengthInBytes, uint32_t initSum) {
  uint32_t sum = CalculatePartialChecksum(buf, lengthInBytes, initSum);
  while((sum >> 16)) {
    sum = (sum & 0xFFFF) + (sum >> 16);
  }
  return (uint16_t)~sum;
}

uint32_t NetworkUtil::CalculatePartialChecksum(const uint16_t* buf, uint32_t lengthInBytes, uint32_t initSum) {
  const uint32_t lengthInWords = lengthInBytes / 2;
  uint32_t sum = initSum;
  for(uint32_t i = 0; i < lengthInWords; ++i) {
    sum += buf[i];
  }
  if (lengthInBytes % 2) {
    sum += ((uint8_t*)buf)[lengthInBytes - 1];
  }
  return sum;
}