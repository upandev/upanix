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
# include <ATATimingManager.h>
# include <MemUtil.h>
# include <DMM.h>

static ATATiming ATATimingManager_Timings[] = {
    { ATA_SPEED_UDMA_6,     0,   0,   0,   0,   0,   0,   0,  15 },
    { ATA_SPEED_UDMA_5,     0,   0,   0,   0,   0,   0,   0,  20 },
    { ATA_SPEED_UDMA_4,     0,   0,   0,   0,   0,   0,   0,  30 },
    { ATA_SPEED_UDMA_3,     0,   0,   0,   0,   0,   0,   0,  45 },
    { ATA_SPEED_UDMA_2,     0,   0,   0,   0,   0,   0,   0,  60 },
    { ATA_SPEED_UDMA_1,     0,   0,   0,   0,   0,   0,   0,  80 },
    { ATA_SPEED_UDMA_0,     0,   0,   0,   0,   0,   0,   0, 120 },
    { ATA_SPEED_MWDMA_2,  25,   0,   0,   0,  70,  25, 120,   0 },
    { ATA_SPEED_MWDMA_1,  45,   0,   0,   0,  80,  50, 150,   0 },
    { ATA_SPEED_MWDMA_0,  60,   0,   0,   0, 215, 215, 480,   0 },
    { ATA_SPEED_PIO_5,     20,  50,  30, 100,  50,  30, 100,   0 },
    { ATA_SPEED_PIO_4,     25,  70,  25, 120,  70,  25, 120,   0 },
    { ATA_SPEED_PIO_3,     30,  80,  70, 180,  80,  70, 180,   0 },
    { ATA_SPEED_PIO,     30, 290,  40, 330, 100,  90, 240,   0 },
    { -1 }
} ;

/************************************ Static Functions ************************************************/

static void ATATimingManager_Quantize(ATATiming* pATATimingS, ATATiming* pATATimingR, unsigned uiTiming, unsigned uiUDMATiming)
{
	pATATimingR->usSetup = EZ(pATATimingS->usSetup * 1000, uiTiming) ;
	pATATimingR->usAct8b = EZ(pATATimingS->usAct8b * 1000, uiTiming) ;
	pATATimingR->usRec8b = EZ(pATATimingS->usRec8b * 1000, uiTiming) ;
	pATATimingR->usCyc8b = EZ(pATATimingS->usCyc8b * 1000, uiTiming) ;
	pATATimingR->usAct = EZ(pATATimingS->usAct * 1000, uiTiming) ;
	pATATimingR->usRec = EZ(pATATimingS->usRec * 1000, uiTiming) ;
	pATATimingR->usCyc = EZ(pATATimingS->usCyc * 1000, uiTiming) ;
	pATATimingR->usUDMA = EZ(pATATimingS->usUDMA * 1000, uiUDMATiming) ;
}

static ATATiming* ATATimingManager_FindMode(int iSpeed)
{
	ATATiming* pATATiming ;
	for(pATATiming = ATATimingManager_Timings; pATATiming->iMode != iSpeed; pATATiming++)
	{
		if(pATATiming->iMode < 0)
			return NULL ;
	}
	return pATATiming ;
}
/********************************************************************************************************/

void ATATimingManager_Merge(ATATiming* pFirst, ATATiming* pSecond, ATATiming* pResult, unsigned uiWhat)
{
	if(uiWhat & ATA_TIMING_SETUP) pResult->usSetup = MAX(pFirst->usSetup, pSecond->usSetup) ;
	if(uiWhat & ATA_TIMING_ACT8B) pResult->usAct8b = MAX(pFirst->usAct8b, pSecond->usAct8b) ;
	if(uiWhat & ATA_TIMING_REC8B) pResult->usRec8b = MAX(pFirst->usRec8b, pSecond->usRec8b) ;
	if(uiWhat & ATA_TIMING_CYC8B) pResult->usCyc8b = MAX(pFirst->usCyc8b, pSecond->usCyc8b) ;
	if(uiWhat & ATA_TIMING_ACTIVE) pResult->usAct = MAX(pFirst->usAct, pSecond->usAct) ;
	if(uiWhat & ATA_TIMING_RECOVER) pResult->usRec = MAX(pFirst->usRec, pSecond->usRec) ;
	if(uiWhat & ATA_TIMING_CYCLE) pResult->usCyc = MAX(pFirst->usCyc, pSecond->usCyc) ;
	if(uiWhat & ATA_TIMING_UDMA) pResult->usUDMA = MAX(pFirst->usUDMA, pSecond->usUDMA) ;
}

void ATATimingManager_Compute(ATAPort* pPort, int iSpeed, ATATiming* pATATiming, unsigned uiTiming, unsigned uiUDMATiming)
{
	ATATiming* pATATimingS ;
	ATATiming ataTimingP ;

	if((pATATimingS = ATATimingManager_FindMode(iSpeed)) == NULL)
    throw upan::exception(XLOC, "No ATATime manager found for speed: %d", iSpeed);

	if(pPort->id.usValid & 0x2) //EIDE Drive
	{
		MemUtil_Set((byte*)&ataTimingP, 0, sizeof(ATATiming)) ;

		if(iSpeed == ATA_SPEED_PIO)
			ataTimingP.usCyc = ataTimingP.usCyc8b = pPort->id.usEIDEPIO ;
		else if(iSpeed <= ATA_SPEED_PIO_5)
			ataTimingP.usCyc = ataTimingP.usCyc8b = pPort->id.usEIDEPIOOrdy ;
		else if(iSpeed <= ATA_SPEED_MWDMA_2)
			ataTimingP.usCyc = pPort->id.usEIDEDMAMin ;

		ATATimingManager_Merge(&ataTimingP, pATATiming, pATATiming, ATA_TIMING_CYCLE | ATA_TIMING_CYC8B) ;
	}

	// Covert Timing to bus clock counts
	ATATimingManager_Quantize(pATATimingS, pATATiming, uiTiming, uiUDMATiming) ;
	
	// Find best PIO Mode
	int i ;
	int iPIOMode = -1 ;
	if(iSpeed > ATA_SPEED_PIO_5)
	{
		for(i = ATA_SPEED_PIO_5; i >= 0; i--)
		{
			if((pPort->uiSupportedPortSpeed & (1 << i)) && (pPort->uiSupportedDriveSpeed * (1 << i)) != 0)
			{
				iPIOMode = i ;
				break ;
			}
		}

		if(iPIOMode < 0)
      throw upan::exception(XLOC, "\nFailed to find a Supported PIO Mode");

		ATATimingManager_Compute(pPort, iPIOMode, &ataTimingP, uiTiming, uiUDMATiming) ;
		ATATimingManager_Merge(&ataTimingP, pATATiming, pATATiming, ATA_TIMING_ALL) ;
	}

	// Increase Active and Recovery Time so that Cycle Time is Correct
	if(pATATiming->usAct8b + pATATiming->usRec8b < pATATiming->usCyc8b)
	{
		pATATiming->usAct8b += (pATATiming->usCyc8b - (pATATiming->usAct8b + pATATiming->usRec8b)) / 2 ;
		pATATiming->usRec8b = pATATiming->usCyc8b - pATATiming->usAct8b ;
	}

	if(pATATiming->usAct + pATATiming->usRec < pATATiming->usCyc)
	{
		pATATiming->usAct += (pATATiming->usCyc - (pATATiming->usAct + pATATiming->usRec)) / 2 ;
		pATATiming->usRec = pATATiming->usCyc - pATATiming->usAct ;
	}
}
