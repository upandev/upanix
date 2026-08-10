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

#include <TRB.h>
class InputContext;

class EventResult
{
public:
  explicit EventResult(int pid) : _pid(pid) {}
  virtual ~EventResult() = default;

  int Pid() const { return _pid; }
  const EventTRB& Result() const { return _result; }

  virtual void Consume(const EventTRB& r) = 0;
protected:
  int      _pid;
  EventTRB _result;
};

class WaitedEventResult : public EventResult
{
public:
  explicit WaitedEventResult(int pid) : EventResult(pid) { }
  void Consume(const EventTRB& r) override;
};

class InterruptEventResult : public EventResult
{
public:
  InterruptEventResult(InputContext& context, int pid, uint64_t dataAddress) : EventResult(pid), _context(context), _dataAddress(dataAddress) { }
  void Consume(const EventTRB &r) override;
  uint64_t InterruptDataAddress() const { return _dataAddress; }

private:
  InputContext& _context;
  uint64_t _dataAddress;
};