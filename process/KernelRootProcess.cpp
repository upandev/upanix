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

#include <KernelRootProcess.h>
#include <GraphicsVideo.h>
#include <ProcessManager.h>
#include <Cpu.h>
#include <signal.h>
#include <sys/un.h>
#include <StorageDriveManager.h>
#include <SocketDescriptor.h>
#include <interopc.h>

extern uintptr_t __tdata_start, __tdata_end;
extern uintptr_t __tbss_start, __tbss_end;

[[noreturn]] void schedule_runner_process() {
  while(true) {
    ProcessManager::Instance().Sleep(0);
  }
}

KernelRootProcess& KernelRootProcess::Instance() {
  static KernelRootProcess instance;
  return instance;
}

KernelRootProcess::KernelRootProcess() :
  _iodTable(NO_PROCESS_ID, NO_PROCESS_ID),
  _scheduleRunnerPid(NO_PROCESS_ID), _sysLogDaemonPid(NO_PROCESS_ID), _processGroup(nullptr) {
}

void KernelRootProcess::createScheduleRunner() {
  _scheduleRunnerPid = ProcessManager::Instance().CreateKernelProcess(".sr", (uintptr_t) &schedule_runner_process,
                                                 ProcessManager::GetCurrentProcessID(), false, upan::vector<uintptr_t>());
}

void KernelRootProcess::initTLS() {
  _tlsp.reset(new ThreadLocalSpace());
  //As per kernel.ld script, the .tbss section comes after .tdata
  //The thread local modules (sections) are stored before TCB
  //The initialization of these thread local modules are done backwards starting at TCB
  _tlsp->add((uintptr_t)&__tbss_end - (uintptr_t)&__tdata_start,
             (uintptr_t)&__tdata_end - (uintptr_t)&__tdata_start,
             (uint8_t*)&__tdata_start);

  Cpu::Instance().MSRwrite(MSR_FS_BASE, THREAD_LOCAL_META_SPACE_ADDRESS);

  _tls.reset(new ThreadLocalStorage(NO_PROCESS_ID, pml4Table(), *_tlsp, 0x7));
  _tls->switchSpace();
}

void KernelRootProcess::initGuiFrame() {
  static bool initialized = false;
  if (initialized) {
    return;
  }
  initialized = true;

  RootGUIConsole::Instance().resetFrameBuffer(GraphicsVideo::Instance().allocateFrameBuffer());
  GraphicsVideo::Instance().addFGProcess(NO_PROCESS_ID);
}

void KernelRootProcess::dispatchKeyboardData(const upanui::KeyboardData& data) {
  const auto ch = (uint8_t)upanui::KeyboardMapper::Instance().resolveKey(data);
  int fgPid = NO_PROCESS_ID;
  if (ch == Keyboard_CTRL_C && (fgPid = _processGroup->GetFGProcessID()) != NO_PROCESS_ID) {
    ProcessManager::Instance().SendSignal(fgPid, SIGINT);
  } else {
    iodTable().get(IODescriptorTable::STDIN)->write((void*) &ch, 1);
  }
}

void KernelRootProcess::dispatchMouseData(const upanui::MouseData& mouseData) {
  //no-op
}

void KernelRootProcess::setEnv(const upan::string& key, const upan::string& value) {
}

upan::option<upan::string> KernelRootProcess::getEnv(const upan::string& key) {
  return upan::option<upan::string>::empty();
}

DMM& KernelRootProcess::dmm() {
  return KernelDMM::Instance();
}

static void SysLogDaemon() {
  try {
    KLog::info("syslogd: server starting");
    upan::uniq_ptr<upan::logger> _sysLogger(new upan::logger());
    upan::string _rootDriveName;

    struct sockaddr_un addr;
    addr.sun_family = AF_LOCAL;
    strcpy(addr.sun_path, SYS_LOG_PATH);

    int sd = socket(AF_LOCAL, SOCK_DGRAM, 0);
    if (sd < 0) {
      throw upan::exception(XLOC, "syslogd: socket open failed");
    }

    if (bind(sd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
      throw upan::exception(XLOC, "syslogd: bind failed");
    }
    KLog::info("syslogd: server started");

    //now, connect to syslogd
    auto ioDesc = KernelRootProcess::Instance().iodTable().get(IODescriptorTable::KSYSLOG);
    auto socketDesc = dynamic_cast<SocketDescriptor*>(ioDesc.get());
    if (socketDesc == nullptr) {
      throw upan::exception(XLOC, "syslogd: KSYSLOG is not a socket descriptor");
    }

    socketDesc->connect(*((struct sockaddr*)&addr), sizeof(addr));
    set_syslog_fd(socketDesc->id());
    KLog::info("syslogd: kernel client connected");

    upan::string curRootDriveName = StorageDriveManager::Instance().rootDriveName();
    if (_rootDriveName != curRootDriveName) {
      _sysLogger->closeFile();
      _rootDriveName = curRootDriveName;
      if (!_rootDriveName.empty()) {
        _sysLogger->openFile(_rootDriveName + "@/var/log/sys.log");
      }
    }

    char buf[MAX_LOG_MESSAGE_SIZE];
    while (true) {
      ssize_t len = recv(sd, buf, sizeof(buf), 0);
      if (len < 0) {
        throw upan::exception(XLOC, "syslogd: recv failed");
      }
      buf[len] = '\0';

      upan::string curRootDriveName = StorageDriveManager::Instance().rootDriveName();
      if (_rootDriveName != curRootDriveName) {
        _sysLogger->closeFile();
        _rootDriveName = curRootDriveName;
        if (!_rootDriveName.empty()) {
          _sysLogger->openFile(_rootDriveName + "@/var/log/sys.log");
        }
      }

      _sysLogger->log(buf);
    }

  } catch(const upan::exception& e) {
    KLog::exception(e);
  } catch(...) {
    KLog::critical("unknown error in syslogd");
  }

  ProcessManager_Exit();
}


void KernelRootProcess::startSysLogDaemon() {
  if (_sysLogDaemonPid != NO_PROCESS_ID) {
    throw upan::exception(XLOC, "syslogd already running");
  }

  _sysLogDaemonPid = ProcessManager::Instance().CreateKernelProcess("syslogd", (uintptr_t) &SysLogDaemon,
                                                 NO_PROCESS_ID, false, upan::vector<uintptr_t>());
}

void KernelRootProcess::stopSysLogDaemon() {
  if (_sysLogDaemonPid == NO_PROCESS_ID) {
    throw upan::exception(XLOC, "syslogd not running");
  }

  closelog();
  ProcessManager::Instance().Kill(_sysLogDaemonPid);
  _sysLogDaemonPid = NO_PROCESS_ID;
}