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
#include <ProcessStat.h>
#include <ProcessManager.h>

ProcessStat::ProcessStat() : _previousCaptureTime(-1), _previousCaptureMode(CaptureMode::NA),
  _rusage({{0, 0}, {0, 0}}),
  _childrenRusage({{0, 0}, {0, 0}}) {
}

void ProcessStat::captureTime(CaptureMode captureMode) {
  ProcessSwitchLock lock;
  if (captureMode == _previousCaptureMode) {
    return;
  }

  if (_previousCaptureMode == CaptureMode::NA) {
    _previousCaptureTime = btime();
    _previousCaptureMode = captureMode;
    return;
  }

  time_t now = btime();
  time_t delta = now - _previousCaptureTime;

  struct timeval& tv = _previousCaptureMode == CaptureMode::USER_MODE ? _rusage.ru_utime : _rusage.ru_stime;
  tv.tv_sec += (delta / 1000);
  tv.tv_usec += (delta % 1000) * 1000;

  _previousCaptureTime = now;
  _previousCaptureMode = captureMode;
}

void ProcessStat::addChildRUsage(ProcessStat& childRUsage) {
  time_t childUTime = childRUsage._rusage.ru_utime.tv_sec * 1000 + childRUsage._rusage.ru_utime.tv_usec / 1000;
  time_t totalUTime = _childrenRusage.ru_utime.tv_sec * 1000 + _childrenRusage.ru_utime.tv_usec / 1000;
  totalUTime += childUTime;

  time_t childSTime = childRUsage._rusage.ru_stime.tv_sec * 1000 + childRUsage._rusage.ru_stime.tv_usec / 1000;
  time_t totalSTime = _childrenRusage.ru_stime.tv_sec * 1000 + _childrenRusage.ru_stime.tv_usec / 1000;
  totalSTime += childSTime;

  _childrenRusage.ru_utime.tv_sec = totalUTime / 1000;
  _childrenRusage.ru_utime.tv_usec = (totalUTime % 1000) * 1000;

  _childrenRusage.ru_stime.tv_sec = totalSTime / 1000;
  _childrenRusage.ru_stime.tv_usec = (totalSTime % 1000) * 1000;
}