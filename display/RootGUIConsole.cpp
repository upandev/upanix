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

#include <RootGUIConsole.h>
#include <GraphicsVideo.h>
#include <ColorPalettes.h>
#include <exception.h>
#include <Viewport.h>
#include <GCoreFunctions.h>

RootGUIConsole* RootGUIConsole::_instance = nullptr;

void RootGUIConsole::Create() {
  static bool _done = false;
  //can be called only once
  if(_done)
    throw upan::exception(XLOC, "cannot create RootGUIConsole driver!");
  _done = true;
  auto f = MultiBoot::Instance().VideoFrameBufferInfo();
  if(f) {
    upanui::FrameBuffer frameBuffer(*f);
    upanui::Viewport viewport(0, 0, f->_width, f->_height);

    static RootGUIConsole guiConsole(frameBuffer, viewport);
    _instance = &guiConsole;
  }
}

RootGUIConsole& RootGUIConsole::Instance() {
  if (_instance == nullptr) {
    throw upan::exception(XLOC, "RootGUIConsole is not created yet");
  }
  return *_instance;
}

RootGUIConsole::RootGUIConsole(const upanui::FrameBuffer& frameBuffer, const upanui::Viewport& viewport)
  : RootConsole(frameBuffer.height() / 16, frameBuffer.width() / 8),
    _frame(frameBuffer, viewport),
    _cursorPos(0), _cursorEnabled(false),
    _consoleUIObject(_frame) {
  GraphicsVideo::Create();
}

void RootGUIConsole::setFontContext(upanui::usfn::Context* context) {
  _textWriter.setFontContext(context);
}

void RootGUIConsole::putChar(int iPos, byte ch, const upanui::CharStyle& style) {
  const int curPos = iPos / upanui::ConsoleBuffer::NO_BYTES_PER_CHARACTER;
  const unsigned x = (curPos % _consoleBuffer.maxColumns());
  const unsigned y = (curPos / _consoleBuffer.maxColumns());

  _textWriter.drawChar(_consoleUIObject, ch, x, y,
                       ColorPalettes::CP16::Get(style.getFGColor()),
                       ColorPalettes::CP16::Get(style.getBGColor() >> 4));
}

void RootGUIConsole::scrollDown() {
  _textWriter.scrollDown(_consoleUIObject);
}

void RootGUIConsole::resetFrameBuffer(uintptr_t frameBufferAddress) {
  if ((uintptr_t)_frame.frameBuffer().buffer() != frameBufferAddress) {
    _frame.resetFrameBufferAddress((uint32_t*) frameBufferAddress);
    _frame.fillRect(0, 0, _frame.viewport().width(), _frame.viewport().height(), upanui::GCoreFunctions::ALPHA_MASK);
    _consoleUIObject.drawBuffer().initLocal(_frame.frameBuffer());
  }
}

void RootGUIConsole::StartCursorBlink() {
  static CursorBlink cursorBlink(*this);
  cursorBlink.start();
  _cursorEnabled = true;
}

void RootGUIConsole::gotoCursor() {
  if (_cursorEnabled) {
    upan::mutex_guard g(_cursorMutex);
    int newCurPos = GetCurrentCursorPosition();
    if (newCurPos != _cursorPos) {
      //erase old cursor
      putCursor(false);
    }
    //draw new cursor
    _cursorPos = newCurPos;
    putCursor(true);
  }
}

void RootGUIConsole::putCursor(bool show) {
  if ((uint32_t)_cursorPos >= _consoleBuffer.maxRows() * _consoleBuffer.maxColumns()) {
    return;
  }

  const upanui::CharStyle style = _consoleBuffer.getChar(_cursorPos * upanui::ConsoleBuffer::NO_BYTES_PER_CHARACTER + 1);
  const auto color = show ? ColorPalettes::CP16::Get(style.getFGColor()) : ColorPalettes::CP16::Get(style.getBGColor() >> 4);
  const auto x = (_cursorPos % _consoleBuffer.maxColumns());
  const auto y = (_cursorPos / _consoleBuffer.maxColumns());

  _textWriter.drawCursor(_consoleUIObject, x, y, color);
}

RootGUIConsole::CursorBlink::CursorBlink(RootGUIConsole& console) : upan::timer_thread(500), _console(console), _showCursorToggle(false) {
}

void RootGUIConsole::CursorBlink::on_timer_trigger() {
  upan::mutex_guard g(_console._cursorMutex);
  _console.putCursor(_showCursorToggle);
  _showCursorToggle = !_showCursorToggle;
}