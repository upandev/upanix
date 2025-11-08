#include "RootConsole.h"

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

#include <KernelComponents.h>
#include <RootGUIConsole.h>
#include <RootVGAConsole.h>
#include <MultiBoot.h>

RootConsole::RootConsole(uint32_t rows, uint32_t columns)
  : _consoleBuffer(*this, (byte*)(MEM_GRAPHICS_TEXT_BUFFER_START), rows, columns) {
}

void RootConsole::LoadMessage(const char* loadMessage, ReturnCode result) {
  byte width = 50;
  unsigned int i;
  char spaces[50] = "\0";

  for(i = 0; loadMessage[i] != '\0'; i++);

  width = width - i;

  for(i = 0; i < width; i++)
    spaces[i] = ' ';
  spaces[i] = '\0';

  Message("\n ", upanui::CharStyle::WHITE_ON_BLACK());
  Message(loadMessage, upanui::CharStyle::WHITE_ON_BLACK());
  Message(spaces, upanui::CharStyle::WHITE_ON_BLACK());

  if(result == Success)
    Message("[ OK ]", upanui::CharStyle(ColorPalettes::CP16::FG_BLACK, ColorPalettes::CP16::BG_GREEN));
  else
    Message("[ FAILED ]", upanui::CharStyle(ColorPalettes::CP16::FG_RED, ColorPalettes::CP16::BG_WHITE));
}

void RootConsole::PutChar(const char ch, const upanui::CharStyle& style) {
  _consoleBuffer.character(ch, style);
}

void RootConsole::Message(const char *message, const upanui::CharStyle& style) {
  _consoleBuffer.message(message, style);
}

void RootConsole::nMessage(const char *message, int n, const upanui::CharStyle& style) {
  _consoleBuffer.nmessage(message, n, style);
}

void RootConsole::MoveCursor(int pos) {
  _consoleBuffer.moveCursor(pos);
}

void RootConsole::ClearLine(int pos) {
  _consoleBuffer.clearLine(pos);
}

void RootConsole::RefreshScreen() {
  _consoleBuffer.refresh();
}

void RootConsole::ClearScreen() {
  _consoleBuffer.clear();
}

int RootConsole::GetCurrentCursorPosition() {
  return _consoleBuffer.getCurPos();
}

void RootConsole::SetCursor(int iCurPos, bool bUpdateCursorOnScreen) {
  _consoleBuffer.setCurPos(iCurPos, bUpdateCursorOnScreen);
}

void RootConsole::RawCharacter(byte ch, const upanui::CharStyle& attr, bool bUpdateCursorOnScreen) {
  _consoleBuffer.rawCharacter(ch, attr, bUpdateCursorOnScreen);
}

void RootConsole::RawCharacterArea(const MChar* src, uint32_t rows, uint32_t cols, int curPos) {
  _consoleBuffer.rawCharacterArea(src, rows, cols, curPos);
}

uint32_t RootConsole::MaxRows() const {
  return _consoleBuffer.maxRows();
}

uint32_t RootConsole::MaxColumns() const {
  return _consoleBuffer.maxColumns();
}

void RootConsole::ShowProgress(const char* msg, int startCur, unsigned progressPercent) {
  for(int c = GetCurrentCursorPosition(); c > startCur; --c)
    MoveCursor(-1);
  ClearLine(upanui::ConsoleBuffer::START_CURSOR_POS);
  printf("%s %u", msg, progressPercent);
}

void RootConsole::Create() {
  static bool initialized = false;
  if(initialized) {
    KC::MConsole().Message("\n Display is already initialized!", upanui::CharStyle::WHITE_ON_BLACK());
    return;
  }

  auto f = MultiBoot::Instance().VideoFrameBufferInfo();
  if(f) {
    RootGUIConsole::Create();
    KC::setConsole(RootGUIConsole::Instance());
  } else {
    static RootVGAConsole vc;
    KC::setConsole(vc);
  }

  KC::MConsole().LoadMessage("Video Initialization", Success);
  initialized = true;
}