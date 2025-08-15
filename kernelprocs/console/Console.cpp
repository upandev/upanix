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
#include <Console.h>
#include <KeyboardHandler.h>
#include <CommandLineParser.h>
#include <ConsoleCommands.h>
#include <SessionManager.h>

void Console_StartUpanixConsole()
{
  Console::Instance().Start();
}

Console::Console() : _currentCommandPos(0)
{
  _commandLine = new char[COMMAND_LINE_SIZE];
	ConsoleCommands_Init() ;
  KC::MConsole().LoadMessage("Console Initialization", Success);
}

void Console::ClearCommandLine()
{
  memset(_commandLine, 0, COMMAND_LINE_SIZE) ;
}

void Console::DisplayCommandLine() {
  printf("\nupanix:%s > ", getenv("PWD"));
}

void Console::Start()
{
  KC::MConsole().RefreshScreen() ;
	
  DisplayCommandLine() ;

	//Default init code
/*	Console_ProcessCommand("eusbprobe");
	Console_ProcessCommand("mount usda");
	Console_ProcessCommand("chd usda");
	Console_ProcessCommand("test");
*/
	while(true) {
    int ch = getchar();

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
				SessionManager_SwitchToSession(SessionManager_KeyToSessionIDMap(ch)) ;
				break ;
			case Keyboard_F9:
			case Keyboard_F10:
				break ;
				
			case Keyboard_CAPS_LOCK:
				break ;
			case Keyboard_BACKSPACE:
        if(_currentCommandPos > 0)
				{
          _currentCommandPos-- ;
          int x = _commandLine[_currentCommandPos] == '\t' ? 4 : 1 ;
          KC::MConsole().MoveCursor(-x) ;
          KC::MConsole().ClearLine(upanui::ConsoleBuffer::START_CURSOR_POS) ;
				}
				break ;
				
			case Keyboard_LEFT_SHIFT:
			case Keyboard_RIGHT_SHIFT:
				break ;
				
			case Keyboard_ESC:
				break ;

			case Keyboard_ENTER:
        ProcessCommand() ;
        DisplayCommandLine() ;
				break ;

			default:

        if(_currentCommandPos != COMMAND_LINE_SIZE) {
          //TODO: putchar()
          printf("%c", ch);
          _commandLine[_currentCommandPos++] = ch ;
				}
		}
	}
}

void Console::ProcessCommand()
{
  _commandLine[_currentCommandPos] = '\0' ;
  _currentCommandPos = 0 ;
  ExecuteCommand(_commandLine);
  ClearCommandLine() ;
}

void Console::ExecuteCommand(const char* szCommandLine)
{
  CommandLineParser::Instance().Parse(szCommandLine);
  auto command = CommandLineParser::Instance().GetCommand();
  if(command)
    ConsoleCommands_ExecuteInternalCommand(command);
}

