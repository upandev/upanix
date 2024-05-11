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

#include <option.h>
#include <mosstd.h>
#include <mutex.h>
#include <FileSystem.h>
#include <FileOperations.h>
#include <ProcessStateInfo.h>
#include <ProcessDLLInfo.h>
#include <RootFrame.h>
#include <KeyboardData.h>
#include <MouseData.h>
#include <GraphicsContext.h>
#include <DMM.h>

class ProcessGroup;
class IODescriptorTable;

class Process {
public:
  enum UIType { NA, TTY, GUI, REDIRECT_TTY };

  virtual bool isKernelProcess() const = 0;
  virtual bool isFGProcessGroup() const = 0;
  virtual int driveID() const = 0;
  virtual uint64_t getProcessBase() const = 0;
  virtual int userID() const = 0;
  virtual bool isChildThread() const = 0;

  virtual DMM& dmm() = 0;

  virtual int processID() const = 0;
  virtual int parentProcessID() const = 0;

  virtual FILE_USER_TYPE fileUserType(const FileNode&) const = 0;
  virtual bool hasFilePermission(const FileNode&, byte mode) const = 0;
  virtual uint64_t* pml4Table() const = 0;

  virtual void setDriveID(int driveID) = 0;
  virtual FileSystem::PresentWorkingDirectory& processPWD() = 0;
  virtual const FileSystem::PresentWorkingDirectory& processPWD() const = 0;
  virtual ProcessStateInfo& stateInfo() = 0;
  virtual PROCESS_STATUS status() const = 0;
  virtual PROCESS_STATUS setStatus(PROCESS_STATUS status) = 0;
  virtual ProcessGroup* processGroup() = 0;
  virtual void setProcessGroup(ProcessGroup* processGroup) = 0;

  virtual IODescriptorTable& iodTable() = 0;
  virtual upan::option<upan::mutex&> dllMutex() {
    throw upan::exception(XLOC, "dllMutex unsupported");
  }

  virtual void LoadELFDLL(const upan::string& szDLLName, const upan::string& szJustDLLName) {
    throw upan::exception(XLOC, "LoadELFDLL unsupported");
  }

  virtual void MapDLLPagesToProcess(uint32_t noOfPagesForDLL, const upan::string& dllName) {
    throw upan::exception(XLOC, "MapDLLPagesToProcess unsupported");
  }

  virtual const ProcessDLLInfo::ELFInfo& getELFInfo() const {
    throw upan::exception(XLOC, "getELFInfo unsupported");
  }

  virtual upan::option<ProcessDLLInfo&> getDLLInfo(const upan::string& dllName) {
    throw upan::exception(XLOC, "getDLLInfo unsupported");
  }

  virtual upan::option<ProcessDLLInfo&> getDLLInfo(int id) {
    throw upan::exception(XLOC, "getDLLInfo unsupported");
  }

  virtual void dispatchKeyboardData(const upanui::KeyboardData& data) {
    throw upan::exception(XLOC, "dispatchKeyboardData unsupported");
  }

  virtual void dispatchMouseData(const upanui::MouseData& data) {
    throw upan::exception(XLOC, "dispatchMouseData unsupported");
  }

  virtual void yield() = 0;

  virtual UIType getUIType() = 0;
  virtual void initGuiFrame() = 0;
  virtual upan::option<RootFrame&> getGuiFrame() = 0;
  virtual void setupAsTtyProcess() {
    throw upan::exception(XLOC, "setupAsTtyProcess unsupported");
  }
  virtual void setupAsRedirectTtyProcess() {
    throw upan::exception(XLOC, "setupAsRedirectTtyProcess unsupported");
  }
  virtual void setupAsGuiProcess(int fdList[]) {
    throw upan::exception(XLOC, "setupAsGuiProcess unsupported");
  }
  virtual bool isGuiBase() const = 0;
  virtual void setGuiBase(bool) = 0;

  virtual upanui::GraphicsContext* getGraphicsContext() {
    throw upan::exception(XLOC, "getGraphicsContext unsupported");
  }
  virtual void setGraphicsContext(upanui::GraphicsContext*) {
    throw upan::exception(XLOC, "setGraphicsContext unsupported");
  }
  virtual void setEnv(const upan::string& key, const upan::string& value) {
    throw upan::exception(XLOC, "setEnv unsupported");
  }
  virtual upan::option<upan::string> getEnv(const upan::string& key) {
    throw upan::exception(XLOC, "setEnv unsupported");
  }

  typedef upan::map<upan::string, upan::string> ProcessEnvMap;
  virtual const ProcessEnvMap& envMap() {
    throw upan::exception(XLOC, "envMap unsupported");
  }
};