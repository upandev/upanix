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

#include <stdlib.h>
#include <IConsole.h>
#include <ConsoleBuffer.h>
#include <ReturnHandler.h>

class RootConsole : public upanui::IConsole {
public:
  static void Create();

  void LoadMessage(const char* loadMessage, ReturnCode result);
  void PutChar(char ch, const upanui::CharStyle& style);
  void Message(const char* message, const upanui::CharStyle& style);
  void nMessage(const char* message, int n, const upanui::CharStyle& style);
  void MoveCursor(int pos);
  void ClearLine(int pos);
  void RefreshScreen();
  void ClearScreen();
  int GetCurrentCursorPosition();
  void ShowProgress(const char* msg, int startCur, unsigned progressPercent);

  void SetCursor(int iCurPos, bool bUpdateCursorOnScreen);
  void RawCharacter(byte ch, const upanui::CharStyle& attr, bool bUpdateCursorOnScreen);
  void RawCharacterArea(const MChar* src, uint32_t rows, uint32_t cols, int curPos);
  uint32_t MaxRows() const;
  uint32_t MaxColumns() const;

  virtual void StartCursorBlink() = 0;

protected:
  RootConsole(uint32_t rows, uint32_t columns);

  upanui::ConsoleBuffer _consoleBuffer;
};