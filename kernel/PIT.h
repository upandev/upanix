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

#include <Global.h>
#include <InterruptHandlers.h>
#include <atomicop.h>

#define TIMECOUNTER_i8254_FREQU 1193182 // Input Frequency of PIT
// Counter 0, 1, 2
#define PIT_COUNTER_0_PORT 0x40 // Channel 0 data port (read/write)
#define PIT_COUNTER_1_PORT 0x41 // Channel 1 data port (read/write)
#define PIT_COUNTER_2_PORT 0x42 // Channel 2 data port (read/write), OUT connected to speaker
#define PIT_MODE_PORT      0x43

#define PIT_COUNTER_2_CTRLPORT   0x61 // Channel 2 gate: the input can be controlled by IO port 0x61, bit 0.

// command register
// bit0
#define SIXTEEN_BIT_BINARY       0x00
#define FOUR_DIGIT_BCD           0x01
//bit1-3
/*In MODES 0, 1, 4, and 5 the Counter wraps around to the highest count (FFFFh or 9999bcd) and continues counting.
In MODES 2 and 3 (which are periodic) the Counter reloads itself with the initial count and continues counting from there. */
#define PIT_MODE0                0x00 // interrupt on terminal count
#define PIT_MODE1                0x02 // hardware re-triggerable one-shot
#define PIT_RATEGENERATOR        0x04 // Mode 2: divide by N counter
#define PIT_SQUAREWAVE           0x06 // Mode 3: square-wave mode
#define PIT_MODE4                0x08 // software triggered strobe
#define PIT_MODE5                0x0A // hardware triggered strobe
//bit4-5
#define PIT_RW_LO_MODE           0x10 // Read/2xWrite bits 0..7 of counter value
#define PIT_RW_HI_MODE           0x20 // Read/2xWrite bits 8..15 of counter value
#define PIT_RW_LO_HI_MODE        0x30 // 2xRead/2xWrite bits 0..7 then 8..15 of counter value
//bit6-7
#define PIT_COUNTER_0            0x00 // select counter 0
#define PIT_COUNTER_1            0x40 // select counter 1
#define PIT_COUNTER_2            0x80 // select counter 2

#define INT_PER_SEC 1000

class IRQ ;

class PIT {
private:
  PIT();
public:
  static PIT& Instance() {
    static PIT instance;
    return instance;
  }

  void Initialize();
  void ContextSwitchHandler(TaskContext& taskContext);
  void Handler();
  void disable();

private:
  const IRQ* _pitIrq;
};
