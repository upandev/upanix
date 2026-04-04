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

class ElfParser;

class UserProcess : public AutonomousProcess {
public:
  typedef upan::map<upan::string, DLLInfo> DLLInfoMap;

public:
  UserProcess(const upan::string &name, int parentID, int userID, bool isFGProcess,
              const upan::vector<upan::string>& argv,
              const upan::vector<upan::string>& envp);
  //used by fork
  explicit UserProcess(UserProcess& parent);

  bool isKernelProcess() const override {
    return false;
  }

  void onLoad() override;
  UserThread& CreateThread(uintptr_t threadCaller, uintptr_t entryAddress, void* arg, bool joinable) override;

  void MapDLLPagesToProcess(uint32_t noOfPagesForDLL, const upan::string& dllName) override;
  const ELFInfo& getELFInfo() const override {
    return _elfInfo;
  }
  upan::option<DLLInfo&> getDLLInfo(const upan::string& dllName) override;
  upan::option<DLLInfo&> getDLLInfo(int id) override;

  DMM& dmm() override { return _dmm; }

  uint64_t* pml4Table() const override {
    return _pml4Table;
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

  ThreadLocalSpace& tlsp() { return *_tlsp; }

  process_init_fini_t* initRelocate();

private:
  typedef upan::map<upan::string, DLLRelocateInfo> RELOCATE_INFO_DLL_MAP;
  typedef upan::map<upan::string, ExeRelocateInfo> RELOCATE_INFO_EXE_MAP;

  void Load(const upan::vector<upan::string>& argv, const upan::vector<upan::string>& envp);
  void LoadDLLs();
  void LoadELFDLL(const upan::string& dllName);
  upan::option<IRelocateInfo&> getRelocateInfo(const upan::string& symName, bool fallbackToExe, int stBind);
  void relocateMainExe(process_init_fini_t& init_fini);
  void relocateDLLs(process_init_fini_t* init_fini_list);

  void AllocateAddressSpace();
  void CopyElfImage(const uint8_t* processImage, int imageSize, uint64_t virtualLoadAddress);
  uint64_t PushProgramInitStackData(const upan::vector<upan::string>& argv, const upan::vector<upan::string>& envp);

  void LoadFromParent(UserProcess& parent);
  void AllocateAndCopyAddressSpaceFromParent(UserProcess& parent);
  void LoadDLLsFromParent(UserProcess& parent);

  void DeallocateResources() override;
  void DeallocateGUIFramebuffer();

  void setupSignalStackFrame(const struct sigaction& action, const Signal& signal) override;

private:
  uint64_t _processSpaceSize;
  uint32_t _totalNoOfPagesForDLL;
  uint64_t _stackPDAddress;
  DLLInfoMap _dllInfoMap;
  upan::mutex _pageFaultMutex;
  upan::mutex _dllMutex;
  upan::mutex _addressSpaceMutex;
  upan::uniq_ptr<RootFrame> _frame;
  ELFInfo _elfInfo;
  RELOCATE_INFO_DLL_MAP _relocateInfoDLLMap;
  RELOCATE_INFO_EXE_MAP _relocateInfoExeMap;
  UserDMM _dmm;
  uint64_t* _pml4Table;
  upan::uniq_ptr<ThreadLocalSpace> _tlsp;
};