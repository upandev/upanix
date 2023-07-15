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
# include <MemUtil.h>

# include <FileSystem.h>
# include <ProcessManager.h>
# include <SysCall.h>
# include <MemManager.h>
# include <PCIBusHandler.h>
# include <DMM.h>
# include <Directory.h>
# include <Floppy.h>
# include <MemUtil.h>
# include <StringUtil.h>
# include <KernelService.h>
# include <ATADeviceController.h>
# include <IODescriptorTable.h>
# include <SessionManager.h>
# include <UserManager.h>
# include <PartitionManager.h>
# include <RTC.h>
# include <MultiBoot.h>
# include <MountManager.h>
# include <USBController.h>
# include <UHCIManager.h>
# include <EHCIManager.h>
# include <XHCIManager.h>
# include <USBMassBulkStorageDisk.h>
# include <USBKeyboard.h>
# include <exception.h>
# include <GraphicsVideo.h>
# include <KeyboardHandler.h>
# include <GenericUtil.h>
# include <Acpi.h>
# include <Cpu.h>
# include <IrqManager.h>
# include <NetworkManager.h>
# include <Mtrr.h>
# include <Pat.h>
#include <PS2Controller.h>
#include <KernelRootProcess.h>
#include <PS2MouseDriver.h>
#include <metrics.h>

/**** Global Variable declaration/definition *****/
byte KERNEL_MODE ;
byte SPECIAL_TASK ;
int debug_point ;
/***********************************************/

[[noreturn]] void UpanixMain_KernelProcess() {
	//MountManager_MountDrives() ;
	ProcessManager::setUpanixKernelProcessID(ProcessManager::GetCurrentProcessID());

	KC::MKernelService().Spawn() ;
	KC::MKernelService().Spawn() ;

	KernelRootProcess::Instance().initGuiFrame();
	RootGUIConsole::Instance().ClearScreen();
  GraphicsVideo::Instance().CreateRefreshTask();

  KC::MConsole().StartCursorBlink();
  KeyboardHandler::Instance().StartDispatcher();
  PS2MouseDriver::Instance().StartDispatcher();

	while(true) {
    const int pid = ProcessManager::Instance().CreateKernelProcess("console", (uintptr_t) &Console_StartUpanixConsole,
                                                                   ProcessManager::GetCurrentProcessID(), true, upan::vector<uintptr_t>());
//	SessionManager_SetSessionIDMap(SessionManager_KeyToSessionIDMap(Keyboard_F1), pid) ;
    ProcessManager::Instance().WaitOnChild(pid);
  }
	ProcessManager_EXIT() ;
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

void Initialize() {
	debug_point = 0 ;

	KERNEL_MODE = true ;
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
    //TODO: SysCall_Initialize();
    KC::MKernelService();
    GraphicsVideo::Instance().Initialize();

  /* Start - Peripheral Device Initialization */
  //TODO: An Abstract Bus Handler which should internally take care of different
  //types of bus like ISA, PCI etc... 
    PCIBusHandler::Instance().Initialize();
    IrqManager::Initialize();
    PIT_Initialize();
    IrqManager::Instance().EnableIRQ(StdIRQ::Instance().TIMER_IRQ) ;
    __asm__ __volatile__("sti");
    while(1);
    DiskDriveManager::Instance();

    PS2Controller::Instance();

    //Floppy_Initialize() ;
    //ATADeviceController_Initialize() ;

    //MountManager_Initialize() ;

  /*End - Peripheral Device Initialization */

    RTC::Initialize() ;
    
    //USB
    USBController::Instance();
    //UHCIManager::Instance();
    //EHCIManager::Instance();
    XHCIManager::Instance().Initialize();

    USBDiskDriver::Register();
    USBKeyboardDriver::Register();

    NetworkManager::Instance();

    KeyboardHandler::Instance().Getch();

    SessionManager_Initialize() ;

    Console::Instance();
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
	Initialize() ;

	ProcessManager::Instance().CreateKernelProcess("kerparent", (uintptr_t) &UpanixMain_KernelProcess, NO_PROCESS_ID, true, upan::vector<uintptr_t>());
//	ProcessManager_CreateKernelImage((unsigned)&Console_StartMOSConsole, NO_PROCESS_ID, true, NULL, NULL, &pid) ;
	ProcessManager::Instance().StartScheduler();
	while(1) ;
}

bool UpanixMain_IsKernelDebugOn()
{
	const char* szVal = getenv("UPANIX_KDEBUG") ;
	if(szVal != NULL)
		if(strcmp(szVal, "1") == 0)
			return true ;

	return false ;
}

bool UpanixMain_isCoProcPresent()
{
	if(CO_PROC_FPU_TYPE == NO_CO_PROC)
		return false ;
	return true ;
}
