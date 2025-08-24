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

#include <signal.h>
#include "InterruptHandlers.h"

class Signal {
public:
  typedef enum {
    SA_TERMINATE,
    SA_IGNORE,
    SA_STOP,
    SA_CONTINUE,
  } DEFAULT_SA_TYPE;

  Signal() = default;
  Signal(SIGNAL signo, const union sigval* value);
  explicit Signal(SIGNAL signo) : Signal(signo, nullptr) {}

  SIGNAL signo() const { return _signo; }
  const union sigval& value() const { return _value; }
  DEFAULT_SA_TYPE defaultActionType() const;
  bool allowCoreDump() const;
  bool isMaskable() const;

private:
  SIGNAL _signo;
  union sigval _value;
};

struct SignalTaskContext {
  TaskContext _context;
  SIGNAL _signal;
  PROCESS_STATUS _processStatus;
  sigset_t _sigMask;
};
