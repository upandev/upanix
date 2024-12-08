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

#include <mosstd.h>
#include <set.h>
#include <map.h>
#include <option.h>
#include <uniq_ptr.h>
#include <atomicop.h>
#include <FileOperations.h>
#include <ProcessConstants.h>
#include <IODescriptorTable.h>
#include <ProcessGroup.h>
#include <Process.h>
#include <InterruptHandlers.h>
#include <ThreadLocalStorage.h>

class SchedulableProcess : public Process
{
public:
  typedef upan::set<int> ProcessIDs;

public:
  SchedulableProcess(const upan::string& name, int parentID, bool isFGProcess);
  virtual ~SchedulableProcess() = 0;

  bool isChildThread() const override {
    return _processID != _mainThreadID;
  }

  virtual void onLoad() = 0;

  //thread synchronization mutex
  virtual upan::option<upan::mutex&> pageAllocMutex() {
    return upan::option<upan::mutex&>::empty();
  }

  virtual SchedulableProcess& forSchedule() {
    throw upan::exception(XLOC, "forSchedule unsupported");
  }

  bool isFGProcessGroup() const override {
    return _processGroup->IsFGProcessGroup();
  }

  void Load(TaskContext& taskState);
  void Store(const TaskContext& taskState);
  void Destroy();
  void Release();
  void switchPageTable() const;
  bool handlePageFault(uint64_t faultyAddress);

  FILE_USER_TYPE fileUserType(const FileNode&) const override;
  bool hasFilePermission(const FileNode&, byte mode) const override;

  uint64_t getProcessBase() const override { return _processBase; }
  upan::string name() const { return _name; }

  int processID() const override { return _processID; }
  int parentProcessID() const override { return _parentProcessID; }
  int mainThreadID() const { return _mainThreadID; }

  void setParentProcessID(int parentProcessID) { _parentProcessID = parentProcessID; }

  PROCESS_STATUS status() const override { return _status; }
  PROCESS_STATUS setStatus(PROCESS_STATUS status) override {
    return (PROCESS_STATUS) upan::atomic::op::swap((__volatile__ uint32_t &) (_status), static_cast<int>(status));
  }

  int driveID() const override { return _driveID; }
  void setDriveID(int driveID) override { _driveID = driveID; }

  int userID() const override { return _userID; }

  ProcessGroup* processGroup() override { return _processGroup; }
  void setProcessGroup(ProcessGroup* processGroup) override { _processGroup = processGroup; }

  ProcessStateInfo& stateInfo() override { return _stateInfo; }

  FileNodeRef pwd() const override { return _pwd; }
  void pwd(const FileNodeRef& pwd) { _pwd = pwd; }

  const ProcessIDs& childProcessIDs() const { return _childProcessIDs; }
  void addChildProcessID(int pid) { _childProcessIDs.insert(pid); }
  void removeChildProcessID(int pid) { _childProcessIDs.erase(pid); }

  void yield() override;
  bool CanPreempt();

private:
  static int _nextPid;

protected:
  virtual void DeallocateResources() = 0;
  virtual void DestroyThreads() {
  }

  class Common {
  public:
    static void SetStackPDTable(uint64_t* pml4Table, uint64_t value);
    static void SwitchStack(uint64_t* pml4Table, uint64_t stackPDAddress);
    static uint64_t AllocateStackSpace();
    static void DeAllocateStackSpace(uint64_t stackPDAddress);

    static uint64_t KernelVirtualStackBase(int stackBlockId);
    static int AllocateKernelStackSpace();
    static void DeallocateKernelStackSpace(int stackBlockId);
  };

protected:
  upan::string _name;
  int _processID;
  int _mainThreadID;
  int _parentProcessID;
  uint64_t _processBase;
  PROCESS_STATUS _status;
  int _driveID;
  int _userID;
  uint32_t _runTick;
  ProcessStateInfo& _stateInfo;
  TaskContext _taskContext;
  FileNodeRef _pwd;
  //this is managed like a shared_ptr
  ProcessGroup* _processGroup;

  ProcessIDs _childProcessIDs;
  upan::uniq_ptr<ThreadLocalStorage> _tls;
};