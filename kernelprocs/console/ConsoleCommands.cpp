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

#include <net/ip_icmp.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <openssl/err.h>
#include <ussl.h>
#include <ConsoleCommands.h>
#include <CommandLineParser.h>
#include <Floppy.h>
#include <ProcessManager.h>
#include <FileSystem.h>
#include <MemManager.h>
#include <DMM.h>
#include <ATADeviceController.h>
#include <PartitionManager.h>
#include <FileOperations.h>
#include <UserManager.h>
#include <GenericUtil.h>
#include <SessionManager.h>
#include <StorageDrive.h>
#include <RTC.h>
#include <MultiBoot.h>
#include <SystemUtil.h>
#include <EHCIManager.h>
#include <XHCIManager.h>
#include <BTree.h>
#include <PS2MouseDriver.h>
#include <exception.h>
#include <stdio.h>
#include <PCSound.h>
#include <Apic.h>
#include <typeinfo.h>
#include "network/NetworkManager.h"
#include <ARPHandler.h>
#include <GraphicsContext.h>
#include <MouseEventHandler.h>
#include <Button.h>
#include <IconButton.h>
#include <UIObjectFactory.h>
#include <RoundCanvas.h>
#include <Line.h>
#include <GCoreFunctions.h>
#include <UIPosition.h>
#include <ImageCanvas.h>
#include <Label.h>
#include <VerticalScroller.h>
#include <TextArea.h>
#include <metrics.h>
#include <BmpEncoder.h>
#include <PngEncoder.h>
#include <metrics.h>
#include <SysCall.h>
#include <StorageDriveManager.h>
#include <Terminal.h>
#include <IconLabel.h>
#include <IconImageMap.h>
#include <KernelRootProcess.h>
#include <math.h>
#include <ConfigFileDB.h>
#include <NetworkDevice.h>
#include <RealNetworkDevice.h>

/**** Command Function Declarations  *****/
static void ConsoleCommands_ChangeDrive() ;
static void ConsoleCommands_ShowDrive() ;
static void ConsoleCommands_MountDrive() ;
static void ConsoleCommands_UnMountDrive() ;
static void ConsoleCommands_FormatDrive() ;

static void ConsoleCommands_ClearScreen() ;

static void ConsoleCommands_CreateDirectory() ;
static void ConsoleCommands_RemoveFile() ;
static void ConsoleCommands_ListDirContent() ;
static void ConsoleCommands_ReadFileContent() ;
static void ConsoleCommands_ChangeDirectory() ;
static void ConsoleCommands_PresentWorkingDir() ;
static void ConsoleCommands_CopyFile() ;

static void ConsoleCommands_InitUser() ;
static void ConsoleCommands_ListUser() ;
static void ConsoleCommands_AddUser() ;
static void ConsoleCommands_DeleteUser() ;

static void ConsoleCommands_OpenSession() ;

static void ConsoleCommands_ShowPartitionTable() ;
static void ConsoleCommands_ClearPartitionTable() ;
static void ConsoleCommands_SetSysIdForPartition() ;
static void ConsoleCommands_CreatePrimaryPartition() ;
static void ConsoleCommands_CreateExtendedPartitionEntry() ;
static void ConsoleCommands_CreateExtendedPartition() ;
static void ConsoleCommands_DeletePrimaryPartition() ;
static void ConsoleCommands_DeleteExtendedPartition() ;

static void ConsoleCommands_LoadExe() ;
static void ConsoleCommands_WaitPID() ;
static void ConsoleCommands_Clone() ;
static void ConsoleCommands_Exit() ;
static void ConsoleCommands_Reboot() ;
static void ConsoleCommands_Date() ;
static void ConsoleCommands_MultiBootHeader() ;
static void ConsoleCommands_ListCommands() ;
static void ConsoleCommands_ListProcess() ;
static void ConsoleCommands_ChangeRootDrive() ;
static void ConsoleCommands_Echo() ;
static void ConsoleCommands_Export() ;
static void ConsoleCommands_PerformECHIHandoff() ;
static void ConsoleCommands_ProbeEHCIUSB() ;
static void ConsoleCommands_ProbeXHCIUSB() ;
static void ConsoleCommands_ListNetworkDevices() ;
static void ConsoleCommands_ARPing() ;
static void ConsoleCommands_SetXHCIEventMode();
static void ConsoleCommands_ShowRawDiskList() ;
static void ConsoleCommands_InitFloppyController() ;
static void ConsoleCommands_InitATAController() ;
static void ConsoleCommands_InitNetwork();
static void ConsoleCommands_PrintKPIs();
static void ConsoleCommands_Test() ;
static void ConsoleCommands_Testv() ;
static void ConsoleCommands_TestNet() ;
static void ConsoleCommands_TestCPP() ;
static void ConsoleCommands_TestGraphics() ;
static void ConsoleCommands_Beep();
static void ConsoleCommands_Sleep();
static void ConsoleCommands_Kill();
static void ConsoleCommands_ResetMouse();
static void ConsoleCommands_MemStats();
static void ConsoleCommands_SysLog();
static void ConsoleCommands_Ping();
static void ConsoleCommands_Host();
static void ConsoleCommands_ListServents();

/*****************************************/

int ConsoleCommands_NoOfInterCommands ;

static const ConsoleCommand ConsoleCommands_CommandList[] = {
	{ "chd",		&ConsoleCommands_ChangeDrive },
	{ "shd",		&ConsoleCommands_ShowDrive },
	{ "mount",		&ConsoleCommands_MountDrive },
	{ "umount",		&ConsoleCommands_UnMountDrive },
	{ "format",		&ConsoleCommands_FormatDrive },

	{ "clear",		&ConsoleCommands_ClearScreen },

	{ "mkdir",		&ConsoleCommands_CreateDirectory },
	{ "rm",			&ConsoleCommands_RemoveFile },
	{ "ls",			&ConsoleCommands_ListDirContent },
	{ "read",		&ConsoleCommands_ReadFileContent },
	{ "cd",			&ConsoleCommands_ChangeDirectory },
	{ "pwd",		&ConsoleCommands_PresentWorkingDir },
	{ "cp",			&ConsoleCommands_CopyFile },

	{ "init_usr",	&ConsoleCommands_InitUser },
	{ "list_usr",	&ConsoleCommands_ListUser },
	{ "add_usr",	&ConsoleCommands_AddUser },
	{ "del_usr",	&ConsoleCommands_DeleteUser },

	{ "session",	&ConsoleCommands_OpenSession },

	{ "show_pt",	&ConsoleCommands_ShowPartitionTable },
	{ "clear_pt",	&ConsoleCommands_ClearPartitionTable },
	{ "setsid_pt",	&ConsoleCommands_SetSysIdForPartition },
	{ "create_ppt",	&ConsoleCommands_CreatePrimaryPartition },
	{ "create_xpt",	&ConsoleCommands_CreateExtendedPartitionEntry },
	{ "create_ept",	&ConsoleCommands_CreateExtendedPartition },
	{ "delete_ppt",	&ConsoleCommands_DeletePrimaryPartition },
	{ "delete_ept",	&ConsoleCommands_DeleteExtendedPartition },

	{ "load",		&ConsoleCommands_LoadExe },
	{ "wait",		&ConsoleCommands_WaitPID },
	{ "clone",		&ConsoleCommands_Clone },
	{ "exit",		&ConsoleCommands_Exit },
	{ "reboot",		&ConsoleCommands_Reboot },
	{ "date",		&ConsoleCommands_Date },
	{ "mtbt",		&ConsoleCommands_MultiBootHeader },

	{ "help",		&ConsoleCommands_ListCommands },
	{ "ps",			&ConsoleCommands_ListProcess },
	{ "crd",		&ConsoleCommands_ChangeRootDrive },
	{ "echo",		&ConsoleCommands_Echo },
	{ "export",		&ConsoleCommands_Export },
	{ "ehcihoff",	&ConsoleCommands_PerformECHIHandoff },
	{ "eusbprobe",	&ConsoleCommands_ProbeEHCIUSB },
	{ "xusbprobe",	&ConsoleCommands_ProbeXHCIUSB },
  { "xhciemode", &ConsoleCommands_SetXHCIEventMode },
  { "initnet", &ConsoleCommands_InitNetwork },
  { "lsnet", &ConsoleCommands_ListNetworkDevices },
  { "arping", &ConsoleCommands_ARPing },
  { "ping", &ConsoleCommands_Ping },
  { "host", &ConsoleCommands_Host },
	{ "showdisk",	&ConsoleCommands_ShowRawDiskList },
	{ "initfdc",	&ConsoleCommands_InitFloppyController },
	{ "initata",	&ConsoleCommands_InitATAController },
  { "kpi",	&ConsoleCommands_PrintKPIs },
	{ "testg",		&ConsoleCommands_TestGraphics },
	{ "test",		&ConsoleCommands_Test },
  { "photos",		&ConsoleCommands_Testv },
	{ "testn",		&ConsoleCommands_TestNet },
	{ "testcpp", &ConsoleCommands_TestCPP },
	{ "beep", &ConsoleCommands_Beep },
	{ "sleep", &ConsoleCommands_Sleep },
	{ "kill", &ConsoleCommands_Kill },
	{ "resetmouse", &ConsoleCommands_ResetMouse },
  { "memstats", &ConsoleCommands_MemStats },
  { "syslog", &ConsoleCommands_SysLog },
  { "lsservent", &ConsoleCommands_ListServents },
	{ "\0",			NULL }
} ;

void ConsoleCommands_Init()
{
	for(ConsoleCommands_NoOfInterCommands = 0; 
		ConsoleCommands_CommandList[ConsoleCommands_NoOfInterCommands].szCommand[0] != '\0';
		ConsoleCommands_NoOfInterCommands++) ;
}

void ConsoleCommands_ExecuteInternalCommand(const char* szCommand)
{
	for(int i = 0; i < ConsoleCommands_NoOfInterCommands; i++)
	{
		if(strcmp(szCommand, ConsoleCommands_CommandList[i].szCommand) == 0)
		{
      try
      {
  			ConsoleCommands_CommandList[i].cmdFunc() ;
      }
      catch(const upan::exception& ex)
      {
        ex.Print();
      }
      return;
		}
	}
}

void ConsoleCommands_ChangeDrive() {
  auto& storageDrive = StorageDriveManager::Instance().GetByDriveName(CommandLineParser::Instance().GetParameterAt(0), false).goodValueOrThrow(XLOC);
  auto& pas = ProcessManager::Instance().GetCurrentPAS();
  pas.setDriveID(storageDrive.Id());
  pas.pwd(storageDrive.fileSystem().root());
}

void ConsoleCommands_ShowDrive()
{
	StorageDriveManager::Instance().DisplayList() ;
}

void ConsoleCommands_MountDrive() {
  StorageDriveManager::Instance().MountDrive(CommandLineParser::Instance().GetParameterAt(0));
  printf("\nDrive Mounted");
}

void ConsoleCommands_UnMountDrive() {
  StorageDriveManager::Instance().UnMountDrive(CommandLineParser::Instance().GetParameterAt(0));
  printf("\nDrive Unmounted");
}

void ConsoleCommands_FormatDrive()
{
  StorageDriveManager::Instance().FormatDrive(CommandLineParser::Instance().GetParameterAt(0));
}

void ConsoleCommands_ClearScreen()
{
  KC::MConsole().ClearScreen() ;
}

void ConsoleCommands_CreateDirectory()
{
  FileOperations::Instance().create((char*)(CommandLineParser::Instance().GetParameterAt(0)), ATTR_DIR_DEFAULT);
  printf("\n DIR Created\n");
}

void ConsoleCommands_RemoveFile()
{
  FileOperations::Instance().remove((char*)(CommandLineParser::Instance().GetParameterAt(0)));
  printf("\n DIR Deleted\n");
}

void ConsoleCommands_ListDirContent() {
	const char* szListDirName = "." ;

  if(CommandLineParser::Instance().GetNoOfParameters())
    szListDirName = CommandLineParser::Instance().GetParameterAt(0) ;

  auto dirp = opendir(szListDirName);
  int i = 0;
  while (auto s = readdir(dirp)) {
		if(!(i % 3) && i != 0) {
      printf("\n");
    }
    printf("%-20s", s->d_name) ;
    ++i;
	}
  closedir(dirp);
}

void ConsoleCommands_ReadFileContent()
{
	char bDataBuffer[513] ;
	
  const char* szFileName = CommandLineParser::Instance().GetParameterAt(0) ;

  auto file = FileOperations::Instance().open(szFileName, O_RDONLY, 0);
  if (file.isEmpty()) {
    throw upan::exception(XLOC, "%s: File not found", szFileName);
  }
  printf("\n");
	while(true) {
    int n = file->read(bDataBuffer, 512);

		bDataBuffer[n] = '\0' ;

    if(n < 512) {
      printf("%s", bDataBuffer);
			break ;
		}

    printf("%s", bDataBuffer);
	}

	if(close(file->id())) {
    printf("\n File Close Failed");
		return ;
	}
}

void ConsoleCommands_ChangeDirectory() {
  FileOperations::Instance().changeDir(CommandLineParser::Instance().GetParameterAt(0), nullptr);
}

void ConsoleCommands_PresentWorkingDir() {
	printf("\n%s", getenv("PWD"));
}

void ConsoleCommands_CopyFile() {
	const int iBufSize = 512 ;
	char bDataBuffer[iBufSize] ;

  const char* szFileName = CommandLineParser::Instance().GetParameterAt(0) ;
  auto file = FileOperations::Instance().open(szFileName, O_RDONLY, 0);
  if (file.isEmpty()) {
    throw upan::exception(XLOC, "%s: File not found", szFileName);
  }
  const char* szDestFile = CommandLineParser::Instance().GetParameterAt(1) ;

  FileOperations::Instance().create(szDestFile, ATTR_FILE_DEFAULT);

  auto file1 = FileOperations::Instance().open(szDestFile, O_RDWR, 0);

  printf("\n Progress = ");
	int cr = KC::MConsole().GetCurrentCursorPosition();
	int i = 0 ;
  const struct stat& fStat = file->getStat();
	unsigned fsize = fStat.st_size ;
	if(fsize == 0)
	{
    printf("\n F Size = 0 !!!") ;
		fsize = 1 ;
		//return ;
	}	

	while(true)
	{
    int n = file->read(bDataBuffer, iBufSize);

    if(n < 512)
		{
			if(n > 0)
			  file1->write(bDataBuffer, n);
      KC::MConsole().ShowProgress("", cr, 100) ;
			break ;
		}
		
    file1->write(bDataBuffer, 512);

		i++ ;
    KC::MConsole().ShowProgress("", cr, (i * iBufSize * 100) / fsize) ;
	}

	if(close(file->id())) {
    printf("\n File Close Failed");
		return ;
	}

	if(close(file1->id())) {
	  printf("\n File1 Close Failed");
		return ;
	}
}

void ConsoleCommands_InitUser()
{
  UserManager::Instance();
}

void ConsoleCommands_ListUser()
{
  const auto& users = UserManager::Instance().GetUserList();
	for(auto i : users)
    printf("\n %s", i.first.c_str());
}

void ConsoleCommands_AddUser()
{
	char szUserName[MAX_USER_LENGTH + 1] ;
	char szPassword[MAX_USER_LENGTH + 1] ;
	char szHomeDirPath[MAX_HOME_DIR_LEN + 1] ;

  printf("\n User Name: ");
	GenericUtil_ReadInput(szUserName, MAX_USER_LENGTH, true) ;

  printf("\n Password: ");
	GenericUtil_ReadInput(szPassword, MAX_USER_LENGTH, false) ;

  printf("\n Home dir path: ");
	GenericUtil_ReadInput(szHomeDirPath, MAX_HOME_DIR_LEN, true) ;

	if(UserManager::Instance().Create(szUserName, szPassword, szHomeDirPath, NORMAL_USER))
    printf("\n User created!");
}

void ConsoleCommands_DeleteUser()
{
	char szUserName[MAX_USER_LENGTH + 1] ;

  printf("\n User Name: ");
	GenericUtil_ReadInput(szUserName, MAX_USER_LENGTH, true) ;

  if(UserManager::Instance().Delete(szUserName))
    printf("\n User deleted!");
}

void ConsoleCommands_OpenSession() {
	const int pid = ProcessManager::Instance().CreateKernelProcess("session", (uintptr_t) &SessionManager_StartSession, NO_PROCESS_ID, true, false, upan::vector<uintptr_t>());
  int exitStatus;
  ProcessManager::Instance().WaitOnChild(pid, exitStatus);
}

RawStorageDrive* ConsoleCommands_CheckDiskParam()
{
  if(CommandLineParser::Instance().GetNoOfParameters() < 1)
	{
		printf("\n Disk name parameter missing") ;
		return NULL ;
	}

  RawStorageDrive* pDisk = StorageDriveManager::Instance().GetRawDiskByName(CommandLineParser::Instance().GetParameterAt(0)) ;

	if(!pDisk)
	{
    printf("\n '%s' no such disk", CommandLineParser::Instance().GetParameterAt(0)) ;
		return NULL ;
	}

	return pDisk ;
}

void ConsoleCommands_ShowPartitionTable()
{
	RawStorageDrive* pDisk = ConsoleCommands_CheckDiskParam() ;

	if(!pDisk)
		return ;

	printf("\n Total Sectors = %d", pDisk->SizeInSectors());
	PartitionTable partitionTable(*pDisk);
  partitionTable.VerbosePrint();
}

void ConsoleCommands_ClearPartitionTable()
{
	RawStorageDrive* pDisk = ConsoleCommands_CheckDiskParam() ;
	if(!pDisk)
		return;
	PartitionTable partitionTable(*pDisk);
  partitionTable.ClearPartitionTable();
}

void ConsoleCommands_SetSysIdForPartition()
{
	RawStorageDrive* pDisk = ConsoleCommands_CheckDiskParam() ;
	if(!pDisk)
		return ;
  pDisk->UpdateSystemIndicator(63, 0x83) ;
}

void ConsoleCommands_CreatePrimaryPartition()
{
	RawStorageDrive* pDisk = ConsoleCommands_CheckDiskParam() ;

	if(!pDisk)
		return ;

	PartitionTable partitionTable(*pDisk);

	char szSizeInSectors[11] ;

  printf("\n Size in Sector = ") ;
	GenericUtil_ReadInput(szSizeInSectors, 10, true) ;

  int sizeInSectors = atoi(szSizeInSectors);
  if(sizeInSectors <= 0)
	{
    printf("\n Invalid sector size: %d", sizeInSectors);
		return ;
	}

  partitionTable.CreatePrimaryPartitionEntry(sizeInSectors, MBRPartitionInfo::NORMAL);
  printf("\n Partition Created!");
}

void ConsoleCommands_CreateExtendedPartitionEntry()
{
	RawStorageDrive* pDisk = ConsoleCommands_CheckDiskParam() ;

	if(!pDisk)
		return ;

	PartitionTable partitionTable(*pDisk);
	if(partitionTable.IsEmpty())
	{
    printf("\n No Primary Partitions") ;
		return ;
	}

	char szSizeInSectors[11] ;

  printf("\n Size in Sector = ") ;
	GenericUtil_ReadInput(szSizeInSectors, 10, true) ;

  int sizeInSectors = atoi(szSizeInSectors);
  if(sizeInSectors <= 0)
	{
    printf("\n Invalid sector size %d", sizeInSectors);
		return ;
	}
  partitionTable.CreatePrimaryPartitionEntry(sizeInSectors, MBRPartitionInfo::EXTENEDED);
  printf("\n Partition Created!");
}

void ConsoleCommands_CreateExtendedPartition()
{
	RawStorageDrive* pDisk = ConsoleCommands_CheckDiskParam() ;

	if(!pDisk)
		return ;

	PartitionTable partitionTable(*pDisk);
	if(partitionTable.IsEmpty())
	{
    printf("\n No Primary Partitions") ;
		return ;
	}

	char szSizeInSectors[11] ;
  printf("\n Size in Sector = ") ;
	GenericUtil_ReadInput(szSizeInSectors, 10, true) ;

  int sizeInSectors = atoi(szSizeInSectors);
  if(sizeInSectors <= 0)
	{
    printf("\n Invalid sector size: %d", sizeInSectors);
		return ;
	}

  partitionTable.CreateExtPartitionEntry(sizeInSectors) ;
	printf("\n Partition Created");
}

void ConsoleCommands_DeletePrimaryPartition()
{
	RawStorageDrive* pDisk = ConsoleCommands_CheckDiskParam() ;

	if(!pDisk)
		return ;

	PartitionTable partitionTable(*pDisk);
	if(partitionTable.IsEmpty())
	{
    printf("\n Disk Not Partitioned") ;
		return ;
	}

	partitionTable.DeletePrimaryPartition() ;
  printf("\n Partition Deleted");
}

void ConsoleCommands_DeleteExtendedPartition()
{
	RawStorageDrive* pDisk = ConsoleCommands_CheckDiskParam() ;

	if(!pDisk)
		return ;

	PartitionTable partitionTable(*pDisk);
	if(partitionTable.IsEmpty())
	{
    printf("\n Disk Not Partitioned") ;
		return ;
	}

	partitionTable.DeleteExtPartition() ;
  printf("\n Partition Deleted");
}

void ConsoleCommands_LoadExe() {
  upan::vector<upan::string> argv;
  argv.push_back("100");
  argv.push_back("200");
  argv.push_back("300");

  upan::vector<upan::string> envp;
  envp.push_back("PATH=/bin");
  envp.push_back("OS=Upanix");

  auto& clp = CommandLineParser::Instance();
  const auto runInBG = clp.GetNoOfParameters() > 1 && strcmp("&", clp.GetParameterAt(1)) == 0;
  const int iChildProcessID = ProcessManager::Instance().Create(clp.GetParameterAt(0),
                                                                ProcessManager::GetCurrentProcessID(), true,
                                                                DERIVE_FROM_PARENT, argv, envp);
  if (iChildProcessID < 0) {
    printf("\n Load User Process Failed: %d", iChildProcessID);
  } else if (!runInBG) {
    int exitStatus;
    ProcessManager::Instance().WaitOnChild(iChildProcessID, exitStatus);
  }
}

void ConsoleCommands_WaitPID()
{
  int pid = atoi(CommandLineParser::Instance().GetParameterAt(0));
  if(pid <= 0)
    printf("\n Invalid PID : %d", pid);
  else {
    int exitStatus;
    ProcessManager::Instance().WaitOnChild(pid, exitStatus);
  }
}

void ConsoleCommands_Exit()
{
  ProcessManager_Exit(0);
}

void ConsoleCommands_Clone()
{
	extern void Console_StartUpanixConsole() ;
  const int pid = ProcessManager::Instance().CreateKernelProcess("console_1", (uintptr_t) &Console_StartUpanixConsole,
                                                 ProcessManager::GetCurrentProcessID(), true, false, upan::vector<uintptr_t>()) ;
  int exitStatus;
  ProcessManager::Instance().WaitOnChild(pid, exitStatus);
}

void ConsoleCommands_Reboot() {
	KC::MKernelService().RequestSystemReboot();
}

void ConsoleCommands_Date()
{
	RTCDateTime rtcDateTime ;
	RTC::GetDateTime(rtcDateTime) ;
	
	printf("\n %d/%d/%d - %d:%d:%d", rtcDateTime._dayOfMonth, rtcDateTime._month,
		rtcDateTime._century * 100 + rtcDateTime._year, rtcDateTime._hour, rtcDateTime._minute,
		rtcDateTime._second) ;
}

void ConsoleCommands_MultiBootHeader()
{
  MultiBoot::Instance().Print();
  if(typeid(IrqManager::Instance()) == typeid(Apic))
    printf("\n Using APIC...");
}

void ConsoleCommands_ListCommands() {
	for(int i = 0; i < ConsoleCommands_NoOfInterCommands; i++) {
		if(!(i % 3))
      printf("\n");
		printf("%-20s", ConsoleCommands_CommandList[i].szCommand);
	}
}

void ConsoleCommands_ListProcess()
{
	unsigned uiSize;
	PS* pPS = ProcessManager::Instance().GetProcList(uiSize);
	
	unsigned i ;
	for(i = 0; i < uiSize; i++) {
    if (i != 0) {
      putchar('\n');
    }
		printf("%-7d%-5d%-5d%-18s%-5d%-10s", pPS[i].pid, pPS[i].iParentProcessID, pPS[i].iProcessGroupID,
           get_proc_status_desc(pPS[i].status), pPS[i].iUserID, pPS[i].pname) ;
	}

	ProcessManager::Instance().FreeProcListMem(pPS, uiSize) ;
}

void ConsoleCommands_ChangeRootDrive() {
  StorageDriveManager::Instance().SetRootDrive(CommandLineParser::Instance().GetParameterAt(0));
}

void ConsoleCommands_Echo()
{
  if(CommandLineParser::Instance().GetNoOfParameters())
    printf("%s", CommandLineParser::Instance().GetParameterAt(0)) ;
}

void ConsoleCommands_Export()
{
    if(CommandLineParser::Instance().GetNoOfParameters() < 1)
        return ;

    const char* szExp = CommandLineParser::Instance().GetParameterAt(0) ;
    const char* var = szExp ;
    char* val = strchr(szExp, '=') ;

    if(val == NULL)
    {
      printf("\n Incorrect syntax");
      return ;
    }

    val[0] = '\0' ;
    val = val + 1 ;

    if(setenv(var, val, 1) < 0) {
      printf("\n Failed to set env variable\n") ;
      return ;
    }
}

void ConsoleCommands_PerformECHIHandoff()
{
	EHCIManager::Instance().RouteToCompanionController() ;
}

void ConsoleCommands_ProbeEHCIUSB()
{
	EHCIManager::Instance().ProbeDevice() ;
}

void ConsoleCommands_ProbeXHCIUSB()
{
	XHCIManager::Instance().ProbeDevice() ;
}

void ConsoleCommands_ListNetworkDevices() {
  bool first = true;
  for(const auto d : NetworkManager::Instance().devices()) {
    if (first) {
      first = false;
    } else {
      printf("\n");
    }
    d->print();
  }

  printf("\n");
  for(const auto& d : NetworkManager::Instance().getIPMACTable()) {
    printf("\n %s -> %s", inet_ntoa({ d.first }), d.second.str().c_str());
  }
}

void ConsoleCommands_ARPing() {
  auto d = NetworkManager::Instance().getDefaultRealDevice();
  if (d.isEmpty()) {
    printf("\nno network device exists");
    return;
  }

  if(CommandLineParser::Instance().GetNoOfParameters() < 1) {
    printf("\nmissing parameter");
    return;
  }

  auto& device = d.value();
  const upan::string param(CommandLineParser::Instance().GetParameterAt(0));

  struct in_addr addr {};
  inet_aton(param.c_str(), &addr);
  const MACAddress mac = device.getARPClient().resolveMacAddress(addr.s_addr);
  printf("MAC: %s", mac.str().c_str());
}

void ConsoleCommands_InitNetwork() {
  NetworkManager::Instance().Initialize();
}

void ConsoleCommands_SetXHCIEventMode()
{
  if(CommandLineParser::Instance().GetNoOfParameters() < 1)
      return ;

  upan::string mode(CommandLineParser::Instance().GetParameterAt(0));
  if(mode == "poll")
    XHCIManager::Instance().SetEventMode(XHCIManager::Poll);
  else if(mode == "int")
    XHCIManager::Instance().SetEventMode(XHCIManager::Interrupt);
  else
    printf("\n Invalid EventMode");
}

void ConsoleCommands_ShowRawDiskList()
{
  unsigned uiCount = CommandLineParser::Instance().GetNoOfParameters() ;
	RawStorageDrive* pDisk = NULL ;

	printf("%-15s %-18s %-10s %-15s %-10s", "Name", "Type", "Sec-Size", "Tot-Sectors", "Size (MB)") ;
	printf("\n-----------------------------------------------------------------------") ;

	class LamdaDisplay
	{
		public:
			void operator()(const RawStorageDrive* pParamDisk)
			{
				static const char szTypes[2][32] = { "ATA Hard Disk", "USB SCSI Disk" } ;

				printf("\n%-15s %-18s %-10d %-15d %-10d", pParamDisk->Name().c_str(), 
          szTypes[pParamDisk->Type() - RawStorageDrive::ATA_HARD_DISK],
					pParamDisk->SectorSize(), 
          pParamDisk->SizeInSectors(), 
          pParamDisk->SectorSize() * pParamDisk->SizeInSectors() / (1024 * 1024));
			}

	} lamdaDisplay ;

	if(uiCount)
	{
		for(unsigned i = 0; i < uiCount; i++)
		{
      const char* szName = CommandLineParser::Instance().GetParameterAt(i) ;
			pDisk = StorageDriveManager::Instance().GetRawDiskByName(szName) ;
			lamdaDisplay(pDisk) ;
		}
	}
	else
	{
    for(auto pDisk : StorageDriveManager::Instance().RawDiskDriveList())
			lamdaDisplay(pDisk) ;
	}
}

void ConsoleCommands_InitFloppyController()
{
	if(!Floppy_GetInitStatus())
		Floppy_Initialize() ;
	else
		printf("Floppy Controller already initialized") ;
}

void ConsoleCommands_InitATAController()
{
  ATADeviceController_Initialize() ;
}

class DragMouseHandler : public upanui::MouseEventHandler {
  void onEvent(upanui::UIObject& uiObject, const upanui::MouseEvent& event) override {
    const upanui::MouseData& data = event.getData();
    if (data.leftButtonState() == upanui::MouseData::HOLD) {
      uiObject.xy(uiObject.x() + data.deltaX(), uiObject.y() + data.deltaY());
    }
  }
};

class PassThroughMouseHandler : public upanui::MouseEventHandler {
public:
  void onEvent(upanui::UIObject& uiObject, const upanui::MouseEvent& event) override {
    const upanui::MouseData& data = event.getData();
    if (data.leftButtonState() == upanui::MouseData::HOLD) {
      uiObject.parent().xy(uiObject.parent().x() + data.deltaX(), uiObject.parent().y() + data.deltaY());
    }
  }
};

class CloseButtonMouseHandler : public upanui::MouseEventHandler {
public:
  void onEvent(upanui::UIObject& uiObject, const upanui::MouseEvent& event) override {
    const upanui::MouseData& data = event.getData();
    if (data.leftButtonState() == upanui::MouseData::RELEASED) {
      exit(0);
    }
  }
};

void graphics_terminal(int x, int y);

class DesktopMouseHandler : public upanui::MouseEventHandler {
public:
  typedef upan::map<upanui::UIObject*, upan::string> IconMap;
  IconMap& _m;

  explicit DesktopMouseHandler(IconMap& m) : _m(m) {}

  void onEvent(upanui::UIObject& uiObject, const upanui::MouseEvent& event) override {
    const upanui::MouseData& data = event.getData();

    auto it = _m.find(&uiObject);
    if (it != _m.end()) {
      auto il = dynamic_cast<upanui::IconLabel*>(&uiObject);
      if (data.isDoubleClick()) {
        il->select(false);
        if (it->second == "terminal") {
          upan::vector<uintptr_t> params;
          params.push_back(100);
          params.push_back(100);
          ProcessManager::Instance().CreateKernelProcess(it->second, (uintptr_t) &graphics_terminal, NO_PROCESS_ID, true, false, params);
        }
      } else if (data.leftButtonState() == upanui::MouseData::PRESSED) {
        il->select(!il->isSelected());
      }
    }
  }
};

class SlideShow : public upan::thread {
public:
  SlideShow(upanui::ImageCanvas& c, const upan::vector<upanui::Image*> images) : _c(c), _images(images) {
  }

  void run() override {
    for(auto image : _images) {
      _c.setImage(*image);
      sleepms(5000);
    }
  }
private:
  upanui::ImageCanvas& _c;
  upan::vector<upanui::Image*> _images;
};

void graphics_photos(int x, int y) {
  const upan::string listDirPath("usdb@/pictures/family/");

  auto dirp = opendir(listDirPath.c_str());
  FileOperations::Instance().changeDir(listDirPath, nullptr);

  upan::vector<upanui::Image*> images;
  while (auto s = readdir(dirp)) {
    if (S_ISFILE(s->d_stat.st_mode)) {
      const auto fileSize = s->d_stat.st_size;
      auto file = FileOperations::Instance().open(s->d_name, O_RDONLY, 0);
      if (file.isEmpty()) {
        continue;
      }
      file->seek(SEEK_SET, 0);
      upan::uniq_ptr<char[]> buffer(new char[fileSize]);
      file->read(buffer.get(), fileSize);
      upanui::BmpEncoder decoder;
      upanui::Image& image = decoder.decode(buffer.get(), upan::option<uint32_t>::empty());
      images.push_back(&image);
      close(file->id());
    }
  }
  closedir(dirp);

  if (images.empty()) {
    printf("\n No images found");
    while(1);
    exit(0);
  }

  const int photoCanvasWidth = 500, photoCanvasHeight = 500;
  upanui::GraphicsContext::Init();
  auto& gc = upanui::GraphicsContext::Instance();
  auto& uiRoot = gc.initUIRoot(x, y, photoCanvasWidth + 10, photoCanvasHeight + 10, true);
  uiRoot.backgroundColor(ColorPalettes::CP256::Get(15));
  uiRoot.borderThickness(5);

  DragMouseHandler mouseHandler;
  uiRoot.registerMouseEventHandler(mouseHandler);
  PassThroughMouseHandler passThroughMouseHandler;

  auto &ic = upanui::UIObjectFactory::createImageCanvas(uiRoot, *images[0], 0, 0, photoCanvasWidth, photoCanvasHeight,
                                                        upanui::HorizontalPlacementType::ABSOLUTE, upanui::VerticalPlacementType::ABSOLUTE);
  ic.registerMouseEventHandler(passThroughMouseHandler);

  SlideShow ss(ic, images);
  ss.start();

  gc.eventManager().startEventLoop();

  exit(0);
}

void graphics_test_process_canvas(int x, int y) {
  upanui::GraphicsContext::Init();
  auto& gc = upanui::GraphicsContext::Instance();
  auto& uiRoot = gc.initUIRoot(x, y, 400, 400, true);
  uiRoot.backgroundColor(ColorPalettes::CP256::Get(15));
  uiRoot.backgroundColorAlpha(upanui::GCoreFunctions::percentToAlpha(50));
  uiRoot.borderThickness(5);

  auto& cc1 = upanui::UIObjectFactory::createRoundCanvas(uiRoot, 120, 10, 120, 120, upanui::HorizontalPlacementType::ABSOLUTE, upanui::VerticalPlacementType::ABSOLUTE);
  cc1.backgroundColor(ColorPalettes::CP256::Get(230));
  cc1.backgroundColorAlpha(upanui::GCoreFunctions::percentToAlpha(80));
  cc1.borderColor(ColorPalettes::CP256::Get(177));
  cc1.borderColorAlpha(upanui::GCoreFunctions::percentToAlpha(100));
  cc1.borderThickness(10);

  auto& cc2 = upanui::UIObjectFactory::createRoundCanvas(uiRoot, 250, 10, 120, 120, upanui::HorizontalPlacementType::ABSOLUTE, upanui::VerticalPlacementType::ABSOLUTE);
  cc2.backgroundColor(ColorPalettes::CP256::Get(230));
  cc2.backgroundColorAlpha(upanui::GCoreFunctions::percentToAlpha(100));
  cc2.borderColor(ColorPalettes::CP256::Get(177));
  cc2.borderColorAlpha(upanui::GCoreFunctions::percentToAlpha(100));
  cc2.borderThickness(10);

  auto& cpt = upanui::UIObjectFactory::createRectangleCanvas(uiRoot, -10, 180, 100, 100,
                                                             upanui::HorizontalPlacementType::ABSOLUTE, upanui::VerticalPlacementType::ABSOLUTE);
  cpt.backgroundColor(ColorPalettes::CP256::Get(30));
  cpt.backgroundColorAlpha(upanui::GCoreFunctions::percentToAlpha(50));

  auto& cpt1 = upanui::UIObjectFactory::createRectangleCanvas(cpt, -10, 20, 60, 60,
                                                              upanui::HorizontalPlacementType::ABSOLUTE, upanui::VerticalPlacementType::ABSOLUTE);
  cpt1.backgroundColor(ColorPalettes::CP256::Get(0));
  cpt1.borderColor(ColorPalettes::CP256::Get(255));
  cpt1.borderThickness(2);
  cpt1.backgroundColorAlpha(upanui::GCoreFunctions::percentToAlpha(50));
  cpt1.borderColorAlpha(upanui::GCoreFunctions::percentToAlpha(50));

  auto& cp1c1 = upanui::UIObjectFactory::createRectangleCanvas(uiRoot, 180, 180, 130, 40, upanui::HorizontalPlacementType::ABSOLUTE, upanui::VerticalPlacementType::ABSOLUTE);
  cp1c1.backgroundColor(ColorPalettes::CP256::Get(0));
  cp1c1.backgroundColorAlpha(upanui::GCoreFunctions::percentToAlpha(25));

  auto& cp1c2 = upanui::UIObjectFactory::createRectangleCanvas(uiRoot, 180, 220, 130, 40, upanui::HorizontalPlacementType::ABSOLUTE, upanui::VerticalPlacementType::ABSOLUTE);
  cp1c2.backgroundColor(ColorPalettes::CP256::Get(10));

  auto& cp1c3 = upanui::UIObjectFactory::createRectangleCanvas(uiRoot, 180, 260, 130, 40, upanui::HorizontalPlacementType::ABSOLUTE, upanui::VerticalPlacementType::ABSOLUTE);
  cp1c3.backgroundColor(ColorPalettes::CP256::Get(20));
  cp1c3.backgroundColorAlpha(upanui::GCoreFunctions::percentToAlpha(25));

  auto& cp1 = upanui::UIObjectFactory::createRectangleCanvas(uiRoot, 200, 200, 100, 100, upanui::HorizontalPlacementType::ABSOLUTE, upanui::VerticalPlacementType::ABSOLUTE);
  cp1.backgroundColorAlpha(upanui::GCoreFunctions::percentToAlpha(70));
  cp1.backgroundColor(ColorPalettes::CP256::Get(25));

  auto& ci1 = upanui::UIObjectFactory::createButton(cp1, 40, 10, 50, 30,
                                                    upanui::HorizontalPlacementType::ABSOLUTE, upanui::VerticalPlacementType::ABSOLUTE);
  ci1.backgroundColor(ColorPalettes::CP256::Get(25));
  auto& ci2 = upanui::UIObjectFactory::createButton(ci1, 10, 2, 30, 6,
                                                    upanui::HorizontalPlacementType::ABSOLUTE, upanui::VerticalPlacementType::ABSOLUTE);
  ci2.backgroundColor(ColorPalettes::CP256::Get(85));

  auto& c1 = upanui::UIObjectFactory::createButton(cp1, -50, 30, 80, 50,
                                                   upanui::HorizontalPlacementType::ABSOLUTE, upanui::VerticalPlacementType::ABSOLUTE);
  c1.backgroundColor(ColorPalettes::CP256::Get(25));
  auto& c2 = upanui::UIObjectFactory::createButton(c1, 10, 10, 30, 10,
                                                   upanui::HorizontalPlacementType::ABSOLUTE, upanui::VerticalPlacementType::ABSOLUTE);
  c2.backgroundColor(ColorPalettes::CP256::Get(45));
  auto& c3 = upanui::UIObjectFactory::createButton(c1, 10, 10, 50, 10,
                                                   upanui::HorizontalPlacementType::ABSOLUTE, upanui::VerticalPlacementType::ABSOLUTE);
  c3.backgroundColor(ColorPalettes::CP256::Get(65));
  auto& c4 = upanui::UIObjectFactory::createButton(c1, 60, 20, 20, 10,
                                                   upanui::HorizontalPlacementType::ABSOLUTE, upanui::VerticalPlacementType::ABSOLUTE);
  c4.backgroundColor(ColorPalettes::CP256::Get(85));
  auto& c5 = upanui::UIObjectFactory::createButton(c1, -10, 35, 80, 10,
                                                   upanui::HorizontalPlacementType::ABSOLUTE, upanui::VerticalPlacementType::ABSOLUTE);
  c5.backgroundColor(ColorPalettes::CP256::Get(75));

  const uint32_t btpColor = ColorPalettes::CP256::Get(10);

  auto& bp1 = upanui::UIObjectFactory::createButton(uiRoot, 10, 50, 100, 100,
                                                    upanui::HorizontalPlacementType::ABSOLUTE, upanui::VerticalPlacementType::ABSOLUTE);
  bp1.backgroundColor(btpColor);
  //bp1.backgroundColorAlpha(upanui::GCoreFunctions::percentToAlpha(0));
  bp1.borderThickness(5);

  const uint32_t btColor = ColorPalettes::CP256::Get(25);
  auto& b1 = upanui::UIObjectFactory::createButton(bp1, 50, 50, 30, 20, upanui::HorizontalPlacementType::ABSOLUTE, upanui::VerticalPlacementType::ABSOLUTE);
  b1.backgroundColor(btColor);

  auto& b2 = upanui::UIObjectFactory::createButton(bp1, 0, 50, 30, 20, upanui::HorizontalPlacementType::ABSOLUTE, upanui::VerticalPlacementType::ABSOLUTE);
  b2.backgroundColor(btColor);

  auto& b2a = upanui::UIObjectFactory::createButton(bp1, bp1.borderThickness(), 75, 30, 20, upanui::HorizontalPlacementType::ABSOLUTE, upanui::VerticalPlacementType::ABSOLUTE);
  b2a.backgroundColor(btColor);

  auto& b3 = upanui::UIObjectFactory::createButton(bp1, 50, 0, 30, 20, upanui::HorizontalPlacementType::ABSOLUTE, upanui::VerticalPlacementType::ABSOLUTE);
  b3.backgroundColor(btColor);

  auto& b4 = upanui::UIObjectFactory::createButton(bp1, -10, 10, 30, 20, upanui::HorizontalPlacementType::ABSOLUTE, upanui::VerticalPlacementType::ABSOLUTE);
  b4.backgroundColor(btColor);

  auto& b5 = upanui::UIObjectFactory::createButton(bp1, 65, -10, 30, 20, upanui::HorizontalPlacementType::ABSOLUTE, upanui::VerticalPlacementType::ABSOLUTE);
  b5.backgroundColor(btColor);

  auto& b6 = upanui::UIObjectFactory::createButton(bp1, 80, -10, 30, 20, upanui::HorizontalPlacementType::ABSOLUTE, upanui::VerticalPlacementType::ABSOLUTE);
  b6.backgroundColor(btColor);

  auto& b7 = upanui::UIObjectFactory::createButton(bp1, 80, 50, 30, 20, upanui::HorizontalPlacementType::ABSOLUTE, upanui::VerticalPlacementType::ABSOLUTE);
  b7.backgroundColor(btColor);

  auto& b8 = upanui::UIObjectFactory::createButton(bp1, 80, 85, 30, 20, upanui::HorizontalPlacementType::ABSOLUTE, upanui::VerticalPlacementType::ABSOLUTE);
  b8.backgroundColor(btColor);

  auto& b9 = upanui::UIObjectFactory::createButton(bp1, 30, 90, 30, 20, upanui::HorizontalPlacementType::ABSOLUTE, upanui::VerticalPlacementType::ABSOLUTE);
  b9.backgroundColor(btColor);

  DragMouseHandler mouseHandler;
  uiRoot.registerMouseEventHandler(mouseHandler);
  cc1.registerMouseEventHandler(mouseHandler);
  PassThroughMouseHandler passThroughMouseHandler;
  cc2.registerMouseEventHandler(passThroughMouseHandler);
//  bp1.addMouseEventHandler(mouseHandler);
  //b1.addMouseEventHandler(mouseHandler);

  gc.eventManager().startEventLoop();

  exit(0);
}

void graphics_test_process_line(int x, int y) {
  upanui::GraphicsContext::Init();
  auto& gc = upanui::GraphicsContext::Instance();
  auto& uiRoot = gc.initUIRoot(x, y, 500, 400, true);
  uiRoot.backgroundColor(ColorPalettes::CP256::Get(15));
  uiRoot.backgroundColorAlpha(upanui::GCoreFunctions::percentToAlpha(50));
  uiRoot.borderThickness(5);

  auto& lineh = upanui::UIObjectFactory::createLine(uiRoot, 10, 20, 100, 20, 10, upanui::HorizontalPlacementType::ABSOLUTE, upanui::VerticalPlacementType::ABSOLUTE);
  lineh.backgroundColor(ColorPalettes::CP256::Get(190));

  auto& linev = upanui::UIObjectFactory::createLine(uiRoot, 120, 10, 120, 50, 10, upanui::HorizontalPlacementType::ABSOLUTE, upanui::VerticalPlacementType::ABSOLUTE);
  linev.backgroundColor(ColorPalettes::CP256::Get(190));

  auto& line2 = upanui::UIObjectFactory::createLine(uiRoot, 345, 10, 365, 300, 25, upanui::HorizontalPlacementType::ABSOLUTE, upanui::VerticalPlacementType::ABSOLUTE);
  line2.backgroundColor(ColorPalettes::CP256::Get(190));
  line2.backgroundColorAlpha(upanui::GCoreFunctions::percentToAlpha(100));

  auto& line2a = upanui::UIObjectFactory::createLine(uiRoot, 375, 10, 395, 300, 25, upanui::HorizontalPlacementType::ABSOLUTE, upanui::VerticalPlacementType::ABSOLUTE);
  line2a.backgroundColor(ColorPalettes::CP256::Get(190));
  line2a.backgroundColorAlpha(upanui::GCoreFunctions::percentToAlpha(50));

  auto& line2o = upanui::UIObjectFactory::createLine(uiRoot, 455, 10, 435, 300, 25, upanui::HorizontalPlacementType::ABSOLUTE, upanui::VerticalPlacementType::ABSOLUTE);
  line2o.backgroundColor(ColorPalettes::CP256::Get(190));
  line2o.backgroundColorAlpha(upanui::GCoreFunctions::percentToAlpha(100));

  auto& line2oa = upanui::UIObjectFactory::createLine(uiRoot, 485, 10, 465, 300, 25, upanui::HorizontalPlacementType::ABSOLUTE, upanui::VerticalPlacementType::ABSOLUTE);
  line2oa.backgroundColor(ColorPalettes::CP256::Get(190));
  line2oa.backgroundColorAlpha(upanui::GCoreFunctions::percentToAlpha(50));

  auto& line21 = upanui::UIObjectFactory::createLine(uiRoot, 260, 10, 280, 300, 2, upanui::HorizontalPlacementType::ABSOLUTE, upanui::VerticalPlacementType::ABSOLUTE);
  line21.backgroundColor(ColorPalettes::CP256::Get(190));

  auto& line21o = upanui::UIObjectFactory::createLine(uiRoot, 310, 10, 290, 300, 2, upanui::HorizontalPlacementType::ABSOLUTE, upanui::VerticalPlacementType::ABSOLUTE);
  line21o.backgroundColor(ColorPalettes::CP256::Get(190));

  auto& line22 = upanui::UIObjectFactory::createLine(uiRoot, 200, 10, 220, 300, 1, upanui::HorizontalPlacementType::ABSOLUTE, upanui::VerticalPlacementType::ABSOLUTE);
  line22.backgroundColor(ColorPalettes::CP256::Get(190));

  auto& line22o = upanui::UIObjectFactory::createLine(uiRoot, 250, 10, 230, 300, 1, upanui::HorizontalPlacementType::ABSOLUTE, upanui::VerticalPlacementType::ABSOLUTE);
  line22o.backgroundColor(ColorPalettes::CP256::Get(190));

  auto& line3 = upanui::UIObjectFactory::createLine(uiRoot, 20, 200, 200, 220, 25, upanui::HorizontalPlacementType::ABSOLUTE, upanui::VerticalPlacementType::ABSOLUTE);
  line3.backgroundColor(ColorPalettes::CP256::Get(190));
  line3.backgroundColorAlpha(upanui::GCoreFunctions::percentToAlpha(100));

  auto& line3a = upanui::UIObjectFactory::createLine(uiRoot, 20, 230, 200, 250, 25, upanui::HorizontalPlacementType::ABSOLUTE, upanui::VerticalPlacementType::ABSOLUTE);
  line3a.backgroundColor(ColorPalettes::CP256::Get(190));
  line3a.backgroundColorAlpha(upanui::GCoreFunctions::percentToAlpha(50));

  auto& line3o = upanui::UIObjectFactory::createLine(uiRoot, 20, 350, 200, 330, 25, upanui::HorizontalPlacementType::ABSOLUTE, upanui::VerticalPlacementType::ABSOLUTE);
  line3o.backgroundColor(ColorPalettes::CP256::Get(190));
  line3o.backgroundColorAlpha(upanui::GCoreFunctions::percentToAlpha(100));

  auto& line3oa = upanui::UIObjectFactory::createLine(uiRoot, 20, 380, 200, 360, 25, upanui::HorizontalPlacementType::ABSOLUTE, upanui::VerticalPlacementType::ABSOLUTE);
  line3oa.backgroundColor(ColorPalettes::CP256::Get(190));
  line3oa.backgroundColorAlpha(upanui::GCoreFunctions::percentToAlpha(50));

  auto& line31 = upanui::UIObjectFactory::createLine(uiRoot, 20, 50, 200, 70, 1, upanui::HorizontalPlacementType::ABSOLUTE, upanui::VerticalPlacementType::ABSOLUTE);
  line31.backgroundColor(ColorPalettes::CP256::Get(190));

  auto& line31o = upanui::UIObjectFactory::createLine(uiRoot, 20, 100, 200, 80, 1, upanui::HorizontalPlacementType::ABSOLUTE, upanui::VerticalPlacementType::ABSOLUTE);
  line31o.backgroundColor(ColorPalettes::CP256::Get(190));

  auto& line32 = upanui::UIObjectFactory::createLine(uiRoot, 20, 110, 80, 170, 5, upanui::HorizontalPlacementType::ABSOLUTE, upanui::VerticalPlacementType::ABSOLUTE);
  line32.backgroundColor(ColorPalettes::CP256::Get(190));

//  auto& line32a = upanui::UIObjectFactory::createLine(uiRoot, 20, 110, 200, 130, 2);
//  line32a.backgroundColor(ColorPalettes::CP256::Get(190));

  auto& line32o = upanui::UIObjectFactory::createLine(uiRoot, 20, 160, 80, 100, 10, upanui::HorizontalPlacementType::ABSOLUTE, upanui::VerticalPlacementType::ABSOLUTE);
  line32o.backgroundColor(ColorPalettes::CP256::Get(190));

//  auto& line32oa = upanui::UIObjectFactory::createLine(uiRoot, 20, 160, 200, 140, 2);
//  line32oa.backgroundColor(ColorPalettes::CP256::Get(190));

  DragMouseHandler mouseHandler;
  uiRoot.registerMouseEventHandler(mouseHandler);
//  bp1.addMouseEventHandler(mouseHandler);
  //b1.addMouseEventHandler(mouseHandler);

  gc.eventManager().startEventLoop();

  exit(0);
}

void graphics_test_flag(int x, int y) {
  upanui::GraphicsContext::Init();
  auto& gc = upanui::GraphicsContext::Instance();
  auto& uiRoot = gc.initUIRoot(x, y, 450, 300, true);
  uiRoot.backgroundColor(ColorPalettes::CP256::Get(15));
  uiRoot.setResizable(true, true);

  auto& orange = upanui::UIObjectFactory::createRectangleCanvas(uiRoot, 0, 0, 450, 100, upanui::HorizontalPlacementType::STRETCHED, upanui::VerticalPlacementType::STRETCHED);
  orange.backgroundColor(0xFF9933);

  auto& white = upanui::UIObjectFactory::createRectangleCanvas(uiRoot, 0, 100, 450, 100, upanui::HorizontalPlacementType::STRETCHED, upanui::VerticalPlacementType::STRETCHED);
  white.backgroundColor(ColorPalettes::CP256::Get(255));

  auto& green = upanui::UIObjectFactory::createRectangleCanvas(uiRoot, 0, 200, 450, 100, upanui::HorizontalPlacementType::STRETCHED, upanui::VerticalPlacementType::STRETCHED);
  green.backgroundColor(0x138808);

  int r = 45;
  int cx = 225;
  int cy = 150;
  float spokeXf[] = { 0.13053, 0.38268, 0.60876};
  float spokeYf[] = { 0.99144, 0.92388, 0.7933 };

  int spokeX[24], spokeY[24];
  for(int i = 0; i < 3; ++i) {
    spokeX[i] = roundtoi(spokeXf[i] * r);
    spokeY[i] = roundtoi(spokeYf[i] * r);

    spokeX[5 - i] = spokeY[i];
    spokeY[5 - i] = spokeX[i];

    spokeX[6 + i] = spokeX[5 - i];
    spokeY[6 + i] = -spokeY[5 - i];

    spokeX[17 - i] = -spokeX[5 - i];
    spokeY[17 - i] = -spokeY[5 - i];

    spokeX[18 + i] = -spokeX[5 - i];
    spokeY[18 + i] = spokeY[5 - i];

    spokeX[11 - i] = spokeX[i];
    spokeY[11 - i] = -spokeY[i];

    spokeX[12 + i] = -spokeX[i];
    spokeY[12 + i] = -spokeY[i];

    spokeX[23 - i] = -spokeX[i];
    spokeY[23 - i] = spokeY[i];
  }

  for(int i = 0; i < 24; ++i) {
    const auto x = cx + spokeX[i];
    const auto y = cy - spokeY[i];
    auto& line = upanui::UIObjectFactory::createLine(uiRoot, cx, cy, x, y, 1, upanui::HorizontalPlacementType::ABSOLUTE, upanui::VerticalPlacementType::ABSOLUTE);
    line.backgroundColor(ColorPalettes::CP256::Get(4));
  }

  auto& wheel = upanui::UIObjectFactory::createRoundCanvas(uiRoot, cx - r - 1, cy - r - 1, 2 * (r + 1), 2 * (r + 1), upanui::HorizontalPlacementType::ABSOLUTE, upanui::VerticalPlacementType::ABSOLUTE);
  wheel.borderColor(ColorPalettes::CP256::Get(4));
  wheel.borderThickness(5);
  wheel.backgroundColorAlpha(upanui::GCoreFunctions::percentToAlpha(0));

  DragMouseHandler mouseHandler;
  uiRoot.registerMouseEventHandler(mouseHandler);
//  bp1.addMouseEventHandler(mouseHandler);
  //b1.addMouseEventHandler(mouseHandler);

  gc.eventManager().startEventLoop();

  exit(0);
}

class DemoClock : public upan::thread {
public:
  DemoClock(upanui::UIRoot& uiRoot)
      : _uiRoot(uiRoot), _csize(uiRoot.width() - (PADDING * 2) - (uiRoot.borderThickness() * 2)), _htomFactor(5.0f / 60),
        _cx(_csize / 2 - BORDER_THICKNESS), _cy(_csize / 2 - BORDER_THICKNESS) {
    const int r = _csize / 2 - BORDER_THICKNESS - LABEL_SPACE;
    populateSteps(_secondSteps, r - 10);
    populateSteps(_minuteSteps, r - 5);
    populateSteps(_hourSteps, r - 20);
    populateSteps(_labels, r + 3);
    initLayout();
  }

  void populateSteps(upanui::UIPosition (&steps)[60], const int r) {
    if (r < 0) {
      throw upan::exception(XLOC, "csize (%u) is smaller than minimum length", _csize);
    }

    for(int i = 0; i < X_Y_CO_SIZE; ++i) {
      steps[i] = upanui::UIPosition(roundtoi(X_CO[i] * r), roundtoi(Y_CO[i] * r));

      if (i == 0) {
        steps[15] = upanui::UIPosition(steps[i].y(), steps[i].x());
        steps[30] = upanui::UIPosition(steps[i].x(), -steps[i].y());
        steps[45] = upanui::UIPosition(-steps[i + 15].x(), steps[i + 15].y());
      } else {
        steps[15 - i] = upanui::UIPosition(steps[i].y(), steps[i].x());
        steps[15 + i] = upanui::UIPosition(steps[15 - i].x(), -steps[15 - i].y());
        steps[45 - i] = upanui::UIPosition(-steps[15 - i].x(), -steps[15 - i].y());
        steps[45 + i] = upanui::UIPosition(-steps[15 - i].x(), steps[15 - i].y());
        steps[30 - i] = upanui::UIPosition(steps[i].x(), -steps[i].y());
        steps[30 + i] = upanui::UIPosition(-steps[i].x(), -steps[i].y());
        steps[60 - i] = upanui::UIPosition(-steps[i].x(), steps[i].y());
      }
    }
  }

  void run() {
    try {
      PassThroughMouseHandler mouseHandler;
      _clockCanvas->registerMouseEventHandler(mouseHandler);

      int c = 0;
      while(true) {
        RTCDateTime dateTime;
        RTC::GetDateTime(dateTime);
        _secondHand->updateXY(_cx, _cy, _cx + _secondSteps[dateTime._second].x(), _cy - _secondSteps[dateTime._second].y());
        _minuteHand->updateXY(_cx, _cy, _cx + _minuteSteps[dateTime._minute].x(), _cy - _minuteSteps[dateTime._minute].y());

        const int h = (dateTime._hour * 5 + int(dateTime._minute * _htomFactor)) % 60;
        _hourHand->updateXY(_cx, _cy, _cx + _hourSteps[h].x(), _cy - _hourSteps[h].y());

        sleepms(1000);
        c = (c + 1) % 20;
        if (c >= 5 && c < 10) {
          _uiRoot.backgroundColorAlpha(upanui::GCoreFunctions::percentToAlpha(50));
          _uiRoot.borderColorAlpha(upanui::GCoreFunctions::percentToAlpha(50));
        } else if (c >= 10 && c < 15){
          _uiRoot.backgroundColorAlpha(upanui::GCoreFunctions::percentToAlpha(0));
          _uiRoot.borderColorAlpha(upanui::GCoreFunctions::percentToAlpha(0));
        } else if (c >= 15 && c < 20) {
          _clockCanvas->backgroundColorAlpha(upanui::GCoreFunctions::percentToAlpha(80));
        } else {
          _clockCanvas->backgroundColorAlpha(upanui::GCoreFunctions::percentToAlpha(100));
          _uiRoot.backgroundColorAlpha(upanui::GCoreFunctions::percentToAlpha(100));
          _uiRoot.borderColorAlpha(upanui::GCoreFunctions::percentToAlpha(100));
        }
      }
    } catch(const upan::exception& e) {
      printf("App failed with error: %s", e.ErrorMsg().c_str());
      exit(1);
    }
  }
private:
  void initLayout() {
    upanui::GraphicsContext::Transaction gcTransaction;
    _clockCanvas = &upanui::UIObjectFactory::createRoundCanvas(_uiRoot, PADDING, PADDING, _csize, _csize, upanui::HorizontalPlacementType::ABSOLUTE, upanui::VerticalPlacementType::ABSOLUTE);
    _clockCanvas->borderThickness(BORDER_THICKNESS);
    _clockCanvas->backgroundColor(0xFFFEF2);
    _clockCanvas->borderColor(0x5A3828);

    _secondHand = &upanui::UIObjectFactory::createLine(*_clockCanvas, _cx, _cy, _cx + _secondSteps[0].x(), _cy - _secondSteps[0].y(), 2, upanui::HorizontalPlacementType::ABSOLUTE, upanui::VerticalPlacementType::ABSOLUTE);
    _secondHand->backgroundColor(0x41394C);

    _minuteHand = &upanui::UIObjectFactory::createLine(*_clockCanvas, _cx, _cy, _cx + _minuteSteps[0].x(), _cy - _minuteSteps[0].y(), 3, upanui::HorizontalPlacementType::ABSOLUTE, upanui::VerticalPlacementType::ABSOLUTE);
    _minuteHand->backgroundColor(0x524C4C);

    _hourHand = &upanui::UIObjectFactory::createLine(*_clockCanvas, _cx, _cy, _cx + _hourSteps[0].x(), _cy - _hourSteps[0].y(), 4, upanui::HorizontalPlacementType::ABSOLUTE, upanui::VerticalPlacementType::ABSOLUTE);
    _hourHand->backgroundColor(0x4B4B4C);

    auto& centerCircle = upanui::UIObjectFactory::createRoundCanvas(*_clockCanvas, _cx - 8, _cy - 8, 16, 16, upanui::HorizontalPlacementType::ABSOLUTE, upanui::VerticalPlacementType::ABSOLUTE);
    centerCircle.backgroundColor(0xFADDBD);

    //VGA font --> 1 character is 8 width, 16 height with font size 16
    const int labelSize = 16;
    const int labelHeight = labelSize;
    const int labelWidth1C = 8;
    const int labelWidth2C = labelWidth1C * 2;
    auto fgColor = ColorPalettes::CP16::Get(ColorPalettes::CP16::FG_BLACK);
    for(int i = 0; i < 12; ++i) {
      const int stepIndex = i * 5;
      const int labelWidth = i == 0 || i > 9 ? labelWidth2C : labelWidth1C;
      char buf[3];
      sprintf(buf, "%d", i == 0 ? 12 : i);
      auto& label = upanui::UIObjectFactory::createLabel(*_clockCanvas,
                                                         _cx + _labels[stepIndex].x() - labelWidth / 2, _cy - _labels[stepIndex].y() - labelHeight / 2,
                                                         labelWidth, labelHeight,
                                                         buf, fgColor,
                                                         upanui::usfn::PreloadedFonts::VGA16,
                                                         upanui::usfn::FAMILY_MONOSPACE, upanui::usfn::STYLE_REGULAR, labelSize,
                                                         upanui::Label::HorizontalTextAlignment::LEFT, upanui::Label::VerticalTextAlignment::TOP,
                                                         upanui::HorizontalPlacementType::ABSOLUTE, upanui::VerticalPlacementType::ABSOLUTE);
      label.backgroundColor(0);
      label.backgroundColorAlpha(upanui::GCoreFunctions::percentToAlpha(0));
    }
  }

private:
  upanui::UIRoot& _uiRoot;
  const uint32_t _csize;
  const float _htomFactor;
  const int _cx;
  const int _cy;
  upanui::UIPosition _secondSteps[60];
  upanui::UIPosition _minuteSteps[60];
  upanui::UIPosition _hourSteps[60];
  upanui::UIPosition _labels[60];
  upanui::Line* _secondHand;
  upanui::Line* _minuteHand;
  upanui::Line* _hourHand;
  upanui::Canvas* _clockCanvas;

public:

  static const int BORDER_THICKNESS = 5;
  static const int LABEL_SPACE = 20;
  static const int PADDING = 2;
  static const int X_Y_CO_SIZE = 8;
  static const float X_CO[];
  static const float Y_CO[];
};

const float DemoClock::X_CO[] = { 0.0, 0.10453, 0.20791, 0.30901, 0.40673, 0.5, 0.5878, 0.66913 };
const float DemoClock::Y_CO[] = { 1.0, 0.99452, 0.97815, 0.95105, 0.91354, 0.86602, 0.80901, 0.74314 };

void graphics_test_clock(int x, int y) {
  const int clockSize = 200;
  upanui::GraphicsContext::Init();
  auto& gc = upanui::GraphicsContext::Instance();
  auto& uiRoot = gc.initUIRoot(x, y, clockSize, clockSize, true);
  //uiRoot.backgroundColorAlpha(upanui::GCoreFunctions::percentToAlpha(0));
  uiRoot.borderThickness(10);
  uiRoot.borderColor(0xf0fff0);
  uiRoot.backgroundColor(0xf0f0ff);

  DemoClock demoClock(uiRoot);
  demoClock.start();

//  bp1.addMouseEventHandler(mouseHandler);
  //b1.addMouseEventHandler(mouseHandler);

  gc.eventManager().startEventLoop();

  exit(0);
}

void graphics_window_app(int x, int y) {
  const int appWidth = 600;
  const int mainHeight = 500;
  const int menuBarHeight = 30;
  upanui::GraphicsContext::Init();
  auto& gc = upanui::GraphicsContext::Instance();
  auto& uiRoot = gc.initUIRoot(x, y, appWidth, mainHeight + menuBarHeight, true);

  auto& uiMenuBar = upanui::UIObjectFactory::createRectangleCanvas(uiRoot, 0, 0, appWidth, menuBarHeight, upanui::HorizontalPlacementType::ABSOLUTE, upanui::VerticalPlacementType::ABSOLUTE);
  uiMenuBar.backgroundColor(0xA59E9D);

  auto& closeBt = upanui::UIObjectFactory::createIconButton(uiMenuBar, upanui::PngImageResource::CLOSE, appWidth - menuBarHeight, 0, menuBarHeight, menuBarHeight, upanui::HorizontalPlacementType::ABSOLUTE, upanui::VerticalPlacementType::ABSOLUTE);

  const int scrollBarWidth = 20;
  auto& vScroller = upanui::UIObjectFactory::createVerticalScroller(uiRoot, 0, menuBarHeight, appWidth, mainHeight, scrollBarWidth, upanui::HorizontalPlacementType::ABSOLUTE, upanui::VerticalPlacementType::ABSOLUTE);

  auto& uiMain = upanui::UIObjectFactory::createRectangleCanvas(vScroller, 0, 0, appWidth - scrollBarWidth, mainHeight + mainHeight, upanui::HorizontalPlacementType::ABSOLUTE, upanui::VerticalPlacementType::ABSOLUTE);
  uiMain.backgroundColor(0xEFE8E6);

  auto& child1 = upanui::UIObjectFactory::createRectangleCanvas(uiMain, 10, 40, 100, 200, upanui::HorizontalPlacementType::ABSOLUTE, upanui::VerticalPlacementType::ABSOLUTE);
  child1.backgroundColor(0xFF0000);

  auto& child2 = upanui::UIObjectFactory::createRectangleCanvas(uiMain, 50, mainHeight - 30, 100, 100, upanui::HorizontalPlacementType::ABSOLUTE, upanui::VerticalPlacementType::ABSOLUTE);
  child2.backgroundColor(0x00FF00);

  auto& child3 = upanui::UIObjectFactory::createRoundCanvas(uiMain, 200, 40, 150, 150, upanui::HorizontalPlacementType::ABSOLUTE, upanui::VerticalPlacementType::ABSOLUTE);
  child3.backgroundColor(0x0000FF);

  auto& child4 = upanui::UIObjectFactory::createLine(uiMain, 10, 20, 150, 150, 5, upanui::HorizontalPlacementType::ABSOLUTE, upanui::VerticalPlacementType::ABSOLUTE);
  child4.backgroundColor(0x0FF0FF);

  upanui::UIObjectFactory::createImageCanvas(uiMain, upanui::PngImageResource::TEST, 100, 100, upanui::HorizontalPlacementType::ABSOLUTE, upanui::VerticalPlacementType::ABSOLUTE);

  DragMouseHandler mouseHandler;
  PassThroughMouseHandler passThroughMouseHandler;
  uiMenuBar.registerMouseEventHandler(passThroughMouseHandler);
  uiMain.captureMouseEvents(true);
  child3.registerMouseEventHandler(mouseHandler);
  child4.registerMouseEventHandler(mouseHandler);

  CloseButtonMouseHandler closeButtonMouseHandler;
  closeBt.registerMouseEventHandler(closeButtonMouseHandler);

  gc.eventManager().startEventLoop();

  exit(0);
}

void graphics_text_editor(int x, int y) {
  const int appWidth = 600;
  const int mainHeight = 500;
  const int menuBarHeight = 30;
  upanui::GraphicsContext::Init();
  auto& gc = upanui::GraphicsContext::Instance();
  auto& uiRoot = gc.initUIRoot(x, y, appWidth, mainHeight + menuBarHeight, true);
  uiRoot.setResizable(true, true);

  auto& uiMenuBar = upanui::UIObjectFactory::createRectangleCanvas(uiRoot, 0, 0, appWidth, menuBarHeight, upanui::HorizontalPlacementType::STRETCHED, upanui::VerticalPlacementType::TOP_FIXED);
  uiMenuBar.backgroundColor(0xA59E9D);

  auto& closeBt = upanui::UIObjectFactory::createIconButton(uiMenuBar, upanui::PngImageResource::CLOSE, appWidth - menuBarHeight, 0, menuBarHeight, menuBarHeight, upanui::HorizontalPlacementType::RIGHT_FIXED, upanui::VerticalPlacementType::TOP_FIXED);

  const int scrollBarWidth = 20;
  auto& vScroller = upanui::UIObjectFactory::createVerticalScroller(uiRoot, 0, menuBarHeight, appWidth, mainHeight, scrollBarWidth, upanui::HorizontalPlacementType::STRETCHED, upanui::VerticalPlacementType::STRETCHED);

  upanui::UIObjectFactory::createTextArea(vScroller, 0, 0, appWidth - scrollBarWidth, mainHeight, upanui::HorizontalPlacementType::STRETCHED, upanui::VerticalPlacementType::STRETCHED);

  DragMouseHandler mouseHandler;
  PassThroughMouseHandler passThroughMouseHandler;
  uiMenuBar.registerMouseEventHandler(passThroughMouseHandler);

  CloseButtonMouseHandler closeButtonMouseHandler;
  closeBt.registerMouseEventHandler(closeButtonMouseHandler);

  gc.eventManager().startEventLoop();

  exit(0);
}

class TCE : public upanui::Terminal::CommandExecutor {
public:
  void setTerminal(upanui::Terminal* terminal) {
    _terminal = terminal;
  }

  void execute(const upan::string& cmdLine) override {
//    _terminal->insertCommandOutput("Hello!\nWorld\n");
//    _terminal->insertCommandOutput(cmdLine + " -> Executed!");
  }

private:
  upanui::Terminal* _terminal;
};

class MenuActionHandler : public upanui::Menu::ActionHandler {
public:
  void invoke(int id, const upan::string& name) override {
  }
};

void graphics_terminal(int x, int y) {
  const int appWidth = 600;
  const int appHeight = 550;
  upanui::GraphicsContext::Init();
  auto& gc = upanui::GraphicsContext::Instance();
  auto& uiRoot = gc.initUIRoot(x, y, appWidth, appHeight, true);
  uiRoot.setResizable(true, true);

  MenuActionHandler menuActionHandler;
  uiRoot.initMenuBar({
    { "File", {
      {1, "New", menuActionHandler },
      {2, "Open", menuActionHandler },
      {3, "Exit", menuActionHandler }
    }},

    { "Edit", {
      {4, "Cut", menuActionHandler },
      {5, "Copy", menuActionHandler },
      {6, "Paste", menuActionHandler },
      {7, "Find", menuActionHandler }
    }}
  });

  const int menuBarHeight = uiRoot.menuBarHeight();
  const int mainHeight = appHeight - menuBarHeight;

  const int scrollBarWidth = 20;
  auto& vScroller = upanui::UIObjectFactory::createVerticalScroller(uiRoot, 0, menuBarHeight, appWidth, mainHeight, scrollBarWidth, upanui::HorizontalPlacementType::STRETCHED, upanui::VerticalPlacementType::STRETCHED);

  TCE tce;
  tce.setTerminal(&upanui::UIObjectFactory::createTerminal(vScroller, 0, 0, appWidth - scrollBarWidth, mainHeight, "msh:/", tce, upanui::HorizontalPlacementType::STRETCHED, upanui::VerticalPlacementType::STRETCHED));

  gc.eventManager().startEventLoop();

  exit(0);
}

class Icon {
public:
  virtual ~Icon() {}
  virtual const upan::string& name() const = 0;
  virtual upanui::Image& image() = 0;
};

class ShortcutIcon : public Icon {
public:
  ShortcutIcon(const upan::string& iconFile) {
  }

  const upan::string& name() const {
    return _name;
  }

  upanui::Image& image() override {
    return *_image;
  }

private:
  upanui::Image* _image;
  upan::string _name;
};

void graphics_desktop(int x, int y) {
  try {
    const int appWidth = 600;
    const int mainHeight = 500;
    const int menuBarHeight = 30;
    upanui::GraphicsContext::Init();
    auto& gc = upanui::GraphicsContext::Instance();

    upanui::IconImageMap::Instance();

    auto& uiRoot = gc.initUIRoot(x, y, appWidth, mainHeight + menuBarHeight, true);

    auto& uiMenuBar = upanui::UIObjectFactory::createRectangleCanvas(uiRoot, 0, 0, appWidth, menuBarHeight,
                                                                     upanui::HorizontalPlacementType::STRETCHED,
                                                                     upanui::VerticalPlacementType::TOP_FIXED);
    uiMenuBar.backgroundColor(0xA59E9D);

    auto& closeBt = upanui::UIObjectFactory::createIconButton(uiMenuBar, upanui::PngImageResource::CLOSE,
                                                              appWidth - menuBarHeight, 0, menuBarHeight, menuBarHeight,
                                                              upanui::HorizontalPlacementType::RIGHT_FIXED,
                                                              upanui::VerticalPlacementType::TOP_FIXED);

    const upan::string desktopImageFile("usdb@/desktop/desktop.png");
    auto file = FileOperations::Instance().open(desktopImageFile, O_RDONLY, 0);
    if (file.isEmpty()) {
      throw upan::exception(XLOC, "Failed to open desktop image file: %s", desktopImageFile.c_str());
    }
    auto fileSize = file->getStat().st_size;

    upan::uniq_ptr<uint8_t[]> buffer(new uint8_t[fileSize]);
    file->read(buffer.get(), fileSize);
    upanui::PngEncoder decoder;
    upanui::Image& bgImage = decoder.decode(buffer.get(), fileSize);

    close(file->id());

    auto& uiMain = upanui::UIObjectFactory::createImageCanvas(uiRoot, bgImage, upanui::ImageComposeType::FIT_IN,
                                                              0, menuBarHeight, appWidth, mainHeight,
                                                              upanui::HorizontalPlacementType::ABSOLUTE,
                                                              upanui::VerticalPlacementType::ABSOLUTE);

    upanui::Image& terminalIconImage = upanui::IconImageMap::Instance().find("terminal", 0);
    auto& icon1 = upanui::UIObjectFactory::createIconLabel(uiMain, terminalIconImage, "Terminal",
                                                           20, 20, 80, 80,
                                                           upanui::HorizontalPlacementType::ABSOLUTE,
                                                           upanui::VerticalPlacementType::ABSOLUTE);

    DesktopMouseHandler::IconMap m;
    m.insert(DesktopMouseHandler::IconMap::value_type (&icon1, "terminal"));
    DesktopMouseHandler desktopMouseHandler(m);

    DragMouseHandler mouseHandler;
    PassThroughMouseHandler passThroughMouseHandler;
    uiMenuBar.registerMouseEventHandler(passThroughMouseHandler);

    uiMain.captureMouseEvents(true);
    icon1.registerMouseEventHandler(desktopMouseHandler);

    CloseButtonMouseHandler closeButtonMouseHandler;
    closeBt.registerMouseEventHandler(closeButtonMouseHandler);

    gc.eventManager().startEventLoop();
  } catch(const upan::exception& e) {
    e.Print();
  }
  exit(0);
}

int testg_id = 0;
void ConsoleCommands_TestGraphics() {
  if (CommandLineParser::Instance().GetNoOfParameters() != 3) {
    throw upan::exception(XLOC, "parameters type, x & y are required");
  }

  int type = atoi(CommandLineParser::Instance().GetParameterAt(0));
  upan::vector<uintptr_t> params;
  params.push_back(atoi(CommandLineParser::Instance().GetParameterAt(1)));
  params.push_back(atoi(CommandLineParser::Instance().GetParameterAt(2)));

  upan::string pname("testg");
  pname += upan::string::to_string(testg_id++);
  switch(type) {
    case 1: {
      ProcessManager::Instance().CreateKernelProcess(pname, (uintptr_t) &graphics_test_process_canvas, NO_PROCESS_ID, true, false, params);
    }
    break;

    case 2: {
      ProcessManager::Instance().CreateKernelProcess(pname, (uintptr_t) &graphics_test_process_line, NO_PROCESS_ID, true, false, params);
    }
    break;

    case 3: {
      ProcessManager::Instance().CreateKernelProcess(pname, (uintptr_t) &graphics_test_clock, NO_PROCESS_ID, true, false, params);
    }
    break;

    case 4: {
      ProcessManager::Instance().CreateKernelProcess(pname, (uintptr_t) &graphics_test_flag, NO_PROCESS_ID, true, false, params);
    }
    break;

    case 5: {
      ProcessManager::Instance().CreateKernelProcess(pname, (uintptr_t) &graphics_window_app, NO_PROCESS_ID, true, false, params);
    }
    break;

    case 6: {
      ProcessManager::Instance().CreateKernelProcess(pname, (uintptr_t) &graphics_text_editor, NO_PROCESS_ID, true, false, params);
    }
    break;

    case 7: {
      ProcessManager::Instance().CreateKernelProcess(pname, (uintptr_t) &graphics_terminal, NO_PROCESS_ID, true, false, params);
    }
    break;

    case 8: {
      ProcessManager::Instance().CreateKernelProcess(pname, (uintptr_t) &graphics_desktop, NO_PROCESS_ID, true, false, params);
    }
    break;

    default:
      printf("\n Invalid graphics test id: %d", type);
  }
}

extern void TestMTerm() ;
extern void DiskCache_ShowTotalDiskReads() ;

class DisplayCache : public BTree::InOrderVisitor
{
	public:
		DisplayCache(StorageDrive* pDiskDrive) : m_pDiskDrive(pDiskDrive), m_iCount(0), m_bAbort(false) { }

		void operator()(const BTreeKey& rKey, BTreeValue* pValue) const
		{
			if(!pValue)
				return ;

			const DiskCacheKey& key = static_cast<const DiskCacheKey&>(rKey) ;
		//	DiskCacheValue* pCacheValue = static_cast<DiskCacheValue*>(pValue) ;

		//	printf(", %d ", key.GetSectorID()) ;
			if(key.GetSectorID() % 20344)
				m_iCount++ ;
		}

		bool Abort() const { return m_bAbort ; }

	private:
		StorageDrive* m_pDiskDrive ;
		mutable int m_iCount ;
		mutable bool m_bAbort ;
} ;

class RankInOrderVisitor : public BTree::InOrderVisitor
{
	private:
		unsigned m_uiSectorID ;
		double m_dRank ;
		const unsigned m_uiCurrent ;

	public:
		RankInOrderVisitor() : m_uiSectorID(0), m_dRank(0), m_uiCurrent(PIT::Instance().GetClockCount()) { }

		void operator()(const BTreeKey& rKey, BTreeValue* pValue) 
		{
			const DiskCacheKey& key = static_cast<const DiskCacheKey&>(rKey) ;
			const DiskCacheValue* value = static_cast<const DiskCacheValue*>(pValue) ;

			unsigned uiTimeDiff = m_uiCurrent - value->GetLastAccess() ;
			double dRank = 	(double)uiTimeDiff / (double)value->GetHitCount() ;
			if(dRank > m_dRank)
			{
				m_uiSectorID = key.GetSectorID() ;
				m_dRank = dRank ;
			}
		}

		inline unsigned GetSectorID() { return m_uiSectorID ; }
		inline double GetRank() { return m_dRank ; }

		bool Abort() const { return false ; }
} ;

typedef struct
{
	int len ;
	unsigned cnt ;
	unsigned time ;
	unsigned begin ;
} ReadStat ;

ReadStat read_stat[256] ;
void _UpdateReadStat(unsigned len, bool bFirst)
{
	unsigned uiTime = PIT::Instance().GetClockCount() ;
	static bool bInit = false ;
	if(!bInit)
	{
		bInit = true ;
		for(int i = 0; i < 256; i++)
		{
			read_stat[i].len = -1 ;
			read_stat[i].cnt = 0 ;
			read_stat[i].time = 0 ;
			read_stat[i].begin = 0 ;
		}
	}

	if(!bFirst)
	{
		for(int i = 0; i < 256; i++)
		{
			if((unsigned)read_stat[i].len == len)
			{
				read_stat[i].time += (uiTime - read_stat[i].begin) ;
				break ;
			}
		}
		return ;
	}

	int free = -1 ;
	bool bUpdated = false ;
	for(int i = 0; i < 256; i++)
	{
		if((unsigned)read_stat[i].len == len)
		{
			read_stat[i].cnt++ ;
			read_stat[i].begin = uiTime ;
			bUpdated = true ;
			break ;
		}

		if(free == -1 && read_stat[i].len == -1)
			free = i ;
	}

	if(!bUpdated)
	{
		if(free >= 0 && free < 256)
		{
			read_stat[free].len = len ;
	 		read_stat[free].cnt	= 1 ;
			read_stat[free].begin = uiTime ;
			read_stat[free].time = 0 ;
		}
	}
}

void _DisplayReadStat()
{
	for(int i = 0 ; i < 256; i++)
	{
		if(read_stat[i].len != -1)
			printf("\n Len: %u, ReadCount: %u, TickCount: %u", read_stat[i].len, read_stat[i].cnt, read_stat[i].time) ;
	}
}

void aThread(void* x) {
  const int n = int64_t(x);
  printf("\n Running thread: %u", n);
  for(int i = 0; i < n; ++i) {
    printf("\nCounter: %d", i);
    sleepms(500);
  }
}

void ConsoleCommands_PrintKPIs() {
  if (CommandLineParser::Instance().GetNoOfParameters() >= 1) {
    const upan::string& name = CommandLineParser::Instance().GetParameterAt(0);
    printf("avg(%s): %lf (%d)", name.c_str(), upan::metrics::instance().avg(name), upan::metrics::instance().count(name));
  } else {
    const auto& names = upan::metrics::instance().kpis();
    bool first = true;
    for(const auto& e : names) {
      if (first) {
        first = false;
      } else {
        putchar('\n');
      }
      printf("avg(%s): %lf (%d)", e.c_str(), upan::metrics::instance().avg(e), upan::metrics::instance().count(e));
    }
  }
}

static thread_local int _t_local_var_data1 = 100;
static thread_local uint8_t _t_local_var_data2 = 200;
static thread_local uint8_t _t_local_var_data3 = 201;
static thread_local int _t_local_var_global1;
static thread_local int _t_local_var_global2;

extern thread_local int _lib_data1_thread_local;
extern thread_local int _lib_global1_thread_local;

class TLSDemo : public upan::thread {
  void run() override {
    _t_local_var_data1 += getpid();
    sleepms(500);
    _t_local_var_global1 += getpid();
    sleepms(500);
    printf("\n %d -> %d\n", _t_local_var_global1, _t_local_var_data1);
    access_thread_local_test();
    printf("\n TLS Lib data1 -> %d", _lib_data1_thread_local);
    printf("\n TLS Lib global1 -> %d", _lib_global1_thread_local);
    tls_xlib_test();
    printf("\n TLS XLib data1 -> %d", _lib_data1_thread_local);
    printf("\n TLS XLib global1 -> %d", _lib_global1_thread_local);
  }
};

class shared_ptr_test {
public:
  shared_ptr_test() {
    printf("\nshared_ptr_test - created");
  }
  ~shared_ptr_test() {
    printf("\nshared_ptr_test - deleted");
  }
};

#define TEST_TCP_SERVER_PORT 8080
#define TEST_TCP_TLS_SERVER_PORT 8443
#define TEST_TCP_SERVER_BUFFER_SIZE 1024

static void test_tcp_server() {
  struct sockaddr_in address;
  char buffer[TEST_TCP_SERVER_BUFFER_SIZE] = {0};

  auto sd = socket(AF_INET, SOCK_STREAM, 0);
  if (sd < 0) {
    throw upan::exception(XLOC, "socket() failed");
  }

  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY; // 0.0.0.0
  address.sin_port = htons(TEST_TCP_SERVER_PORT);

  if (bind(sd, (struct sockaddr*) &address, sizeof(address)) < 0) {
    close(sd);
    throw upan::exception(XLOC, "bind() failed");
  }

  if (listen(sd, 1) < 0) {
    close(sd);
    throw upan::exception(XLOC, "listen() failed");
  }

  printf("\nServer is listening on port %d...", TEST_TCP_SERVER_PORT);

  // Accept a connection
  socklen_t addrlen;
  auto new_sd = accept(sd, (struct sockaddr*) &address, (socklen_t*) &addrlen);
  if (new_sd < 0) {
    close(sd);
    throw upan::exception(XLOC, "accept() failed");
  }

  int n = recv(new_sd, buffer, TEST_TCP_SERVER_BUFFER_SIZE - 1, 0);
  if (n >= 0) {
    buffer[n] = '\0'; // Null-terminate
    printf("\nReceived: %s", buffer);
  }

  strcpy(buffer, "Hello from Upanix server!");
  ssize_t bytes_sent = send(new_sd, buffer, strlen(buffer), 0);
  if (bytes_sent < 0) {
    throw upan::exception(XLOC, "send() failed");
  } else {
    printf("\nSent %d bytes: %s", bytes_sent, buffer);
  }

  close(new_sd);
  close(sd);
}

static void test_tcp_client(const upan::string& host) {
  const int server_port = 12345;
  char message[1024] = "Hello, TCP Server!";

  int sd = socket(AF_INET, SOCK_STREAM, 0);
  if (sd < 0) {
    throw upan::exception(XLOC, "socket() failed");
  }

  struct timeval timeout {};
  timeout.tv_sec = 5;
  timeout.tv_usec = 0;
  if (setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
    close(sd);
    throw upan::exception(XLOC, "failed to set socket option: SO_RCVTIMEO");
  }

  struct sockaddr_in server_addr;
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(server_port);
  server_addr.sin_addr.s_addr = upan::net::inet_strton(host.c_str());

  if (connect(sd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
    close(sd);
    throw upan::exception(XLOC, "connect() failed");
  }

  printf("\nConnected to %s:%d", host.c_str(), server_port);

  ssize_t bytes_sent = send(sd, message, strlen(message), 0);
  if (bytes_sent < 0) {
    throw upan::exception(XLOC, "send() failed");
  } else {
    printf("\nSent %d bytes: %s", bytes_sent, message);
  }

  // 5. Receive response
  ssize_t bytes_received = recv(sd, message, 1024 - 1, 0);
  if (bytes_received > 0) {
    message[bytes_received] = '\0';
    printf("\nReceived from server: %s", message);
  }

  close(sd);
}

static void test_tcp_tls_server() {
  struct sockaddr_in address;

  auto sd = socket(AF_INET, SOCK_STREAM, 0);
  if (sd < 0) {
    throw upan::exception(XLOC, "socket() failed");
  }

  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY; // 0.0.0.0
  address.sin_port = htons(TEST_TCP_TLS_SERVER_PORT);

  if (bind(sd, (struct sockaddr*) &address, sizeof(address)) < 0) {
    close(sd);
    throw upan::exception(XLOC, "bind() failed");
  }

  if (listen(sd, 1) < 0) {
    close(sd);
    throw upan::exception(XLOC, "listen() failed");
  }

  printf("\nTLS Server is listening on port %d...", TEST_TCP_TLS_SERVER_PORT);

  // Accept a connection
  socklen_t addrlen;
  auto new_sd = accept(sd, (struct sockaddr*) &address, (socklen_t*) &addrlen);
  if (new_sd < 0) {
    close(sd);
    throw upan::exception(XLOC, "accept() failed");
  }

  SSL *ssl = SSL_new(upan::net::ssl_context::instance().getServerCtx());
  SSL_set_fd(ssl, new_sd);

  if (SSL_accept(ssl) <= 0) {
    ERR_print_errors_fp(stderr);
  } else {
    char buffer[1024] = {0};
    SSL_read(ssl, buffer, sizeof(buffer));
    printf("Client says: %s\n", buffer);

    const char msg[] = "Hello from Upanix TLS Server!";
    SSL_write(ssl, msg, strlen(msg) + 1);
  }

  SSL_shutdown(ssl);
  SSL_free(ssl);
  close(new_sd);
  close(sd);
}

static void test_tcp_tls_client(const upan::string& host) {
  const int server_port = 8443;

  int sd = socket(AF_INET, SOCK_STREAM, 0);
  if (sd < 0) {
    throw upan::exception(XLOC, "socket() failed");
  }

  struct timeval timeout {};
  timeout.tv_sec = 5;
  timeout.tv_usec = 0;
  if (setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
    close(sd);
    throw upan::exception(XLOC, "failed to set socket option: SO_RCVTIMEO");
  }

  struct sockaddr_in server_addr;
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(server_port);
  server_addr.sin_addr.s_addr = upan::net::inet_strton(host.c_str());

  if (connect(sd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
    close(sd);
    throw upan::exception(XLOC, "connect() failed");
  }

  printf("\nConnected to %s:%d", host.c_str(), server_port);

  SSL *ssl = SSL_new(upan::net::ssl_context::instance().getClientCtx());
  SSL_set_fd(ssl, sd);

  if (SSL_connect(ssl) <= 0) {
    ERR_print_errors_fp(stderr);
  } else {
    char buffer[1024] = "Hello from Upanix TLS Client!";
    SSL_write(ssl, buffer, strlen(buffer) + 1);

    buffer[0] = '\0';
    SSL_read(ssl, buffer, sizeof(buffer));
    printf("\nTLS Server replied: %s\n", buffer);
  }

  SSL_shutdown(ssl);
  SSL_free(ssl);
  close(sd);
}

void sig_handler(int signum) {
  printf("\nCaught signal: %d", signum);
}

void sig_action(int signum, siginfo_t* info, void* context) {
  printf("\nCaught signal: %d, SigInfo val: %d", signum, info->si_value.sival_int);
}

void sig_child_nowait(int signum, siginfo_t* info, void* context) {
  printf("\nCaught SIGCHLD for child process: %d (nowait)", signum, info->si_value.sival_int);
}

void sig_child_wait(int signum, siginfo_t* info, void* context) {
  printf("\nCaught SIGCHLD for child process: %d (wait)", signum, info->si_value.sival_int);
  int exitStatus;
  waitpid(info->si_value.sival_int, &exitStatus, 0);
  printf("\ndone");
}

void ConsoleCommands_Test() {
  upan::string test;

  if (CommandLineParser::Instance().GetNoOfParameters() >= 1) {
    test = CommandLineParser::Instance().GetParameterAt(0);
  }

  if (test == "sigalarm") {
    printf("\n setting alarm for 5 seconds");
    alarm(5);
  }
  else if (test == "sigchld") {
    if (CommandLineParser::Instance().GetNoOfParameters() < 2) {
      throw upan::exception(XLOC, "parameter signal number is required");
    }
    upan::string option = CommandLineParser::Instance().GetParameterAt(1);
    if (option == "ignore") {
      struct sigaction sa;
      memset(&sa, 0, sizeof(sa));
      sa.sa_handler = (sa_handler_t )SIG_IGN;
      sigaction(SIGCHLD, &sa, nullptr);
      printf("\nSIGCHLD ignored");
    } else if (option == "default") {
      struct sigaction sa;
      memset(&sa, 0, sizeof(sa));
      sa.sa_handler = (sa_handler_t )SIG_DFL;
      sigaction(SIGCHLD, &sa, nullptr);
      printf("\nSIGCHLD default");
    } else if (option == "nowait") {
      struct sigaction sa;
      memset(&sa, 0, sizeof(sa));
      sa.sa_flags = SA_SIGINFO | SA_NOCLDWAIT;
      sa.sa_sigaction = sig_child_nowait;
      sigaction(SIGCHLD, &sa, nullptr);
      printf("\nSIGCHLD handler with nowait");
    } else if (option == "wait") {
      struct sigaction sa;
      memset(&sa, 0, sizeof(sa));
      sa.sa_flags = SA_SIGINFO;
      sa.sa_sigaction = sig_child_wait;
      sigaction(SIGCHLD, &sa, nullptr);
      printf("\nSIGCHLD handler with wait");
    } else {
      throw upan::exception(XLOC, "invalid option");
    }
  } else if (test == "sig_handler") {
    if (CommandLineParser::Instance().GetNoOfParameters() < 2) {
      throw upan::exception(XLOC, "parameter signal number is required");
    }
    int signal = atoi(CommandLineParser::Instance().GetParameterAt(1));
    struct sigaction sa;
    memset(&sa, 0, sizeof(struct sigaction));
    sa.sa_handler = sig_handler;
    if (sigaction(signal, &sa, nullptr)) {
      printf("\nFailed to set signal handler");
    } else {
      printf("\nRegistered set signal handler");
    }
  } else if (test == "sig_action") {
    if (CommandLineParser::Instance().GetNoOfParameters() < 2) {
      throw upan::exception(XLOC, "parameter signal number is required");
    }
    int signal = atoi(CommandLineParser::Instance().GetParameterAt(1));
    struct sigaction sa;
    memset(&sa, 0, sizeof(struct sigaction));
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = sig_action;

    struct sigaction old_sa;
    if (sigaction(signal, &sa, &old_sa)) {
      printf("\nFailed to set signal action handler");
    } else {
      printf("\nRegistered set signal action handler - old_sa: %d", old_sa.sa_flags);
    }
  } else if (test == "tcp-client") {
    if (CommandLineParser::Instance().GetNoOfParameters() < 2) {
      throw upan::exception(XLOC, "server IP parameter is required");
    }
    upan::string host(CommandLineParser::Instance().GetParameterAt(1));
    test_tcp_client(host);
  } else if (test == "tcp-server") {
    test_tcp_server();
  } else if (test == "tcp-tls-client") {
    if (CommandLineParser::Instance().GetNoOfParameters() < 2) {
      throw upan::exception(XLOC, "server IP parameter is required");
    }
    upan::string host(CommandLineParser::Instance().GetParameterAt(1));
    test_tcp_tls_client(host);
  } else if (test == "tcp-tls-server") {
    test_tcp_tls_server();
  } else if (test == "config") {
      upan::ConfigFileDB configFileDb("/var/db/test.cfg", upan::ConfigFileDB::OpType::RDWR);

      printf("\n **** Initial content");
      for (const auto& i : configFileDb.getAll()) {
        printf("\n%s %s", i.first.c_str(), i.second.c_str());
      }

      configFileDb.set("Protocol", "DHCP", "");
      configFileDb.set("LeaseTime", "1234", "This is lease time");
      configFileDb.set("IP Address", "192.168.50.27", "This is the IP address");
      configFileDb.set("Gateway", "192.168.255.255", "");
      printf("\n **** First update");
      for (const auto& i : configFileDb.getAll()) {
        printf("\n%s %s", i.first.c_str(), i.second.c_str());
      }

      {
        upan::ConfigFileDB::BatchWriteGuard g(configFileDb);
        configFileDb.set("Protocol", "DHCP", "This is the protocol type");
        configFileDb.set("LeaseTime", "12345678", "This is lease time");
        configFileDb.set("IP Address", "192.168.50.27", "This is the IP");
        configFileDb.set("Gateway", "192.168.255.255", "This is Gateway IP");
      }
      printf("\n **** Second update");
      for (const auto& i : configFileDb.getAll()) {
        printf("\n%s %s", i.first.c_str(), i.second.c_str());
      }

      configFileDb.set("Protocol", "DHCP", "This is the protocol type");
      configFileDb.set("LeaseTime", "12345678", "This is lease time");
      configFileDb.set("IP Address", "192.168.50.27", "This is the IP");
      configFileDb.remove("Gateway");
      configFileDb.set("Router", "192.168.255.255", "This is Gateway/Router IP");
      printf("\n **** Third update");
      for (const auto& i : configFileDb.getAll()) {
        printf("\n%s %s", i.first.c_str(), i.second.c_str());
      }
    } else if (test == "tls") {
    printf("\n %d", _t_local_var_data1);
    printf("\n %d", (int) _t_local_var_data2);
    printf("\n %d", (int) _t_local_var_data3);
    printf("\n %d", (int) _t_local_var_global1);
    printf("\n %d", (int) _t_local_var_global2);

    access_thread_local_test();

    printf("\n Static Library Thread Local Data -> %d", _lib_data1_thread_local);
    printf("\n Static Library Thread Local Global -> %d", _lib_global1_thread_local);

    tls_xlib_test();
    printf("\n X Library Thread Local Data -> %d", _lib_data1_thread_local);
    printf("\n X Library Thread Local Global -> %d", _lib_global1_thread_local);

    TLSDemo t1, t2;
    _t_local_var_global1 = getpid();
    t1.start();
    printf("\n %d -> %d\n", _t_local_var_global1, _t_local_var_data1);

    t2.start();
    printf("\n %d -> %d\n", _t_local_var_global1, _t_local_var_data1);

    int exitStatus;
    waitpid(t1.pid(), &exitStatus, 0);
    waitpid(t2.pid(), &exitStatus, 0);
  } else if (test == "smart_ptr") {
    printf("\nshared ptr test....");
    {
      upan::shared_ptr<shared_ptr_test> s3;
      upan::shared_ptr<shared_ptr_test[]> a3;
      {
        upan::shared_ptr<shared_ptr_test> s1(new shared_ptr_test());
        upan::shared_ptr<shared_ptr_test> s2(s1);
        s3 = s1;

        auto a = new shared_ptr_test[2];
        upan::shared_ptr<shared_ptr_test[]> a1(a);
        upan::shared_ptr<shared_ptr_test[]> a2(a1);
        a3 = a1;
      }
      printf("\n internal shared ptrs destroyed");
    }
    {
      printf("\nuniq ptr test....");
      upan::uniq_ptr<shared_ptr_test> u3;
      upan::uniq_ptr<shared_ptr_test[]> ua3;
      {
        upan::uniq_ptr<shared_ptr_test> u1(new shared_ptr_test());
        u3 = upan::move(u1);
        upan::uniq_ptr<shared_ptr_test> u2(upan::move(u3));

        auto a = new shared_ptr_test[2];
        upan::uniq_ptr<shared_ptr_test[]> ua1(a);
        ua3 = upan::move(ua1);
        upan::uniq_ptr<shared_ptr_test[]> ua2(upan::move(ua3));
      }
      printf("\n internal uniq ptrs destroyed");
    }
  } else if (test == "syscall-stats") {
    pid_t pid = -1;
    if (CommandLineParser::Instance().GetNoOfParameters() >= 2) {
      pid = atoi(CommandLineParser::Instance().GetParameterAt(1));
    }
    if (pid == -1) {
      for (auto& e: get_syscall_stats()) {
        printf("\n %lu -> ", e.first);
        int total = 0;
        for (auto& i: e.second) {
          printf("%d : %d, ", i.first, i.second);
          total += i.second;
        }
        printf(" -> Total: %d", total);
      }
    } else {
      int col = 0;

      for (auto& e: get_syscall_stats()) {
        if (col % 5 == 0) {
          printf("\n");
        }
        col++;
        printf("%lu -> ", e.first);
        auto i = e.second.find(pid);
        if (i != e.second.end()) {
          printf(" %d , ", i->second);
        }
      }
    }
  }
  //MemManager::Instance().DisplayPageAllocationStats();
  //printf("\n Kernel Heap Available Size: %llu", KernelDMM::Instance().availableHeapSize());
}

void ConsoleCommands_Testv() {
  upan::vector<uintptr_t> params;
  params.push_back(0);
  params.push_back(0);
  upan::string pname("photos");
  ProcessManager::Instance().CreateKernelProcess(pname, (uintptr_t) &graphics_photos, NO_PROCESS_ID, true, false, params);
}

void ConsoleCommands_TestNet() {
  upan::atomic::integral<int> a(10);
  printf("\n add atomic: %d", a.inc());
  printf("\n sub atomic: %d", a.dec());

  struct in_addr inp;
  inet_aton("192.168.1.1", &inp);
  printf("\n IP Address: %s", upan::net::inet_ntostr(inp.s_addr).c_str());
  printf("\n Broadcast MAC - %02x:%02x:%02x:%02x:%02x:%02x",
         INADDR_MAC_BROADCAST[0],
         INADDR_MAC_BROADCAST[1],
         INADDR_MAC_BROADCAST[2],
         INADDR_MAC_BROADCAST[3],
         INADDR_MAC_BROADCAST[4],
         INADDR_MAC_BROADCAST[5]);

  printf("\n Time based on boot = %u", PIT::Instance().GetCurrentTimeFromBoot());
  printf("\n Time based on RTC = %u", SystemUtil_GetTimeOfDay() * 1000);
}

class Global
{
	public:
		Global()
		{
			printf("GLOBAL CONSTR\n");
		}
		~Global()
		{
			printf("GLOBAL DESTR\n");
			while(1);
		};
};

static Global GLOBAL;

//#include <vector>
void ConsoleCommands_TestCPP()
{
//	std::vector<int> x;
//	std::vector<char> y;
//	for(int i = 0; i < 10; i++)
//	{
//		x.push_back(i);
//		y.push_back(40 + i);
//	}
//
//	for(int i = 0; i < 10; i++)
//	{
//		printf("\n%d -> %c, ", x[i], y[i]);
//	}
//	printf("\n");
Global x;
}

void ConsoleCommands_Beep()
{
  PCSound::Instance().Beep();
}

void ConsoleCommands_Sleep()
{
  uint32_t t = atoi(CommandLineParser::Instance().GetParameterAt(0));
  ProcessManager::Instance().Sleep(t * 1000);
}

void ConsoleCommands_Kill() {
  if(CommandLineParser::Instance().GetNoOfParameters() == 0) {
    throw upan::exception(XLOC, "required parameter pid");
  }

  int argCount = CommandLineParser::Instance().GetNoOfParameters();
  if (argCount != 2) {
    throw upan::exception(XLOC, "invalid number of parameters: Usage: kill <pid> <signal>");
  }

  pid_t pid = atoi(CommandLineParser::Instance().GetParameterAt(0));
  auto signo = (SIGNAL)atoi(CommandLineParser::Instance().GetParameterAt(1));
  const union sigval val = { .sival_int = pid };
  ProcessManager::Instance().SendSignal(pid, signo, &val);
}

void ConsoleCommands_ResetMouse() {
  PS2MouseDriver::Instance().ResetMousePosition();
}

void ConsoleCommands_MemStats() {
  MemManager::Instance().DisplayPageAllocationStats();
  printf("\n Kernel Heap Available Size: %llu", KernelDMM::Instance().availableHeapSize());
}

#define PACKET_SIZE 64
uint16_t _icmp_seq = 0;

void ConsoleCommands_Ping() {
  if (CommandLineParser::Instance().GetNoOfParameters() != 1) {
    throw upan::exception(XLOC, "required parameter: <ip address>");
  }
  const char* host = CommandLineParser::Instance().GetParameterAt(0);

  struct sockaddr_in addr;
  struct hostent* hinfo = gethostbyname(host);
  if (!hinfo) {
    throw upan::exception(XLOC, "invalid host");
  }

  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  memcpy(&addr.sin_addr, hinfo->h_addr_list[0], hinfo->h_length);

  freehostinfo(hinfo);

  int sd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
  if (sd < 0) {
    throw upan::exception(XLOC, "failed to create raw socket");
  }

  char packet[PACKET_SIZE];
  auto icmp_hdr = (struct icmp*) packet;
  memset(packet, 0, sizeof(packet));

  struct timeval timeout {};
  timeout.tv_sec = 2;
  timeout.tv_usec = 0;
  if (setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
    close(sd);
    throw upan::exception(XLOC, "failed to set socket option: SO_RCVTIMEO");
  }

  icmp_hdr->icmp_type = ICMP_ECHO;
  icmp_hdr->icmp_code = 0;
  icmp_hdr->icmp_id = htons(getpid() & 0xFFFF);
  icmp_hdr->icmp_seq = htons(_icmp_seq++);

  struct timeval start, end;
  gettimeofday(&start, NULL);
  memcpy(icmp_hdr->icmp_data, &start, sizeof(start));
  memset(icmp_hdr->icmp_data + sizeof(start), 0xA5, PACKET_SIZE - sizeof(struct icmp) - sizeof(start));

  icmp_hdr->icmp_cksum = 0;
  icmp_hdr->icmp_cksum = NetworkUtil::CalculateChecksum((uint16_t*) packet, PACKET_SIZE, 0);

  if (sendto(sd, packet, PACKET_SIZE, 0, (struct sockaddr*) &addr, sizeof(addr)) <= 0) {
    close(sd);
    throw upan::exception(XLOC, "failed to sendPacket packet");
  }

  socklen_t len = sizeof(addr);
  if (recvfrom(sd, packet, sizeof(packet), 0, (struct sockaddr*) &addr, &len) <= 0) {
    close(sd);
    throw upan::exception(XLOC, "failed to receive packet");
  }

  gettimeofday(&end, NULL);

  auto ip_hdr = (struct ip*) packet;
  int ip_hdr_len = ip_hdr->ip_hl << 2;
  auto reply_icmp = (struct icmp*) (packet + ip_hdr_len);

  if (reply_icmp->icmp_type == ICMP_ECHOREPLY && ntohs(reply_icmp->icmp_id) == (getpid() & 0xFFFF)) {
    double rtt = (end.tv_sec - start.tv_sec) * 1000.0 +
                 (end.tv_usec - start.tv_usec) / 1000.0;
    printf("\nReply from %s: seq=%d time=%.2f ms\n", inet_ntoa(addr.sin_addr), ntohs(reply_icmp->icmp_seq), rtt);
  } else {
    printf("\nReceived ICMP packet, but it's not an echo reply.\n");
  }

  close(sd);
}

void ConsoleCommands_Host() {
  if (CommandLineParser::Instance().GetNoOfParameters() != 1) {
    throw upan::exception(XLOC, "required parameter: <ip address> or <hostname>");
  }
  const char* host = CommandLineParser::Instance().GetParameterAt(0);

  struct hostent* hinfo = nullptr;
  struct in_addr addr;
  if (inet_aton(host, &addr) == 1) {
    hinfo = gethostbyaddr(&addr, IPV4_ADDR_LEN, AF_INET);
  } else {
    hinfo = gethostbyname(host);
  }

  if (!hinfo) {
    throw upan::exception(XLOC, "invalid host");
  }

  printf("\nHost name: %s", hinfo->h_name);

  printf("\nAliases:");
  for(int i = 0; hinfo->h_aliases[i]; ++i) {
    printf("\n %s", hinfo->h_aliases[i]);
  }

  printf("\nAddress list:");
  for(int i = 0; hinfo->h_addr_list[i]; ++i) {
    printf("\n %s", inet_ntoa(*(struct in_addr*) hinfo->h_addr_list[i]));
  }

  freehostinfo(hinfo);
}

void ConsoleCommands_SysLog() {
  if (CommandLineParser::Instance().GetNoOfParameters() == 0) {
    throw upan::exception(XLOC, "required parameter: <cmd -> reset, enable trace|debug|info|warn|error, disable trace|debug|info|warn|error>");
  }

  const upan::string option = CommandLineParser::Instance().GetParameterAt(0);

  if (option == "enable") {
    KLog::enable(CommandLineParser::Instance().GetParameterAt(1));
  } else if (option == "disable") {
    KLog::disable(CommandLineParser::Instance().GetParameterAt(1));
  } else {
    throw upan::exception(XLOC, "invalid option: %s", option.c_str());;
  }
}

void print_servent(struct servent* s) {
  printf("\n%s %d/%s", s->s_name, s->s_port, s->s_proto);
  for(int i = 0; s->s_aliases[i]; ++i) {
    printf(" %s", s->s_aliases[i]);
  }
}

void ConsoleCommands_ListServents() {
  if (CommandLineParser::Instance().GetNoOfParameters() == 0) {
    setservent(0);
    struct servent* e;
    while ((e = getservent()) != NULL) {
      print_servent(e);
    }
  } else {
    const char* p1 = CommandLineParser::Instance().GetParameterAt(0);
    const char* p2 = NULL;
    if (CommandLineParser::Instance().GetNoOfParameters() >= 2) {
      p2 = CommandLineParser::Instance().GetParameterAt(1);
    }

    struct servent* e;
    if ((e = getservbyname(p1, p2)) != NULL) {
      print_servent(e);
    } else if ((e = getservbyport(atoi(p1), p2)) != NULL) {
      print_servent(e);
    } else {
      printf("\nNo such service");
    }
  }
}