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
#include <ProcessGroup.h>
#include <ProcessManager.h>
#include <MemManager.h>
#include <DMM.h>
#include <exception.h>

int ProcessGroup::_idSeq = 0;
ProcessGroup* ProcessGroup::_fgProcessGroup = nullptr;

ProcessGroup::ProcessGroup(bool isFGProcessGroup) 
  : _id(++_idSeq), _iProcessCount(0)
{
  if(isFGProcessGroup)
    _fgProcessGroup = this;
}

void ProcessGroup::PutOnFGProcessList(int iProcessID)
{
  _fgProcessList.push_front(iProcessID);
}

bool ProcessGroup::IsOnFGProcessList(int iProcessID) {
  auto i = upan::find_if( _fgProcessList.begin(), _fgProcessList.end(), [iProcessID](int i){ return i == iProcessID; });
  return i != _fgProcessList.end();
}

void ProcessGroup::RemoveFromFGProcessList(int iProcessID)
{
  _fgProcessList.erase(iProcessID);
}

void ProcessGroup::AddProcess()
{
	++_iProcessCount;
}

void ProcessGroup::RemoveProcess()
{
	--_iProcessCount;
}

void ProcessGroup::SwitchToFG()
{
  _fgProcessGroup = this;
}

bool ProcessGroup::IsFGProcessGroup() const
{
	return this == _fgProcessGroup;
}

bool ProcessGroup::IsFGProcess(int iProcessID) const
{
  if(_fgProcessList.empty())
    return false;
  return *_fgProcessList.begin() == iProcessID;
}

int ProcessGroup::GetFGProcessID() {
  if(_fgProcessList.empty())
    return NO_PROCESS_ID;
  return *_fgProcessList.begin();
}