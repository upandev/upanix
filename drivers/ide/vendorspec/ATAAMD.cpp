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
# include <ATAAMD.h>
# include <DMM.h>
# include <StringUtil.h>
# include <MemUtil.h>
# include <ATATimingManager.h>

AMDIDE AMDIDEList[] = {
    { "AMD Cobra", 0x1022, 0x7401, 0x40, AMD_UDMA_33 },
    { "AMD Viper", 0x1022, 0x7409, 0x40, AMD_UDMA_66 },
    { "AMD Viper", 0x1022, 0x7411, 0x40, AMD_UDMA_100 | AMD_BAD_FIFO },
    { "AMD Opus", 0x1022, 0x7441, 0x40, AMD_UDMA_100 },
    { "AMD 8111", 0x1022, 0x7469, 0x40, AMD_UDMA_100 },
    { "nVIDIA nForce", 0x10DE, 0x01BC, 0x50, AMD_UDMA_100 },
    { "nVIDIA nForce 2", 0x10DE, 0x0065, 0x50, AMD_UDMA_133 },
    { "nVIDIA nForce 2S", 0x10DE, 0x0085, 0x50, AMD_UDMA_133 },
    { "nVIDIA nForce 2S SATA", 0x10DE, 0x008E, 0x50, AMD_UDMA_133 | AMD_SATA },
    { "nVIDIA nForce 3", 0x10DE, 0x00D5, 0x50, AMD_UDMA_133 },
    { "nVIDIA nForce 3S", 0x10DE, 0x00E5, 0x50, AMD_UDMA_133 },
    { "nVIDIA nForce 3S SATA", 0x10DE, 0x00E3, 0x50, AMD_UDMA_133 | AMD_SATA },
    { "nVIDIA nForce 3S SATA", 0x10DE, 0x00EE, 0x50, AMD_UDMA_133 | AMD_SATA },
} ;

static byte AMDCYC2UDMA[] = {
	6, 6, 5, 4, 0, 1, 1, 2, 2, 3, 3, 3, 3, 3, 3, 7
} ;

#define AMD_IDE_ENABLE		( 0x00 + pAMDIDE->usBase )
#define AMD_IDE_CONFIG		( 0x01 + pAMDIDE->usBase )
#define AMD_CABLE_DETECT	( 0x02 + pAMDIDE->usBase )
#define AMD_DRIVE_TIMING	( 0x08 + pAMDIDE->usBase )
#define AMD_8BIT_TIMING		( 0x0e + pAMDIDE->usBase )
#define AMD_ADDRESS_SETUP	( 0x0c + pAMDIDE->usBase )
#define AMD_UDMA_TIMING		( 0x10 + pAMDIDE->usBase )

/******************************************** Static Functions ***********************************/

static void ATAAMD_SetPortSpeed(AMDIDE* pAMDIDE, PCIEntry* pPCIEntry, ATAPort* pPort, ATATiming* pATATiming)
{
	unsigned uiDriveNumber = pPort->uiChannel * pPort->pController->uiPortsPerChannel + pPort->uiPort ;

	byte bTemp ;

  pPCIEntry->ReadPCIConfig(AMD_ADDRESS_SETUP, 1, &bTemp);
	
  bTemp = (bTemp & ~(3 << ((3 - uiDriveNumber) << 1))) | ((FIT(pATATiming->usSetup, 1, 4) - 1) << ((3 - uiDriveNumber) << 1)) ;

  pPCIEntry->WritePCIConfig(AMD_ADDRESS_SETUP, 1, bTemp);
	
  pPCIEntry->WritePCIConfig(AMD_8BIT_TIMING + (1 - (uiDriveNumber >> 1)), 1,
                            ((FIT(pATATiming->usAct8b, 1, 16) - 1) << 4) | (FIT(pATATiming->usRec8b, 1, 16) - 1));

  pPCIEntry->WritePCIConfig(AMD_DRIVE_TIMING + (3 - uiDriveNumber), 1,
                            ((FIT(pATATiming->usAct, 1, 16) - 1) << 4) | (FIT(pATATiming->usRec, 1, 16) - 1));

	switch(pAMDIDE->usFlags & AMD_UDMA)
	{
		case AMD_UDMA_33:
			bTemp = pATATiming->usUDMA ? (0xC0 | (FIT(pATATiming->usUDMA, 2, 5) - 2)) : 0x03 ;
			break ;

		case AMD_UDMA_66:
			bTemp = pATATiming->usUDMA ? (0xC0 | AMDCYC2UDMA[FIT(pATATiming->usUDMA, 2, 10)]) : 0x03 ;
			break ;

		case AMD_UDMA_100:
			bTemp = pATATiming->usUDMA ? (0xC0 | AMDCYC2UDMA[FIT(pATATiming->usUDMA, 1, 10)]) : 0x03 ;
			break ;

		case AMD_UDMA_133:
			bTemp = pATATiming->usUDMA ? (0xC0 | AMDCYC2UDMA[FIT(pATATiming->usUDMA, 1, 15)]) : 0x03 ;
			break ;

		default:
      return;
	}
	
  pPCIEntry->WritePCIConfig(AMD_UDMA_TIMING + (3 - uiDriveNumber), 1, bTemp);
}

void ATAAMD_PortConfigure(ATAPort* pPort)
{
	ATAController* pController = pPort->pController ;
	
	AMDIDEInfo* pAMDIDEInfo = (AMDIDEInfo*)pPort->pVendorSpecInfo ;
	AMDIDE* pAMDIDE = pAMDIDEInfo->pAMDIDE ;

  if(pPort->uiPort == 0 && !(pPort->uiDevice >= ATA_DEV_ATA && pController->pPort[pPort->uiChannel * pController->uiPortsPerChannel + 1]->uiDevice < ATA_DEV_ATA))
    return;

	ATAPort* pFirstPort = NULL ;
	ATAPort* pSecondPort = NULL ;

	if(pPort->uiPort == 1)
		pFirstPort = pPort ;

	if(pPort->uiPort == 1)
	{
		if(pPort->uiDevice < ATA_DEV_ATA)
      return;

		if(pController->pPort[pPort->uiChannel * pController->uiPortsPerChannel]->uiDevice < ATA_DEV_ATA)
		{
			pFirstPort = pPort ;
		}
		else
		{
			pFirstPort = pController->pPort[pPort->uiChannel * pController->uiPortsPerChannel] ;
			pSecondPort = pPort ;
		}
	}

	// Calculate Timing
	unsigned uiTiming = 1000000000 / AMD_CLOCK ;
	unsigned uiUDMATiming ;

	switch(pAMDIDE->usFlags & AMD_UDMA)
	{
		case AMD_UDMA_33:
			uiUDMATiming = uiTiming ;
			break ;

		case AMD_UDMA_66:
			uiUDMATiming = uiTiming / 2 ;
			break ;
	
		case AMD_UDMA_100:
			uiUDMATiming = uiTiming / 3 ;
			break ;
	
		case AMD_UDMA_133:
			uiUDMATiming = uiTiming / 4 ;
			break ;

		default:
			uiUDMATiming = uiTiming ;
	}

	ATATiming ataTimingResult1, ataTimingResult2 ;

	// Calculate Timing of First Port
  ATATimingManager_Compute(pFirstPort, pFirstPort->uiCurrentSpeed, &ataTimingResult1, uiTiming, uiUDMATiming);

	// Calculate Timing of Second Port and Merge Both
	if(pSecondPort)
	{
    ATATimingManager_Compute(pSecondPort, pSecondPort->uiCurrentSpeed, &ataTimingResult2, uiTiming, uiUDMATiming);
		ATATimingManager_Merge(&ataTimingResult2, &ataTimingResult1, &ataTimingResult1, ATA_TIMING_8BIT) ;
	}
		
	if(pFirstPort && pFirstPort->uiCurrentSpeed == ATA_SPEED_UDMA_5)
		ataTimingResult1.usUDMA = 1 ;
	if(pFirstPort && pFirstPort->uiCurrentSpeed == ATA_SPEED_UDMA_6)
		ataTimingResult1.usUDMA = 15 ;

	if(pSecondPort && pSecondPort->uiCurrentSpeed == ATA_SPEED_UDMA_5)
		ataTimingResult2.usUDMA = 1 ;
	if(pSecondPort && pSecondPort->uiCurrentSpeed == ATA_SPEED_UDMA_6)
		ataTimingResult2.usUDMA = 15 ;

	// Set Port Speed
	if(pSecondPort)
    ATAAMD_SetPortSpeed(pAMDIDE, &pAMDIDEInfo->pciEntry, pSecondPort, &ataTimingResult2);

	if(pFirstPort)
    ATAAMD_SetPortSpeed(pAMDIDE, &pAMDIDEInfo->pciEntry, pFirstPort, &ataTimingResult1);
}

void ATAAMD_InitController(const PCIEntry* pPCIEntry, ATAController* pController)
{
	AMDIDE* pAMDIDE = NULL ;
  PCIEntry* pIDE = nullptr;
	unsigned uiDeviceIndex ;
	byte bIDEFound = false ;

	for(auto p : PCIBusHandler::Instance().PCIEntries())
	{
		if(p->bHeaderType & PCI_HEADER_BRIDGE)
			continue ;

		for(uiDeviceIndex = 0; 
			uiDeviceIndex < (sizeof(AMDIDEList) / sizeof(AMDIDE)); uiDeviceIndex++)
		{
			if(p->usVendorID == AMDIDEList[uiDeviceIndex].usVendorID 
				&& p->usDeviceID == AMDIDEList[uiDeviceIndex].usDeviceID)
			{
        pIDE = p;
				pAMDIDE = &AMDIDEList[uiDeviceIndex] ;
				bIDEFound = true ;
				break ;
			}
    }

		if(bIDEFound == true)
			break ;
    }

	if(bIDEFound == false)
	{
	  printf("\n\tUnknown AMD/nVIDIA ATA Controller Detected");
		return ;
	}

	if(pAMDIDE->usFlags & AMD_SATA)
	{
	  printf("\n\tnVIDIA Serial ATA Controller Detected");
		strcpy(pController->szName, "nVIDIA Serial ATA Controller") ;

		DMM_DeAllocateForKernel((uintptr_t)pController->pPort[1]) ;
		DMM_DeAllocateForKernel((uintptr_t)pController->pPort[3]) ;

		pController->pPort[1] = pController->pPort[2] ;
		pController->pPort[2] = nullptr ;
		
		pController->uiPortsPerChannel = 1 ;

		pController->pPort[0]->uiCable = pController->pPort[1]->uiCable = ATA_CABLE_SATA ;

		return ;
	}

	printf("\n\tAMD %s ATA Controller Detected", pAMDIDE->szName) ;

	byte bTemp ;
	unsigned ui80W = 0, uiUDMATiming, i ;

	switch(pAMDIDE->usFlags & AMD_UDMA)
	{
		case AMD_UDMA_66:
      pIDE->ReadPCIConfig(AMD_UDMA_TIMING, 4, &uiUDMATiming);

			for(i = 24; i >= 0; i -= 8)
			{
				if((uiUDMATiming >> i) & 4)
					ui80W |= ( 1 << ( 1 - ( i >> 4 ) ) ) ;
			}
			break ;

		case AMD_UDMA_100:
		case AMD_UDMA_133:
      pIDE->ReadPCIConfig(AMD_CABLE_DETECT, 1, &bTemp);
      pIDE->ReadPCIConfig(AMD_UDMA_TIMING, 4, &uiUDMATiming);
			
			ui80W = ((bTemp & 0x3) ? 1 : 0) | ((bTemp & 0xC) ? 2 : 0) ;
		
			for(i = 24; i >= 0; i -= 8)
			{
				if(((uiUDMATiming >> i) & 4) && !(ui80W & (1 << (1 - (i >> 4)))))
					ui80W |= (1 << (1 - (i >> 4))) ;
			}
			break ;
	}

  pController->pPort[0]->uiCable = pController->pPort[1]->uiCable = (ui80W & 0x01) ? ATA_CABLE_PATA80 : ATA_CABLE_PATA40 ;

  pController->pPort[2]->uiCable = pController->pPort[3]->uiCable = (ui80W & 0x02) ? ATA_CABLE_PATA80 : ATA_CABLE_PATA40 ;

  pIDE->ReadPCIConfig(AMD_IDE_CONFIG, 1, &bTemp);

  pIDE->WritePCIConfig(AMD_IDE_CONFIG, 1, (pAMDIDE->usFlags & AMD_BAD_FIFO) ? (bTemp & 0x0F) : (bTemp | 0xF0));

	AMDIDEInfo* pAMDIDEInfo = (AMDIDEInfo*)DMM_AllocateForKernel(sizeof(AMDIDEInfo)) ;

  memcpy(&pAMDIDEInfo->pciEntry, pPCIEntry, sizeof(PCIEntry));
	pAMDIDEInfo->pAMDIDE = pAMDIDE ;	

	for(i = 0; i < pController->uiChannels * pController->uiPortsPerChannel; i++)
	{
		ATAPort* pPort = pController->pPort[i] ;
		byte b80Bit = ( i & 0x02 ) ? ( ui80W & 0x02 ) : ( ui80W & 0x01 ) ;

		pPort->uiSupportedPortSpeed = 0xFF ;
		
		if(pAMDIDE->usFlags & AMD_UDMA)
			pPort->uiSupportedPortSpeed |= 0x700 ;
		
		if((pAMDIDE->usFlags & AMD_UDMA_66) && b80Bit)
			pPort->uiSupportedPortSpeed |= 0x1F00 ;

		if((pAMDIDE->usFlags & AMD_UDMA_100) && b80Bit)
			pPort->uiSupportedPortSpeed |= 0x3FFF ;

		if((pAMDIDE->usFlags & AMD_UDMA_133) && b80Bit)
			pPort->uiSupportedPortSpeed |= 0x7FFF ;

		pPort->portOperation.Configure = ATAAMD_PortConfigure ;
		pPort->pVendorSpecInfo = pAMDIDEInfo ;
	}

	strcpy(pController->szName, "AMD/nVIDIA ATA Controller") ;
}

