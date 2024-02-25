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

#include <SchedulableProcess.h>
#include <AutonomousProcess.h>

class Thread : public SchedulableProcess {
public:
  Thread(AutonomousProcess& parent);

  IODescriptorTable& iodTable() override {
    return _parent.iodTable();
  }

  AutonomousProcess& threadParent() {
    return _parent;
  }

  upan::option<RootFrame&> getGuiFrame() override {
    return _parent.getGuiFrame();
  }

  void initGuiFrame() override {
    _parent.initGuiFrame();
  }

  void dispatchKeyboardData(const upanui::KeyboardData& data) override {
    _parent.dispatchKeyboardData(data);
  }

  void dispatchMouseData(const upanui::MouseData& mouseData) override {
    _parent.dispatchMouseData(mouseData);
  }

  UIType getUIType() override {
    return _parent.getUIType();
  }

  void setupAsTtyProcess() override {
    _parent.setupAsTtyProcess();
  }

  void setupAsRedirectTtyProcess() override {
    _parent.setupAsRedirectTtyProcess();
  }

  void setupAsGuiProcess(int fdList[]) override {
    return _parent.setupAsGuiProcess(fdList);
  }

  bool isGuiBase() const override {
    return _parent.isGuiBase();
  }

  void setGuiBase(bool v) override {
    _parent.setGuiBase(v);
  }

  void setEnv(const upan::string& key, const upan::string& value) override {
    _parent.setEnv(key, value);
  }

  upan::option<upan::string> getEnv(const upan::string& key) override {
    return _parent.getEnv(key);
  }

  const ProcessEnvMap& envMap() override {
    return _parent.envMap();
  }

  uint64_t* pml4Table() const override {
    return _parent.pml4Table();
  }

protected:
  AutonomousProcess& _parent;
};