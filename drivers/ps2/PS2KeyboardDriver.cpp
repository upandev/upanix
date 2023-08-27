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
#include <PS2KeyboardDriver.h>
#include <AsmUtil.h>
#include <PortCom.h>
#include <IrqManager.h>
#include <KeyboardHandler.h>
#include <PS2Controller.h>
#include <InterruptHandlers.h>

void PS2KeyboardDriver::Handler() {
  PS2Controller::Instance().WaitForRead();
  if(!(PortCom_ReceiveByte(PS2Controller::COMMAND_PORT) & 0x20))
    PS2KeyboardDriver::Instance().Process((PortCom_ReceiveByte(PS2Controller::DATA_PORT)) & 0xFF);
  IrqManager::Instance().SendEOI(StdIRQ::Instance().KEYBOARD_IRQ);
}

static const byte EXTRA_KEYS = 224;
static const int MAX_KEYBOARD_CHARS = 84;

static const KeyboardKeys Keyboard_GENERIC_KEY_MAP[MAX_KEYBOARD_CHARS] = { Keyboard_NA_CHAR,

  Keyboard_ESC, Keyboard_1, Keyboard_2, Keyboard_3, Keyboard_4, Keyboard_5, Keyboard_6, Keyboard_7, Keyboard_8, Keyboard_9,

  Keyboard_0, Keyboard_MINUS, Keyboard_EQUAL, Keyboard_BACKSPACE, Keyboard_TAB, Keyboard_q, Keyboard_w, Keyboard_e, Keyboard_r,

  Keyboard_t, Keyboard_y, Keyboard_u, Keyboard_i, Keyboard_o, Keyboard_p, Keyboard_OPEN_BRACKET, Keyboard_CLOSE_BRACKET, Keyboard_ENTER,

  Keyboard_LEFT_CTRL, Keyboard_a, Keyboard_s, Keyboard_d, Keyboard_f, Keyboard_g, Keyboard_h, Keyboard_j, Keyboard_k,

  Keyboard_l, Keyboard_SEMICOLON, Keyboard_SINGLEQUOTE, Keyboard_BACKQUOTE, Keyboard_LEFT_SHIFT, Keyboard_BACKSLASH, Keyboard_z, Keyboard_x, Keyboard_c,

  Keyboard_v, Keyboard_b, Keyboard_n, Keyboard_m, Keyboard_COMMA, Keyboard_FULLSTOP, Keyboard_SLASH_DIVIDE, Keyboard_RIGHT_SHIFT, Keyboard_STAR_MULTIPLY,

  Keyboard_LEFT_ALT, Keyboard_SPACE, Keyboard_CAPS_LOCK, Keyboard_F1, Keyboard_F2,

  Keyboard_F3, Keyboard_F4, Keyboard_F5, Keyboard_F6, Keyboard_F7,

  Keyboard_F8, Keyboard_F9, Keyboard_F10, Keyboard_NA_CHAR, Keyboard_NA_CHAR, Keyboard_KEY_HOME,

  Keyboard_KEY_UP, Keyboard_KEY_PG_UP, Keyboard_NA_CHAR, Keyboard_KEY_LEFT, Keyboard_NA_CHAR,

  Keyboard_KEY_RIGHT, Keyboard_NA_CHAR, Keyboard_KEY_END, Keyboard_KEY_DOWN,

  Keyboard_KEY_PG_DOWN, Keyboard_KEY_INST, Keyboard_KEY_DEL
};

extern "C" {
  void _keyboard_interrupt_handler();
}

PS2KeyboardDriver::PS2KeyboardDriver() : _isShiftKey(false), _isCapsLock(false), _isCtrlKey(false)
{
  KeyboardHandler::Instance();

  IrqManager::Instance().DisableIRQ(StdIRQ::Instance().KEYBOARD_IRQ);
  IrqManager::Instance().RegisterIRQ(StdIRQ::Instance().KEYBOARD_IRQ, (uintptr_t)&_keyboard_interrupt_handler);
  IrqManager::Instance().EnableIRQ(StdIRQ::Instance().KEYBOARD_IRQ);

  PortCom_ReceiveByte(PS2Controller::DATA_PORT);

  KC::MConsole().LoadMessage("PS2 Keyboard Driver Initialization", Success);
}

void PS2KeyboardDriver::Process(byte rawKey) {
  KeyboardKeys key = Keyboard_NA_CHAR;
  bool isKeyReleased = false;

  if (rawKey != EXTRA_KEYS) {
    isKeyReleased = rawKey & 0x80;
    if (isKeyReleased)
      rawKey -= 0x80;

    if (rawKey < MAX_KEYBOARD_CHARS) {
      key = Keyboard_GENERIC_KEY_MAP[rawKey];
    }
  }

  KeyboardHandler::Instance().Process(key, isKeyReleased);
  StdIRQ::Instance().KEYBOARD_IRQ.Signal();
}