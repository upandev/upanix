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

#include <stdlib.h>
#include <string.h>
#include <DMM.h>
#include <RawNetPacket.h>

RawNetPacket::RawNetPacket() : _buf(nullptr), _len(0) {
}

RawNetPacket::RawNetPacket(const uintptr_t addr, const uint32_t len) :
  _buf(new uint8_t[len]), _len(len) {
  memcpy(_buf, (uint8_t*)addr, _len);
}

RawNetPacket::~RawNetPacket() {
  if (_buf) {
    delete[] _buf;
    _buf = nullptr;
  }
}

RawNetPacket::RawNetPacket(const RawNetPacket& o) {
  assign(o);
}

RawNetPacket& RawNetPacket::operator=(const RawNetPacket& o) {
  assign(o);
  return *this;
}

void RawNetPacket::assign(const RawNetPacket& o) {
  this->_buf = o._buf;
  this->_len = o._len;
  const_cast<RawNetPacket&>(o)._buf = nullptr;
}
