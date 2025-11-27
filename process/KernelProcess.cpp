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
#include <KernelProcess.h>
#include <UserManager.h>
#include <ProcessManager.h>
#include <KernelThread.h>
#include <GraphicsVideo.h>
#include <DMM.h>
#include <PortCom.h>
#include <KernelRootProcess.h>

KernelProcess::KernelProcess(const upan::string& name, uintptr_t taskAddress, int parentID, bool isFGProcess, bool isCoreProcess, const upan::vector<uintptr_t>& params)
  : AutonomousProcess(name, parentID, isFGProcess), _graphicsContext(nullptr), _isCoreProcess(isCoreProcess) {
  _processBase = 0;

  _stackBlockId = SchedulableProcess::Common::AllocateKernelStackSpace();
  _tls.reset(new ThreadLocalStorage(_processID, false, pml4Table(), KernelRootProcess::Instance().tlsp(), 0x3));

  const auto noOfStackParams = params.size() > PROCESS_ARGUMENTS_ON_REGS_X86_64 ? params.size() - PROCESS_ARGUMENTS_ON_REGS_X86_64 : 0;

  const uint64_t stackTop = SchedulableProcess::Common::KernelVirtualStackBase(_stackBlockId) - (noOfStackParams + 1) * sizeof(uintptr_t);

  //the first stack param is return address - which is pushed as per x86 64 ABI
  for(int i = 0; i < noOfStackParams; ++i) {
    ((uintptr_t*)stackTop)[i] = params[i + PROCESS_ARGUMENTS_ON_REGS_X86_64];
  }

  if (params.size() >= 1) _taskContext.rdi = params[0];
  if (params.size() >= 2) _taskContext.rsi = params[1];
  if (params.size() >= 3) _taskContext.rdx = params[2];
  if (params.size() >= 4) _taskContext.rcx = params[3];
  if (params.size() >= 5) _taskContext.r8 = params[4];
  if (params.size() >= 6) _taskContext.r9 = params[5];

  _taskContext.interruptState.cs = SYS_CODE_SELECTOR;
  _taskContext.interruptState.rip = taskAddress;
  _taskContext.interruptState.ss = SYS_DATA_SELECTOR;
  _taskContext.interruptState.rsp = stackTop;
  _taskContext.interruptState.rflags = 0x202;
  _userID = ROOT_USER_ID ;

  auto parentProcess = ProcessManager::Instance().GetSchedulableProcess(parentID);
  parentProcess.ifPresent([this](SchedulableProcess& p) { p.addChildProcessID(_processID); });
}

KernelThread& KernelProcess::CreateThread(uintptr_t threadCaller, uintptr_t entryAddress, void* arg, bool joinable) {
  return *new KernelThread(*this, threadCaller, entryAddress, arg, joinable);
}

void KernelProcess::DeallocateResources() {
  SchedulableProcess::Common::DeallocateKernelStackSpace(_stackBlockId);
  DeAllocateGUIFramebuffer();
  //explicitly pass gc param because the underlying Destroy method
  //can't use process-lookup-table to get current process as that will be pointing
  //to KernelRootProcess when running in kernel-mode at the time of destroying this kernel process
  upanui::GraphicsContext::Destroy(_graphicsContext);
}

DMM& KernelProcess::dmm() {
  return KernelDMM::Instance();
}

void KernelProcess::initGuiFrame() {
  if (_frame.get() == nullptr) {
    FrameBufferInfo frameBufferInfo;
    const auto f = MultiBoot::Instance().VideoFrameBufferInfo();
    frameBufferInfo._pitch = f->_pitch;
    frameBufferInfo._width = f->_width;
    frameBufferInfo._height = f->_height;
    frameBufferInfo._bpp = f->_bpp;
    frameBufferInfo._frameBuffer = (uint32_t*)GraphicsVideo::Instance().allocateFrameBuffer();
    upanui::FrameBuffer frameBuffer(frameBufferInfo);
    upanui::Viewport viewport(0, 0, frameBufferInfo._width, frameBufferInfo._height);
    auto rootFrame = new RootFrame(frameBuffer, viewport);
    rootFrame->enableDoubleBuffer(true);
    _frame.reset(rootFrame);

    //let processes write to stdout of the parent for debugging purpose.
    //_iodTable.setupNullStdOut();

    GraphicsVideo::Instance().addFGProcess(_processID);
  }
}

void KernelProcess::DeAllocateGUIFramebuffer() {
  if (_frame.get() != nullptr) {
    KernelDMM::Instance().free((uint64_t)_frame->frameBuffer().buffer());
    GraphicsVideo::Instance().removeFGProcess(_processID);
  }
}

void KernelProcess::setupSignalStackFrame(const struct sigaction& action, const Signal& signal) {
  SchedulableProcess::Common::SetupKernelSignalStackFrame(*this, _stackBlockId, action, signal);
}
