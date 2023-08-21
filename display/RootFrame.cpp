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

#include <RootFrame.h>
#include <DMM.h>
#include <string.h>

void RootFrame::enableDoubleBuffer(bool enable) {
  if (!enable) {
    DMM_DeAllocateForKernel((uintptr_t)_doubleBuffer);
  }
  _hasDoubleBuffer = enable;
}

void RootFrame::updateViewport(const ViewportInfo &info) {
  bool doubleBufferReallocated = false;
  if (_hasDoubleBuffer) {
    if (viewport().width() != info._width || viewport().height() != info._height) {
      DMM_DeAllocateForKernel((uintptr_t)_doubleBuffer);
      _doubleBuffer = (uint32_t*)DMM_AllocateForKernel(info._width * info._height * frameBuffer().bytesPerPixel(), 32);
      doubleBufferReallocated = true;
    }
  }
  if(upanui::BaseFrame::_updateViewport(info._x, info._y, info._width, info._height)) {
    if (doubleBufferReallocated) {
      copyToDoubleBuffer();
    }
    _isDirty.set(true);
  }
}

void RootFrame::copyToDoubleBuffer() {
  if (_doubleBuffer) {
    for (int i = 0; i < viewport().height(); ++i) {
      memcpy(_doubleBuffer + i * viewport().width(), frameBuffer().buffer() + i * frameBuffer().width(),
             viewport().width() * frameBuffer().bytesPerPixel());
    }
  }
}

void RootFrame::touch() {
  copyToDoubleBuffer();
  _isDirty.set(true);
}