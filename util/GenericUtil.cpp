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
# include <GenericUtil.h>
# include <KeyboardHandler.h>
# include <StringUtil.h>
# include <FileOperations.h>
# include <DMM.h>
#include <ProcessManager.h>

void GenericUtil_ReadInput(char* szInputBuffer, const int iMaxReadLength, byte bEcho)
{
	int iCurrentReadPos = 0 ;

	while(SUCCESS)
	{
    const auto& data = KeyboardHandler::Instance().GetCharInBlockMode();
    uint8_t ch = data.getCh();

		switch(ch)
		{
			case Keyboard_LEFT_ALT:
			case Keyboard_LEFT_CTRL:
				break ;
			case Keyboard_F1:
			case Keyboard_F2:
			case Keyboard_F3:
			case Keyboard_F4:
			case Keyboard_F5:
			case Keyboard_F6:
			case Keyboard_F7:
			case Keyboard_F8:
//				SessionManager_SwitchToSession(SessionManager_KeyToSessionIDMap(ch)) ;
				break ;
			case Keyboard_F9:
			case Keyboard_F10:
				break ;
				
			case Keyboard_CAPS_LOCK:
				break ;
			case Keyboard_BACKSPACE:
				if(iCurrentReadPos > 0)
				{
					if(bEcho)
					{
            KC::MConsole().MoveCursor(-1) ;
            KC::MConsole().ClearLine(upanui::ConsoleBuffer::START_CURSOR_POS) ;
					}
					iCurrentReadPos-- ;
				}
				break ;
				
			case Keyboard_LEFT_SHIFT:
			case Keyboard_RIGHT_SHIFT:
				break ;
				
			case Keyboard_ESC:
				break ;

			case Keyboard_ENTER:
      case Keyboard_CTRL_J:
				szInputBuffer[iCurrentReadPos] = '\0' ;
				return ;

			default:

				if(iCurrentReadPos != iMaxReadLength)
				{
					szInputBuffer[iCurrentReadPos++] = ch ;
					if(bEcho)
            printf("%c", ch);
				}
		}
	}
}

void debug_step(const char* msg)
{
  printf("\n%s...", msg);
  KeyboardHandler::Instance().Getch();
  printf("\n%s - done!");
}
