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
#ifndef _KERNEL_SERVICE_H_
#define _KERNEL_SERVICE_H_

#include <Global.h>
#include <mutex.h>
#include <string.h>
#include <StringUtil.h>
#include <ProcessManager.h>
#include <list.h>

class KernelService
{
	private:
		class Request
		{
			private:
				int m_iRequestProcessID ;

			public:
        Request() : m_iRequestProcessID(ProcessManager::Instance().GetCurProcId()) { }
				virtual ~Request() { }
        virtual void Execute() = 0 ;

				inline int GetRequestProcessID() { return m_iRequestProcessID ; }	
		} ;

	private:
		upan::list<Request*> m_qRequest ;
		upan::mutex m_mutexQRequest ;

		upan::list<int> m_lServerList ;
		upan::mutex m_mutexServer ;


	public:
		KernelService() { }
		~KernelService() { }

		int Spawn() ;
		bool Stop(int iServerProcessID) ;

		// RequestFactory
		bool RequestDLLAlloCopy(unsigned uiNoOfPages, const upan::string& dllName) ;
		unsigned RequestFlatAddress(unsigned uiAddress) ;
		bool RequestPageFault(uintptr_t faultyAddress) ;
		int RequestProcessExec(const char* szFile, int iNoOfArgs, const char** szArgs) ;
		int RequestThreadExec(uint32_t threadCaller, uint32_t entryAddresss, void* arg);
    void RequestProcessGUIFramebufferAllocate(UserProcess& userProcess);

	private:
		static void Server(KernelService* pService) ;

		void AddRequest(Request* pRequest) ;
		Request* GetRequest() ;

	private:
		class DLLAllocCopy : public Request
		{
			private:
				unsigned _noOfPagesForDLL;
				const upan::string _dllName;
				bool _status;

			public:
				DLLAllocCopy(unsigned uiNoOfPages, const upan::string& dllName) ;
        void Execute() ;
				inline bool GetStatus() { return _status ; }
		} ;

		class FlatAddress : public Request
		{
			private:
				unsigned m_uiAddress ;
				unsigned m_uiFlatAddress ;

			public:
				FlatAddress(unsigned uiVirtualAddress) ;
        void Execute() ;
				inline unsigned GetFlatAddress() { return m_uiFlatAddress ; }
		} ;

		class PageFault : public Request
		{
			private:
				uintptr_t _faultyAddress ;
				bool m_bStatus ;

			public:
				PageFault(uintptr_t faultyAddress) ;
        void Execute() ;
				inline bool GetStatus() { return m_bStatus ; }
		} ;

		class ProcessExec : public Request
		{
			private:
				int m_iNoOfArgs;
				upan::string _szFile;
				char** m_szArgs;
				int m_iNewProcId;

			public:
				ProcessExec(int iNoOfArgs, const char* szFile, const char** szArgs) ;
				~ProcessExec() ;
        void Execute() ;
				inline int GetNewProcId() { return m_iNewProcId ; }
		};

		class ThreadExec : public Request {
		private:
		  uint32_t _threadCaller;
		  uint32_t _entryAddress;
		  void* _arg;
		  int _threadID;

		public:
		  ThreadExec(uint32_t threadCaller, uint32_t entryAddress, void* arg)
		    : _threadCaller(threadCaller), _entryAddress(entryAddress), _arg(arg), _threadID(-1) {}
		  void Execute() override;
		  int GetThreadID() const {
		    return _threadID;
		  }
		};

		class ProcessGUIFramebufferAllocate : public Request {
		private:
		  UserProcess& _userProcess;
		public:
		  ProcessGUIFramebufferAllocate(UserProcess& userProcess) : _userProcess(userProcess) {}
		  void Execute() override;
		};
} ;

#endif
