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

#include <ProcessStateInfo.h>
#include <IrqManager.h>
#include <ProcessConstants.h>

ProcessStateInfo::ProcessStateInfo() :
        _sleepTsc(0),
        _irq(&StdIRQ::Instance().NO_IRQ),
        _waitChildProcId(NO_PROCESS_ID),
        _waitQueueId(0),
        _eventCompleted(false),
        _kernelServiceComplete(false),
        _waitLock(nullptr), _newVal(0), _oldVal(0), _isForkReady(false) {
}

bool ProcessStateInfo::IsEventCompleted() {
  for (int i = 0; i < 10; ++i) {
    if (_eventCompleted.get()) {
      _eventCompleted.set(false);
      return true;
    }
  }
  return false;
}

void ProcessStateInfo::EventCompleted() {
  _eventCompleted.set(true);
}

void ProcessStateInfo::WaitOnLock(upan::atomic::integral<int> *lock, int oldVal, int newVal) {
  _waitLock = lock;
  _oldVal = newVal;
  _newVal = newVal;
}

bool ProcessStateInfo::IsWaitOnLockCompleted() {
  if (!_waitLock) {
    return true;
  }

  const int prevVal = _waitLock->compare_set(_oldVal, _newVal);
  if (prevVal == _oldVal || prevVal == _newVal) {
    return true;
  }

  if (!isprocessalive(prevVal)) {
    _waitLock->set(_newVal);
    return true;
  }

  return false;
}
