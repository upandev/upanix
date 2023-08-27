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
#include <KeyboardHandler.h>
#include "ProcessManager.h"

[[noreturn]] void schedule_runner_process() {
  while(true) {
    ProcessManager::Instance().GetCurrentPAS().yield();
  }
}

void KernelRootProcess::createScheduleRunner() {
  ProcessManager::Instance().CreateKernelProcess(".sr", (uintptr_t) &schedule_runner_process,
                                                                 ProcessManager::GetCurrentProcessID(), false, upan::vector<uintptr_t>());
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
  const auto ch = (uint8_t)KeyboardHandler::Instance().mapToTTYKey(data);
  iodTable().get(IODescriptorTable::STDIN).write((void*)&ch, 1);
}

void KernelRootProcess::dispatchMouseData(const upanui::MouseData& mouseData) {
  //no-op
}

void KernelRootProcess::setEnv(const upan::string& key, const upan::string& value) {
}

upan::option<upan::string> KernelRootProcess::getEnv(const upan::string& key) {
  return upan::option<upan::string>::empty();
}