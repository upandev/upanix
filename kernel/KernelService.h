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
		};

	private:
		upan::list<Request*> m_qRequest ;
		upan::mutex m_mutexQRequest ;

	public:
		KernelService() { }
		~KernelService() { }

		int Spawn();

		// RequestFactory
		bool RequestDLLAlloCopy(unsigned uiNoOfPages, const upan::string& dllName) ;
    uint64_t RequestFlatAddress(uint64_t uiAddress) ;
		int RequestProcessExec(const upan::string& fileName, const char** argv, const char** envp) ;
    int RequestFork();
		int RequestThreadExec(uintptr_t threadCaller, uintptr_t entryAddresss, void* arg, bool joinable);
    void RequestProcessGUIFramebufferAllocate(UserProcess& userProcess);
    void RequestSystemReboot();

	private:
    [[noreturn]] static void Server(KernelService* pService) ;

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
				uint64_t m_uiAddress ;
        uint64_t m_uiFlatAddress ;

			public:
				FlatAddress(uint64_t uiVirtualAddress) ;
        void Execute() ;
				inline uint64_t GetFlatAddress() { return m_uiFlatAddress ; }
		} ;

		class ProcessExec : public Request
		{
			private:
				upan::string _szFile;
				upan::vector<upan::string> _argv;
        upan::vector<upan::string> _envp;
				int m_iNewProcId;

			public:
				ProcessExec(const upan::string& szFile, const char** argv, const char** envp) ;
        void Execute() override;
				int GetNewProcId() const { return m_iNewProcId ; }
		};

    class ProcessFork : public Request {
    private:
      int _newPid;
      UserProcess& _parent;

    public:
      explicit ProcessFork(UserProcess& parent) : _newPid(-1), _parent(parent) {}
      void Execute() override;
      int getNewPid() const { return _newPid; }
    };

		class ThreadExec : public Request {
		private:
      uintptr_t _threadCaller;
      uintptr_t _entryAddress;
		  void* _arg;
		  int _threadID;
      bool _joinable;

		public:
		  ThreadExec(uintptr_t threadCaller, uintptr_t entryAddress, void* arg, bool joinable)
		    : _threadCaller(threadCaller), _entryAddress(entryAddress), _arg(arg), _threadID(-1), _joinable(joinable) {}
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

    class SystemReboot : public Request {
    public:
      void Execute() override;
    };
} ;

#endif
