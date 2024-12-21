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
# include <SysCall.h>
# include <SysCallDisplay.h>
# include <MultiBoot.h>
#include <BaseFrame.h>

bool SysCallDisplay_IsPresent(uint64_t sysCallID)
{
	return (sysCallID > SYS_CALL_DISPLAY_START && sysCallID < SYS_CALL_DISPLAY_END) ;
}

void SysCallDisplay_Handle(uint64_t *retVal, uint64_t sysCallID, bool doAddrTranslation, uint64_t p1,
                           uint64_t p2, uint64_t p3, uint64_t p4, uint64_t p5)
{
	switch(sysCallID)
	{
		case SYS_CALL_DISPLAY_MESSAGE :
			// P1 => Address of DisplayString relative to processBase 
			// P2 => Color Attr
			{
				char* szMessageAddress = ( char*) p1;
        KC::MConsole().Message(szMessageAddress, p2) ;
			}
			break ;

		case SYS_CALL_DISPLAY_CLR_SCR :
			{
        KC::MConsole().ClearScreen() ;
			}
			break ;

		case SYS_CALL_DISPLAY_MOV_CURSOR :
			// P1 => No of Positions to Move
			{
        KC::MConsole().MoveCursor((int)p1) ;
			}
			break ;

		case SYS_CALL_DISPLAY_CLR_LINE :
			// P1 => Position to Clear From
			{
        KC::MConsole().ClearLine((int)p1) ;
			}
			break ;

		case SYS_CALL_DISPLAY_SET_CURSOR :
			// P1 => Cursor Position
			// P2 => Update Cursor On Screen
			{
        KC::MConsole().SetCursor((int)p1, p2) ;
			}
			break ;

		case SYS_CALL_DISPLAY_GET_CURSOR :
			{
				*retVal = KC::MConsole().GetCurrentCursorPosition();
			}
			break ;

		case SYS_CALL_DISPLAY_RAW_CHAR :
			// P1 => Character to be displayed
			// P2 => Color Attr
			// P3 => Update Cursor On Screen
			{
        KC::MConsole().RawCharacter((char)(p1), p2, p3) ;
			}
			break ;

    case SYS_CALL_DISPLAY_RAW_CHAR_AREA:
      {
        const MChar* src = ( MChar*) p1;
        KC::MConsole().RawCharacterArea(src, p2, p3, (int)p4);
      }
      break;

    case SYS_CALL_DISPLAY_CONSOLE_SIZE:
      // P1 => Row size (return)
      // P2 => Column size (return)
      {
        auto maxRows = ( unsigned*) p1;
        auto maxCols = ( unsigned*) p2;
        *maxRows = KC::MConsole().MaxRows();
        *maxCols = KC::MConsole().MaxColumns();
      }
      break;

	  case SYS_CALL_DISPLAY_INIT_GUI_FRAME:
	    // P1 => Return address of FramebufferInfo
    {
      auto& process = ProcessManager::Instance().GetCurrentPAS();
      process.initGuiFrame();
      RootFrame& frame = process.getGuiFrame().value();
      auto frameBufferInfo = ( FrameBufferInfo*) p1;
      frameBufferInfo->_pitch = frame.frameBuffer().pitch();
      frameBufferInfo->_width = frame.frameBuffer().width();
      frameBufferInfo->_height = frame.frameBuffer().height();
      frameBufferInfo->_bpp = frame.frameBuffer().bpp();

      if (process.isKernelProcess()) {
        frameBufferInfo->_frameBuffer = frame.frameBuffer().buffer();
      } else {
        frameBufferInfo->_frameBuffer = (uint32_t*)(PROCESS_GUI_FRAMEBUFFER_ADDRESS);
      }
    }
    break;

	  case SYS_CALL_DISPLAY_GUI_FRAME_TOUCH:
	  {
	    ProcessManager::Instance().GetCurrentPAS().getGuiFrame().ifPresent([](RootFrame& f) { f.touch(); });
	  }
	  break;

    case SYS_CALL_DISPLAY_GUI_FRAME_HAS_ALPHA:
    {
      ProcessManager::Instance().GetCurrentPAS().getGuiFrame().ifPresent([p1](RootFrame& f) { f.hasAlpha((bool)p1); });
    }
    break;

	  case SYS_CALL_DISPLAY_SET_GUI_BASE:
    {
      ProcessManager::Instance().GetCurrentPAS().setGuiBase((bool)p1);
    }
    break;

    case SYS_CALL_DISPLAY_SET_MOUSE_CURSOR_TYPE:
    {
      ProcessManager::Instance().GetCurrentPAS().setMouseCursorType((MouseCursorType)p1);
    }
    break;

    case SYS_CALL_DISPLAY_INIT_TERM_CONSOLE:
	  {
      ProcessManager::Instance().GetCurrentPAS().setupAsTtyProcess();
	  }
	  break;

	  case SYS_CALL_DISPLAY_INIT_GUI_EVENT_STREAM:
	  {
	    int* fdList = ( int*) p1;
	    ProcessManager::Instance().GetCurrentPAS().setupAsGuiProcess(fdList);
	  }
	  break;

	  case SYS_CALL_DISPLAY_SET_VIEWPORT:
	  {
	    const auto* viewportInfo = ( ViewportInfo*) p1;
	    ProcessManager::Instance().GetCurrentPAS().getGuiFrame().value().updateViewport(*viewportInfo);
	  }
	  break;

	  case SYS_CALL_DISPLAY_GET_VIEWPORT:
	  {
	    auto* viewportInfo = ( ViewportInfo*) p1;
	    const auto& viewport = ProcessManager::Instance().GetCurrentPAS().getGuiFrame().value().viewport();
	    viewportInfo->_x = viewport.x1();
	    viewportInfo->_y = viewport.y1();
	    viewportInfo->_width = viewport.width();
	    viewportInfo->_height = viewport.height();
	  }
	  break;
	}
}
