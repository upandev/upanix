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

#include <sys/un.h>
#include <logger.h>
#include <uniq_ptr.h>
#include <string.h>

class KernelSysLog {
private:
  KernelSysLog();

public:
  static KernelSysLog& Instance();
  void start();
  void stop();
  void run();

private:
  int setupServerConnection();
  void setupClientConnection();
  void handleRootDriveChange();

private:
  int _sysLogDaemonPid;
  upan::string _rootDriveName;
  upan::uniq_ptr<upan::logger> _sysLogger;
  struct sockaddr_un _syslogLocalAddr;
};