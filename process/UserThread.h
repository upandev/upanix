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

#include <Thread.h>
#include <AutonomousProcess.h>

class UserThread : public Thread {
public:
  UserThread(AutonomousProcess& parent, uint32_t threadCaller, uint32_t entryAddress, void* arg);

  bool isKernelProcess() const override {
    return false;
  }

  void onLoad() override;

  uint32_t startPDEForDLL() const override {
    return _parent.startPDEForDLL();
  }

  void LoadELFDLL(const upan::string& szDLLName, const upan::string& szJustDLLName) override {
    _parent.LoadELFDLL(szDLLName, szJustDLLName);
  }

  void MapDLLPagesToProcess(uint32_t noOfPagesForDLL, const upan::string& dllName) override {
    return _parent.MapDLLPagesToProcess(noOfPagesForDLL, dllName);
  }

  upan::option<const ProcessDLLInfo&> getDLLInfo(const upan::string& dllName) const override {
    return _parent.getDLLInfo(dllName);
  }
  upan::option<const ProcessDLLInfo&> getDLLInfo(int id) const override {
    return _parent.getDLLInfo(id);
  }

  uintptr_t getAUTAddress() const override {
    return _parent.getAUTAddress();
  }

  void setAUTAddress(uintptr_t addr) {
    _parent.setAUTAddress(addr);
  }

  upan::option<upan::mutex&> heapMutex() override {
    return _parent.heapMutex();
  }

  upan::option<upan::mutex&> pageAllocMutex() override {
    return _parent.pageAllocMutex();
  }

  upan::option<upan::mutex&> dllMutex() override {
    return _parent.dllMutex();
  }

private:
  uint32_t PushProgramInitStackData(uint32_t entryAddress, void* arg);
  void DeAllocateResources() override;

private:
  uint32_t _stackPTEAddress;
};
