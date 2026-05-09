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

#include <Global.h>
#include <mosstd.h>
#include <list.h>
#include <map.h>
#include <option.h>
#include <MemConstants.h>
#include <FileSystem.h>
#include <ElfSectionHeader.h>
#include <PIC.h>
#include <mutex.h>
#include <UserProcess.h>
#include <dtime.h>
#include <PIT.h>
#include <FSTerminalDevice.h>
#include <sys/resource.h>

void ProcessManager_Exit(int exitStatus);

class AutonomousProcess;

class ProcessManager
{
  private:
    ProcessManager();

    typedef upan::list<int> WaitQueue;
  public:
    static ProcessManager& Instance()
    {
      static ProcessManager instance;
      return instance;
    }

    upan::option<Process&> GetProcess(int pid);
    upan::option<SchedulableProcess&> GetSchedulableProcess(int pid);
    Process& GetCurrentPAS();
    AutonomousProcess& GetThreadParentProcess(int pid);

    PS* GetProcList(unsigned& uiListSize);
    void FreeProcListMem(PS* procList, unsigned uiListSize);
    void AddToSchedulerList(SchedulableProcess& process);
    void AddToProcessMap(SchedulableProcess& process);
    void RemoveFromProcessMap(SchedulableProcess& process);
    void Sleep(uint32_t sleepTime);
    void WaitOnInterrupt(const IRQ&);
    void WaitOnInterruptWithTimeout(const IRQ& irq, uint32_t timeout);
    int GetCurProcId();
    void WakeUpFromKSWait(int iProcessID);
    bool IsAlive(int pid);
    bool IsChildAlive(int iChildProcessID);
    int CreateKernelProcess(const upan::string& name, const uintptr_t uiTaskAddress, int iParentProcessID,
                            bool isFGProcess, bool isCoreProcess, const upan::vector<uintptr_t>& params);
    int Create(const upan::string& name, int iParentProcessID, byte bIsFGProcess, int iUserID,
               const upan::vector<upan::string>& argv,
               const upan::vector<upan::string>& envp);
    int Fork(UserProcess& mainParent, SchedulableProcess& forkingParent);
    int CreateThreadTask(int parentID, uintptr_t threadCaller, uintptr_t threadEntryAddress, void* arg, bool joinable);
    bool IsDMMOn(int iProcessID);
    int WaitOnChild(int iChildProcessID, int& exitStatus);
    void WaitOnLock(upan::atomic::integral<int>* waitLock, int oldVal, int newVal);
    void WaitOnQueue(int id, upan::mutex &waitMutex, time_t timeoutInMs, bool isKernelSpace);
    void WaitDequeue(int id, bool, bool isKernelSpace);
    void WaitOnIODescriptor(int fd, IODescriptorTable::IO_OP_TYPES waitType, time_t timeoutInMs);
    void WaitOnTerminalIO(const upan::string& path, FSTerminalDevice::TERMINAL_IO_TYPES waitType, time_t timeoutInMs);
    void WaitOnIODescriptors(const upan::vector<IODescriptorTable::io_descriptor>& waitIODescriptors, time_t timeoutInMs);
    void WaitOnKernelService();
    void WaitOnKernelServiceFork();
    bool IsKernelProcess(int iProcessID);
    bool ConditionalWait(const volatile unsigned* registry, unsigned bitPos, bool waitfor);
    void WaitForEvent();
    void EventCompleted(int pid);
    void ContextSwitch(TaskContext &);
    void closeAllFiles(StorageDrive& storageDrive);
    void updateAllIODescriptorRedirections(pid_t pid, int srcFD, const IODescriptor::Ptr& targetDesc);
    void getProcessRUsage(Process& p, RUSAGE_ID who, struct rusage& ru);

    static int GetCurrentProcessID() {
      return _currentProcessID;
    }
    static int UpanixKernelProcessID() {
      return _upanixKernelProcessID;
    }
    static void setUpanixKernelProcessID(int pid) {
      _upanixKernelProcessID = pid;
    }

    static bool EnableTaskSwitch() ;
    static bool DisableTaskSwitch() ;
    static bool IsTaskSwitchEnabled() { return _taskSwitch == 1; }
    static bool IsContextSwitch() { return _contextSwitch; }
    static void SetContextSwitch(bool flag) { _contextSwitch = flag; }

    void MaskSignal(SIG_MASKING_TYPE how, const sigset_t *set, sigset_t *oldset);
    void SendSignal(pid_t pid, SIGNAL signo, const union sigval* value);
    void SetSignalAction(SIGNAL signo, const struct sigaction* newact, struct sigaction* oldact);
    void SignalReturn(SignalTaskContext& signalContext);

    WaitQueue& getWaitQueue(int spaceId, int queueId) {
      return _processWaitQueueMap[spaceId][queueId];
    }

    void stopUserProcesses();
    void stopKernelProcesses();

    time_t SetAlarm(uint32_t seconds);
  private:
    void stopProcesses(upan::function<bool, SchedulableProcess&> stopCondition);
    bool DoPollWait();
    bool IsEventCompleted(int pid);
    ProcessStateInfo& GetProcessStateInfo(int pid);

    typedef upan::map<int, WaitQueue> WaitQueueMap;
    typedef upan::map<int, WaitQueueMap> ProcessWaitQueueMap;
    ProcessWaitQueueMap _processWaitQueueMap;

    ProcessStateInfo _kernelModeStateInfo;
    typedef upan::map<int, SchedulableProcess*> ProcessMap;
    ProcessMap _processMap;

    typedef upan::list<SchedulableProcess*> ProcessSchedulerList;
    ProcessSchedulerList _processSchedulerList;
    ProcessSchedulerList::list_iterator _processSchedulerIt;

    //This is required even before initializing the ProcessManager for fetching
    static int _currentProcessID;
    static int _upanixKernelProcessID;
    static uint32_t _taskSwitch;
    static bool _contextSwitch;
};

class ProcessSwitchLock {
public:
  ProcessSwitchLock() : _isOwner(false) {
    _isOwner = ProcessManager::DisableTaskSwitch();
  }

  ~ProcessSwitchLock() {
    if (_isOwner) {
      ProcessManager::EnableTaskSwitch();
    }
  }

private:
  bool _isOwner;
};
