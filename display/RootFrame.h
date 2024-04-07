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

#include <BaseFrame.h>
#include <atomicop.h>

class RootFrame : public upanui::BaseFrame {
public:
  RootFrame(const upanui::FrameBuffer& frameBuffer, const upanui::Viewport& viewport) :
    BaseFrame(frameBuffer, viewport), _isDirty(false), _hasAlpha(false), _hasDoubleBuffer(false), _doubleBuffer(nullptr) {
  }

  void resetFrameBufferAddress(uint32_t* frameAddr) {
    const_cast<upanui::FrameBuffer&>(frameBuffer()).resetFrameBufferAddress(frameAddr);
    touch();
  }

  void touch() override;

  bool hasAlpha() const {
    return _hasAlpha;
  }

  void hasAlpha(bool hasAlpha) {
    _hasAlpha = hasAlpha;
  }

  bool isDirty() {
    return _isDirty.get();
  }

  void clean() {
    _isDirty.set(false);
  }

  uint32_t* buffer() {
    return _doubleBuffer ? _doubleBuffer : frameBuffer().buffer();
  }

  int bufferLineWidth() {
    return _hasDoubleBuffer && _doubleBuffer ? viewport().width() : frameBuffer().width();
  }

  void enableDoubleBuffer(bool enable);
  void updateViewport(const ViewportInfo& info);

private:
  void copyToDoubleBuffer();

private:
  upan::atomic::integral<bool> _isDirty;
  bool _hasAlpha;
  bool _hasDoubleBuffer;
  uint32_t* _doubleBuffer;
};