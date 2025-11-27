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

#include <Thread.h>
#include <AutonomousProcess.h>

Thread::Thread(AutonomousProcess& parent, bool joinable) : SchedulableProcess("", parent.processID(), false), _parent(parent), _joinable(joinable) {
  _name = _parent.name() + "_T" + upan::string::to_string(_processID);
  _processBase = _parent.getProcessBase();
  _userID = _parent.userID();
}

void Thread::Destroy() {
  setStatus(TERMINATED);

  // Deallocate Resources
  Deallocate();

  dmm().releaseLocks(_processID);
  pageAllocMutex().ifPresent([this](upan::mutex& m) { m.unlock(_processID); });

  //TODO: release all the mutex held by the process or an individual thread

  if(_parentProcessID == NO_PROCESS_ID || !_joinable) {
    Release();
  }
}