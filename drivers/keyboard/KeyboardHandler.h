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
#ifndef _KB_DRIVER_H_
#define _KB_DRIVER_H_

#include <queue.h>
#include <KeyboardData.h>

class KeyboardHandler
{
  private:
    KeyboardHandler();
  public:
    static KeyboardHandler& Instance() {
      static KeyboardHandler instance;
      return instance;
    }

    upanui::KeyboardData GetCharInBlockMode();
    upan::option<upanui::KeyboardData> GetCharInNonBlockMode();
    bool Process(const KeyboardKeys key, const bool isKeyReleased);

    void Getch();

    void StartDispatcher();

    bool isShift() const { return _isShift; }
    bool isCtrl() const { return _isCtrl; }
  private:
    upan::option<upanui::KeyboardData> GetFromQueueBuffer();

    upan::queue<upanui::KeyboardData> _qBuffer;
    bool _isShift;
    bool _isAlt;
    bool _isCtrl;
    bool _isCaps;
};

#endif

