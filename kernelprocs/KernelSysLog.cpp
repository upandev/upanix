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

#include <KernelSysLog.h>
#include <ProcessManager.h>
#include <KernelRootProcess.h>
#include <StorageDriveManager.h>
#include <SocketDescriptor.h>
#include <interopc.h>
#include <StorageDrive.h>

KernelSysLog& KernelSysLog::Instance() {
  static KernelSysLog instance;
  return instance;
}

KernelSysLog::KernelSysLog() : _sysLogDaemonPid(NO_PROCESS_ID) {
  _syslogLocalAddr.sun_family = AF_LOCAL;
  strcpy(_syslogLocalAddr.sun_path, SYS_LOG_PATH);
}

int KernelSysLog::setupServerConnection() {
  int sd = socket(AF_LOCAL, SOCK_DGRAM, 0);
  if (sd < 0) {
    throw upan::exception(XLOC, "ksyslogd: socket open failed");
  }

  if (bind(sd, (struct sockaddr *) &_syslogLocalAddr, sizeof(_syslogLocalAddr)) < 0) {
    throw upan::exception(XLOC, "ksyslogd: bind failed");
  }

  return sd;
}

void KernelSysLog::setupClientConnection() {
  auto ioDesc = KernelRootProcess::Instance().iodTable().get(IODescriptorTable::KSYSLOG);
  auto socketDesc = dynamic_cast<SocketDescriptor*>(ioDesc.get());
  if (socketDesc == nullptr) {
    throw upan::exception(XLOC, "ksyslogd: KSYSLOG is not a socket descriptor");
  }

  socketDesc->connect(*((struct sockaddr*)&_syslogLocalAddr), sizeof(_syslogLocalAddr));
  set_syslog_fd(socketDesc->id());
}

void KernelSysLog::handleRootDriveChange() {
  auto curRootDrive = StorageDriveManager::Instance().GetRootDrive();
  if (curRootDrive.isEmpty() && !_rootDriveName.empty()) {
    _sysLogger->closeFile();
    _rootDriveName = "";
  } else if (!curRootDrive.isEmpty() && _rootDriveName != curRootDrive.value().DriveName()) {
    _sysLogger->closeFile();
    _rootDriveName = curRootDrive.value().DriveName();
    _sysLogger->openFile(_rootDriveName + "@/var/log/sys.log");
  }
}

void KernelSysLog::run() {
  KLog::info("ksyslogd: server starting");
  _sysLogger.reset(new upan::logger());

  const int sd = setupServerConnection();
  KLog::info("ksyslogd: server started");

  KLog::info("ksyslogd: connecting to ksyslogd (local) address");
  setupClientConnection();
  KLog::info("ksyslogd: kernel client connected");

  handleRootDriveChange();

  char buf[MAX_LOG_MESSAGE_SIZE];
  while (true) {
    ssize_t len = recv(sd, buf, sizeof(buf), 0);
    if (len < 0) {
      close(sd);
      throw upan::exception(XLOC, "ksyslogd: recv failed");
    }
    buf[len] = '\0';

    handleRootDriveChange();
    _sysLogger->log(buf);
  }
}

static void KernelSysLogProcess(KernelSysLog* kernelSysLog) {
  try {
    kernelSysLog->run();
  } catch(const upan::exception& e) {
    KLog::exception(e);
  } catch(...) {
    KLog::critical("unknown error in ksyslogd");
  }
  printf("\n ksyslogd: aborting!!\n");
  ProcessManager_Exit();
}

void KernelSysLog::start() {
  if (_sysLogDaemonPid != NO_PROCESS_ID) {
    throw upan::exception(XLOC, "ksyslogd already running");
  }

  upan::vector<uintptr_t> params;
  params.push_back((uintptr_t)this);
  _sysLogDaemonPid = ProcessManager::Instance().CreateKernelProcess("ksyslogd", (uintptr_t)&(KernelSysLogProcess),
                                                 NO_PROCESS_ID, false, true, params);
}

void KernelSysLog::stop() {
  if (_sysLogDaemonPid == NO_PROCESS_ID) {
    throw upan::exception(XLOC, "ksyslogd not running");
  }

  closelog();
  _rootDriveName = "";
  _sysLogger.reset(nullptr);

  ProcessManager::Instance().SendSignal(_sysLogDaemonPid, SIGKILL, nullptr);
  _sysLogDaemonPid = NO_PROCESS_ID;
}