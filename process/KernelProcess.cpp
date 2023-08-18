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
#include <InterruptHandlers.h>

KernelProcess::KernelProcess(const upan::string& name, uintptr_t taskAddress, int parentID, bool isFGProcess, const upan::vector<uintptr_t>& params)
  : AutonomousProcess(name, parentID, isFGProcess), _iodTable(_processID, parentID), _graphicsContext(nullptr) {
  _mainThreadID = _processID;
  _processBase = 0;
  _stackBlockId = SchedulableProcess::Common::AllocateKernelStackSpace();
  const uint64_t uiStackTop = SchedulableProcess::Common::KernelVirtaulStackBase(_stackBlockId) + PROCESS_KERNEL_STACK_BASE - 1;

  _taskContext.interruptState.cs = SYS_CODE_SELECTOR;
  _taskContext.interruptState.rip = taskAddress;
  _taskContext.interruptState.ss = SYS_DATA_SELECTOR;
  _taskContext.interruptState.rsp = uiStackTop;
  _taskContext.interruptState.rflags = 0x202;
  //_taskState.BuildForKernel(taskAddress, uiStackTop, params);
  //_processLDT.BuildForKernel();
  _userID = ROOT_USER_ID ;

  auto parentProcess = ProcessManager::Instance().GetSchedulableProcess(parentID);
  parentProcess.ifPresent([this](SchedulableProcess& p) { p.addChildProcessID(_processID); });
}

KernelThread& KernelProcess::CreateThread(uint32_t threadCaller, uint32_t entryAddress, void* arg) {
  return *new KernelThread(*this, threadCaller, entryAddress, arg);
}

void KernelProcess::DeallocateResources() {
  SchedulableProcess::Common::DeallocateKernelStackSpace(_stackBlockId);
  DeAllocateGUIFramebuffer();
  upanui::GraphicsContext::Destroy();
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
    DMM_DeAllocateForKernel((uint64_t)_frame->frameBuffer().buffer());
    GraphicsVideo::Instance().removeFGProcess(_processID);
  }
}
