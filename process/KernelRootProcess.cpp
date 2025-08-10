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
  _scheduleRunnerPid(NO_PROCESS_ID), _processGroup(nullptr) {
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