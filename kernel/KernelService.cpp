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
# include <KernelService.h>
# include <DMM.h>
# include <UserManager.h>
# include <GenericUtil.h>
# include <MemManager.h>
# include <file_util.h>
# include <SystemUtil.h>

KernelService::DLLAllocCopy::DLLAllocCopy(unsigned uiNoOfPages, const upan::string& dllName) : _noOfPagesForDLL(uiNoOfPages), _dllName(dllName) {
}

void KernelService::DLLAllocCopy::Execute() {
  try {
    SchedulableProcess &pas = ProcessManager::Instance().GetSchedulableProcess(GetRequestProcessID()).value();
    pas.MapDLLPagesToProcess(_noOfPagesForDLL, _dllName);
  } catch(upan::exception& e) {
    e.Print();
    _status = false;
    return;
  }
  _status = true ;
}

KernelService::FlatAddress::FlatAddress(uint64_t uiVirtualAddress) : m_uiAddress(uiVirtualAddress)
{ 
}

void KernelService::FlatAddress::Execute() {
  auto& pas = ProcessManager::Instance().GetSchedulableProcess(GetRequestProcessID()).value();
	m_uiFlatAddress = MemManager::Instance().GetFlatAddress(pas.pml4Table(), m_uiAddress) ;
}

KernelService::ProcessExec::ProcessExec(const upan::string& szFile, const char** argv, const char** envp) : _szFile(szFile) {
  if (argv) {
    for(int i = 0; argv[i]; ++i) {
      _argv.push_back(argv[i]);
    }
  }

  if (envp) {
    for(int i = 0; envp[i]; ++i) {
      _envp.push_back(envp[i]);
    }
  }
}

void KernelService::ProcessExec::Execute() {
  auto& srcPAS = ProcessManager::Instance().GetSchedulableProcess(GetRequestProcessID()).value();
  auto& curProc = ProcessManager::Instance().GetCurrentPAS();

  const int curDriveId = curProc.driveID();
  const FileNodeRef curPwd = curProc.pwd();

  curProc.setDriveID(srcPAS.driveID());
  curProc.pwd(srcPAS.pwd());

  m_iNewProcId = ProcessManager::Instance().Create(_szFile.c_str(), GetRequestProcessID(), true, DERIVE_FROM_PARENT, _argv, _envp);

  curProc.setDriveID(curDriveId);
  curProc.pwd(curPwd);
}

void KernelService::ProcessFork::Execute() {
  auto& curProc = ProcessManager::Instance().GetCurrentPAS();

  const int curDriveId = curProc.driveID();
  const FileNodeRef curPwd = curProc.pwd();

  curProc.setDriveID(_forkingParent.driveID());
  curProc.pwd(_forkingParent.pwd());

  while(!_forkingParent.stateInfo().isForkReady());
  _newPid = ProcessManager::Instance().Fork(_mainParent, _forkingParent);

  curProc.setDriveID(curDriveId);
  curProc.pwd(curPwd);
}

void KernelService::ThreadExec::Execute() {
  _threadID = ProcessManager::Instance().CreateThreadTask(GetRequestProcessID(), _threadCaller,
                                                          _entryAddress, _arg,
                                                          _joinable);
}

void KernelService::ProcessGUIFramebufferAllocate::Execute() {
  _userProcess.allocateGUIFramebuffer();
}

void KernelService::SystemReboot::Execute() {
  SystemUtil_Reboot();
}

bool KernelService::RequestDLLAlloCopy(unsigned uiNoOfPages, const upan::string& dllName)
{
	KernelService::DLLAllocCopy* pRequest = new KernelService::DLLAllocCopy(uiNoOfPages, dllName) ;
	AddRequest(pRequest) ;

  ProcessManager::Instance().WaitOnKernelService();

	bool bStatus = pRequest->GetStatus() ;

	delete pRequest ;

	return bStatus ;
}

uint64_t KernelService::RequestFlatAddress(uint64_t uiVirtualAddress)
{
	KernelService::FlatAddress* pRequest = new KernelService::FlatAddress(uiVirtualAddress) ;
	AddRequest(pRequest) ;

  ProcessManager::Instance().WaitOnKernelService();

	auto uiFlatAddress = pRequest->GetFlatAddress() ;

	delete pRequest ;

	return uiFlatAddress ;
}

int KernelService::RequestProcessExec(const upan::string& fileName, const char** argv, const char** envp) {
  upan::string fullPath = fileName;
  if (fileName.find('/') < 0) {
    const auto& r = upan::file_path::resolve(fileName, PATH_ENV, BIN_PATH);
    if (r.isEmpty()) {
      throw upan::exception(XLOC, "Executable file not found: %s", fileName.c_str());
    }
    fullPath = r.value();
	}
	
	auto pRequest = new KernelService::ProcessExec(fullPath, argv, envp);

	AddRequest(pRequest) ;
  ProcessManager::Instance().WaitOnKernelService();

	int iNewProcId = pRequest->GetNewProcId() ;

	delete pRequest ;

	return iNewProcId ;
}

int KernelService::RequestFork() {
  const auto curPid = ProcessManager::Instance().GetCurProcId();
  auto& p = ProcessManager::Instance().GetThreadParentProcess(curPid);
  auto mainParent  = dynamic_cast<UserProcess*>(&p);
  if (mainParent == nullptr) {
    throw upan::exception(XLOC, "Process %d is not a user-process - can't fork", curPid);
  }
  auto& forkingParent = ProcessManager::Instance().GetSchedulableProcess(curPid).value();

  auto pRequest = new KernelService::ProcessFork(*mainParent, forkingParent);
  AddRequest(pRequest);
  ProcessManager::Instance().WaitOnKernelServiceFork();

  // The fork path must ensure that the child does not reference any kernel resources
  // belonging to the parent, especially the process pointer, to avoid treating parent
  // resources as if they belonged to the child.

  if (ProcessManager::Instance().GetCurProcId() == curPid) {
    auto newPid = pRequest->getNewPid();
    delete pRequest;
    return newPid;
  } else {
    return 0;
  }
}

int KernelService::RequestThreadExec(uintptr_t threadCaller, uintptr_t entryAddresss, void* arg, bool joinable) {
  auto pRequest = new KernelService::ThreadExec(threadCaller, entryAddresss, arg, joinable);

  AddRequest(pRequest) ;
  ProcessManager::Instance().WaitOnKernelService();

  int threadID = pRequest->GetThreadID();
  delete pRequest;
  return threadID;
}

void KernelService::RequestProcessGUIFramebufferAllocate(UserProcess& userProcess) {
  auto request = new KernelService::ProcessGUIFramebufferAllocate(userProcess);
  AddRequest(request);
  ProcessManager::Instance().WaitOnKernelService();
  delete request;
}

void KernelService::RequestSystemReboot() {
  AddRequest(new KernelService::SystemReboot());
}

void KernelService::AddRequest(Request* pRequest)
{
  upan::mutex_guard g(m_mutexQRequest);
	m_qRequest.push_back(pRequest) ;
}

KernelService::Request* KernelService::GetRequest()
{
  upan::mutex_guard g(m_mutexQRequest);

	Request* pRequest = NULL;
  if(!m_qRequest.empty())
  {
    pRequest = m_qRequest.front();
    m_qRequest.pop_front();
  }

	return pRequest ;
}

[[noreturn]] void KernelService::Server(KernelService* pService)
{
  while(true)
	{
		Request* pRequest = pService->GetRequest() ;
		if(!pRequest)
		{
			ProcessManager::Instance().Sleep(1) ;
			continue ;
		}

		Process* pPAS = &ProcessManager::Instance().GetCurrentPAS();
		auto ksProcessGroupID = pPAS->processGroup();
		pPAS->setProcessGroup(ProcessManager::Instance().GetSchedulableProcess(pRequest->GetRequestProcessID()).value().processGroup());
    pRequest->Execute() ;
		pPAS->setProcessGroup(ksProcessGroupID);
    
		ProcessManager::Instance().WakeUpFromKSWait(pRequest->GetRequestProcessID()) ;
	}
}

int KernelService::Spawn() {
	static const char* szKS = "kers-" ;
	static int iID = 0 ;

  upan::string szName(szKS);
  szName += upan::string::to_string(iID);
	++iID;

	upan::vector<uintptr_t> params;
	params.push_back((uintptr_t)this);
	int pid = ProcessManager::Instance().CreateKernelProcess(szName, (uintptr_t) &(KernelService::Server),
                                                          ProcessManager::GetCurrentProcessID(), false, true, params);
	if(pid < 0) {
		throw upan::exception(XLOC, "Failed to create Kernel Service Process %s", szName.c_str()) ;
	}

	return pid;
}
