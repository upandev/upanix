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

KeyboardHandler::KeyboardHandler() : _qBuffer(1024), _isShift(false), _isAlt(false), _isCtrl(false), _isCaps(false) {
  for(int i = 0; i < MAX_KEYS; ++i) {
    _shiftedKeyMap[i] = Keyboard_NA_CHAR;
  }

  for(int i = 0; i < MAX_KEYS; ++i) {
    _ctrlKeyMap[i] = Keyboard_NA_CHAR;
  }

  const auto& mapKey = [](KeyboardKeys* keyMap, const KeyboardKeys from, const KeyboardKeys to) {
    if (from > MAX_KEYS) {
      throw upan::exception(XLOC, "KeyboardKey %d > MAX_KEYS (%d)", from, MAX_KEYS);
    }
    keyMap[from] = to;
  };

  mapKey(_shiftedKeyMap, Keyboard_a, Keyboard_A);
  mapKey(_shiftedKeyMap, Keyboard_b, Keyboard_B);
  mapKey(_shiftedKeyMap, Keyboard_c, Keyboard_C);
  mapKey(_shiftedKeyMap, Keyboard_d, Keyboard_D);
  mapKey(_shiftedKeyMap, Keyboard_e, Keyboard_E);
  mapKey(_shiftedKeyMap, Keyboard_f, Keyboard_F);
  mapKey(_shiftedKeyMap, Keyboard_g, Keyboard_G);
  mapKey(_shiftedKeyMap, Keyboard_h, Keyboard_H);
  mapKey(_shiftedKeyMap, Keyboard_i, Keyboard_I);
  mapKey(_shiftedKeyMap, Keyboard_j, Keyboard_J);
  mapKey(_shiftedKeyMap, Keyboard_k, Keyboard_K);
  mapKey(_shiftedKeyMap, Keyboard_l, Keyboard_L);
  mapKey(_shiftedKeyMap, Keyboard_m, Keyboard_M);
  mapKey(_shiftedKeyMap, Keyboard_n, Keyboard_N);
  mapKey(_shiftedKeyMap, Keyboard_o, Keyboard_O);
  mapKey(_shiftedKeyMap, Keyboard_p, Keyboard_P);
  mapKey(_shiftedKeyMap, Keyboard_q, Keyboard_Q);
  mapKey(_shiftedKeyMap, Keyboard_r, Keyboard_R);
  mapKey(_shiftedKeyMap, Keyboard_s, Keyboard_S);
  mapKey(_shiftedKeyMap, Keyboard_t, Keyboard_T);
  mapKey(_shiftedKeyMap, Keyboard_u, Keyboard_U);
  mapKey(_shiftedKeyMap, Keyboard_v, Keyboard_V);
  mapKey(_shiftedKeyMap, Keyboard_w, Keyboard_W);
  mapKey(_shiftedKeyMap, Keyboard_x, Keyboard_X);
  mapKey(_shiftedKeyMap, Keyboard_y, Keyboard_Y);
  mapKey(_shiftedKeyMap, Keyboard_z, Keyboard_Z);

  mapKey(_shiftedKeyMap, Keyboard_A, Keyboard_a);
  mapKey(_shiftedKeyMap, Keyboard_B, Keyboard_b);
  mapKey(_shiftedKeyMap, Keyboard_C, Keyboard_c);
  mapKey(_shiftedKeyMap, Keyboard_D, Keyboard_d);
  mapKey(_shiftedKeyMap, Keyboard_E, Keyboard_e);
  mapKey(_shiftedKeyMap, Keyboard_F, Keyboard_f);
  mapKey(_shiftedKeyMap, Keyboard_G, Keyboard_g);
  mapKey(_shiftedKeyMap, Keyboard_H, Keyboard_h);
  mapKey(_shiftedKeyMap, Keyboard_I, Keyboard_i);
  mapKey(_shiftedKeyMap, Keyboard_J, Keyboard_j);
  mapKey(_shiftedKeyMap, Keyboard_K, Keyboard_k);
  mapKey(_shiftedKeyMap, Keyboard_L, Keyboard_l);
  mapKey(_shiftedKeyMap, Keyboard_M, Keyboard_m);
  mapKey(_shiftedKeyMap, Keyboard_N, Keyboard_n);
  mapKey(_shiftedKeyMap, Keyboard_O, Keyboard_o);
  mapKey(_shiftedKeyMap, Keyboard_P, Keyboard_p);
  mapKey(_shiftedKeyMap, Keyboard_Q, Keyboard_q);
  mapKey(_shiftedKeyMap, Keyboard_R, Keyboard_r);
  mapKey(_shiftedKeyMap, Keyboard_S, Keyboard_s);
  mapKey(_shiftedKeyMap, Keyboard_T, Keyboard_t);
  mapKey(_shiftedKeyMap, Keyboard_U, Keyboard_u);
  mapKey(_shiftedKeyMap, Keyboard_V, Keyboard_v);
  mapKey(_shiftedKeyMap, Keyboard_W, Keyboard_w);
  mapKey(_shiftedKeyMap, Keyboard_X, Keyboard_x);
  mapKey(_shiftedKeyMap, Keyboard_Y, Keyboard_y);
  mapKey(_shiftedKeyMap, Keyboard_Z, Keyboard_z);

  mapKey(_shiftedKeyMap, Keyboard_BACKQUOTE, Keyboard_TILDE);
  mapKey(_shiftedKeyMap, Keyboard_1, Keyboard_EXCLAIMATION);
  mapKey(_shiftedKeyMap, Keyboard_2, Keyboard_AT);
  mapKey(_shiftedKeyMap, Keyboard_3, Keyboard_HASH);
  mapKey(_shiftedKeyMap, Keyboard_4, Keyboard_DOLLAR);
  mapKey(_shiftedKeyMap, Keyboard_5, Keyboard_PERCENT);
  mapKey(_shiftedKeyMap, Keyboard_6, Keyboard_CARROT);
  mapKey(_shiftedKeyMap, Keyboard_7, Keyboard_AND);
  mapKey(_shiftedKeyMap, Keyboard_8, Keyboard_STAR_MULTIPLY);
  mapKey(_shiftedKeyMap, Keyboard_9, Keyboard_OPEN_PAREN);
  mapKey(_shiftedKeyMap, Keyboard_0, Keyboard_CLOSE_PAREN);
  mapKey(_shiftedKeyMap, Keyboard_MINUS, Keyboard_UNDERSCORE);
  mapKey(_shiftedKeyMap, Keyboard_EQUAL, Keyboard_PLUS);
  mapKey(_shiftedKeyMap, Keyboard_OPEN_BRACKET, Keyboard_OPEN_BRACE);
  mapKey(_shiftedKeyMap, Keyboard_CLOSE_BRACKET, Keyboard_CLOSE_BRACE);
  mapKey(_shiftedKeyMap, Keyboard_BACKSLASH, Keyboard_OR);
  mapKey(_shiftedKeyMap, Keyboard_SEMICOLON, Keyboard_COLON);
  mapKey(_shiftedKeyMap, Keyboard_SINGLEQUOTE, Keyboard_DOUBLEQUOTE);
  mapKey(_shiftedKeyMap, Keyboard_COMMA, Keyboard_LESSER);
  mapKey(_shiftedKeyMap, Keyboard_FULLSTOP, Keyboard_GREATER);
  mapKey(_shiftedKeyMap, Keyboard_SLASH_DIVIDE, Keyboard_QUESTION);

  mapKey(_ctrlKeyMap, Keyboard_a, Keyboard_CTRL_A);
  mapKey(_ctrlKeyMap, Keyboard_b, Keyboard_CTRL_B);
  mapKey(_ctrlKeyMap, Keyboard_c, Keyboard_CTRL_C);
  mapKey(_ctrlKeyMap, Keyboard_d, Keyboard_CTRL_D);
  mapKey(_ctrlKeyMap, Keyboard_e, Keyboard_CTRL_E);
  mapKey(_ctrlKeyMap, Keyboard_f, Keyboard_CTRL_F);
  mapKey(_ctrlKeyMap, Keyboard_g, Keyboard_CTRL_G);
  mapKey(_ctrlKeyMap, Keyboard_h, Keyboard_CTRL_H);
  mapKey(_ctrlKeyMap, Keyboard_i, Keyboard_CTRL_I);
  mapKey(_ctrlKeyMap, Keyboard_j, Keyboard_CTRL_J);
  mapKey(_ctrlKeyMap, Keyboard_k, Keyboard_CTRL_K);
  mapKey(_ctrlKeyMap, Keyboard_l, Keyboard_CTRL_L);
  mapKey(_ctrlKeyMap, Keyboard_m, Keyboard_CTRL_M);
  mapKey(_ctrlKeyMap, Keyboard_n, Keyboard_CTRL_N);
  mapKey(_ctrlKeyMap, Keyboard_o, Keyboard_CTRL_O);
  mapKey(_ctrlKeyMap, Keyboard_p, Keyboard_CTRL_P);
  mapKey(_ctrlKeyMap, Keyboard_q, Keyboard_CTRL_Q);
  mapKey(_ctrlKeyMap, Keyboard_r, Keyboard_CTRL_R);
  mapKey(_ctrlKeyMap, Keyboard_s, Keyboard_CTRL_S);
  mapKey(_ctrlKeyMap, Keyboard_t, Keyboard_CTRL_T);
  mapKey(_ctrlKeyMap, Keyboard_u, Keyboard_CTRL_U);
  mapKey(_ctrlKeyMap, Keyboard_v, Keyboard_CTRL_V);
  mapKey(_ctrlKeyMap, Keyboard_w, Keyboard_CTRL_W);
  mapKey(_ctrlKeyMap, Keyboard_x, Keyboard_CTRL_X);
  mapKey(_ctrlKeyMap, Keyboard_y, Keyboard_CTRL_Y);
  mapKey(_ctrlKeyMap, Keyboard_z, Keyboard_CTRL_Z);

  mapKey(_ctrlKeyMap, Keyboard_A, Keyboard_CTRL_A);
  mapKey(_ctrlKeyMap, Keyboard_B, Keyboard_CTRL_B);
  mapKey(_ctrlKeyMap, Keyboard_C, Keyboard_CTRL_C);
  mapKey(_ctrlKeyMap, Keyboard_D, Keyboard_CTRL_D);
  mapKey(_ctrlKeyMap, Keyboard_E, Keyboard_CTRL_E);
  mapKey(_ctrlKeyMap, Keyboard_F, Keyboard_CTRL_F);
  mapKey(_ctrlKeyMap, Keyboard_G, Keyboard_CTRL_G);
  mapKey(_ctrlKeyMap, Keyboard_H, Keyboard_CTRL_H);
  mapKey(_ctrlKeyMap, Keyboard_I, Keyboard_CTRL_I);
  mapKey(_ctrlKeyMap, Keyboard_J, Keyboard_CTRL_J);
  mapKey(_ctrlKeyMap, Keyboard_K, Keyboard_CTRL_K);
  mapKey(_ctrlKeyMap, Keyboard_L, Keyboard_CTRL_L);
  mapKey(_ctrlKeyMap, Keyboard_M, Keyboard_CTRL_M);
  mapKey(_ctrlKeyMap, Keyboard_N, Keyboard_CTRL_N);
  mapKey(_ctrlKeyMap, Keyboard_O, Keyboard_CTRL_O);
  mapKey(_ctrlKeyMap, Keyboard_P, Keyboard_CTRL_P);
  mapKey(_ctrlKeyMap, Keyboard_Q, Keyboard_CTRL_Q);
  mapKey(_ctrlKeyMap, Keyboard_R, Keyboard_CTRL_R);
  mapKey(_ctrlKeyMap, Keyboard_S, Keyboard_CTRL_S);
  mapKey(_ctrlKeyMap, Keyboard_T, Keyboard_CTRL_T);
  mapKey(_ctrlKeyMap, Keyboard_U, Keyboard_CTRL_U);
  mapKey(_ctrlKeyMap, Keyboard_V, Keyboard_CTRL_V);
  mapKey(_ctrlKeyMap, Keyboard_W, Keyboard_CTRL_W);
  mapKey(_ctrlKeyMap, Keyboard_X, Keyboard_CTRL_X);
  mapKey(_ctrlKeyMap, Keyboard_Y, Keyboard_CTRL_Y);
  mapKey(_ctrlKeyMap, Keyboard_Z, Keyboard_CTRL_Z);

  mapKey(_ctrlKeyMap, Keyboard_AT, Keyboard_CTRL_AT);
  mapKey(_ctrlKeyMap, Keyboard_OPEN_BRACKET, Keyboard_CTRL_OPEN_BRACKET);
  mapKey(_ctrlKeyMap, Keyboard_BACKSLASH, Keyboard_CTRL_BACKSLASH);
  mapKey(_ctrlKeyMap, Keyboard_CLOSE_BRACKET, Keyboard_CTRL_CLOSE_BRACKET);
  mapKey(_ctrlKeyMap, Keyboard_CARROT, Keyboard_CTRL_CARROT);
  mapKey(_ctrlKeyMap, Keyboard_UNDERSCORE, Keyboard_CTRL_UNDERSCORE);

  KC::MConsole().LoadMessage("Keyboard Initialization", Success) ;
}

KeyboardKeys KeyboardHandler::getShiftKey(const KeyboardKeys key) {
  if (key > MAX_KEYS) {
    throw upan::exception(XLOC, "Can't shift map key %d as it's > MAX_KEYS(%d)", key, MAX_KEYS);
  }
  const auto mappedKey = _shiftedKeyMap[key];
  return mappedKey == Keyboard_NA_CHAR ? key : mappedKey;
}

KeyboardKeys KeyboardHandler::getCtrlKey(const KeyboardKeys key) {
  if (key > MAX_KEYS) {
    throw upan::exception(XLOC, "Can't ctrl map key %d as it's > MAX_KEYS(%d)", key, MAX_KEYS);
  }
  const auto mappedKey = _ctrlKeyMap[key];
  return mappedKey == Keyboard_NA_CHAR ? key : mappedKey;
}

KeyboardKeys KeyboardHandler::mapToTTYKey(const upanui::KeyboardData& data) {
  if (data.isCtrlPressed()) {
    return getCtrlKey((KeyboardKeys)data.getCh());
  }
  return (KeyboardKeys)data.getCh();
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
  const auto& data = _qBuffer.front();
  _qBuffer.pop_front();
  return upan::option<upanui::KeyboardData>(data);
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
        res = getShiftKey(key);
      }

      if (_isCaps) {
        if ((res >= Keyboard_a && res <= Keyboard_z) || (res >= Keyboard_A && res <= Keyboard_Z)) {
          res = getShiftKey(res);
        }
      }
    }
  }

  if (isKeyReleased) {
    return false;
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
  ProcessManager_Exit();
}

void KeyboardHandler::StartDispatcher() {
  static bool started = false;
  if (started) {
    return;
  }
  started = true;
  ProcessManager::Instance().CreateKernelProcess("kbed", (uintptr_t) &Keyboard_Event_Dispatcher,
                                                 ProcessManager::GetCurrentProcessID(), false, upan::vector<uintptr_t>());

}