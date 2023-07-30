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

#include <list.h>
#include <AutonomousProcess.h>
#include <UserThread.h>

class UserProcess : public AutonomousProcess {
public:
  typedef upan::map<upan::string, ProcessDLLInfo> DLLInfoMap;

public:
  UserProcess(const upan::string &name, int parentID, int userID, bool isFGProcess, int noOfParams, char** args);

  bool isKernelProcess() const override {
    return false;
  }

  void onLoad() override;
  UserThread& CreateThread(uint32_t threadCaller, uint32_t entryAddress, void* arg) override;

  uint32_t startPDEForDLL() const override {
    return _startPDEForDLL;
  }

  void LoadELFDLL(const upan::string& szDLLName, const upan::string& szJustDLLName) override;
  void MapDLLPagesToProcess(uint32_t noOfPagesForDLL, const upan::string& dllName) override;
  const ProcessDLLInfo::ELFInfo& getELFInfo() const override {
    return _elfInfo;
  }
  upan::option<ProcessDLLInfo&> getDLLInfo(const upan::string& dllName) override;
  upan::option<ProcessDLLInfo&> getDLLInfo(int id) override;

  AllocationUnitTracker* getAUTAddress() const override {
    return autAddress;
  }
  void setAUTAddress(AllocationUnitTracker* addr) {
    autAddress = addr;
  }

  IODescriptorTable& iodTable() override {
    return _iodTable;
  }

  upan::option<upan::mutex&> heapMutex() override {
    return upan::option<upan::mutex&>(_heapMutex);
  }
  upan::option<upan::mutex&> pageAllocMutex() override {
    return upan::option<upan::mutex&>(_pageFaultMutex);
  }
  upan::option<upan::mutex&> dllMutex() override {
    return upan::option<upan::mutex&>(_dllMutex);
  }

  upan::option<RootFrame&> getGuiFrame() override {
    return _frame.toOption();
  }
  void initGuiFrame() override;
  void allocateGUIFramebuffer();

private:
  void Load(int noOfParams, char** szArgumentList);
  uint32_t AllocateAddressSpace();
  void CopyElfImage(unsigned uiPDEAddr, byte* bProcessImage, unsigned uiMemImageSize);
  uint32_t PushProgramInitStackData(int iNumberOfParameters, char** szArgumentList);
  void AllocatePTE(const unsigned uiPDEAddress);
  void InitializeProcessSpaceForOS(const unsigned uiPDEAddress);
  void InitializeProcessSpaceForProcess(const unsigned uiPDEAddress);

  void DeAllocateResources() override;
  void DeAllocateDLLPages();
  void DeAllocateAddressSpace();
  void DeAllocateProcessSpace();
  void DeAllocatePTE();
  void DeAllocateGUIFramebuffer();

private:
  AllocationUnitTracker* autAddress;
  uint32_t _noOfPagesForPTE;
  uint32_t _noOfPagesForProcess;
  uint32_t _noOfPagesForDLLPTE;
  uint32_t _totalNoOfPagesForDLL;
  uint32_t _startPDEForDLL;
  uint32_t _stackPTEAddress;
  upan::vector<upan::string> _loadedDLLs;
  DLLInfoMap _dllInfoMap;
  upan::mutex _heapMutex;
  upan::mutex _pageFaultMutex;
  upan::mutex _dllMutex;
  upan::mutex _addressSpaceMutex;
  IODescriptorTable _iodTable;
  upan::uniq_ptr<RootFrame> _frame;
  ProcessDLLInfo::ELFInfo _elfInfo;
};