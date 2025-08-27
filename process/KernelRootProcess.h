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

#include <Process.h>
#include <RootGUIConsole.h>
#include <IODescriptorTable.h>
#include <drive.h>
#include <UserManager.h>
#include <MemManager.h>
#include <ThreadLocalSpace.h>
#include <ThreadLocalStorage.h>

class KernelRootProcess : public Process {
private:
  KernelRootProcess();

public:
  static KernelRootProcess& Instance();

  void createScheduleRunner();
  int scheduleRunnerPid() const { return _scheduleRunnerPid; }
  void initTLS();

  bool isKernelProcess() const override {
    return true;
  }

  bool isFGProcessGroup() const override {
    return true;
  }

  int processID() const override {
    return NO_PROCESS_ID;
  }

  int parentProcessID() const override {
    return NO_PROCESS_ID;
  }

  int driveID() const override {
    return ROOT_DRIVE;
  }

  uint64_t getProcessBase() const override {
    return 0;
  }

  int userID() const override {
    return ROOT_USER_ID;
  }

  bool isChildThread() const override {
    return false;
  }

  IODescriptorTable& iodTable() override {
    return _iodTable;
  }

  FILE_USER_TYPE fileUserType(const FileNode&) const override {
    return USER_OWNER;
  }

  bool hasFilePermission(const FileNode&, byte mode) const override {
    return true;
  }

  uint64_t* pml4Table() const override {
    return MEM_PML4_TABLE;
  }

  void yield() override {
    throw upan::exception(XLOC, "yield() unsupported");
  }

  void setDriveID(int driveID) override {
    throw upan::exception(XLOC, "setDriveID() unsupported");
  }

  FileNodeRef pwd() const override {
    throw upan::exception(XLOC, "pwd() unsupported");
  }

  void pwd(const FileNodeRef&) override {
    throw upan::exception(XLOC, "pwd(FileNodeRef&) unsupported");
  }

  ProcessStateInfo& stateInfo() {
    throw upan::exception(XLOC, "stateInfo() unsupported");
  }

  PROCESS_STATUS status() const {
    throw upan::exception(XLOC, "status() unsupported");
  }

  PROCESS_STATUS setStatus(PROCESS_STATUS status) {
    throw upan::exception(XLOC, "setStatus() unsupported");
  }

  ProcessGroup* processGroup() override {
    return _processGroup;
  }

  void setProcessGroup(ProcessGroup* processGroup) override {
    _processGroup = processGroup;
  }

  DMM& dmm() override;
  void switchPageTable();

  upan::option<RootFrame&> getGuiFrame() override {
    return upan::option<RootFrame&>(RootGUIConsole::Instance().frame());
  }

  UIType getUIType() override {
    return Process::UIType::NA;
  }
  void initGuiFrame() override;
  void dispatchKeyboardData(const upanui::KeyboardData& data) override;
  void dispatchMouseData(const upanui::MouseData& mouseData) override;

  bool isGuiBase() const override {
    return true;
  }
  void setGuiBase(bool v) override {
    throw upan::exception(XLOC, "KernelRootProcess is always GuiBase process - can't modify this flag");
  }

  MouseCursorType mouseCursorType() const override { return MouseCursorType::NORMAL; }
  void setMouseCursorType(MouseCursorType type) override {
    throw upan::exception(XLOC, "setMouseCursorType() unsupported");
  }

  ThreadLocalSpace& tlsp() { return *_tlsp; }

private:
  IODescriptorTable _iodTable;
  upan::uniq_ptr<ThreadLocalSpace> _tlsp;
  upan::uniq_ptr<ThreadLocalStorage> _tls;
  int _scheduleRunnerPid;
  ProcessGroup* _processGroup;
};