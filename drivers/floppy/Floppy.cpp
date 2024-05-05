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

#include <Floppy.h>
#include <IDT.h>
#include <PIC.h>
#include <DMA.h>
#include <PortCom.h>
#include <ProcessManager.h>
#include <StorageDrive.h>
#include <StringUtil.h>
#include <DMM.h>
#include <KernelUtil.h>
#include <try.h>
#include <StorageDriveManager.h>

#define	SRA_STATUS_REGA				0x3F0
#define	SRB_STATUS_REGB				0x3F1
#define	DOR_DIGITAL_OUTPUT_REG		0x3F2
#define	TDR_TAPE_DRIVE_REG			0x3F3
#define MSR_MAIN_STATUS_REG			0x3F4
#define DSR_DATA_RATE_SELECT_REG	0x3F4
#define DFR_DATA_FIFO_REG			0x3F5
#define DIR_DIGITAL_INPUT_REG		0x3F7
#define CCR_CONFIG_CONTROL_REG		0x3F7

#define DOR_DMA_GATE				0x08
#define DOR_RESET					0x04
#define FDD_VERSION_ENHANCED		0x90

#define FDD_VERSION_CMD				0x10
#define FDD_RECALIBRATE_CMD			0x07
#define FDD_SENSE_INT_STATUS_CMD	0x08
#define FDD_SENSE_DRIVE_STATUS_CMD	0x04
#define FDD_SEEK_CMD				0x0F
#define FDD_SPECIFY_CMD				0x03
#define FDD_READ_CMD				0xE6 // Test 0xE6 --> MT --> Multi Track
#define FDD_WRITE_CMD				0xC5
#define FDD_FORMAT_TRACK_CMD		0x4D

#define MSR_RQM						0x80
#define MSR_DIO						0x40
#define MSR_NON_DMA					0x20
#define MSR_CMD_BUSY				0x10
#define MSR_DRV3_BUSY				0x08
#define MSR_DRV2_BUSY				0x04
#define MSR_DRV1_BUSY				0x02
#define MSR_DRV0_BUSY				0x01

#define GAP		0x1B
#define RATE	0x00
#define SRTHUT	0xCF	/* StepRate - HeadUnload Times */
#define HLT_ND	0x10	/* StepRate - HeadUnload Times + DMA Flag*/

#define MAX_DRIVES 4

static byte Floppy_ReplyBuffer[MAX_RESULT_PHASE_REPLIES] ;
static byte Floppy_bMotorOn[MAX_DRIVES] ;
static byte Floppy_bRequestMotorOff[MAX_DRIVES] ;

const byte DOR_MOTOR_NO[] = { 0x10, 0x20, 0x40, 0x80 } ;
static bool Floppy_bInitStatus = false ;

typedef struct FloppyFormat
{
	byte bTrack ;
	byte bHead ;
	byte bSector ;
	byte bSectorSize ;
} PACKED Floppy_FormatFields ;

/****************************** Static Functions **************************************/

static void Floppy_SetDataRate(byte bDataRate) ;
static void Floppy_EnableMotorAndDrive(DRIVE_NO driveNo) ;
static void Floppy_DisableMotorAndDrive(DRIVE_NO driveNo) ;
static void Floppy_Reset() ;
static void Floppy_InitDMAController(DMA_MODE DMAMode, unsigned uiWordCount) ;

static void Floppy_SendControlCommand(byte bCmd) ;
static void Floppy_SendSpecifyCommand() ;
static void Floppy_CompleteResultPhase(byte bNoOfResultReplies) ;
static void Floppy_SenseInterruptStatus() ;
static void Floppy_WaitForInterrupt() ;
static void Floppy_ReCaliberate(DRIVE_NO driveNo) ;
static void Floppy_Seek(DRIVE_NO driveNo, Floppy_HEAD_NO headNo, unsigned uiSeekTrack) ;
static void Floppy_FormatTrack(const StorageDrive* pDiskDrive, const Floppy_HEAD_NO headNo) ;
static void Floppy_ReadWrite(const StorageDrive* pDiskDrive, unsigned uiStartSectorNo, unsigned uiEndSectorNo, Floppy_MODE mode) ;
static void Floppy_FullFormat(const StorageDrive* pDiskDrive) ;

struct FloppyMotorController : public KernelUtil::TimerTask
{
  bool TimerTrigger()
  {
    for(uint32_t i = 0; i < MAX_DRIVES; i++)
    {
      if(Floppy_bRequestMotorOff[i] == true)
      {
        Floppy_bMotorOn[i] = false ;
        Floppy_bRequestMotorOff[i] = false ;
        Floppy_DisableMotorAndDrive((DRIVE_NO)i) ;
      }
    }
    return true;
  }
};

static void Floppy_StartMotor(DRIVE_NO driveNo)
{
	Floppy_bRequestMotorOff[driveNo] = false ;
	if(Floppy_bMotorOn[driveNo] == true)
		return ;

	Floppy_EnableMotorAndDrive(driveNo) ;
	Floppy_bMotorOn[driveNo] = true ;
}

static void Floppy_StopMotor(DRIVE_NO driveNo)
{
	Floppy_bRequestMotorOff[driveNo] = true ;
}

static  void Floppy_SetDataRate(byte bDataRate)
{
	PortCom_SendByte(CCR_CONFIG_CONTROL_REG, bDataRate & 0x03) ;
}

static  void Floppy_EnableMotorAndDrive(DRIVE_NO driveNo)
{
	PortCom_SendByte(DOR_DIGITAL_OUTPUT_REG, (DOR_MOTOR_NO [ driveNo ] | DOR_DMA_GATE | DOR_RESET | driveNo)) ;
}

static  void Floppy_DisableMotorAndDrive(DRIVE_NO driveNo)
{
	PortCom_SendByte(DOR_DIGITAL_OUTPUT_REG, (0x00 | DOR_DMA_GATE | DOR_RESET | driveNo)) ;
}

static  void Floppy_Reset()
{
	PortCom_SendByte(DOR_DIGITAL_OUTPUT_REG, 0x08) ;
	PortCom_SendByte(DOR_DIGITAL_OUTPUT_REG, 0x0C) ;
}

static void Floppy_InitDMAController(DMA_MODE DMAMode, unsigned uiWordCount)
{
	DMA_Request DMARequest ;
	DMARequest.DMAChannelNo = DMA_CH2 ;
	DMARequest.bDeviceID = DMA_DEVICE_ID_FLOPPY ;
	DMARequest.uiPhysicalAddress = MEM_DMA_FLOPPY_START ;
	DMARequest.uiWordCount = uiWordCount ;
	DMARequest.DMAMode = DMAMode ;

	byte bStatus;
  if((bStatus = DMA_RequestChannel(&DMARequest)) != DMA_SUCCESS)
    throw upan::exception(XLOC, "Init DMA in Floppy Failed. ErrorCode: %d\n", bStatus);
}

static void Floppy_SendControlCommand(byte bCmd)
{
	unsigned int uiCounter ;
	byte bStatus ;
	
	for(uiCounter = 0; uiCounter < 100000; uiCounter++)
	{
		bStatus = PortCom_ReceiveByte(MSR_MAIN_STATUS_REG) ;
		bStatus &= (MSR_RQM | MSR_DIO) ;
		if(bStatus == MSR_RQM)
		{
			PortCom_SendByte(DFR_DATA_FIFO_REG, bCmd) ;
      return;
		}
	}

  throw upan::exception(XLOC, "timedout while sending control command %d to floppy", bCmd);
}

static void Floppy_SendSpecifyCommand()
{
  Floppy_SendControlCommand(FDD_SPECIFY_CMD);
  Floppy_SendControlCommand(SRTHUT);
  Floppy_SendControlCommand(HLT_ND);
}

static void Floppy_CompleteResultPhase(byte bNoOfResultReplies)
{
	unsigned int uiCounter ;
	byte bCurrentNoOfResultReplies ;
	
	if(bNoOfResultReplies > MAX_RESULT_PHASE_REPLIES)
      throw upan::exception(XLOC, "Floppy: Invalid no. of result replies: %d", bNoOfResultReplies);
			
	for(uiCounter = 0, bCurrentNoOfResultReplies = 0; uiCounter < 10000; uiCounter++)
	{
    byte bStatus = PortCom_ReceiveByte(MSR_MAIN_STATUS_REG) ;
		bStatus &= (MSR_RQM | MSR_DIO | MSR_CMD_BUSY) ;

		if(bStatus == MSR_RQM)
		{
			if(bCurrentNoOfResultReplies < bNoOfResultReplies)
        throw upan::exception(XLOC, "Floppy underrun");
      return;
		}

		if(bStatus == (MSR_RQM | MSR_DIO | MSR_CMD_BUSY))
		{
			if(bCurrentNoOfResultReplies >= bNoOfResultReplies)
        throw upan::exception(XLOC, "Floppy overrun");
			Floppy_ReplyBuffer[bCurrentNoOfResultReplies++] = PortCom_ReceiveByte(DFR_DATA_FIFO_REG) ;
		}
	}
  throw upan::exception(XLOC, "Failed to complete result phase");
}

static void Floppy_SenseInterruptStatus()
{
  Floppy_SendControlCommand(FDD_SENSE_INT_STATUS_CMD);
  Floppy_CompleteResultPhase(2) ;
}

static void Floppy_WaitForInterrupt()
{
	unsigned int uiCounter ;
	
	for(uiCounter = 0; uiCounter < 100; uiCounter++)
		asm("nop") ;
}

static void Floppy_ReCaliberate(DRIVE_NO driveNo)
{
	Floppy_SendControlCommand(FDD_RECALIBRATE_CMD) ;
	Floppy_SendControlCommand(driveNo) ;

	ProcessManager::Instance().WaitOnInterrupt(StdIRQ::Instance().FLOPPY_IRQ);
	Floppy_SenseInterruptStatus() ;

  if (Floppy_ReplyBuffer[0] & 0x20 && Floppy_ReplyBuffer[1] == 0x00)
    return;

  throw upan::exception(XLOC, "Failed to recalibrate floppy");
}

static void Floppy_Seek(DRIVE_NO driveNo, Floppy_HEAD_NO headNo, unsigned uiSeekTrack)
{
	Floppy_SendControlCommand(FDD_SEEK_CMD) ;
	Floppy_SendControlCommand((headNo << 2) | driveNo) ;
 	Floppy_SendControlCommand(uiSeekTrack) ;

	ProcessManager::Instance().WaitOnInterrupt(StdIRQ::Instance().FLOPPY_IRQ);
	Floppy_SenseInterruptStatus() ;

	if (Floppy_ReplyBuffer[0] & 0x20)
    return;

  throw upan::exception(XLOC, "Floppy: failed to seek");
}

static void Floppy_FormatTrack(const StorageDrive* pDiskDrive, const Floppy_HEAD_NO headNo)
{
  Floppy_SendControlCommand(FDD_FORMAT_TRACK_CMD);
  Floppy_SendControlCommand(((headNo << 2) & 4) | pDiskDrive->DriveNumber()); //Drive 1
  Floppy_SendControlCommand(2); // No of Bytes/Sector = 512
  Floppy_SendControlCommand(pDiskDrive->SectorsPerTrack());
  Floppy_SendControlCommand(GAP);
  Floppy_SendControlCommand(0x00); // Data Field Filler Value
  ProcessManager::Instance().WaitOnInterrupt(StdIRQ::Instance().FLOPPY_IRQ);

  Floppy_CompleteResultPhase(MAX_RESULT_PHASE_REPLIES);

  if((Floppy_ReplyBuffer[0] & 0xC0) == 0x00)
    return;

  throw upan::exception(XLOC, "Floppy: failed to format");
}

static void Floppy_ReadWrite(const StorageDrive* pDiskDrive, unsigned uiStartSectorNo, unsigned uiEndSectorNo, Floppy_MODE mode)
{
	if(uiStartSectorNo + 1 > pDiskDrive->SizeInSectors()
			|| uiEndSectorNo + 1 > pDiskDrive->SizeInSectors()
			|| uiStartSectorNo >= uiEndSectorNo)
    throw upan::exception(XLOC, "floppy: invalid sector %d - %d", uiStartSectorNo, uiEndSectorNo);
		
	unsigned uiStartSector = uiStartSectorNo % pDiskDrive->SectorsPerTrack();
//	unsigned uiEndSector = uiEndSectorNo % pDiskDrive->SectorsPerTrack();
	unsigned uiRawTrack = uiStartSectorNo /	pDiskDrive->SectorsPerTrack();
	unsigned uiHead = uiRawTrack % pDiskDrive->NoOfHeads();
	unsigned uiTrack = uiRawTrack / pDiskDrive->NoOfHeads() ;
	unsigned uiSeekTrack = uiTrack ;

	Floppy_StartMotor(pDiskDrive->DriveNumber()) ;

//	ProcessManager_Sleep(100) ;
	
//	Floppy_SetDataRate(RATE) ;

	DMA_MODE DMAMode = DMA_MODE_WRITE ;
	byte bFloppyCmd = FDD_READ_CMD ;
	if(mode == FD_WRITE)
	{
		DMAMode = DMA_MODE_READ ;
		bFloppyCmd = FDD_WRITE_CMD ;
	}

	unsigned uiSeekRetryCount ;
	unsigned uiOperationRetryCount ;
	const unsigned uiNoOfRetrys = 3 ;

  upan::string lastErr;
	for(uiSeekRetryCount = 0; uiSeekRetryCount < uiNoOfRetrys; uiSeekRetryCount++)
	{
    try {
		if(uiSeekRetryCount > 0)
      Floppy_ReCaliberate(pDiskDrive->DriveNumber());

//		ProcessManager_Sleep(100) ;

    Floppy_Seek(pDiskDrive->DriveNumber(), (Floppy_HEAD_NO)uiHead, uiSeekTrack);

      for(uiOperationRetryCount = 0; uiOperationRetryCount < uiNoOfRetrys; uiOperationRetryCount++)
      {
        try {
          DMA_ReleaseChannel(DMA_CH2) ;
          Floppy_InitDMAController(DMAMode, ((uiEndSectorNo - uiStartSectorNo) * 512 - 1));

          Floppy_SendControlCommand(bFloppyCmd);
          Floppy_SendControlCommand(((uiHead << 2) & 4) | pDiskDrive->DriveNumber()); //Drive 1
          Floppy_SendControlCommand(uiTrack); //Track 0
          Floppy_SendControlCommand(uiHead); // Head 0
          Floppy_SendControlCommand(uiStartSector + 1); //sector 1
          Floppy_SendControlCommand(2); // No of Bytes/Sector = 512
          Floppy_SendControlCommand(pDiskDrive->SectorsPerTrack());
          Floppy_SendControlCommand(GAP);
          Floppy_SendControlCommand(0xFF); // DTL = 0xFF

          ProcessManager::Instance().WaitOnInterrupt(StdIRQ::Instance().FLOPPY_IRQ);

          Floppy_CompleteResultPhase(MAX_RESULT_PHASE_REPLIES);

          if(Floppy_ReplyBuffer[0] & (0x00 | ((uiHead << 2) & 4) | pDiskDrive->DriveNumber()))
            return;
        } catch(const upan::exception& ex) { lastErr = ex.ErrorMsg(); }

      }
    } catch(const upan::exception& ex) { lastErr = ex.ErrorMsg(); }
	}

  throw upan::exception(XLOC, "Floppy read/write failed - last error: %s", lastErr.c_str());
}

static void Floppy_FullFormat(const StorageDrive* pDiskDrive)
{
	DRIVE_NO driveNo = pDiskDrive->DriveNumber();
	
	if(driveNo != FD_DRIVE0 && driveNo != FD_DRIVE1)
    throw upan::exception(XLOC, "Floppy: invalid drive: %d", driveNo);
		
	Floppy_StartMotor(driveNo) ;
	
	Floppy_SetDataRate(RATE) ;

  Floppy_ReCaliberate(pDiskDrive->DriveNumber());

	Floppy_FormatFields Floppy_FormatData[pDiskDrive->SectorsPerTrack()] ;
	
	unsigned uiWordCount = pDiskDrive->SectorsPerTrack() * sizeof(Floppy_FormatFields)/* = 4 bytes */ ;
	unsigned uiTrackCount ;
	for(uiTrackCount = 0; uiTrackCount < pDiskDrive->TracksPerHead(); uiTrackCount++)
	{
		ProcessManager::Instance().Sleep(100) ;

    Floppy_Seek(pDiskDrive->DriveNumber(), (Floppy_HEAD_NO)0, uiTrackCount);

		DMA_ReleaseChannel(DMA_CH2) ;

    Floppy_InitDMAController(DMA_MODE_READ, uiWordCount);
			
		unsigned uiHeadCount ;
		for(uiHeadCount = 0; uiHeadCount < pDiskDrive->NoOfHeads(); uiHeadCount++)
		{
			unsigned uiSectorCount ;
			for(uiSectorCount = 0; uiSectorCount < pDiskDrive->SectorsPerTrack(); uiSectorCount++)
			{
				Floppy_FormatData[uiSectorCount].bTrack = uiTrackCount ; /* 0 based Index */
				Floppy_FormatData[uiSectorCount].bHead = uiHeadCount ;
				Floppy_FormatData[uiSectorCount].bSector = uiSectorCount + 1 ;
				Floppy_FormatData[uiSectorCount].bSectorSize = 2 ; /* 512bytes/Sector */
			}

      memcpy((void*)MEM_DMA_FLOPPY_START, &Floppy_FormatData, uiWordCount);

      Floppy_FormatTrack(pDiskDrive, (Floppy_HEAD_NO)uiHeadCount);
		}
	}
}

/********************************************************************************************/
extern "C" {
  void _floppy_interrupt_handler();
}

void Floppy_Handler()
{
	StdIRQ::Instance().FLOPPY_IRQ.Signal();
	IrqManager::Instance().SendEOI(StdIRQ::Instance().FLOPPY_IRQ);
}

void Floppy_Initialize()
{
	bool bInitStatus = false ;
	Floppy_bInitStatus = false ;
	Floppy_Reset() ;

	if(IrqManager::Instance().RegisterIRQ(StdIRQ::Instance().FLOPPY_IRQ, (uint64_t)&_floppy_interrupt_handler))
	{
		IrqManager::Instance().EnableIRQ(StdIRQ::Instance().FLOPPY_IRQ) ;
		
		Floppy_SetDataRate(RATE) ;

	//	ProcessManager_WaitOnInterrupt(StdIRQ::Instance().FLOPPY_IRQ) ;
    Floppy_WaitForInterrupt();
    bInitStatus = true ;
    for(int i = 0; i < 4; i++)
      bInitStatus = upan::trycall([]() { Floppy_SenseInterruptStatus(); }).isGood();

    if(bInitStatus)
      bInitStatus = upan::trycall([]() { Floppy_SendSpecifyCommand(); }).isGood();
	}
	else
	{
		printf("\n Failed to register Floppy IRQ: %d", StdIRQ::Instance().FLOPPY_IRQ.GetIRQNo());
		bInitStatus = false ;
	}

	if(bInitStatus == true)
	{
    if (upan::trycall([]() {
      if(Floppy_IsEnhancedController())
        printf("\n\tEnhanced Floppy Controller : 8272A");
      else
        printf("\n\tOlder Floppy Controller : 8272A");
    }).isBad())
    {
      printf("\n\tFailed To Get Floppy Controller Version");
    }
	
    StorageDriveManager::Instance().Create("floppya", DEV_FLOPPY, FD_DRIVE1,
                                           0, 2880,
                                           18, 80, 2,
                                           nullptr, StorageDriveManager::Instance().CreateRawDisk("floppy", RawStorageDrive::FLOPPY_DISK, NULL), 2048);
		
    for(auto i = 0; i < MAX_DRIVES; i++)
		{
			Floppy_bMotorOn[i] = false ;
			Floppy_bRequestMotorOff[i] = false ;
		}

    KernelUtil::ScheduleTimedTask("fmoncont", 5000, *new FloppyMotorController()) ;
	}

	Floppy_bInitStatus = bInitStatus ;
  KC::MConsole().LoadMessage("Floppy Initialization", bInitStatus ? Success : Failure) ;
}

bool Floppy_GetInitStatus()
{
	return Floppy_bInitStatus ;
}

bool Floppy_IsEnhancedController()
{
	Floppy_SendControlCommand(FDD_VERSION_CMD) ;

  Floppy_CompleteResultPhase(1);

  return Floppy_ReplyBuffer[0] == FDD_VERSION_ENHANCED;
}

void Floppy_Read(const StorageDrive* pDiskDrive, unsigned uiStartSectorNo, unsigned uiEndSectorNo, byte* bSectorBuffer)
{
  auto result = upan::trycall([&]() { Floppy_ReadWrite(pDiskDrive, uiStartSectorNo, uiEndSectorNo, FD_READ); });
  Floppy_StopMotor(pDiskDrive->DriveNumber()) ;
  DMA_ReleaseChannel(DMA_CH2) ;

  if(result.isGood())
    memcpy(bSectorBuffer, (void*)MEM_DMA_FLOPPY_START, (uiEndSectorNo - uiStartSectorNo) * 512);

  result.goodValueOrThrow(XLOC);
}

void Floppy_Write(const StorageDrive* pDiskDrive, unsigned uiStartSectorNo, unsigned uiEndSectorNo, byte* bSectorBuffer)
{
  memcpy((void*)MEM_DMA_FLOPPY_START, bSectorBuffer, (uiEndSectorNo - uiStartSectorNo) * 512);

  auto result = upan::trycall([&]() { Floppy_ReadWrite(pDiskDrive, uiStartSectorNo, uiEndSectorNo, FD_WRITE); });
	Floppy_StopMotor(pDiskDrive->DriveNumber()) ;
	DMA_ReleaseChannel(DMA_CH2) ;

  result.goodValueOrThrow(XLOC);
}

void Floppy_Format(const StorageDrive* pDiskDrive)
{
  Floppy_FullFormat(pDiskDrive) ;
	Floppy_StopMotor(pDiskDrive->DriveNumber()) ;
}


