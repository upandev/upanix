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

#include <atomicop.h>
#include <pair.h>
#include "vector.h"
#include "mosstd.h"

class IRQ;

class ProcessStateInfo {
public:
  typedef enum {
    NO_ERROR,
    INTERRUPTED,
    TIMEOUT,
  } Error;
  ProcessStateInfo();

  uint32_t SleepTime() const { return _sleepTime; }
  void SleepTime(const uint32_t s) { _sleepTime = s; }

  int WaitChildProcId() const { return _waitChildProcId; }
  void WaitChildProcId(const int id) { _waitChildProcId = id; }

  int WaitQueueId() const { return _waitQueueId; }
  void WaitQueueId(int id) { _waitQueueId = id; }

  int WaitQueueSpaceId() const { return _waitQueueSpaceId; }
  void WaitQueueSpaceId(int space) { _waitQueueSpaceId = space; }

  const upan::vector<io_descriptor>& GetIODescriptors() const {
    return _ioDescriptors;
  }
  void SetIODescriptors(const upan::vector<io_descriptor>& ioDescriptors) {
    _ioDescriptors = ioDescriptors;
  }

  bool IsKernelServiceComplete() const { return _kernelServiceComplete; }
  void KernelServiceComplete(const bool v) { _kernelServiceComplete = v; }

  const IRQ* Irq() const { return _irq; }
  void Irq(const IRQ* irq) { _irq = irq; }

  bool IsEventCompleted();
  void EventCompleted();

  void WaitOnLock(upan::atomic::integral<int>* lock, int oldVal, int newVal);
  bool IsWaitOnLockCompleted();

  Error getError() const { return _error; }
  void setError(Error error) { _error = error; }

private:
  time_t         _sleepTime;
  const IRQ*     _irq;
  int            _waitChildProcId;
  int            _waitQueueId;
  int            _waitQueueSpaceId;
  upan::atomic::integral<bool> _eventCompleted;
  bool           _kernelServiceComplete ;
  upan::vector<io_descriptor> _ioDescriptors;
  Error          _error;

  upan::atomic::integral<int>* _waitLock;
  int _newVal;
  int _oldVal;
};