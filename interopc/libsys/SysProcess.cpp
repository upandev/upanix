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
# include <SysCall.h>
# include <malloc.h>
# include <string.h>
# include <stdlib.h>
# include <stdio.h>

int exec(const char* szFileName, ...) {
  uint64_t iProcessID;
	int argc;
	char** argv = nullptr;

	int i ;
  uintptr_t* ref = (uintptr_t*)&szFileName + 1 ;
	for(argc = 0; *(ref + argc); argc++) ;

	if(argc)
	{
		argv = (char**)malloc(sizeof(int) * argc) ;
		if(!argv)
			return -1 ;

		for(i = 0; i < argc; i++)
		{
			argv[i] = (char*)malloc(strlen((char*)(*(ref + i))) + 1) ;
			strcpy((char*)argv[i], (char*)(*(ref + i))) ;
		}
	}

  SysCallProc_Handle(&iProcessID, SYS_CALL_PROCESS_EXEC, false, (uintptr_t) szFileName, (unsigned) argc,
                     (uintptr_t) argv, 4, 5);

	for(i = 0; i < argc; i++)
		free((void*)argv[i]) ;
	free((void*)argv) ;
	return iProcessID ;
}

process_init_fini_t* SysProcess_InitRelocate() {
  //no-op for kernel
  return nullptr;
}

int SysProcess_Exec(const char* szFileName, int iNoOfArgs, char *const szArgList[])
{
  uint64_t iProcessID ;
  SysCallProc_Handle(&iProcessID, SYS_CALL_PROCESS_EXEC, false, (uintptr_t) szFileName, (unsigned) iNoOfArgs,
                     (uintptr_t) szArgList, 4, 5);
	return iProcessID ;
}

int SysProcess_WaitPID(pid_t pid, int *status, int options) {
  uint64_t retStatus ;
  SysCallProc_Handle(&retStatus, SYS_CALL_PROCESS_WAIT_PID, false, (uint64_t)pid, (uint64_t)status, (uint64_t)options, 4, 5);
  return (int)retStatus;
}

void SysProcess_Exit(int iExitStatus)
{
  uint64_t retStatus ;
  SysCallProc_Handle(&retStatus, SYS_CALL_PROCESS_EXIT, false, (uint64_t) iExitStatus, 2, 3, 4, 5);
}

void SysProcess_Yield()
{
  uint64_t retStatus ;
  SysCallProc_Handle(&retStatus, SYS_CALL_PROCESS_YIELD, false, 1, 2, 3, 4, 5);
}

int SysProcess_Sleep(unsigned millisec)
{
  uint64_t retStatus ;
  SysCallProc_Handle(&retStatus, SYS_CALL_PROCESS_SLEEP, false, millisec, 2, 3, 4, 5);
  return (int)retStatus;
}

void SysProcess_WaitOnLock(uint64_t lockAddress, int newVal, int curVal) {
  uint64_t retStatus ;
  SysCallProc_Handle(&retStatus, SYS_CALL_PROCESS_WAIT_ON_LOCK, false, lockAddress, newVal, curVal, 4, 5);
}

int SysProcess_WaitQueue(int id, void* mutex, const struct timeval* timeout) {
  time_t timeoutInMs = 0;
  if (timeout) {
    timeoutInMs = timeout->tv_sec * 1000 + timeout->tv_usec / 1000;
  }
  ProcessManager::Instance().WaitOnQueue(id, *reinterpret_cast<upan::mutex*>(mutex), timeoutInMs, true);

  const auto r = ProcessManager::Instance().GetCurrentPAS().stateInfo().getError();
  if (r != ProcessStateInfo::NO_ERROR) {
    return -1;
  }
  return 0;
}

void SysProcess_WaitDequeue(int id, bool all) {
  ProcessManager::Instance().WaitDequeue(id, all, true);
}

int SysProcess_GetProcList(PS** pProcList, unsigned* uiListSize)
{
  uint64_t retStatus ;
  SysCallProc_Handle(&retStatus, SYS_CALL_PROCESS_GET_PS_LIST, false, (uintptr_t) pProcList, (uintptr_t) uiListSize, 3,
                     4, 5);
	return retStatus ;
}

void SysProcess_FreeProcListMem(PS* pProcList, unsigned uiListSize)
{
  uint64_t retStatus ;
  SysCallProc_Handle(&retStatus, SYS_CALL_PROCESS_FREE_PS_LIST, false, (uintptr_t) pProcList, uiListSize, 3, 4, 5);
}

int SysProcess_IsProcessAlive(int pid) {
  uint64_t retStatus ;
  SysCallProc_Handle(&retStatus, SYS_CALL_PROCESS_ALIVE, false, (unsigned) pid, 2, 3, 4, 5);
  return retStatus ;
}

int SysProcess_ThreadExec(uintptr_t threadCaller, uintptr_t entryAddress, void* arg, bool joinable) {
  uint64_t retStatus ;
  SysCallProc_Handle(&retStatus, SYS_CALL_THREAD_EXEC, false, threadCaller, entryAddress, (uintptr_t) arg, (uint64_t)joinable, 5);
  return retStatus ;
}

int SysProcess_ThreadDetach(int threadId) {
  uint64_t retStatus ;
  SysCallProc_Handle(&retStatus, SYS_CALL_THREAD_DETACH, false, threadId, 2, 3, 4, 5);
  return retStatus;
}

int SysProcess_IsChildAlive(int iProcessID) {
  uint64_t retStatus ;
  SysCallProc_Handle(&retStatus, SYS_CALL_PROCESS_CHILD_ALIVE, false, iProcessID, 2, 3, 4, 5);
  return retStatus ;
}

int SysProcess_IsKernel() {
  return IsKernel() ? 1 : 0;
}

int SysProcess_MaskSignal(SIG_MASKING_TYPE how, const sigset_t *set, sigset_t *oldset) {
  uint64_t retStatus ;
  SysCallProc_Handle(&retStatus, SYS_CALL_PROCESS_MASK_SIGNAL, false, (uint64_t)how, (uint64_t)set, (uint64_t)oldset, 4, 5);
  return (int)retStatus;
}

int SysProcess_SendSignal(pid_t pid, SIGNAL signo, const union sigval* value) {
  uint64_t retStatus ;
  SysCallProc_Handle(&retStatus, SYS_CALL_PROCESS_SIGNAL, false, (uint64_t)pid, (uint64_t)signo, (uint64_t)value, 4, 5);
  return (int)retStatus;
}

void SysProcess_SignalReturn(void* signalContext) {
  uint64_t retStatus ;
  SysCallProc_Handle(&retStatus, SYS_CALL_PROCESS_SET_SIGNAL_RETURN, false, (uint64_t)signalContext, 2, 3, 4, 5);
}

int SysProcess_SetSignalAction(int signo, const struct sigaction *act, struct sigaction *oldact) {
  uint64_t retStatus ;
  SysCallProc_Handle(&retStatus, SYS_CALL_PROCESS_SET_SIGNAL_ACTION, false, (uint64_t)signo, (uint64_t)act, (uint64_t)oldact, 4, 5);
  return (int)retStatus;
}

int SysProcess_SetSID() {
  uint64_t retStatus ;
  SysCallProc_Handle(&retStatus, SYS_CALL_PROCESS_SET_SID, false, 1, 2, 3, 4, 5);
  return (int)retStatus;
}