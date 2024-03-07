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
# include <ATADeviceController.h>
# include <ATAPortOperation.h>
# include <ATAPortManager.h>
# include <DMM.h>
# include <StringUtil.h>
# include <PortCom.h>
# include <ATAIntel.h>
# include <ATAAMD.h>
# include <ATAVIA.h>
# include <ATASIS.h>
# include <ATADrive.h>
# include <PartitionManager.h>
# include <FileSystem.h>
# include <stdio.h>

static unsigned ATADeviceController_uiHDDDeviceID ;
static unsigned ATADeviceController_uiCDDeviceID ;

static const IRQ* HD_PRIMARY_IRQ ;
static const IRQ* HD_SECONDARY_IRQ ;

byte ATADeviceController_bLegacyControllerFound ;
unsigned ATADeviceController_uiPortIDSequence ;
ATAController* ATADeviceController_pFirstController ;

ATAPCIDevice ATADeviceController_Devices[] = {
	/* VIA */
	{ 0x1106, 0x1571, ATAVIA_InitController },
	{ 0x1106, 0x0571, ATAVIA_InitController },
	//{ 0x1106, 0x3149, NULL }, //Disabled, SATA
	/* Intel */
	{ 0x8086, 0x122E, ATAIntel_InitController },
	{ 0x8086, 0x1230, ATAIntel_InitController },
	{ 0x8086, 0x1234, ATAIntel_InitController },
	{ 0x8086, 0x7010, ATAIntel_InitController },
	{ 0x8086, 0x7111, ATAIntel_InitController },
	{ 0x8086, 0x7198, ATAIntel_InitController },
	{ 0x8086, 0x7601, ATAIntel_InitController },
	{ 0x8086, 0x84CA, ATAIntel_InitController },
	{ 0x8086, 0x2421, ATAIntel_InitController },
	{ 0x8086, 0x2411, ATAIntel_InitController },
	{ 0x8086, 0x244B, ATAIntel_InitController },
	{ 0x8086, 0x244A, ATAIntel_InitController },
	{ 0x8086, 0x248A, ATAIntel_InitController },
	{ 0x8086, 0x248B, ATAIntel_InitController },
	{ 0x8086, 0x24CB, ATAIntel_InitController },
	{ 0x8086, 0x24DB, ATAIntel_InitController },
	{ 0x8086, 0x245B, ATAIntel_InitController },
	{ 0x8086, 0x24CA, ATAIntel_InitController },
	{ 0x8086, 0x24D1, ATAIntel_InitController },
	
	// Beta Check
	//{ 0x8086, 0x2651, ATAIntel_InitController },
	//{ 0x8086, 0x266A, ATAIntel_InitController },

	/* SIS */
	{ 0x1039, 0x5513, ATASIS_InitController },
	/* AMD */
	{ 0x1022, 0x7401, ATAAMD_InitController },
	{ 0x1022, 0x7409, ATAAMD_InitController },
	{ 0x1022, 0x7411, ATAAMD_InitController },
	{ 0x1022, 0x7441, ATAAMD_InitController },
	{ 0x1022, 0x7469, ATAAMD_InitController },
	/* nVIDIA */
	{ 0x10DE, 0x01BC, ATAAMD_InitController },
	{ 0x10DE, 0x0065, ATAAMD_InitController },
	{ 0x10DE, 0x0085, ATAAMD_InitController },
	{ 0x10DE, 0x008E, ATAAMD_InitController },
	{ 0x10DE, 0x00D5, ATAAMD_InitController },
	{ 0x10DE, 0x00E5, ATAAMD_InitController },
	{ 0x10DE, 0x00E3, ATAAMD_InitController },
	{ 0x10DE, 0x00EE, ATAAMD_InitController }
} ;

/******************************** static functions **************************************************/

extern "C" {
  void _ata_primary_interrupt_handler();
  void _ata_secondary_interrupt_handler();
}

void ATADeviceController_PrimaryIRQHandler()
{
  HD_PRIMARY_IRQ->Signal();
	IrqManager::Instance().SendEOI(*HD_PRIMARY_IRQ) ;
}

void ATADeviceController_SecondaryIRQHandler()
{
  HD_SECONDARY_IRQ->Signal();
	IrqManager::Instance().SendEOI(*HD_SECONDARY_IRQ) ;
}

static unsigned ATADeviceController_GetNextPortID()
{
	return ATADeviceController_uiPortIDSequence++ ;
}
static ATAController* ATADeviceController_AllocateController(unsigned uiChannels, unsigned uiPortsPerChannel)
{
	ATAController* pController = (ATAController*)DMM_AllocateForKernel(sizeof(ATAController)) ;

	pController->uiChannels = uiChannels ;
	pController->uiPortsPerChannel = uiPortsPerChannel ;
	strcpy(pController->szName, "ATA Controller") ;
	
	return pController ;
}

static ATAPort* ATADeviceController_AllocatePort(ATAController* pController)
{
	ATAPort* pPort = (ATAPort*)DMM_AllocateForKernel(sizeof(ATAPort)) ;

	// Set Default values
	pPort->pController = pController ;
	pPort->portOperation.Reset = NULL ;
	pPort->portOperation.Select = ATAPortOperation_PortSelect ;
	pPort->portOperation.Configure = ATAPortOperation_PortConfigure ;
	pPort->portOperation.PrepareDMARead = ATAPortOperation_PortPrepareDMARead ;
	pPort->portOperation.PrepareDMAWrite = ATAPortOperation_PortPrepareDMAWrite ;
	pPort->portOperation.StartDMA = ATAPortOperation_StartDMA ;
	pPort->portOperation.FlushRegs = NULL ;

	pPort->pPRDTable = (ATAPRD*)DMM_AllocateForKernel(DMA_MEM_BLOCK_SIZE, 4) ;
	pPort->pDMATransferAddr = DMM_AllocateForKernel(HDD_DMA_BUFFER_SIZE, 4) ;

	return pPort ;
}

static void ATADeviceController_EnableDisabledController(const PCIEntry* pPCIEntry)
{
	unsigned short usCommand ;
  pPCIEntry->ReadPCIConfig(PCI_COMMAND, 2, &usCommand);
	if((usCommand & (PCI_COMMAND_IO | PCI_COMMAND_MASTER)) != (PCI_COMMAND_IO | PCI_COMMAND_MASTER))
	{
    pPCIEntry->WritePCIConfig(PCI_COMMAND, 2, (usCommand | PCI_COMMAND_IO | PCI_COMMAND_MASTER));
    pPCIEntry->ReadPCIConfig(PCI_COMMAND, 2, &usCommand);
		if((usCommand & (PCI_COMMAND_IO | PCI_COMMAND_MASTER)) != (PCI_COMMAND_IO | PCI_COMMAND_MASTER))
      throw upan::exception(XLOC, "\nFailed	to Enable Controller");
    printf("\nController Enabled");
	}
}

static void ATADeviceController_CheckControllerMode(const PCIEntry* pPCIEntry,
					VendorSpecificATAController_Initialize* pInitFunc, ATAController** pController)
{
	unsigned uiPrimaryCommand, uiSecondaryCommand ;
	unsigned uiPrimaryControl, uiSecondaryControl ;
	unsigned uiPrimaryDMA, uiSecondaryDMA ;
	byte bPrimaryIRQ, bSecondaryIRQ ;

	byte bDMAPossible ;
	byte bNativeMode ;
	byte bMMIO ;

	byte bInterface ;

  pPCIEntry->ReadPCIConfig(PCI_INTERFACE, 1, &bInterface);
	
	bNativeMode = false ;
	bDMAPossible = true ;
	bMMIO = false ;

	if((bInterface & 0x05) == 0x00)
	{
    printf("\nController is in Legacy Mode");

		if(ATADeviceController_bLegacyControllerFound == true)
		{
      printf("\nAlready a Controller is in Legacy Mode... Trying to Switch to Native Mode");

			if((bInterface & 0x0A) != 0x0A)
        throw upan::exception(XLOC, "Switch to Native Mode Failed...");

      pPCIEntry->WritePCIConfig(PCI_INTERFACE, 1, (bInterface | 0x05));

			bNativeMode = true ;
		}
		else
		{
			uiPrimaryCommand = 0x1F0 ;
			uiPrimaryControl = 0x3F6 ;
			uiSecondaryCommand = 0x170 ;
			uiSecondaryControl = 0x376 ;

			bPrimaryIRQ = 14 ;
			bSecondaryIRQ = 15 ;

			uiPrimaryDMA = uiSecondaryDMA = 0 ;

      pPCIEntry->ReadPCIConfig(PCI_BASE_REGISTERS + 16, 4, &uiPrimaryDMA);
	
			if(!((uiPrimaryDMA & 0x01) && (uiPrimaryDMA & PCI_ADDRESS_IO_MASK) ))
			{
        printf("\nDMA Registers in MMIO Mode or Not Present");
        printf("\nDisabling DMA...");
				bDMAPossible = false ;
				uiPrimaryDMA = 0 ;
			}
			else
			{
				uiPrimaryDMA &= PCI_ADDRESS_IO_MASK ;
				uiSecondaryDMA = uiPrimaryDMA + 0x8 ;
			}

			ATADeviceController_bLegacyControllerFound = true ;
		}
	}
	else
	{
		bNativeMode = true ;
	}

	if(bNativeMode == true)
	{
    pPCIEntry->ReadPCIConfig(PCI_BASE_REGISTERS + 0, 4, &uiPrimaryCommand);

    pPCIEntry->ReadPCIConfig(PCI_BASE_REGISTERS + 4, 4, &uiPrimaryControl);

    pPCIEntry->ReadPCIConfig(PCI_BASE_REGISTERS + 8, 4, &uiSecondaryCommand);

    pPCIEntry->ReadPCIConfig(PCI_BASE_REGISTERS + 12, 4, &uiSecondaryControl);

    pPCIEntry->ReadPCIConfig(PCI_INTERRUPT_LINE, 1, &bPrimaryIRQ);
		bSecondaryIRQ = bPrimaryIRQ;
		
		unsigned uiMask ;
		if(!(uiPrimaryCommand & 0x01))
		{
      printf("\nController uses MMIO registers");
			bMMIO = true ;
			uiMask = PCI_ADDRESS_MEMORY_32_MASK ;
		}
		else
		{
      printf("\nController uses PIO registers");
			uiMask = PCI_ADDRESS_IO_MASK ;
		}

		if(bPrimaryIRQ != 14)
		{
      printf("Primary IRQ != 14. Disabling DMA");
			bDMAPossible = false ;
		}

		if(bPrimaryIRQ < 5 || bPrimaryIRQ > 24 || bSecondaryIRQ < 5 || bSecondaryIRQ > 24)
		{
      printf("\nInvalid IRQs. Disabling DMA");
			bDMAPossible = false ;
		}

		uiPrimaryCommand &= uiMask ;
		uiSecondaryCommand &= uiMask ;
		uiPrimaryControl = (uiPrimaryControl & uiMask) | 2 ;
		uiSecondaryControl = (uiSecondaryControl & uiMask) | 2 ;

		uiPrimaryDMA = uiSecondaryDMA = 0 ;

    pPCIEntry->ReadPCIConfig(PCI_BASE_REGISTERS + 16, 4, &uiPrimaryDMA);
		
		if( ((uiPrimaryDMA & 0x01) == bMMIO) || !(uiPrimaryDMA & uiMask))
		{
      printf("\n DMA registers in different mode than Base registers");
      printf("\nDisabling DMA...");
			bDMAPossible = false ;
			uiPrimaryDMA = 0 ;
		}
		else
		{
			uiPrimaryDMA &= uiMask ;
			uiSecondaryDMA = uiPrimaryDMA + 0x08 ;
		}
	}

  printf("\n\tChannel 0: %s, Cmd: %u, Ctrl: %u, DMA: %u, IRQ: %u",
         (bMMIO) ? "MMIO " : "PIO ", uiPrimaryCommand, uiPrimaryControl, uiPrimaryDMA, bPrimaryIRQ);
  printf("\n\tChannel 1: %s, Cmd: %u, Ctrl: %u, DMA: %u, IRQ: %u",
         (bMMIO) ? "MMIO " : "PIO ", uiSecondaryCommand, uiSecondaryControl, uiSecondaryDMA, bSecondaryIRQ);

	*pController = ATADeviceController_AllocateController(2, 2) ;
	strcpy((*pController)->szName, "PCI ATA Controller") ;

	unsigned i ;
	for(i = 0; i < (*pController)->uiChannels * (*pController)->uiPortsPerChannel; i++)
	{
		ATAPort* pPort = ATADeviceController_AllocatePort(*pController) ;
		
		unsigned uiRegBase = (i & 2) ? uiSecondaryCommand : uiPrimaryCommand ;
		unsigned uiDMARegBase = (i & 2) ? uiSecondaryDMA : uiPrimaryDMA ;
		byte bDMAMask = (i & 1) ? ATA_DMA_STATUS_1_EN : ATA_DMA_STATUS_0_EN ;
		//TODO: UnderStand
// enableSpeeds = 0 -- > Global Variable
		unsigned uiDMASpeedMask = 0 ? 0xFFFFFFFF : ((1 << ATA_SPEED_PIO) | (1 << ATA_SPEED_DMA)) ;

		pPort->portOperation.Reset = ATAPortOperation_PortReset ;
		pPort->bMMIO = bMMIO ;

		if(bDMAPossible && bMMIO && (*(reinterpret_cast<byte*>(uiDMARegBase) + 2) & bDMAMask))
			pPort->uiSupportedPortSpeed = uiDMASpeedMask ;
		else if(bDMAPossible && !bMMIO && (PortCom_ReceiveByte(uiDMARegBase + 2) & bDMAMask))
			pPort->uiSupportedPortSpeed = uiDMASpeedMask ;
		else
			pPort->uiSupportedPortSpeed = (1 << ATA_SPEED_PIO) ;
		
		pPort->uiType = ATA_PATA ;
		pPort->uiCable = ATA_CABLE_UNKNOWN ;

		unsigned j ;
		for(j = 0; j < ATA_TOTAL_REGS - 1; j++)
			pPort->uiRegs[j] = uiRegBase + j ;

		pPort->uiRegs[ATA_REG_CONTROL] = (i & 2) ? uiSecondaryControl : uiPrimaryControl ;

		pPort->uiDMARegs[ATA_REG_DMA_CONTROL] = uiDMARegBase ;
		pPort->uiDMARegs[ATA_REG_DMA_STATUS] = uiDMARegBase + 2 ;
		pPort->uiDMARegs[ATA_REG_DMA_TABLE] = uiDMARegBase + 4 ;

		(*pController)->pPort[i] = pPort ;
	}

	if(bDMAPossible)
	{
		HD_PRIMARY_IRQ = IrqManager::Instance().RegisterIRQ(bPrimaryIRQ, (uintptr_t)&_ata_primary_interrupt_handler) ;
		if(!HD_PRIMARY_IRQ)
      throw upan::exception(XLOC, "Failed to register HD Primary IRQ %d", bPrimaryIRQ);

		IrqManager::Instance().EnableIRQ(*HD_PRIMARY_IRQ);

		if(bPrimaryIRQ != bSecondaryIRQ)
		{
			HD_SECONDARY_IRQ = IrqManager::Instance().RegisterIRQ(bSecondaryIRQ, (uintptr_t)&_ata_secondary_interrupt_handler) ;
			if(!HD_SECONDARY_IRQ)
        throw upan::exception(XLOC, "Failed to register HD Secondary IRQ %d", bSecondaryIRQ);

      IrqManager::Instance().EnableIRQ(*HD_SECONDARY_IRQ);
		}
	}

	if(pInitFunc && bDMAPossible)
		pInitFunc(pPCIEntry, *pController) ;
}

static void ATADeviceController_AddATADrive(RawDiskDrive* pDisk)
{
	char driveCh = 'a' ;
	char driveName[5] = { 'h', 'd', 'd', '\0', '\0' } ;

	ATAPort* pPort = (ATAPort*)pDisk->Device();
	PartitionTable partitionTable(*pDisk);
	if(partitionTable.IsEmpty())
	{
		printf("\n Disk Not Partitioned");
		return;
	}

	/*** Calculate MountSpacePerPartition, TableCacheSize *****/
	const unsigned uiSectorsInFreePool = 4096 ;

	/*** DONE - Calculating mount stuff ***/
  for(const auto& pe : partitionTable.GetPartitions())
	{
		driveName[3] = driveCh + ATADeviceController_uiHDDDeviceID++ ;
    DiskDriveManager::Instance().Create(driveName, DEV_ATA_IDE, HDD_DRIVE0,
      pe.LBAStartSector(),
      pe.LBASize(),
      pPort->id.usSectors,
      pPort->id.usCylinders,
      pPort->id.usHead,
      pPort,
      pDisk,
      uiSectorsInFreePool);
	}
}

static void ATADeviceController_AddATAPIDrive(ATAPort* pPort)
{
	char driveCh = 'a' ;
	char driveName[4] = { 'c', 'd', '\0', '\0' } ;
	driveName[2] = driveCh + ATADeviceController_uiCDDeviceID++;

	const unsigned uiSectorsInFreePool = 4096 ;
	
  unsigned uiSizeInSectors;
	if(pPort->bLBA48Bit)
		uiSizeInSectors = pPort->id.uiLBACapacity48 ;
	else
		uiSizeInSectors = pPort->id.uiLBASectors ;

  DiskDriveManager::Instance().Create(driveName, DEV_ATAPI, CD_DRIVE0, 0, uiSizeInSectors,
                                      pPort->id.usSectors, pPort->id.usCylinders, pPort->id.usHead, pPort, nullptr, uiSectorsInFreePool);
}

static void ATADeviceController_Add(ATAController* pController)
{
  for(uint32_t i = 0; i < pController->uiChannels; i++)
	{
    for(uint32_t j = 0; j < pController->uiPortsPerChannel; j++)
		{
			ATAPort* pPort = pController->pPort[i * pController->uiPortsPerChannel + j] ;

			if(pPort->portOperation.Reset == NULL)
			{
        printf("\nNo Reset Function provided...");
        return;
			}

			if(pPort->portOperation.Configure == NULL)
			{
        printf("\nNo Configure Function provided...");
        return;
			}

			pPort->uiID = ATADeviceController_GetNextPortID() ;
			pPort->uiChannel = i ;
			pPort->uiPort = j ;
// TODO: Remove Data Buffer if not required
//			pPort->pDataBuffer = (byte*)DMM_AllocateForKernel(16 * PAGE_SIZE + 4) ;
		}
	}

	// Add ATA Controller to List
	pController->pNextATAController = ATADeviceController_pFirstController ;
	ATADeviceController_pFirstController = pController ;

	// Probe Ports
  for(uint32_t i = 0; i < pController->uiChannels * pController->uiPortsPerChannel; i++)
		ATAPortManager_Probe(pController->pPort[i]) ;

	// Configure Ports
  for(uint32_t i = 0; i < pController->uiChannels * pController->uiPortsPerChannel; i++)
	{
		ATAPortManager_ConfigureDrive(pController->pPort[i]) ;

		if(pController->pPort[i]->portOperation.Configure != NULL)
			pController->pPort[i]->portOperation.Configure(pController->pPort[i]) ;

		if(0) //TODO: ForcePIO Kernel Param
			pController->pPort[i]->uiCurrentSpeed = ATA_SPEED_PIO ;
	}

	char szName[10] = "hddisk0" ;
	int iCntIndex = 6 ;

  for(uint32_t i = 0; i < pController->uiChannels * pController->uiPortsPerChannel; i++)
	{	
		if(pController->pPort[i]->uiDevice == ATA_DEV_ATA)
		{
			szName[iCntIndex] += i ;
			RawDiskDrive* pDisk = DiskDriveManager::Instance().CreateRawDisk(szName, ATA_HARD_DISK, pController->pPort[ i ]) ;
      try
      {
  			ATADeviceController_AddATADrive(pDisk) ;
      }
      catch(const upan::exception& ex)
      {
        printf("\n failed to read partition table - %s", ex.ErrorMsg().c_str());
      }
		}
		else if(pController->pPort[i]->uiDevice == ATA_DEV_ATAPI)
			ATADeviceController_AddATAPIDrive(pController->pPort[i]) ;
	}
}

static byte ATADeviceController_InitController(const PCIEntry* pPCIEntry, VendorSpecificATAController_Initialize* pInitFunc)
{
	printf("\n\tATA PCI Controller found on Bus: %u, Device: %u, Function: %u, VendorID: %u, DeviceID: %u",
        pPCIEntry->uiBusNumber,	pPCIEntry->uiDeviceNumber, pPCIEntry->uiFunction, pPCIEntry->usVendorID, pPCIEntry->usDeviceID);

	ATAController* pController ;
  try
  {
    ATADeviceController_EnableDisabledController(pPCIEntry);
    ATADeviceController_CheckControllerMode(pPCIEntry, pInitFunc, &pController);
    ATADeviceController_Add(pController);
  }
  catch(const upan::exception& e)
  {
    e.Print();
    return ATADeviceController_FAILURE;
  }
	return ATADeviceController_SUCCESS ;
}

/******************************** static functions **************************************************/

void ATADeviceController_Initialize()
{
  static bool ATADeviceController_bInitStatus = false;

  if(ATADeviceController_bInitStatus)
  {
    printf("\n ATA Controller already initialized");
    return;
  }
  ATADeviceController_bInitStatus = false;

	ATADeviceController_uiHDDDeviceID = 0 ;
	ATADeviceController_uiCDDeviceID = 0 ;

	ATADeviceController_bLegacyControllerFound = false ;
	ATADeviceController_uiPortIDSequence = 0 ;
	ATADeviceController_pFirstController = NULL ;

	byte bControllerFound = false ;

  try
  {
    for(auto pPCIEntry : PCIBusHandler::Instance().PCIEntries())
    {
      if(pPCIEntry->bHeaderType & PCI_HEADER_BRIDGE)
        continue ;

      for(uint32_t uiDeviceIndex = 0;
          uiDeviceIndex < (sizeof(ATADeviceController_Devices) / sizeof(ATAPCIDevice)); uiDeviceIndex++)
      {
        if(pPCIEntry->usVendorID == ATADeviceController_Devices[uiDeviceIndex].usVendorID
          && pPCIEntry->usDeviceID == ATADeviceController_Devices[uiDeviceIndex].usDeviceID)
        {
          if(ATADeviceController_InitController(pPCIEntry, ATADeviceController_Devices[uiDeviceIndex].InitFunc) == ATADeviceController_SUCCESS)
          {
            bControllerFound = true ;
          }
        }
      }

      if(bControllerFound == false && pPCIEntry->bClassCode == PCI_MASS_STORAGE
        && pPCIEntry->bSubClass == PCI_IDE && pPCIEntry->usDeviceID != 0x3149)
      {
        if(ATADeviceController_InitController(pPCIEntry, NULL) == ATADeviceController_SUCCESS)
          bControllerFound = true ;
      }
    }
    ATADeviceController_bInitStatus = bControllerFound ;
  }
  catch(const upan::exception& e)
  {
    ATADeviceController_bInitStatus = false;
    e.Print();
  }

  KC::MConsole().LoadMessage("IDE Initialization", ATADeviceController_bInitStatus ? Success : Failure);
}

unsigned ATADeviceController_GetDeviceSectorLimit(ATAPort* pPort)
{
	return (pPort->bLBA48Bit) ? pPort->id.uiLBACapacity48 : pPort->id.uiLBASectors ;
}

const IRQ& ATADeviceController_GetHDInterruptNo(ATAPort* pPort)
{
	if(pPort->uiDevice == ATA_DEV_ATAPI)
		return *HD_SECONDARY_IRQ ;
	return *HD_PRIMARY_IRQ ;
}

