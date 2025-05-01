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

void SysProcess_DLLInitRelocate() {
  //no-op for kernel
}

int SysProcess_Exec(const char* szFileName, int iNoOfArgs, char *const szArgList[])
{
  uint64_t iProcessID ;
  SysCallProc_Handle(&iProcessID, SYS_CALL_PROCESS_EXEC, false, (uintptr_t) szFileName, (unsigned) iNoOfArgs,
                     (uintptr_t) szArgList, 4, 5);
	return iProcessID ;
}

void SysProcess_WaitPID(int iProcessID)
{
  uint64_t retStatus ;
  SysCallProc_Handle(&retStatus, SYS_CALL_PROCESS_WAIT_PID, false, (uint64_t) iProcessID, 2, 3, 4, 5);
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

void SysProcess_Sleep(unsigned millisec)
{
  uint64_t retStatus ;
  SysCallProc_Handle(&retStatus, SYS_CALL_PROCESS_SLEEP, false, millisec, 2, 3, 4, 5);
}

void SysProcess_WaitOnLock(uint64_t lockAddress, int newVal, int curVal) {
  uint64_t retStatus ;
  SysCallProc_Handle(&retStatus, SYS_CALL_PROCESS_WAIT_ON_LOCK, false, lockAddress, newVal, curVal, 4, 5);
}

void SysProcess_WaitQueue(int id, void* mutex) {
  ProcessManager::Instance().WaitOnQueue(id, *reinterpret_cast<upan::mutex*>(mutex), true);
}

void SysProcess_WaitDequeue(int id, bool all) {
  ProcessManager::Instance().WaitDequeue(id, all, true);
}

int SysProcess_GetEnv(const char* szVar, char* retVal)
{
  uint64_t retStatus ;
  SysCallProc_Handle(&retStatus, SYS_CALL_PROCESS_GET_ENV, false, (uintptr_t) szVar, (uintptr_t) retVal, 3, 4, 5);
	return retStatus ;
}

int SysProcess_SetEnv(const char* szVar, const char* szVal)
{
  uint64_t retStatus ;
  SysCallProc_Handle(&retStatus, SYS_CALL_PROCESS_SET_ENV, false, (uintptr_t) szVar, (uintptr_t) szVal, 3, 4, 5);
	return retStatus ;
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

int SysProcess_ThreadExec(uintptr_t threadCaller, uintptr_t entryAddress, void* arg) {
  uint64_t retStatus ;
  SysCallProc_Handle(&retStatus, SYS_CALL_THREAD_EXEC, false, threadCaller, entryAddress, (uintptr_t) arg, 4, 5);
  return retStatus ;
}

int SysProcess_IsChildAlive(int iProcessID) {
  uint64_t retStatus ;
  SysCallProc_Handle(&retStatus, SYS_CALL_PROCESS_CHILD_ALIVE, false, iProcessID, 2, 3, 4, 5);
  return retStatus ;
}

int SysProcess_IsKernel() {
  return IsKernel() ? 1 : 0;
}