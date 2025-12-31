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

#include <AutonomousProcess.h>
#include <Thread.h>
#include <StreamBufferDescriptor.h>
#include <ProcessManager.h>
#include <GraphicsVideo.h>
#include <FSDeviceManager.h>
#include <RedirectDescriptor.h>

AutonomousProcess::AutonomousProcess(const upan::string& name, int parentID, bool isFGProcess)
  : SchedulableProcess(name, parentID, isFGProcess), _nextThreadIt(_threadSchedulerList.begin()),
    _uiType(Process::UIType::NA), _uiKeyboardEventStreamFD(nullptr), _uiMouseEventStreamFD(nullptr),
    _isGuiBase(false), _iodTable(_processID), _alarmTime(0), _alarmExpiry(0) {

  auto& parentIODTable = ProcessManager::Instance().GetProcess(parentID)
          .valueOrThrow(XLOC, "failed to create process as parent process not found")
          .iodTable();
  _iodTable.allocate([&](int fd) { return new RedirectDescriptor(_processID, fd, parentIODTable.get(IODescriptorTable::STDIN)); });
  _iodTable.allocate([&](int fd) { return new RedirectDescriptor(_processID, fd, parentIODTable.get(IODescriptorTable::STDOUT)); });
  _iodTable.allocate([&](int fd) { return new RedirectDescriptor(_processID, fd, parentIODTable.get(IODescriptorTable::STDERR)); });
  _iodTable.allocate([&](int fd) { return new RedirectDescriptor(_processID, fd, parentIODTable.get(IODescriptorTable::TERMINAL_MASTER)); });
  _iodTable.allocate([&](int fd) { return new RedirectDescriptor(_processID, fd, parentIODTable.get(IODescriptorTable::KSYSLOG)); });
}

SchedulableProcess& AutonomousProcess::forSchedule() {
  if (_status == TERMINATED || _status == RELEASED || _status == STOPPED) {
    return *this;
  }

  if (_nextThreadIt == _threadSchedulerList.end()) {
    _nextThreadIt = _threadSchedulerList.begin();
    return *this;
  }

  while (_nextThreadIt != _threadSchedulerList.end()) {
    auto curThreadIt = _nextThreadIt++;
    Thread& thread = **curThreadIt;

    if (thread.status() == RELEASED || thread.status() == TERMINATED) {
      if (!thread.isJoinable() || thread.status() == RELEASED) {
        _threadSchedulerList.erase(curThreadIt);
        ProcessManager::Instance().RemoveFromProcessMap(thread);
        delete &thread;
      }
    } else {
      return thread;
    }
  }

  return *this;
}

//TODO: use a process level lock instead of global process switch lock
void AutonomousProcess::addToThreadScheduler(Thread& thread) {
  ProcessSwitchLock lock;
  _threadSchedulerList.push_back(&thread);
  thread.setStatus(RUN);
}

// 1. KernelProcess can have child processes of type either KernelProcess or UserProcess or KernelThread
// 2. UserProcess can have a child UserProcess or UserThread
// 3. If a parent process (Kernel or User) is terminated then
//   a. all terminated child processes are released and all non-terminated child processes are redirected to the parent of the current process
//   b. all child threads are destroyed and released
void AutonomousProcess::Destroy() {
  setStatus(TERMINATED);
  captureTime(ProcessStat::CaptureMode::NA);

  DestroyThreads();

  // child processes of this process (if any) will be redirected to the parent of the current process
  auto parentProcess = ProcessManager::Instance().GetSchedulableProcess(_parentProcessID);
  parentProcess.ifPresent([&](SchedulableProcess& p) { processStat().addChildRUsage(_processStat); });

  for(auto pid : _childProcessIDs) {
    ProcessManager::Instance().GetSchedulableProcess(pid).ifPresent([&parentProcess](SchedulableProcess &p) {
      if (p.status() == TERMINATED) {
        p.Release();
      } else {
        parentProcess.ifPresent([&p](SchedulableProcess& pp) {
          p.setParentProcessID(pp.processID());
          pp.addChildProcessID(p.processID());
        });
      }
    });
  }

  // Deallocate Resources
  Deallocate();

  // Release From Process Group
  _processGroup->RemoveFromFGProcessList(_processID);
  _processGroup->RemoveProcess();

  if(_processGroup->Size() == 0) {
    delete _processGroup;
  }

  dmm().releaseLocks(_processID);
  pageAllocMutex().ifPresent([this](upan::mutex& m) { m.unlock(_processID); });

  //TODO: release all the mutex held by the process or an individual thread

  if(_parentProcessID == NO_PROCESS_ID) {
    Release();
  } else {
    if (!parentProcess.isEmpty()) {
      auto signalHandler = parentProcess.value().getSignalAction(SIGCHLD);
      if (signalHandler.isEmpty() || isignoreaction(&signalHandler.value()) || isdefaultaction(&signalHandler.value())) {
        Release();
      } else {
        union sigval sigval {_processID };
        ProcessManager::Instance().SendSignal(_parentProcessID, SIGCHLD, &sigval);
        if (signalHandler.value().sa_flags & SA_NOCLDWAIT) {
          Release();
        }
      }
    }
  }

  if (!ownerControllingTerminal().isEmpty()) {
    FSDeviceManager::Instance().removeDevice(ownerControllingTerminal()->path());
  }
}

void AutonomousProcess::DestroyThreads() {
  // child threads should be destroyed
  // threads must be destroyed before dealing with child processes because
  // child processes if any of a thread will be redirected to current process (main thread)
  for(auto t : _threadSchedulerList) {
    if (t->status() != TERMINATED && t->status() != RELEASED) {
      t->stateInfo().setExitStatusNormal(0);
      t->Destroy();
    }
    t->Release();
    ProcessManager::Instance().RemoveFromProcessMap(*t);
    delete t;
  }
  _threadSchedulerList.clear();
}

void AutonomousProcess::sendKeyboardDataToControllingTerminal(const upanui::KeyboardData& data) {
  const auto ch = (uint8_t)upanui::KeyboardMapper::Instance().resolveKey(data);
  if (ch == Keyboard_CTRL_C) {
    kill(_processID, SIGINT);
  } else if (ch != Keyboard_NA_CHAR) {
    iodTable().get(IODescriptorTable::TERMINAL_MASTER)->write((void*)&ch, 1);
  }
}

void AutonomousProcess::dispatchKeyboardData(const upanui::KeyboardData& data) {
  switch (_uiType) {
    case Process::REDIRECT_TTY:
    case Process::TTY:
      sendKeyboardDataToControllingTerminal(data);
      break;

    case Process::GUI: {
      if (ownerControllingTerminal().isEmpty()) {
        const auto ch = (uint8_t) upanui::KeyboardMapper::Instance().resolveKey(data);
        if (ch == Keyboard_CTRL_C) {
          kill(_processID, SIGINT);
        } else {
          _uiKeyboardEventStreamFD->write((void*) &data, sizeof(upanui::KeyboardData));
        }
      } else {
        sendKeyboardDataToControllingTerminal(data);
      }
    }
    break;

    case Process::NA:
      break;
  }
}

void AutonomousProcess::dispatchMouseData(const upanui::MouseData& mouseData) {
  if (_uiType == Process::GUI) {
    _uiMouseEventStreamFD->write((void*)&mouseData, sizeof(upanui::MouseData));
  }
}

void AutonomousProcess::setupAsTtyProcess() {
  if (_uiType != Process::UIType::NA) {
    throw upan::exception(XLOC, "Process %d is already initialized with UIType %d", _processID, _uiType);
  }
  iodTable().setupStreamedStdio();
  _uiType = Process::UIType::TTY;
}

void AutonomousProcess::setupAsRedirectTtyProcess() {
  if (_uiType != Process::UIType::NA) {
    throw upan::exception(XLOC, "Process %d is already initialized with UIType %d", _processID, _uiType);
  }
  _uiType = Process::UIType::REDIRECT_TTY;
}

void AutonomousProcess::setupAsGuiProcess(int fdList[]) {
  if (_uiType != Process::UIType::NA) {
    throw upan::exception(XLOC, "Process %d is already initialized with UIType %d", _processID, _uiType);
  }

  _uiType = Process::UIType::GUI;
  _uiKeyboardEventStreamFD = iodTable().allocate([&](int fd) {
    return new StreamBufferDescriptor(_processID, fd, 4096, O_NONBLOCK | O_RDWR);
  });

  _uiMouseEventStreamFD = iodTable().allocate([&](int fd) {
    return new StreamBufferDescriptor(_processID, fd, 4096, O_NONBLOCK | O_RDWR);
  });

  fdList[0] = _uiKeyboardEventStreamFD->id();
  fdList[1] = _uiMouseEventStreamFD->id();
}

void AutonomousProcess::setGuiBase(bool val) {
  if (getGuiFrame().isEmpty()) {
    throw upan::exception(XLOC, "can not set non-GUI process as GUI base");
  }

  if (_isGuiBase == val) {
    return;
  }

  ProcessSwitchLock pLock;
  _isGuiBase = val;
  if (_isGuiBase) {
    GraphicsVideo::Instance().addGuiBase(_processID);
  } else {
    GraphicsVideo::Instance().removeGuiBase(_processID);
  }
  getGuiFrame().value().touch();
}

//this is always called via ProcessManager::SetSignalAction - which locks context switch to protect the action map access across context switches
void AutonomousProcess::setSignalAction(SIGNAL signo, const struct sigaction* newact, struct sigaction* oldact) {
  const Signal signal(signo);
  if (!signal.isMaskable()) {
    if (newact && isignoreaction(newact)) {
      throw upan::exception(XLOC, "can't register signal handler for non-maskable signal: %d", signo);
    }
  }

  if (newact) {
    if ((newact->sa_flags & SA_SIGINFO) && !(uint64_t) newact->sa_sigaction) {
      throw upan::exception(XLOC, "sa_sigaction must be set for if SA_SIGINFO flag is set for signal: %d", signo);
    }
    if (!(uint64_t) newact->sa_sigaction && !(uint64_t) newact->sa_handler) {
      throw upan::exception(XLOC, "sa_handler must be set for signal: %d", signo);
    }
  }
  auto i = _signalHandler.find(signo);
  if (i != _signalHandler.end()) {
    if (oldact) {
      *oldact = i->second;
    }

    if (!newact) {
      _signalHandler.erase(i);
    } else {
      i->second = *newact;
    }
  } else {
    if (oldact) {
      oldact->sa_flags = 0;
      oldact->sa_handler = nullptr;
      sigemptyset(&oldact->sa_mask);
    }

    if (newact) {
      _signalHandler.insert(SIGNAL_ACTION_MAP::value_type(signo, *newact));
    }
  }
}

upan::option<struct sigaction&> AutonomousProcess::getSignalAction(SIGNAL signo) {
  auto i = _signalHandler.find(signo);
  if (i == _signalHandler.end()) {
    return upan::option<struct sigaction&>::empty();
  }
  return upan::option<struct sigaction&>(i->second);
}

void AutonomousProcess::setSID() {
  if (_processGroup) {
    auto isFGProcess = _processGroup->IsOnFGProcessList(_processID);
    auto isFGProcessGroup = _processGroup->IsFGProcessGroup();

    if (isFGProcess) {
      _processGroup->RemoveFromFGProcessList(_processID);
    }
    _processGroup->RemoveProcess();
    if (_processGroup->Size() == 0) {
      delete _processGroup;
    }

    _processGroup = new ProcessGroup(isFGProcessGroup);
    _processGroup->AddProcess();
    if (isFGProcess) {
      _processGroup->PutOnFGProcessList(_processID);
    }
  }

  if (!_terminalDevice.isEmpty()) {
    FSDeviceManager::Instance().removeDevice(_terminalDevice->path());
    _terminalDevice.reset(nullptr);
  }
}

upan::shared_ptr<FSTerminalDevice> AutonomousProcess::controllingTerminal() {
  if (_terminalDevice.isEmpty()) {
    auto parentProcess =  ProcessManager::Instance().GetProcess(_parentProcessID);
    if (parentProcess.isEmpty()) {
      return {};
    }
    return parentProcess.value().controllingTerminal();
  }
  return _terminalDevice;
}

upan::pair<uint32_t, time_t> AutonomousProcess::setAlarm(uint32_t seconds) {
  auto at = _alarmTime;
  auto ae = _alarmExpiry;
  if (seconds) {
    _alarmTime = seconds;
    _alarmExpiry = btime() + seconds * 1000;
  } else {
    _alarmTime = 0;
    _alarmExpiry = 0;
  }
  return { at, ae };
}
