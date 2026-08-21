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
#include <KeyboardHandler.h>
#include <PIC.h>
#include <IDT.h>
#include <ProcessManager.h>
#include <GraphicsVideo.h>
#include <KBInputHandler.h>
#include <KeyboardMapper.h>

KeyboardHandler::KeyboardHandler() : _qBuffer(1024), _isShift(false), _isAlt(false), _isCtrl(false), _isCaps(false) {
  KC::MConsole().LoadMessage("Keyboard Initialization", Success) ;
}

upanui::KeyboardData KeyboardHandler::GetCharInBlockMode() {
  while(true) {
    const auto& data = GetFromQueueBuffer();
    if (data.isEmpty()) {
      ProcessManager::Instance().WaitOnInterrupt(StdIRQ::Instance().KEYBOARD_IRQ);
    } else {
      return data.value();
    }
  }
}

upan::option<upanui::KeyboardData> KeyboardHandler::GetCharInNonBlockMode() {
  return GetFromQueueBuffer();
}

upan::option<upanui::KeyboardData> KeyboardHandler::GetFromQueueBuffer() {
  if(_qBuffer.empty())
    return upan::option<upanui::KeyboardData>::empty();
  upan::option<upanui::KeyboardData> data(_qBuffer.front());
  _qBuffer.pop_front();
  return data;
}

bool KeyboardHandler::Process(const KeyboardKeys key, const bool isKeyReleased) {
  KeyboardKeys res = key;

  if (key == Keyboard_NA_CHAR) {
    return false;
  }

  if (isKeyReleased) {
    if(key == Keyboard_LEFT_SHIFT || key == Keyboard_RIGHT_SHIFT) {
      _isShift = false;
    } else if(key == Keyboard_LEFT_CTRL || key == Keyboard_RIGHT_CTRL) {
      _isCtrl = false;
    } else if(key == Keyboard_LEFT_ALT || key == Keyboard_RIGHT_ALT) {
      _isAlt = false;
    }
  } else {
    if(key == Keyboard_CAPS_LOCK) {
      _isCaps = !_isCaps;
    } else if(key == Keyboard_LEFT_SHIFT || key == Keyboard_RIGHT_SHIFT) {
      _isShift = true;
    } else if(key == Keyboard_LEFT_CTRL || key == Keyboard_RIGHT_CTRL) {
      _isCtrl = true;
    } else if(key == Keyboard_LEFT_ALT || key == Keyboard_RIGHT_ALT) {
      _isAlt = true;
    } else {
      if (_isShift) {
        res = upanui::KeyboardMapper::Instance().getShiftKey(key);
      }

      if (_isCaps) {
        if ((res >= Keyboard_a && res <= Keyboard_z) || (res >= Keyboard_A && res <= Keyboard_Z)) {
          res = upanui::KeyboardMapper::Instance().getShiftKey(res);
        }
      }
    }
  }

  if (isKeyReleased) {
    return false;
  }

  switch(key) {
    case Keyboard_CAPS_LOCK:
    case Keyboard_LEFT_SHIFT:
    case Keyboard_RIGHT_SHIFT:
    case Keyboard_LEFT_CTRL:
    case Keyboard_RIGHT_CTRL:
    case Keyboard_LEFT_ALT:
    case Keyboard_RIGHT_ALT:
      return false;
  }

  if (key == Keyboard_ENTER) {
    res = Keyboard_CTRL_J;
  }

  upanui::KeyboardData data((uint8_t)res, _isShift, _isAlt, _isCtrl);

  if(!KBInputHandler_Process(data)) {
    return _qBuffer.push_back(data);
  }
  return false;
}

//just wait for a keyboard char input
void KeyboardHandler::Getch() {
  while(GetCharInNonBlockMode().isEmpty());
}

static void Keyboard_Event_Dispatcher() {
  try {
    while(true) {
      const auto& data = KeyboardHandler::Instance().GetCharInBlockMode();
      ProcessManager::Instance().GetProcess(GraphicsVideo::Instance().getInputEventFGProcess()).ifPresent([&](Process& p) {
        p.dispatchKeyboardData(data);
      });
    }
  } catch(upan::exception& e) {
    printf("\n Error in KB event dispatcher: %s", e.Error().Msg().c_str());
  }
  ProcessManager_Exit(0);
}

void KeyboardHandler::StartDispatcher() {
  static bool started = false;
  if (started) {
    return;
  }
  started = true;
  ProcessManager::Instance().CreateKernelProcess("kb.eh", (uintptr_t) &Keyboard_Event_Dispatcher,
                                                 ProcessManager::GetCurrentProcessID(), false, true, upan::vector<uintptr_t>());

}