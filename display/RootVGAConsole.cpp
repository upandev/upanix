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

#include <RootVGAConsole.h>
#include <PortCom.h>

#define VIDEO_BUFFER_ADDRESS 0xB8000
#define CURSOR_HIEGHT 16
#define CRT_INDEX_REG 0x03D4
#define CRT_DATA_REG 0x03D5

RootVGAConsole::RootVGAConsole() : RootConsole(25, 80) {
  // Get Cursor Size Start
  PortCom_SendByte(CRT_INDEX_REG, 0x0A);
  byte bCurStart = PortCom_ReceiveByte(CRT_DATA_REG);
  // Get Cursor Size End
  PortCom_SendByte(CRT_INDEX_REG, 0x0A);
  byte bCurEnd = PortCom_ReceiveByte(CRT_DATA_REG);

  bCurStart = (bCurStart & 0xC0 ) | (CURSOR_HIEGHT - 3);
  bCurEnd = (bCurEnd & 0xE0 ) | (CURSOR_HIEGHT - 2);

  // Set Cursor Size Start
  PortCom_SendByte(CRT_INDEX_REG, 0x0A);
  PortCom_SendByte(CRT_DATA_REG, bCurStart);
  // Set Cursor Size End
  PortCom_SendByte(CRT_INDEX_REG, 0x0B);
  PortCom_SendByte(CRT_DATA_REG, bCurEnd);
}

void RootVGAConsole::gotoCursor() {
  int iPos = _consoleBuffer.getCurPos();
  if (iPos >= _consoleBuffer.maxRows() * _consoleBuffer.maxColumns()) {
    return;
  }

  PortCom_SendByte(CRT_INDEX_REG, 15);
  PortCom_SendByte(CRT_DATA_REG, iPos & 0xFF);

  PortCom_SendByte(CRT_INDEX_REG, 14);
  PortCom_SendByte(CRT_DATA_REG, (iPos >> 8) & 0xFF);
}

void RootVGAConsole::putChar(int iPos, byte ch, const upanui::CharStyle& attr) {
  static byte* vga_frame_buffer = (byte*)(VIDEO_BUFFER_ADDRESS);
  vga_frame_buffer[iPos] = ch;
  vga_frame_buffer[iPos + 1] = attr.get();
}

void RootVGAConsole::scrollDown() {
  static const unsigned NO_OF_DISPLAY_BYTES = (_consoleBuffer.maxRows() - 1) * _consoleBuffer.maxColumns() * upanui::ConsoleBuffer::NO_BYTES_PER_CHARACTER;
  static const unsigned OFFSET = _consoleBuffer.maxColumns() * upanui::ConsoleBuffer::NO_BYTES_PER_CHARACTER;

  static byte* vga_frame_buffer = (byte*)(VIDEO_BUFFER_ADDRESS);
  memcpy(vga_frame_buffer, vga_frame_buffer + OFFSET, NO_OF_DISPLAY_BYTES);
  for(unsigned i = NO_OF_DISPLAY_BYTES;
    i < NO_OF_DISPLAY_BYTES + _consoleBuffer.maxColumns() * upanui::ConsoleBuffer::NO_BYTES_PER_CHARACTER;
    i += upanui::ConsoleBuffer::NO_BYTES_PER_CHARACTER) {
    vga_frame_buffer[i] = ' ';
    vga_frame_buffer[i + 1] = upanui::CharStyle::WHITE_ON_BLACK();
  }
}

