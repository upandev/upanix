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
# include <ATASIS.h>
# include <StringUtil.h>
# include <DMM.h>

static SISIDE SISIDEList[] = {
    { "SiS 745",    0x0745, SIS_UDMA_100	},
    { "SiS 735",    0x0735, SIS_UDMA_100	},
    { "SiS 733",    0x0733, SIS_UDMA_100	},
    { "SiS 635",    0x0635, SIS_UDMA_100	},
    { "SiS 633",    0x0633, SIS_UDMA_100	},
    { "SiS 730",    0x0730, SIS_UDMA_100a	},
    { "SiS 550",    0x0550, SIS_UDMA_100a	},
    { "SiS 640",    0x0640, SIS_UDMA_66		},
    { "SiS 630",    0x0630, SIS_UDMA_66		},
    { "SiS 620",    0x0620, SIS_UDMA_66		},
    { "SiS 540",    0x0540, SIS_UDMA_66		},
    { "SiS 530",    0x0530, SIS_UDMA_66		},
    { "SiS 5600",   0x5600, SIS_UDMA_33		},
    { "SiS 5598",   0x5598, SIS_UDMA_33		},
    { "SiS 5597",   0x5597, SIS_UDMA_33		},
    { "SiS 5591/2", 0x5591, SIS_UDMA_33		},
    { "SiS 5582",   0x5582, SIS_UDMA_33		},
    { "SiS 5581",   0x5581, SIS_UDMA_33		},
    { "SiS 5596",   0x5596, SIS_16		},
    { "SiS 5571",   0x5571, SIS_16		},
    { "SiS 551x",   0x5511, SIS_16		}
} ;

// {0, ATA_16, ATA_33, ATA_66, ATA_100a, ATA_100, ATA_133}
static byte ATASIS_bCycleTimeOffset[] = { 0, 0, 5, 4, 4, 0, 0 } ;
static byte ATASIS_bCycleTimeRange[] = { 0, 0, 2, 3, 3, 4, 4 } ;
static byte ATASIS_bCycleTimeValue[][ ATA_SPEED_UDMA_6 - ATA_SPEED_UDMA_0 + 1 ] = {
    {  0,  0, 0, 0, 0, 0, 0 }, /* no udma */
    {  0,  0, 0, 0, 0, 0, 0 }, /* no udma */
    {  3,  2, 1, 0, 0, 0, 0 }, /* ATA_33 */
    {  7,  5, 3, 2, 1, 0, 0 }, /* ATA_66 */
    {  7,  5, 3, 2, 1, 0, 0 }, /* ATA_100a (730 specific), differences are on cycle_time range and offset */
    { 11,  7, 5, 4, 2, 1, 0 }, /* ATA_100 */
    { 15, 10, 7, 5, 3, 2, 1 }, /* ATA_133a (earliest 691 southbridges) */
    { 15, 10, 7, 5, 3, 2, 1 }, /* ATA_133 */
} ;

// CRC Valid Setup Time Vary Across IDE Clock Setting 33/66/100/133
//	Refer SiS962 Data Sheet.
static byte ATASIS_bCVSTimeValue[][ ATA_SPEED_UDMA_6 - ATA_SPEED_UDMA_0 + 1 ] = {
    { 0, 0, 0, 0, 0, 0, 0 }, /* no udma */
    { 0, 0, 0, 0, 0, 0, 0 }, /* no udma */
    { 2, 1, 1, 0, 0, 0, 0 },
    { 4, 3, 2, 1, 0, 0, 0 },
    { 4, 3, 2, 1, 0, 0, 0 },
    { 6, 4, 3, 1, 1, 1, 0 },
    { 9, 6, 4, 2, 2, 2, 2 },
    { 9, 6, 4, 2, 2, 2, 2 },
} ;

//TODO: Tune PIO Modes
void ATASIS_PortConfigure(ATAPort* pPort)
{
	SISIDEInfo* pSISIDEInfo = (SISIDEInfo*)pPort->pVendorSpecInfo ;
	PCIEntry* pPCIEntry = &pSISIDEInfo->pciEntry ;
	unsigned uiSpeed = pSISIDEInfo->uiSpeed ;

	unsigned uiDriveNumber = pPort->uiChannel * pPort->pController->uiPortsPerChannel + pPort->uiPort ;
	unsigned uiRegDW = 0 ;
	byte bReg = 0 ;

	byte bDrivePCI = 0x40 ;

	unsigned uiNewSpeed = pPort->uiCurrentSpeed ;

	if(uiSpeed >= SIS_UDMA_133)
	{
		unsigned ui54 ;

    pPCIEntry->ReadPCIConfig(0x54, 4, &ui54);

		if(ui54 & 0x40000000)
			bDrivePCI = 0x70 ;

		bDrivePCI += uiDriveNumber * 0x04 ;

    pPCIEntry->ReadPCIConfig((unsigned)bDrivePCI, 4, &uiRegDW);

		if(uiNewSpeed < ATA_SPEED_UDMA_0)
		{
			uiRegDW &= 0xFFFFFFFB ;
      pPCIEntry->WritePCIConfig((unsigned)bDrivePCI, 4, uiRegDW);
		}
	}
	else
	{
		bDrivePCI += uiDriveNumber * 0x02 ;
    pPCIEntry->ReadPCIConfig((unsigned)bDrivePCI + 1, 1, &bReg);
		
		if((uiNewSpeed < ATA_SPEED_UDMA_0) && (uiSpeed > SIS_16))
		{
			bReg &= 0x7F ;
      pPCIEntry->WritePCIConfig((unsigned)bDrivePCI + 1, 1, bReg);
		}
	}

	switch(uiNewSpeed)
	{
		case ATA_SPEED_UDMA_6:
		case ATA_SPEED_UDMA_5:
		case ATA_SPEED_UDMA_4:
		case ATA_SPEED_UDMA_3:
		case ATA_SPEED_UDMA_2:
		case ATA_SPEED_UDMA_1:
		case ATA_SPEED_UDMA_0:
			if(uiSpeed >= SIS_UDMA_133)
			{
				uiRegDW |= 0x04 ;
				uiRegDW &= 0xFFFFF00F ;

				if(uiRegDW & 0x08)
				{
					uiRegDW |= (unsigned)ATASIS_bCycleTimeValue[SIS_UDMA_133][uiNewSpeed - ATA_SPEED_UDMA_0] << 4 ;
					uiRegDW |= (unsigned)ATASIS_bCVSTimeValue[SIS_UDMA_133][uiNewSpeed - ATA_SPEED_UDMA_0] << 8 ;
				}
				else
				{
					if(uiNewSpeed > ATA_SPEED_UDMA_5)
						uiNewSpeed = ATA_SPEED_UDMA_5 ;

					uiRegDW |= (unsigned)ATASIS_bCycleTimeValue[SIS_UDMA_100][uiNewSpeed - ATA_SPEED_UDMA_0] << 4 ;
					uiRegDW |= (unsigned)ATASIS_bCVSTimeValue[SIS_UDMA_100][uiNewSpeed - ATA_SPEED_UDMA_0] << 8 ;
				}

        pPCIEntry->WritePCIConfig((unsigned)bDrivePCI, 4, uiRegDW);
			}
			else
			{
				bReg |= 0x80 ;
				bReg &= ~((0xFF >> (8 - ATASIS_bCycleTimeRange[uiSpeed])) << ATASIS_bCycleTimeOffset[uiSpeed]) ;

				// Set Reg Cycle Time Bits

        bReg |= ATASIS_bCycleTimeValue[uiSpeed][uiNewSpeed - ATA_SPEED_UDMA_0] << ATASIS_bCycleTimeOffset[uiSpeed] ;
	
        pPCIEntry->WritePCIConfig((unsigned)bDrivePCI + 1, 1, bReg);
			}
			break ;
	}
}

void ATASIS_InitController(const PCIEntry* pPCIEntry, ATAController* pController)
{
	unsigned uiSpeed = 0 ;
	unsigned ui80W = 0 ;
		
	// Scan PCI Bus
	unsigned uiDeviceIndex ;
	byte bIDEFound = false ;
  PCIEntry* pIDE = nullptr;

	for(auto p : PCIBusHandler::Instance().PCIEntries())
	{
		if(p->bHeaderType & PCI_HEADER_BRIDGE)
			continue ;

		for(uiDeviceIndex = 0; uiDeviceIndex < (sizeof(SISIDEList) / sizeof(SISIDE)); uiDeviceIndex++)
		{
			if(p->usVendorID == SIS_VENDOR_ID && p->usDeviceID == SISIDEList[uiDeviceIndex].usDeviceID)
			{
        pIDE = p;
				uiSpeed = SISIDEList[uiDeviceIndex].usFlags ;
				bIDEFound = true ;

				// Special case for SIS630 : 630S/ET is ATA_100a
				if(p->usDeviceID == 0x0630 && p->bRevisionID >= 0x30)
					uiSpeed = SIS_UDMA_100a ;

				printf("\n\t%s ATA Controller Detected", SISIDEList[uiDeviceIndex].szName);
				strcpy(pController->szName, SISIDEList[uiDeviceIndex].szName) ;
				break ;
			}
        }

		if(bIDEFound == true)
			break ;
    }

	if(bIDEFound == false)
	{
		unsigned uiMisc ;
		unsigned short usTrueID ;
		byte bPrefCtrl ;
		byte bIDEConfig ;
		byte bSBRev ;

    pIDE->ReadPCIConfig(0x54, 4, &uiMisc);
    pIDE->WritePCIConfig(0x54, 4, (uiMisc & 0x7FFFFFFF));
    pIDE->ReadPCIConfig(PCI_DEVICE_ID, 2, &usTrueID);
    pIDE->WritePCIConfig(0x54, 4, uiMisc);
		
		if(usTrueID == 0x5518)
		{
			uiSpeed = SIS_UDMA_133 ;
      printf("\nSIS 962/963 MuTIOL IDE UDMA133 Controller Detected");
		}
		else
		{
      pIDE->ReadPCIConfig(0x4A, 1, &bIDEConfig);
      pIDE->WritePCIConfig(0x4A, 1, bIDEConfig | 0x10);
      pIDE->ReadPCIConfig(PCI_DEVICE_ID, 2, &usTrueID);
      pIDE->WritePCIConfig(0x4A, 1, bIDEConfig);
			
			if(usTrueID == 0x5517)
			{
        PCIBusHandler::Instance().ReadPCIConfig(0, 2, 0, PCI_REVISION, 1, &bSBRev);
        pIDE->ReadPCIConfig(0x49, 1, &bPrefCtrl);
			
				if(bSBRev == 0x10 && (bPrefCtrl & 0x80))
				{
					uiSpeed = SIS_UDMA_133a ;
          printf("\nSIS 961B MuTIOL IDE UDMA133 Controller Detected");
				}
				else
				{
					uiSpeed = SIS_UDMA_100 ;
          printf("\nSIS 961 MuTIOL IDE UDMA100 Controller Detected");
				}
			}
		}

    printf("\nunKnown SIS ATA Controller Detected");
    return;
	}

	byte bReg ;
	unsigned short usRegW ;

	switch(uiSpeed)
	{
		case SIS_UDMA_133:
        pIDE->ReadPCIConfig(0x50, 2, &usRegW);
				
				if(usRegW & 0x08)
          pIDE->WritePCIConfig(0x50, 2, usRegW & 0xFFF7);

        pIDE->ReadPCIConfig(0x52, 2, &usRegW);

				if(usRegW & 0x08)
          pIDE->WritePCIConfig(0x52, 2, usRegW & 0xFFF7);
				break ;

		case SIS_UDMA_133a:
		case SIS_UDMA_100:
        pIDE->WritePCIConfig(PCI_LATENCY, 1, 0x80);
        pIDE->ReadPCIConfig(0x49, 1, &bReg);

				if(!(bReg & 0x01))
          pIDE->WritePCIConfig(0x49, 1, bReg | 0x01);
				break ;

		case SIS_UDMA_100a:
		case SIS_UDMA_66:
        pIDE->WritePCIConfig(PCI_LATENCY, 1, 0x10);
        pIDE->ReadPCIConfig(0x52, 1, &bReg);

				if(!(bReg & 0x04))
          pIDE->WritePCIConfig(0x52, 1, bReg | 0x04);
				break ;

		case SIS_UDMA_33:
        pIDE->ReadPCIConfig(0x09, 1, &bReg);

				if(bReg & 0x0F)
          pIDE->WritePCIConfig(0x09, 1, bReg | 0xF0);
				break ;

		case SIS_16:
        pIDE->ReadPCIConfig(0x52, 1, &bReg);

				if(!(bReg & 0x08))
          pIDE->WritePCIConfig(0x52, 1, bReg | 0x08);
				break ;
	}

	unsigned i ;
	unsigned short usRegAddr ;
	byte bMask ;

	if(uiSpeed >= SIS_UDMA_133)
	{
		for(i = 0; i < 2; i++)
		{
			usRegAddr = i ? 0x52 : 0x50 ;
      pIDE->ReadPCIConfig(usRegAddr, 2, &usRegW);
			ui80W |= (usRegW & 0x8000) ? 0 : (1 << i) ;
		}
	}
	else if(uiSpeed >= SIS_UDMA_66)
	{
		for(i = 0; i < 2; i++)
		{
			bMask = i ? 0x20 : 0x10 ;
      pIDE->ReadPCIConfig(0x48, 1, &bReg);
			ui80W |= (bReg & bMask) ? 0 : (1 << i) ;		
		}
	}

  pController->pPort[0]->uiCable = pController->pPort[1]->uiCable = (ui80W & 0x01) ? ATA_CABLE_PATA80 : ATA_CABLE_PATA40 ;

  pController->pPort[2]->uiCable = pController->pPort[3]->uiCable = (ui80W & 0x02) ? ATA_CABLE_PATA80 : ATA_CABLE_PATA40 ;

	//Add Speeds
	SISIDEInfo* pSISIDEInfo = (SISIDEInfo*)KernelDMM::Instance().allocate(sizeof(SISIDEInfo)) ;

  memcpy(&pSISIDEInfo->pciEntry, pPCIEntry, sizeof(PCIEntry));
	pSISIDEInfo->uiSpeed = uiSpeed ;

	for(i = 0; i < pController->uiChannels * pController->uiPortsPerChannel; i++)
	{
		ATAPort* pPort = pController->pPort[i] ;
		byte b80Bit = (i & 0x02) ? (ui80W & 0x02) : (ui80W & 0x01) ;

		//Add all MultiWord DMA and PIO Modes
		pPort->uiSupportedPortSpeed = 0xFF ;

		if(uiSpeed >= SIS_UDMA_33)
			pPort->uiSupportedPortSpeed |= 0x700 ;

		if(uiSpeed >= SIS_UDMA_66 && b80Bit)
			pPort->uiSupportedPortSpeed |= 0x1F00 ;

		if(uiSpeed >= SIS_UDMA_100a && b80Bit)
			pPort->uiSupportedPortSpeed |= 0x3FFF ;

		if(uiSpeed >= SIS_UDMA_133a && b80Bit)
			pPort->uiSupportedPortSpeed |= 0x7FFF ;

		pPort->portOperation.Configure = ATASIS_PortConfigure ;
		pPort->pVendorSpecInfo = pSISIDEInfo ;
	}

	strcpy(pController->szName, "SIS ATA Controller") ;
}
