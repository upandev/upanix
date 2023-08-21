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
# include <ATAVIA.h>
# include <ATATimingManager.h>
# include <DMM.h>
# include <StringUtil.h>
# include <MemUtil.h>

static VIAIDE VIAIDEList[] = {
    { "VT8237",    0x3227, 0x00, 0x2F, VIA_UDMA_133 | VIA_BAD_AST	},
    { "VT8235",    0x3177, 0x00, 0x2F, VIA_UDMA_133 | VIA_BAD_AST	},
    { "VT8233a",   0x3147, 0x00, 0x2F, VIA_UDMA_133 | VIA_BAD_AST	},
    { "VT8233c",   0x3109, 0x00, 0x2F, VIA_UDMA_100			},
    { "VT8233",    0x3074, 0x00, 0x2F, VIA_UDMA_100			},
    { "VT8231",    0x8231, 0x00, 0x2F, VIA_UDMA_100			},
    { "VT82C686b", 0x0686, 0x40, 0x4F, VIA_UDMA_100			},
    { "VT82C686a", 0x0686, 0x10, 0x2F, VIA_UDMA_66			},
    { "VT82C686",  0x0686, 0x00, 0x0F, VIA_UDMA_33 | VIA_BAD_CLK66	},
    { "VT82C596b", 0x0596, 0x10, 0x2F, VIA_UDMA_66			},
    { "VT82C596a", 0x0596, 0x00, 0x0F, VIA_UDMA_33 | VIA_BAD_CLK66	},
    { "VT82C586b", 0x0586, 0x47, 0x4F, VIA_UDMA_33 | VIA_SET_FIFO	},
    { "VT82C586b", 0x0586, 0x40, 0x46, VIA_UDMA_33 | VIA_SET_FIFO | VIA_BAD_CLK66 },
    { "VT82C586b", 0x0586, 0x30, 0x3F, VIA_UDMA_33 | VIA_SET_FIFO	},
    { "VT82C586a", 0x0586, 0x20, 0x2F, VIA_UDMA_33 | VIA_SET_FIFO	},
    { "VT82C586",  0x0586, 0x00, 0x0F, VIA_UDMA_NONE| VIA_SET_FIFO	},
    { "VT82C576b", 0x0576, 0x00, 0x2F, VIA_UDMA_NONE | VIA_SET_FIFO | VIA_NO_UNMASK }
} ;

/*************************************** Static Functions **************************************************/

static void ATAVIA_SetPortSpeed(VIAIDE* pVIAIDE, PCIEntry* pPCIEntry, ATAPort* pPort, ATATiming* pTiming)
{
	unsigned uiDriveNumber = pPort->uiChannel * pPort->pController->uiPortsPerChannel + pPort->uiPort ;
	byte bTemp ;

	if(!(pVIAIDE->usFlags & VIA_BAD_AST))
	{
    pPCIEntry->ReadPCIConfig(VIA_ADDRESS_SETUP, 1, &bTemp);

		bTemp = (bTemp & ~(3 << ((3 - uiDriveNumber) << 1))) |
				((FIT(pTiming->usSetup, 1, 4) - 1) << ((3 - uiDriveNumber) << 1)) ;

    pPCIEntry->WritePCIConfig(VIA_ADDRESS_SETUP, 1, bTemp);
	}

  pPCIEntry->WritePCIConfig(VIA_8BIT_TIMING + (1 - (uiDriveNumber >> 1)), 1, ((FIT(pTiming->usAct8b, 1, 16) - 1) << 4) | (FIT(pTiming->usRec8b, 1, 16) - 1));

  pPCIEntry->WritePCIConfig(VIA_DRIVE_TIMING + (3 - uiDriveNumber), 1, ((FIT(pTiming->usAct, 1, 16) - 1) << 4) | (FIT(pTiming->usRec, 1, 16) - 1));

	switch(pVIAIDE->usFlags & VIA_UDMA)
	{
		case VIA_UDMA_33:
			bTemp = pTiming->usUDMA ? (0xE0 | (FIT(pTiming->usUDMA, 2, 5) - 2)) : 0x03 ;
			break ;

		case VIA_UDMA_66:
			bTemp = pTiming->usUDMA ? (0xE8 | (FIT(pTiming->usUDMA, 2, 9) - 2)) : 0x0F ;
			break ;

		case VIA_UDMA_100:
			bTemp = pTiming->usUDMA ? (0xE0 | (FIT(pTiming->usUDMA, 2, 9) - 2)) : 0x07 ;
			break ;

		case VIA_UDMA_133:
			bTemp = pTiming->usUDMA ? (0xE0 | (FIT(pTiming->usUDMA, 2, 9) - 2)) : 0x07 ;
			break ;

		default:
      return;
	}

  pPCIEntry->WritePCIConfig(VIA_UDMA_TIMING + (3 - uiDriveNumber), 1, bTemp);
}

/***************************************************************************************************/

void ATAVIA_PortConfigure(ATAPort* pPort)
{
	ATAController* pController = pPort->pController ;
	ATAPort* pFirstPort = NULL ;
	ATAPort* pSecondPort = NULL ;

	ATATiming ataTimingResult1, ataTimingResult2 ;
	unsigned uiTiming, uiUDMATiming ;

	VIAIDEInfo* pVIAIDEInfo = (VIAIDEInfo*)pPort->pVendorSpecInfo ;
	VIAIDE* pVIAIDE = pVIAIDEInfo->pVIAIDE ;

	// Check Configuration
  if(pPort->uiPort == 0 && !(pPort->uiDevice >= ATA_DEV_ATA && pController->pPort[pPort->uiChannel * pController->uiPortsPerChannel + 1]->uiDevice < ATA_DEV_ATA))
    return;

	if(pPort->uiPort == 0)
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
	uiTiming = 1000000000 / VIA_CLOCK ;

	switch(pVIAIDE->usFlags & VIA_UDMA)
	{
		case VIA_UDMA_33:
			uiUDMATiming = uiTiming ;
			break ;

		case VIA_UDMA_66:
			uiUDMATiming = uiTiming / 2 ;
			break ;

		case VIA_UDMA_100:
			uiUDMATiming = uiTiming / 3 ;
			break ;

		case VIA_UDMA_133:
			uiUDMATiming = uiTiming / 4 ;
			break ;

		default: 
			uiUDMATiming = uiTiming ;
	}

	// Calculate Timing of First Port
  ATATimingManager_Compute(pFirstPort, pFirstPort->uiCurrentSpeed, &ataTimingResult1, uiTiming, uiUDMATiming);

	// Calculate Timing of Second Port and Merge Both
	if(pSecondPort)
	{
    ATATimingManager_Compute(pSecondPort, pSecondPort->uiCurrentSpeed, &ataTimingResult2, uiTiming, uiUDMATiming);
		ATATimingManager_Merge(&ataTimingResult2, &ataTimingResult1, &ataTimingResult1, ATA_TIMING_8BIT) ;
	}

	// Set Speed
	if(pSecondPort)
    ATAVIA_SetPortSpeed(pVIAIDE, &pVIAIDEInfo->pciEntry, pSecondPort, &ataTimingResult2);

	if(pFirstPort)
    ATAVIA_SetPortSpeed(pVIAIDE, &pVIAIDEInfo->pciEntry, pFirstPort, &ataTimingResult1);
}

void ATAVIA_InitController(const PCIEntry* pPCIEntry, ATAController* pController)
{
	// Check for VIA SATA
	if(pPCIEntry->usVendorID == VIA_VENDOR_ID && pPCIEntry->usDeviceID == 0x3149)
	{
		strcpy(pController->szName, "VIA Serial ATA Controller") ;

		DMM_DeAllocateForKernel((uintptr_t)pController->pPort[1]) ;
		DMM_DeAllocateForKernel((uintptr_t)pController->pPort[3]) ;

		pController->pPort[1] = pController->pPort[2] ;
		pController->pPort[2] = nullptr ;
		pController->uiPortsPerChannel = 1 ;
		pController->pPort[0]->uiCable = pController->pPort[1]->uiCable = ATA_CABLE_SATA ;

		return ;
	}

	// Scan PCI Bus
	unsigned uiDeviceIndex ;
	byte bIDEFound = false ;
	VIAIDE* pVIAIDE = NULL ;
  PCIEntry* pIDE = nullptr;

	for(auto p : PCIBusHandler::Instance().PCIEntries())
	{
		if(p->bHeaderType & PCI_HEADER_BRIDGE)
			continue ;

		for(uiDeviceIndex = 0; uiDeviceIndex < (sizeof(VIAIDEList) / sizeof(VIAIDE)); uiDeviceIndex++)
		{
			if(p->usVendorID == VIA_VENDOR_ID && p->usDeviceID == VIAIDEList[uiDeviceIndex].usDeviceID
				&& p->bRevisionID >= VIAIDEList[uiDeviceIndex].bMinRevision
				&& p->bRevisionID <= VIAIDEList[uiDeviceIndex].bMaxRevision)
			{
        pIDE = p;
				pVIAIDE = &VIAIDEList[uiDeviceIndex] ;
				bIDEFound = true ;
				break ;
			}
        }

		if(bIDEFound == true)
			break ;
    }

	if(bIDEFound == false)
	{
    printf("\nUnKnown VIA ATA Controller Detected");
		return ;
	}

  printf("\nVIA %s Controller Detected", pVIAIDE->szName);

	// Check Cable Type
	unsigned uiUDMATiming, ui80W = 0 ;
	int i ;
	switch(pVIAIDE->usFlags & VIA_UDMA)
	{
		case VIA_UDMA_66:
      pIDE->ReadPCIConfig(VIA_UDMA_TIMING, 4, &uiUDMATiming);

      pIDE->WritePCIConfig(VIA_UDMA_TIMING, 4, uiUDMATiming | 0x80008);

			for(i = 24; i >= 0; i -= 8)
			{
				if(((uiUDMATiming >> (i & 16)) & 8) && ((uiUDMATiming >> i) & 0x20) && (((uiUDMATiming >> i) & 7) < 2))
				{
					//BIOS 80-wire bit or UDMA w/ < 60ns/cycle
					ui80W |= (1 << (1 - (i >> 4))) ;	
				}
			}
			break ;

		case VIA_UDMA_100:
      pIDE->ReadPCIConfig(VIA_UDMA_TIMING, 4, &uiUDMATiming);

			for(i = 24; i >= 0; i -= 8)
			{
				if(((uiUDMATiming >> i) & 0x10) && ((uiUDMATiming >> i) & 0x20) && (((uiUDMATiming >> i) & 7) < 4))
				{
					//BIOS 80-wire bit or UDMA w/ < 60ns/cycle
					ui80W |= (1 << (1 - (i >> 4))) ;	
				}
			}
			break ;

		case VIA_UDMA_133:
      pIDE->ReadPCIConfig(VIA_UDMA_TIMING, 4, &uiUDMATiming);

			for(i = 24; i >= 0; i -= 8)
			{
				if(((uiUDMATiming >> i) & 0x10) || (((uiUDMATiming >> i) & 0x20) && (((uiUDMATiming >> i) & 7) < 6)))
				{
					//BIOS 80-wire bit or UDMA w/ < 60ns/cycle
					ui80W |= (1 << (1 - (i >> 4))) ;	
				}
			}
			break ;
	}
				
  pController->pPort[0]->uiCable = pController->pPort[1]->uiCable = (ui80W & 0x01) ? ATA_CABLE_PATA80 : ATA_CABLE_PATA40 ;

  pController->pPort[2]->uiCable = pController->pPort[3]->uiCable = (ui80W & 0x02) ? ATA_CABLE_PATA80 : ATA_CABLE_PATA40 ;

	if(pVIAIDE->usFlags & VIA_BAD_CLK66)
	{
    pIDE->ReadPCIConfig(VIA_UDMA_TIMING, 4, &uiUDMATiming);

    pIDE->WritePCIConfig(VIA_UDMA_TIMING, 4, uiUDMATiming & ~0x80008);
	}
	
	// Check if Interfaces are Enabled
	byte bIDEEnabled, bFIFOConfig ;
  pIDE->ReadPCIConfig(VIA_IDE_ENABLE, 1, &bIDEEnabled);
  pIDE->ReadPCIConfig(VIA_FIFO_CONFIG, 1, &bFIFOConfig);

	if(pVIAIDE->usFlags & VIA_BAD_PREQ)
		bFIFOConfig &= 0x7F ;

	// Set FIFO Sizes
	if(pVIAIDE->usFlags & VIA_SET_FIFO)
	{
		bFIFOConfig &= (bFIFOConfig & 0x9F) ;
		switch(bIDEEnabled & 0x3)
		{
			case 1:
				bFIFOConfig |= 0x60 ;
				break ;

			case 2:
				bFIFOConfig |= 0x00 ;
				break ;

			case 3:
				bFIFOConfig |= 0x20 ;
				break ;
		}
	}	

  pIDE->WritePCIConfig(VIA_FIFO_CONFIG, 1, bFIFOConfig);

	// Add Speeds
	VIAIDEInfo* pVIAIDEInfo = (VIAIDEInfo*)DMM_AllocateForKernel(sizeof(VIAIDEInfo)) ;

  memcpy(&pVIAIDEInfo->pciEntry, pPCIEntry, sizeof(PCIEntry));
	pVIAIDEInfo->pVIAIDE = pVIAIDE ;

	for(unsigned i = 0; i < pController->uiChannels * pController->uiPortsPerChannel; i++)
	{
		ATAPort* pPort = pController->pPort[i] ;
		byte b80Bit = (i & 0x02) ? (ui80W & 0x02) : (ui80W & 0x01) ;

		//Add all MultiWord DMA and PIO Modes
		pPort->uiSupportedPortSpeed = 0xFF ;

		if(pVIAIDE->usFlags & VIA_UDMA)
			pPort->uiSupportedPortSpeed |= 0x700 ;

		if(pVIAIDE->usFlags & VIA_UDMA_66 & b80Bit)
			pPort->uiSupportedPortSpeed |= 0x1F00 ;
		
		if(pVIAIDE->usFlags & VIA_UDMA_100 & b80Bit)
			pPort->uiSupportedPortSpeed |= 0x3FFF ;

		if(pVIAIDE->usFlags & VIA_UDMA_133 & b80Bit)
			pPort->uiSupportedPortSpeed |= 0x7FFF ;

		pPort->portOperation.Configure = ATAVIA_PortConfigure ;
		pPort->pVendorSpecInfo = pVIAIDEInfo ;
	}

	strcpy(pController->szName, "VIA ") ;
	strcat(pController->szName, pVIAIDE->szName) ;
}
