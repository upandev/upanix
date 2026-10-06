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

#include <stdint.h>
#include <time.h>

class TscClock {
private:
  static TscClock* _instance;
  TscClock();
  ~TscClock() = default;

public:
  TscClock(const TscClock&) = delete;
  TscClock& operator=(const TscClock&) = delete;

  static void initialize();
  static TscClock& instance();

  uint64_t rdtsc() const;
  uint64_t rdtsc(uint64_t duration) const;
  //duration in micro seconds
  time_t duration(uint64_t start) const;
  time_t duration(uint64_t start, uint64_t end) const;
  uint64_t durationToCycles(uint64_t duration) const;
  void busyWait(uint64_t timeInMicroSec) const;

  time_t bootTime() const { return _bootTime; }
  time_t currentTime() const;

private:
  uint64_t _frequencyHz;
  uint64_t _bootTsc;
  time_t _bootTime;
};