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

#include <AutonomousProcess.h>
#include <KernelThread.h>
#include <vector.h>
#include <BaseFrame.h>
#include <GraphicsContext.h>

//A KernelProcess is similar to a Thread in that they all share same address space (page tables), heap but different stack
//But it is a process in that if the parent process dies before child, then child kernel process will continue to execute under the root kernel process
class KernelProcess : public AutonomousProcess {
public:
  KernelProcess(const upan::string& name, uintptr_t taskAddress, int parentID, bool isFGProcess, const upan::vector<uintptr_t>& params);

  bool isKernelProcess() const override {
    return true;
  }

  void onLoad() override {}

  KernelThread& CreateThread(uintptr_t threadCaller, uintptr_t entryAddress, void* arg) override;

  IODescriptorTable& iodTable() override {
    return _iodTable;
  }

  DMM& dmm() override;

  void initGuiFrame() override;

  upan::option<RootFrame&> getGuiFrame() override {
    return _frame.toOption();
  }

  upanui::GraphicsContext* getGraphicsContext() override {
    return _graphicsContext;
  }

  void setGraphicsContext(upanui::GraphicsContext* graphicsContext) override {
    _graphicsContext = graphicsContext;
  }

  uint64_t* pml4Table() const override {
    return MEM_PML4_TABLE;
  }

private:
  void DeallocateResources() override;
  void DeAllocateGUIFramebuffer();
  void setupSignalStackFrame(const struct sigaction& action, const Signal& signal) override;

private:
  int _stackBlockId;
  IODescriptorTable _iodTable;
  upan::uniq_ptr<RootFrame> _frame;
  //interop variable
  upanui::GraphicsContext* _graphicsContext;
};