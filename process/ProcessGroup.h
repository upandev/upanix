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
#ifndef _PROCESS_GROUP_H_
#define _PROCESS_GROUP_H_

#include <Global.h>
#include <mutex.h>
#include <list.h>

class ProcessGroup
{
  public:
    explicit ProcessGroup(bool isFGProcessGroup);
    ~ProcessGroup() = default;

    int Id() const { return _id; }
    int Size() const { return _iProcessCount; }
    bool IsFGProcess(int iProcessID) const;
    bool IsFGProcessGroup() const;

    void PutOnFGProcessList(int iProcessID);
    bool IsOnFGProcessList(int iProcessID);
    void RemoveFromFGProcessList(int iProcessID);
    void AddProcess();
    void RemoveProcess();
    void SwitchToFG();
    int GetFGProcessID();

    static ProcessGroup* GetFGProcessGroup() { return _fgProcessGroup; }

  private:
    int             _id;
    int	            _iProcessCount;
    upan::list<int> _fgProcessList;

    static int _idSeq;
    static ProcessGroup* _fgProcessGroup;
};

#endif
