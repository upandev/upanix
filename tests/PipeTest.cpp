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

#include <PipeTest.h>
#include <stdlib.h>
#include <exception.h>
#include <fs.h>
#include <FSDeviceManager.h>
#include <FSPipeDevice.h>

void PipeTest::testUnamedPipe() {
  int pipeFd[2];
  if (pipe(pipeFd)) {
    throw upan::exception(XLOC, "create pipe() failed");
  }
  const char message[] = "Hello Pipe!";

  write(pipeFd[1], message, strlen(message));
  char buf[128];

  int n = read(pipeFd[0], buf, sizeof(buf));
  buf[n] = '\0';
  if (strcmp(buf, message) != 0) {
    throw upan::exception(XLOC, "pipe message mismatch: %s", buf);
  }

  int pipeDevCount = 0;
  FSDeviceManager::Devices& devices = FSDeviceManager::Instance().devices();
  for(FSDeviceManager::Devices::iterator it = devices.begin(); it != devices.end(); ++it) {
    if (it->first.find("/dev/upipe") >= 0) {
      //printf("\n Pipe device found: %s", it->first.c_str());
      ++pipeDevCount;
    }
  }

  close(pipeFd[0]);
  int pipeDevCount1 = 0;
  for(FSDeviceManager::Devices::iterator it = devices.begin(); it != devices.end(); ++it) {
    if (it->first.find("/dev/upipe") >= 0) {
      ++pipeDevCount1;
    }
  }

  if (pipeDevCount != pipeDevCount1) {
    throw upan::exception(XLOC, "pipe device count mismatch: %d != %d", pipeDevCount, pipeDevCount1);
  }

  close(pipeFd[1]);

  int pipeDevCount2 = 0;
  for(FSDeviceManager::Devices::iterator it = devices.begin(); it != devices.end(); ++it) {
    if (it->first.find("/dev/upipe") >= 0) {
      ++pipeDevCount2;
    }
  }

  if (pipeDevCount2 != (pipeDevCount - 1)) {
    throw upan::exception(XLOC, "pipe device is not deleted upon pipe desc closure");
  }
}

void PipeTest::testNamedPipe() {
  const upan::string pipeName("/dev/pipe_test");
  int pipeFd[2];
  if (mkfifo(pipeName.c_str(), pipeFd)) {
    throw upan::exception(XLOC, "mkfifo failed");
  }
  const char message[] = "Hello Named Pipe!";

  write(pipeFd[1], message, strlen(message));
  char buf[128];

  int n = read(pipeFd[0], buf, sizeof(buf));
  buf[n] = '\0';
  if (strcmp(buf, message) != 0) {
    throw upan::exception(XLOC, "pipe message mismatch: %s", buf);
  }

  {
    auto device = FSDeviceManager::Instance().getPipeDevice(pipeName);
    if (device.isEmpty()) {
      throw upan::exception(XLOC, "pipe device not found for %s", pipeName.c_str());
    }
  }

  struct stat pstat;
  if (stat(pipeName.c_str(), &pstat)) {
    throw upan::exception(XLOC, "stat failed for pipe %s", pipeName.c_str());
  }

  if (!(pstat.st_mode & S_IFIFO)) {
    throw upan::exception(XLOC, "pipe %s is not a fifo", pipeName.c_str());
  }

  close(pipeFd[0]);

  {
    auto device = FSDeviceManager::Instance().getPipeDevice(pipeName);
    if (device.isEmpty()) {
      throw upan::exception(XLOC, "pipe device not found for %s", pipeName.c_str());
    }
  }

  if (stat(pipeName.c_str(), &pstat)) {
    throw upan::exception(XLOC, "stat failed for pipe %s", pipeName.c_str());
  }

  if (!(pstat.st_mode & S_IFIFO)) {
    throw upan::exception(XLOC, "pipe %s is not a fifo", pipeName.c_str());
  }

  close(pipeFd[1]);

  if (stat(pipeName.c_str(), &pstat) == 0) {
    throw upan::exception(XLOC, "pipe %s still exists after closing both fds", pipeName.c_str());
  }

  {
    auto device = FSDeviceManager::Instance().getPipeDevice(pipeName);
    if (!device.isEmpty()) {
      throw upan::exception(XLOC, "pipe device still exists for %s", pipeName.c_str());
    }
  }
}