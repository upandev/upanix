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
# include <KBInputHandler.h>
# include <PS2KeyboardDriver.h>
# include <SessionManager.h>

/******************* Static Functions ***************************/

byte KBInputHandler_InRegList(byte key)
{
	switch(key)
	{
		case Keyboard_F1:
		case Keyboard_F2:
		case Keyboard_F3:
		case Keyboard_F4:
		case Keyboard_F5:
		case Keyboard_F6:
		case Keyboard_F7:
		case Keyboard_F8:
			return true ;
	}

	return false ;
}

/***************************************************************/

bool KBInputHandler_Process(const upanui::KeyboardData& data) {
  if(KBInputHandler_InRegList(data.getCh()))
	{
	  SessionManager_SwitchToSession(SessionManager_KeyToSessionIDMap(data.getCh())) ;
		return true ;
	}
	return false ;
}

