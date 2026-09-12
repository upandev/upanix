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

#include <LineBuffer.h>
#include <kb.h>

LineBuffer::LineBuffer(StreamBuffer& streamBuffer) : _streamBuffer(streamBuffer), _cursorPos(0) {
}

LineBuffer::Result LineBuffer::process(const uint8_t ch, upan::function<void> waitFunc) {
  upan::mutex_guard g(_ioSync);
  Result r { false, 0 };
  if (ch == Keyboard_KEY_HOME) {
    r._distance = _cursorPos;
    r._processed = true;
    _cursorPos = 0;
  } else if (ch == Keyboard_KEY_END) {
    r._distance = _line.length() - _cursorPos;
    r._processed = true;
    _cursorPos = _line.length();
  } else if (ch == Keyboard_KEY_LEFT) {
    if (_cursorPos > 0) {
      r._processed = true;
      _cursorPos--;
    }
  } else if (ch == Keyboard_KEY_RIGHT) {
    if (_cursorPos < _line.length()) {
      r._processed = true;
      _cursorPos++;
    }
  } else if (ch == Keyboard_BACKSPACE) {
    if (_cursorPos > 0) {
      r._processed = true;
      _line.erase(_cursorPos - 1, 1);
      _cursorPos--;
    }
  } else if (ch == Keyboard_DEL || ch == Keyboard_KEY_DEL) {
    if (_cursorPos < _line.length()) {
      r._processed = true;
      _line.erase(_cursorPos, 1);
    }
  } else if (is_new_line(ch)) {
    r._processed = true;
    _streamBuffer.write((_line + "\n").c_str(), _line.length() + 1, false, [&]() { waitFunc(); });
    _line.clear();
    _cursorPos = 0;
  } else if (!is_command_key(ch)) {
    r._processed = true;
    _line.insert(_cursorPos, ch);
    _cursorPos++;
  }
  return r;
}