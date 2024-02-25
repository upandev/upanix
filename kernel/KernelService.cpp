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
# include <AsmUtil.h>
# include <DynamicLinkLoader.h>
# include <UserManager.h>
# include <GenericUtil.h>
# include <MemManager.h>

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

KernelService::PageFault::PageFault(uintptr_t faultyAddress) : _faultyAddress(faultyAddress)
{
}

void KernelService::PageFault::Execute() {
  m_bStatus = MemManager::Instance().AllocatePage(GetRequestProcessID(), _faultyAddress) == Success;
}

KernelService::ProcessExec::ProcessExec(int iNoOfArgs, const char* szFile, const char** szArgs)
   	: m_iNoOfArgs(iNoOfArgs), _szFile(szFile), m_szArgs(NULL)
{
	m_szArgs = new char*[iNoOfArgs] ;
	for(int i = 0; i < iNoOfArgs; i++)
	{
		m_szArgs[i] = new char[strlen(szArgs[i]) + 1] ;
		strcpy(m_szArgs[i], szArgs[i]) ;
	}
}

KernelService::ProcessExec::~ProcessExec()
{
	for(int i = 0; i < m_iNoOfArgs; i++)
		delete[] m_szArgs[i] ;
	delete[] m_szArgs ;
}

void KernelService::ProcessExec::Execute()
{
	int iOldDDriveID ;
	FileSystem::PresentWorkingDirectory mOldPWD ;
	ProcessManager::Instance().CopyDiskDrive(GetRequestProcessID(), iOldDDriveID, mOldPWD) ;

  m_iNewProcId = ProcessManager::Instance().Create(_szFile.c_str(), GetRequestProcessID(), true, DERIVE_FROM_PARENT, m_iNoOfArgs, m_szArgs) ;

  auto& curProc = ProcessManager::Instance().GetCurrentPAS();
  curProc.setDriveID(iOldDDriveID);
	memcpy((void*)&curProc.processPWD(), (void*)&mOldPWD, sizeof(FileSystem::PresentWorkingDirectory));
}

void KernelService::ThreadExec::Execute() {
  _threadID = ProcessManager::Instance().CreateThreadTask(GetRequestProcessID(), _threadCaller, _entryAddress, _arg);
}

void KernelService::ProcessGUIFramebufferAllocate::Execute() {
  _userProcess.allocateGUIFramebuffer();
}

bool KernelService::RequestDLLAlloCopy(unsigned uiNoOfPages, const upan::string& dllName)
{
	KernelService::DLLAllocCopy* pRequest = new KernelService::DLLAllocCopy(uiNoOfPages, dllName) ;
	AddRequest(pRequest) ;

	ProcessManager::Instance().WaitOnKernelService() ;

	bool bStatus = pRequest->GetStatus() ;

	delete pRequest ;

	return bStatus ;
}

uint64_t KernelService::RequestFlatAddress(uint64_t uiVirtualAddress)
{
	KernelService::FlatAddress* pRequest = new KernelService::FlatAddress(uiVirtualAddress) ;
	AddRequest(pRequest) ;

	ProcessManager::Instance().WaitOnKernelService() ;

	auto uiFlatAddress = pRequest->GetFlatAddress() ;

	delete pRequest ;

	return uiFlatAddress ;
}

//This request to allocate a page upon page-fault is triggered via PageFault interrupt 0xE
//So, this function is called from an interrupt handler - and hence, there won't be any other interrupts while
//this interrupt is active ==> No PIT interrupts ==> No task switch
bool KernelService::RequestPageFault(uintptr_t faultyAddress) {
  if (faultyAddress < PAGE_SIZE) {
    printf("\nPageFault at lower address: %x!!\n", faultyAddress);
  }
	KernelService::PageFault* pRequest = new KernelService::PageFault(faultyAddress) ;

	AddRequest(pRequest) ;
	ProcessManager::Instance().WaitOnKernelService() ;

	bool bStatus = pRequest->GetStatus() ;
	delete pRequest ;
	return bStatus ;
}

int KernelService::RequestProcessExec(const char* szFile, int iNoOfArgs, const char** szArgs)
{
	char szProcessPath[128] ;
	char szFullProcPath[128] ;
	
	if(String_Chr(szFile, '/') < 0)
	{
		if(!GenericUtil_GetFullFilePathFromEnv(PATH_ENV, BIN_PATH, szFile, szProcessPath))
		{
			return -2 ;
		}

		strcpy(szFullProcPath, szProcessPath) ;
		strcat(szFullProcPath, szFile) ;
	}
	else
	{
		strcpy(szFullProcPath, szFile) ;
	}
	
	auto pRequest = new KernelService::ProcessExec(iNoOfArgs, szFullProcPath, szArgs) ;

	AddRequest(pRequest) ;
	ProcessManager::Instance().WaitOnKernelService() ;

	int iNewProcId = pRequest->GetNewProcId() ;

	delete pRequest ;

	return iNewProcId ;
}

int KernelService::RequestThreadExec(uintptr_t threadCaller, uintptr_t entryAddresss, void* arg) {
  auto pRequest = new KernelService::ThreadExec(threadCaller, entryAddresss, arg);

  AddRequest(pRequest) ;
  ProcessManager::Instance().WaitOnKernelService() ;

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
			ProcessManager::Instance().Sleep(10) ;
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

int KernelService::Spawn()
{
  upan::mutex_guard g(m_mutexServer);

	static const char* szKS = "kers-" ;
	static int iID = 0 ;

  upan::string szName(szKS);
  szName += upan::string::to_string(iID);
	++iID;

	upan::vector<uintptr_t> params;
	params.push_back((uintptr_t)this);
	int pid = ProcessManager::Instance().CreateKernelProcess(szName, (uintptr_t) &(KernelService::Server),
                                                          ProcessManager::GetCurrentProcessID(), false, params);
	if(pid < 0) {
		printf("\n Failed to create Kernel Service Process %s", szName.c_str()) ;
	} else {
    m_lServerList.push_back(pid);
  }

	return pid ;
}

bool KernelService::Stop(int iServerProcessID)
{
  upan::mutex_guard g(m_mutexServer);

	if(!m_lServerList.erase(iServerProcessID))
	{
		printf("\n Invalid KernelService ProcessID %d", iServerProcessID) ;
		return false ;
	}

	ProcessManager::Instance().Kill(iServerProcessID) ;

	return true;
}

