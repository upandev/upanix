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
# include <stdio.h>
# include <UpanixMain.h>

# include <ProcessManager.h>
# include <SysCall.h>
# include <MemManager.h>
# include <PCIBusHandler.h>
# include <SessionManager.h>
# include <RTC.h>
# include <MultiBoot.h>
# include <USBController.h>
# include <EHCIManager.h>
# include <XHCIManager.h>
# include <USBMassBulkStorageDisk.h>
# include <USBKeyboard.h>
# include <GraphicsVideo.h>
# include <KeyboardHandler.h>
# include <Acpi.h>
# include <Cpu.h>
# include <IrqManager.h>
# include "network/NetworkManager.h"
# include <Mtrr.h>
# include <Pat.h>
# include <PS2Controller.h>
# include <KernelRootProcess.h>
# include <PS2MouseDriver.h>
# include <metrics.h>
# include <StorageDriveManager.h>
# include <NetworkOperations.h>
# include <KLog.h>
# include <LocalDataGramResolver.h>

/**** Global Variable declaration/definition *****/
bool KERNEL_MODE;
bool KERNEL_ROOT;
bool SPECIAL_TASK;

/***********************************************/

[[noreturn]] void UpanixMain_KernelProcess() {
  try {
    //MountManager_MountDrives() ;
    ProcessManager::setUpanixKernelProcessID(ProcessManager::GetCurrentProcessID());

    LocalDataGramResolver::Instance();
    KernelRootProcess::Instance().createScheduleRunner();

    KC::MKernelService().Spawn();
    KC::MKernelService().Spawn();

    KernelRootProcess::Instance().initGuiFrame();
    //RootGUIConsole::Instance().ClearScreen();
    GraphicsVideo::Instance().CreateRefreshTask();

    KernelRootProcess::Instance().initFSDevices();

    KC::MConsole().StartCursorBlink();

    KeyboardHandler::Instance().StartDispatcher();
    PS2MouseDriver::Instance().StartDispatcher();

    KernelRootProcess::Instance().setProcessGroup(ProcessManager::Instance().GetCurrentPAS().processGroup());

    while (true) {
      const int pid = ProcessManager::Instance().CreateKernelProcess("console", (uintptr_t) &Console_StartUpanixConsole,
                                                                     ProcessManager::GetCurrentProcessID(), true, true,
                                                                     upan::vector<uintptr_t>());
//	SessionManager_SetSessionIDMap(SessionManager_KeyToSessionIDMap(Keyboard_F1), pid) ;
      int exitStatus;
      ProcessManager::Instance().WaitOnChild(pid, exitStatus);
    }
  } catch(const upan::exception& ex) {
    ex.Print();
  }

  ProcessManager_Exit(0);
}

extern "C" void _cxx_global_init();

void TestThrowStr()
{
	throw "String Exception";
}

void TestThrowInt()
{
	throw 100;
}

class Exception
{
	public:
	Exception(const char* x) : _val(x) {}
	const char* _val;
};

void TestThrowObj()
{
	throw Exception("Exception Class");
}

void TestException()
{
	try { TestThrowStr(); } catch(const char* ex) { printf("\nCaught Exception: %s", ex); }
	try { TestThrowInt(); } catch(int ex) { printf("\nCaught Exception: %d", ex); }
	try { TestThrowObj(); } catch(const Exception& ex) { printf("\nCaught Exception: %s", ex._val); }

	try {
	try { TestThrowStr(); } catch(const char* ex) { printf("\nRethrow Caught Exception: %s", ex); throw; }
	} catch(const char* ex) { printf("\nCaught Rethrown Exception: %s", ex); }

	try {
	try { TestThrowStr(); } catch(const char* ex) { printf("\nRethrow Caught Exception: %s", ex); throw 100; }
	} catch(const char* ex) { printf("\nCaught Rethrown Exception: %s", ex); }
	catch(...) { printf("\nCaught Rethrown unknown Exception"); }
}

class TG {
public:
  TG() {
    printf("\n Global TG initialized");
  }
};

static const TG x;

bool IsKernel() {
  return KERNEL_MODE == true || KERNEL_ROOT == true || ProcessManager::GetCurrentProcessID() == NO_PROCESS_ID;
}

bool IsKernelProcess(int pid) {
  return SPECIAL_TASK || ProcessManager::Instance().IsKernelProcess(pid);
}
void SetKernelMode(bool val) {
  KERNEL_MODE = val;
}

void SetKernelRootMode(bool val) {
  KERNEL_ROOT = val;
}

void Initialize() {
	SPECIAL_TASK = false ;

	MultiBoot::Instance();
  RootConsole::Create();
  MemManager::Instance();
  KC::MConsole().Message("\n **** _/\\_ Welcome to Upanix _/\\_ ****\n", upanui::CharStyle::WHITE_ON_BLACK());

  ProcessManager::Instance();

  //KernelRootProcess must be initialized to setup kernel FD table with stdin/out/err before using stdio functions like printf.
  KernelRootProcess::Instance();
  MultiBoot::Instance().Print();
  MemManager::Instance().PrintInitStatus();

	//defined in osutils/crti.s - this is C++ init to call global objects' constructor
	_cxx_global_init();

	//	TestException(); while(1);
  try {
    upan::metrics::create();

    IDT::Instance();
    Cpu::Instance();
    Acpi::Instance();
    Pat::Instance();
    Mtrr::Instance();
    DMA_Initialize();
    StdIRQ::Instance();
    SysCall_Initialize();
    openlog("Upanix", LOG_CONS, LOG_KERN);
    DynamicLinkLoader::Instance();
    KC::MKernelService();
    GraphicsVideo::Instance().Initialize();

  /* Start - Peripheral Device Initialization */
  //TODO: An Abstract Bus Handler which should internally take care of different
  //types of bus like ISA, PCI etc... 
    PCIBusHandler::Instance().Initialize();
    IrqManager::Initialize();
    PIT::Instance().Initialize();
    __asm__ __volatile__("sti");
    StorageDriveManager::Instance();

    PS2Controller::Instance();

    //Floppy_Initialize() ;
    //ATADeviceController_Initialize() ;

    //MountManager_Initialize() ;

  /*End - Peripheral Device Initialization */

    RTC::Initialize() ;
    //USB
    USBController::Instance();
    EHCIManager::Instance();
    XHCIManager::Instance().Initialize();

    USBDiskDriver::Register();
    USBKeyboardDriver::Register();

    NetworkManager::Instance();

    KeyboardHandler::Instance().Getch();

    FileOperations::Instance();
    NetworkOperations::Instance();
    SessionManager_Initialize() ;

    Console::Instance();

    KernelRootProcess::Instance().initTLS();
    //Now that the TLS is initialized for KernelRoot, getpid() can get the PID from the thread local space
    openlog("Upanix", LOG_PID | LOG_CONS, LOG_KERN);
    KLog::info("Kernel Root TLS is Initialized");
  }
  catch(const upan::exception& ex)
  {
    printf("%s\n", ex.ErrorMsg().c_str());
    printf("KERNEL PANIC!\n");
    while(1);
  }
  catch(...)
  {
    printf("\n Unknown error!! KERNEL PANIC!\n");
    while(1);
  }
}

upan::mutex& UpanixMain_GetDMMMutex()
{
	static upan::mutex mDMMMutex ;
	return mDMMMutex ;
}

void UpanixMain() {
  SetKernelMode(true);
  SetKernelRootMode(false);

  Initialize();
	ProcessManager::Instance().CreateKernelProcess("kerparent", (uintptr_t) &UpanixMain_KernelProcess, NO_PROCESS_ID, true, true, upan::vector<uintptr_t>());

  SetKernelMode(false);
	ProcessManager::Instance().EnableTaskSwitch();
	while(1) ;
}

bool UpanixMain_IsKernelDebugOn() {
  const char* szVal = getenv("UPANIX_KDEBUG");
  if (!szVal) {
    return strcmp(szVal, "1") == 0;
  }
	return false ;
}