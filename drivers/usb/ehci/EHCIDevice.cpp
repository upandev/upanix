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
#include <Global.h>
#include <DMM.h>
#include <USBDataHandler.h>
#include <EHCIController.h>
#include <EHCIDataHandler.h>
#include <EHCITransaction.h>
#include <EHCIDevice.h>

EHCIDevice::EHCIDevice(EHCIController& controller) 
  : _controller(controller),
    _pControlQH(controller.CreateDeviceQueueHead(64, 0, 0)),
    _bFirstBulkRead(true), _bFirstBulkWrite(true)
{
  if(!SetAddress())
    throw upan::exception(XLOC, "SetAddress Failed");

  if(!GetDeviceDescriptor(&_deviceDesc))
    throw upan::exception(XLOC, "GetDevDesc Failed");
  _deviceDesc.DebugPrint();

	byte bConfigValue = 0;
  if(!GetConfigValue(bConfigValue))
    throw upan::exception(XLOC, "GetConfigVal Failed") ;
	printf("\n ConfigValue: %d", bConfigValue) ;

	if(!GetConfigDescriptor(&_pArrConfigDesc))
	  throw upan::exception(XLOC, "GeConfigDesc Failed");

  CheckConfiguration(bConfigValue);

  if(GetStringDescriptorZero())
	  SetLangId();
  else
//    throw upan::exception(XLOC, "GetStringDescZero Failed");
    printf("\n String DESC not supported!");
	
  GetDeviceStringDesc(_manufacturer, _deviceDesc.indexManufacturer);
  GetDeviceStringDesc(_product, _deviceDesc.indexProduct);
  GetDeviceStringDesc(_serialNum, _deviceDesc.indexSerialNum);
  PrintDeviceStringDetails();
}

bool EHCIDevice::SetConfiguration(byte bConfigValue) {
	// Setup TDs
	EHCIQTransferDesc* pTDStart = EHCIDataHandler_CreateAsyncQTransferDesc() ;

	pTDStart->uiAltpTDPointer = 1 ;
	pTDStart->uipTDToken = (8 << 16) | (3 << 10) | (2 << 8) | (1 << 7) ;

	if(SetupAllocBuffer(pTDStart, sizeof(USBDevRequest)) != EHCIController_SUCCESS)
	{
		KernelDMM::Instance().free((uintptr_t)pTDStart) ;
		return false;
	}

	auto pDevRequest = (USBDevRequest*)(pTDStart->uiBufferPointer[ 0 ]) ;
	pDevRequest->bRequestType = 0x00 ;
	pDevRequest->bRequest = 9 ;
	pDevRequest->usWValue = bConfigValue ;
	pDevRequest->usWIndex = 0 ;
	pDevRequest->usWLength = 0 ;

	EHCIQTransferDesc* pTD1 = EHCIDataHandler_CreateAsyncQTransferDesc() ;
	pTDStart->uiNextpTDPointer = (uint64_t)pTD1 ;
	pTD1->uiNextpTDPointer = 1 ;
	pTD1->uiAltpTDPointer = 1 ;
	pTD1->uipTDToken = (1 << 31) | (3 << 10) | (1 << 8) | (1 << 7) ;

	EHCITransaction aTransaction(_pControlQH, pTDStart);

	if(!aTransaction.PollWait())
	{
		printf("\n Transaction Failed: ") ;
		DisplayTransactionState(_pControlQH, pTDStart) ;
		_controller.DisplayStats() ;
    aTransaction.Clear();
		return false;
	}

  aTransaction.Clear();
	return true;
}

void EHCIDevice::CheckConfiguration(byte& bConfigValue)
{
	if(_deviceDesc.bNumConfigs <= 0)
    throw upan::exception(XLOC, "Invalid NumConfigs: %d", _deviceDesc.bNumConfigs);

	if(bConfigValue <= 0 || bConfigValue > _deviceDesc.bNumConfigs)
	{
		printf("\n Current Configuration %d -> Invalid. Setting to Configuration 1", bConfigValue) ;
    if(!SetConfiguration(1))
      throw upan::exception(XLOC, "SetConfiguration Failed");
		bConfigValue = 1 ;
	}

  SetConfigIndex(bConfigValue);
}

bool EHCIDevice::GetConfigDescriptor(USBStandardConfigDesc** pConfigDesc)
{
  bool status = true;
	int index ;

	USBStandardConfigDesc* pCD = (USBStandardConfigDesc*)KernelDMM::Instance().allocate(sizeof(USBStandardConfigDesc) * _deviceDesc.bNumConfigs) ;

	for(index = 0; index < (int)_deviceDesc.bNumConfigs; index++)
		USBDataHandler_InitConfigDesc(&pCD[index]) ;

	for(index = 0; index < (int)_deviceDesc.bNumConfigs; index++)
	{
		unsigned short usDescValue = (0x2 << 8) | (index & 0xFF) ;

		if(!(status = GetDescriptor(usDescValue, 0, -1, &(pCD[index]))))
			break ;

		int iLen = pCD[index].wTotalLength;

		void* pBuffer = (void*)KernelDMM::Instance().allocate(iLen) ;

		if(!(status = GetDescriptor(usDescValue, 0, iLen, pBuffer)))
		{
			KernelDMM::Instance().free((uintptr_t)pBuffer) ;
			break ;
		}

		USBDataHandler_CopyConfigDesc(&pCD[index], pBuffer, pCD[index].bLength) ;

		pCD[index].DebugPrint();

		printf("\n Parsing Interface information for Configuration: %d", index) ;
		printf("\n Number of Interfaces: %d", (int)pCD[ index ].bNumInterfaces) ;

		void* pInterfaceBuffer = (char*)pBuffer + pCD[index].bLength ;

		pCD[index].pInterfaces = (USBStandardInterface*)KernelDMM::Instance().allocate(sizeof(USBStandardInterface) * pCD[index].bNumInterfaces) ;
		int iI ;

		for(iI = 0; iI < (int)pCD[index].bNumInterfaces; iI++)
		{
			USBDataHandler_InitInterfaceDesc(&(pCD[index].pInterfaces[iI])) ;
		}

		for(iI = 0; iI < (int)pCD[index].bNumInterfaces; iI++)
		{
			USBStandardInterface* pInt = (USBStandardInterface*)(pInterfaceBuffer) ;

			int iIntLen = sizeof(USBStandardInterface) - sizeof(USBStandardEndPt*) ;
			int iCopyLen = pInt->bLength ;

			if(iCopyLen > iIntLen)
				iCopyLen = iIntLen ;

      memcpy(&(pCD[index].pInterfaces[iI]), pInt, iCopyLen);

			pCD[index].pInterfaces[iI].DebugPrint();

			USBStandardEndPt* pEndPtBuffer = (USBStandardEndPt*)((char*)pInterfaceBuffer + pInt->bLength) ;

			int iNumEndPoints = pCD[index].pInterfaces[iI].bNumEndpoints ;

			pCD[index].pInterfaces[iI].pEndPoints = (USBStandardEndPt*)KernelDMM::Instance().allocate(
														sizeof(USBStandardEndPt) * iNumEndPoints) ;

			printf("\n Parsing EndPoints for Interface: %d of Configuration: %d", iI, index) ;

			int iE ;
			for(iE = 0; iE < iNumEndPoints; iE++)
				USBDataHandler_InitEndPtDesc(&(pCD[index].pInterfaces[iI].pEndPoints[iE])) ;

			for(iE = 0; iE < iNumEndPoints; iE++)
			{
				USBStandardEndPt* pEndPt = &pEndPtBuffer[iE] ;

				int iELen = sizeof(USBStandardEndPt) ;
				int iECopyLen = pEndPt->bLength ;
				
				if(iECopyLen > iELen)
					iECopyLen = iELen ;

        memcpy(&(pCD[index].pInterfaces[iI].pEndPoints[iE]), pEndPt, iECopyLen);

				pCD[index].pInterfaces[iI].pEndPoints[iE].DebugPrint();
			}

			pInterfaceBuffer = (USBStandardInterface*)((char*)pEndPtBuffer + iNumEndPoints * sizeof(USBStandardEndPt)) ;
		}

		KernelDMM::Instance().free((uintptr_t)pBuffer) ;
		pBuffer = nullptr ;
	}

	if(!status)
	{
		USBDataHandler_DeAllocConfigDesc(pCD, _deviceDesc.bNumConfigs) ;
    return false;
	}

	*pConfigDesc = pCD ;

	return true;
}

bool EHCIDevice::GetStringDescriptorZero()
{
	unsigned short usDescValue = (0x3 << 8) ;
	char szPart[ 8 ];

	if(!GetDescriptor(usDescValue, 0, -1, szPart))
    return false;

	int iLen = ((USBStringDescZero*)&szPart)->bLength ;
	printf("\n String Desc Zero Len: %d", iLen) ;

	byte* pStringDescZero = (byte*)KernelDMM::Instance().allocate(iLen) ;

	if(!GetDescriptor(usDescValue, 0, iLen, pStringDescZero))
	{
		KernelDMM::Instance().free((uintptr_t)pStringDescZero) ;
		return false;
	}

	_pStrDescZero = (USBStringDescZero*)KernelDMM::Instance().allocate(sizeof(USBStringDescZero)) ;

	USBDataHandler_CopyStrDescZero(_pStrDescZero, pStringDescZero) ;
	USBDataHandler_DisplayStrDescZero(_pStrDescZero);

	KernelDMM::Instance().free((uintptr_t)pStringDescZero) ;

	return true;
}

void EHCIDevice::GetDeviceStringDesc(upan::string& desc, int descIndex)
{
  desc = "Unknown";
	if(_usLangID == 0)
    return;

	unsigned short usDescValue = (0x3 << 8);
	char szPart[ 8 ];

  if(!GetDescriptor(usDescValue | descIndex, _usLangID, -1, szPart))
    return;

  int iLen = ((USBStringDescZero*)&szPart)->bLength ;
//  printf("\n String Index: %u, String Desc Size: %d", descIndex, iLen) ;
  if(iLen == 0)
    return;

  char* strDesc = new char[iLen];
  if(!GetDescriptor(usDescValue | descIndex, _usLangID, iLen, strDesc))
  {
    delete[] strDesc;
    return;
  }

  int j, k;
  for(j = 0, k = 0; j < (iLen - 2); k++, j += 2)
  {
    //TODO: Ignoring Unicode 2nd byte for the time.
    strDesc[k] = strDesc[j + 2];
  }
  strDesc[k] = '\0';

  desc = strDesc;
  delete[] strDesc;
}

byte EHCIDevice::GetMaxLun()
{
	// Setup TDs
	EHCIQTransferDesc* pTDStart = EHCIDataHandler_CreateAsyncQTransferDesc() ;

	pTDStart->uiAltpTDPointer = 1 ;
	pTDStart->uipTDToken = (8 << 16) | (3 << 10) | (2 << 8) | (1 << 7) ;

	if(SetupAllocBuffer(pTDStart, sizeof(USBDevRequest)) != EHCIController_SUCCESS)
	{
		KernelDMM::Instance().free((uintptr_t)pTDStart) ;
    throw upan::exception(XLOC, "Failed to setup buffer");
	}

	USBDevRequest* pDevRequest = (USBDevRequest*)(pTDStart->uiBufferPointer[ 0 ]) ;
	pDevRequest->bRequestType = USB_TYPE_CLASS | USB_RECIP_INTERFACE | USB_DIR_IN ;
	pDevRequest->bRequest = 0xFE ;
	pDevRequest->usWValue = 0 ;
	pDevRequest->usWIndex = _bInterfaceNumber ;
	pDevRequest->usWLength = 1 ;

	EHCIQTransferDesc* pTD1 = EHCIDataHandler_CreateAsyncQTransferDesc() ;
	pTDStart->uiNextpTDPointer = (uint64_t)pTD1;
	pTD1->uiAltpTDPointer = 1 ;
	pTD1->uipTDToken = (1 << 31) | (1 << 16) | (3 << 10) | (1 << 8) | (1 << 7) ;
	
	if(SetupAllocBuffer(pTD1, 1) != EHCIController_SUCCESS)
	{
		KernelDMM::Instance().free((uintptr_t)pTDStart) ;
		KernelDMM::Instance().free((uintptr_t)pTD1) ;
    throw upan::exception(XLOC, "Failed to setup buffer");
	}

	// It is important to store the data buffer address now for later read as the data buffer address
	// in TD will be incremented by Host Controller with the number of bytes read
	uintptr_t uiDataBuffer = pTD1->uiBufferPointer[ 0 ] ;

	EHCIQTransferDesc* pTD2 = EHCIDataHandler_CreateAsyncQTransferDesc() ;
	pTD1->uiNextpTDPointer = (uint64_t)pTD2 ;
	pTD2->uiNextpTDPointer = 1 ;
	pTD2->uiAltpTDPointer = 1 ;
	pTD2->uipTDToken = (3 << 10) | (1 << 7) ;

	EHCITransaction aTransaction(_pControlQH, pTDStart);

	if(!aTransaction.PollWait())
	{
		printf("\n Transaction Failed: ") ;
		DisplayTransactionState(_pControlQH, pTDStart) ;
		_controller.DisplayStats() ;
    aTransaction.Clear();
    throw upan::exception(XLOC, "EHCI transaction failed");
	}
	
	byte bLUN = *reinterpret_cast<char*>(uiDataBuffer);

  aTransaction.Clear();

	return bLUN;
}

bool EHCIDevice::CommandReset()
{
	// Setup TDs
	EHCIQTransferDesc* pTDStart = EHCIDataHandler_CreateAsyncQTransferDesc() ;

	pTDStart->uiAltpTDPointer = 1 ;
	pTDStart->uipTDToken = (8 << 16) | (3 << 10) | (2 << 8) | (1 << 7) ;

	if(SetupAllocBuffer(pTDStart, sizeof(USBDevRequest)) != EHCIController_SUCCESS)
	{
		KernelDMM::Instance().free((uintptr_t)pTDStart) ;
		return false ;
	}

	USBDevRequest* pDevRequest = (USBDevRequest*)(pTDStart->uiBufferPointer[ 0 ]) ;
	pDevRequest->bRequestType = USB_TYPE_CLASS | USB_RECIP_INTERFACE ;
	pDevRequest->bRequest = 0xFF ;
	pDevRequest->usWValue = 0 ;
	pDevRequest->usWIndex = _bInterfaceNumber ;
	pDevRequest->usWLength = 0 ;
	
	EHCIQTransferDesc* pTD1 = EHCIDataHandler_CreateAsyncQTransferDesc() ;
	pTDStart->uiNextpTDPointer = uint64_t(pTD1) ;
	pTD1->uiNextpTDPointer = 1 ;
	pTD1->uiAltpTDPointer = 1 ;
	pTD1->uipTDToken = (1 << 31) | (3 << 10) | (1 << 8) | (1 << 7) ;

	EHCITransaction aTransaction(_pControlQH, pTDStart);
	
	if(!aTransaction.PollWait())
	{
		printf("\n Transaction Failed: ") ;
		DisplayTransactionState(_pControlQH, pTDStart) ;
		_controller.DisplayStats() ;
    aTransaction.Clear();
		return false ;
	}

  aTransaction.Clear();

	return true ;
}

bool EHCIDevice::ClearHaltEndPoint(USBulkDisk* pDisk, bool bIn)
{
	unsigned uiEndPoint = (bIn) ? pDisk->uiEndPointIn : pDisk->uiEndPointOut ;

	// Setup TDs
	EHCIQTransferDesc* pTDStart = EHCIDataHandler_CreateAsyncQTransferDesc() ;

	pTDStart->uiAltpTDPointer = 1 ;
	pTDStart->uipTDToken = (8 << 16) | (3 << 10) | (2 << 8) | (1 << 7) ;

	if(SetupAllocBuffer(pTDStart, sizeof(USBDevRequest)) != EHCIController_SUCCESS)
	{
		KernelDMM::Instance().free((uintptr_t)pTDStart) ;
		return false ;
	}

	USBDevRequest* pDevRequest = (USBDevRequest*)(pTDStart->uiBufferPointer[ 0 ]) ;
	pDevRequest->bRequestType = USB_RECIP_ENDPOINT ;
	pDevRequest->bRequest = 1 ;
	pDevRequest->usWValue = 0 ;
	pDevRequest->usWIndex = (uiEndPoint & 0xF) | ((bIn) ? 0x80 : 0x00) ;
	pDevRequest->usWLength = 0 ;
	
	EHCIQTransferDesc* pTD1 = EHCIDataHandler_CreateAsyncQTransferDesc() ;
	pTDStart->uiNextpTDPointer = uint64_t(pTD1) ;
	pTD1->uiNextpTDPointer = 1 ;
	pTD1->uiAltpTDPointer = 1 ;
	pTD1->uipTDToken = (1 << 31) | (3 << 10) | (1 << 8) | (1 << 7) ;

	EHCITransaction aTransaction(_pControlQH, pTDStart);
	
	if(!aTransaction.PollWait())
	{
		printf("\n Transaction Failed: ") ;
		DisplayTransactionState(_pControlQH, pTDStart) ;
		_controller.DisplayStats() ;
    aTransaction.Clear();
		return false ;
	}

  aTransaction.Clear();

	return true ;
}

extern void _UpdateReadStat(unsigned, bool) ;
bool EHCIDevice::BulkRead(USBulkDisk* pDisk, void* pDataBuf, unsigned uiLen)
{
	_UpdateReadStat(uiLen, true) ;
	if(uiLen == 0)
		return true ;

	unsigned uiMaxLen = pDisk->usInMaxPacketSize * MAX_EHCI_TD_PER_BULK_RW ;
	if(uiLen > uiMaxLen)
	{
		printf("\n Max Bulk Transfer per Frame is %d bytes", uiMaxLen) ;
		return false ;
	}

	int iNoOfTDs = uiLen / EHCI_MAX_BYTES_PER_TD ;
	iNoOfTDs += ((uiLen % EHCI_MAX_BYTES_PER_TD) != 0) ;

	if(iNoOfTDs > MAX_EHCI_TD_PER_BULK_RW)
	{
		printf("\n No. of TDs per Bulk Read/Wrtie exceeded limit %d !!", MAX_EHCI_TD_PER_BULK_RW) ;
		return false ;
	}

  const unsigned uiMaxPacketSize = pDisk->usInMaxPacketSize;

	if(_bFirstBulkRead)
	{
		_bFirstBulkRead = false ;

		for(int i = 0; i < MAX_EHCI_TD_PER_BULK_RW; i++)
			_ppBulkReadTDs[ i ] = EHCIDataHandler_CreateAsyncQTransferDesc() ; 

		_pBulkInEndPt = _controller.CreateDeviceQueueHead(uiMaxPacketSize, pDisk->uiEndPointIn, _devAddr) ;
	}
	else
	{
		int i ;
		for(i = 0; i < MAX_EHCI_TD_PER_BULK_RW; i++)
			memset((void*)(_ppBulkReadTDs[i]), 0, sizeof(EHCIQTransferDesc)) ;

		EHCIDataHandler_CleanAysncQueueHead(_pBulkInEndPt) ;
	}

	int iIndex ;

	unsigned uiYetToRead = uiLen ;
	unsigned uiCurReadLen ;
	for(iIndex = 0; iIndex < iNoOfTDs; iIndex++)
	{
		uiCurReadLen = (uiYetToRead > EHCI_MAX_BYTES_PER_TD) ? EHCI_MAX_BYTES_PER_TD : uiYetToRead ;
		uiYetToRead -= uiCurReadLen ;

		EHCIQTransferDesc* pTD = _ppBulkReadTDs[ iIndex ] ;

		pTD->uiAltpTDPointer = 1 ;
		pTD->uipTDToken = (pDisk->bEndPointInToggle << 31) | (uiCurReadLen << 16) | (3 << 10) | (1 << 8) | (1 << 7) ;
		unsigned toggle = uiCurReadLen / uiMaxPacketSize ;
		if(uiCurReadLen % uiMaxPacketSize)
			toggle++ ;
		if(toggle % 2)
			pDisk->bEndPointInToggle ^= 1 ;

    uintptr_t uiBufAddr = (uintptr_t)(pDisk->pRawAlignedBuffer + (iIndex * EHCI_MAX_BYTES_PER_TD)) ;
		if(SetupBuffer(pTD, uiBufAddr, uiCurReadLen)	!= EHCIController_SUCCESS)
		{
			printf("\n EHCI Transfer Buffer setup failed") ;
			return false ;
		}

		if(iIndex > 0)
			_ppBulkReadTDs[ iIndex - 1 ]->uiNextpTDPointer = (uintptr_t)pTD ;
	}

	_ppBulkReadTDs[ iIndex - 1 ]->uiNextpTDPointer = 1 ;

	EHCITransaction aTransaction(_pBulkInEndPt, _ppBulkReadTDs[ 0 ]);

	if(!aTransaction.PollWait())
	{
		printf("\n Bulk Read Transaction Failed: ") ;
		DisplayTransactionState(_pBulkInEndPt, _ppBulkReadTDs[0]) ;
		_controller.DisplayStats() ;
		return false ;
	}

	EHCIDataHandler_CleanAysncQueueHead(_pBulkInEndPt) ;

	memcpy(pDataBuf, pDisk->pRawAlignedBuffer, uiLen) ;

	_UpdateReadStat(uiLen, false) ;
	return true ;
}

bool EHCIDevice::BulkWrite(USBulkDisk* pDisk, void* pDataBuf, unsigned uiLen)
{
	if(uiLen == 0)
		return true ;

	unsigned uiMaxLen = pDisk->usOutMaxPacketSize * MAX_EHCI_TD_PER_BULK_RW ;
	if(uiLen > uiMaxLen)
	{
		printf("\n Max Bulk Transfer per Frame is %d bytes", uiMaxLen) ;
		return false ;
	}

	int iNoOfTDs = uiLen / EHCI_MAX_BYTES_PER_TD ;
	iNoOfTDs += ((uiLen % EHCI_MAX_BYTES_PER_TD) != 0) ;

	if(iNoOfTDs > MAX_EHCI_TD_PER_BULK_RW)
	{
		printf("\n No. of TDs per Bulk Read/Wrtie exceeded limit %d !!", MAX_EHCI_TD_PER_BULK_RW) ;
		return false ;
	}

  const unsigned uiMaxPacketSize = pDisk->usOutMaxPacketSize;

	if(_bFirstBulkWrite)
	{
		_bFirstBulkWrite = false ;

		for(int i = 0; i < MAX_EHCI_TD_PER_BULK_RW; i++)
			_ppBulkWriteTDs[ i ] = EHCIDataHandler_CreateAsyncQTransferDesc() ; 

		_pBulkOutEndPt = _controller.CreateDeviceQueueHead(uiMaxPacketSize, pDisk->uiEndPointOut, _devAddr) ;
	}
	else
	{
		int i ;
		for(i = 0; i < MAX_EHCI_TD_PER_BULK_RW; i++)
			memset((void*)(_ppBulkWriteTDs[i]), 0, sizeof(EHCIQTransferDesc)) ;

		EHCIDataHandler_CleanAysncQueueHead(_pBulkOutEndPt) ;
	}

	int iIndex ;

	memcpy(pDisk->pRawAlignedBuffer, pDataBuf, uiLen) ;

	unsigned uiYetToWrite = uiLen ;
	unsigned uiCurWriteLen ;

	for(iIndex = 0; iIndex < iNoOfTDs; iIndex++)
	{
		uiCurWriteLen = (uiYetToWrite > EHCI_MAX_BYTES_PER_TD) ? EHCI_MAX_BYTES_PER_TD : uiYetToWrite ;
		uiYetToWrite -= uiCurWriteLen ;

		EHCIQTransferDesc* pTD = _ppBulkWriteTDs[ iIndex ] ;

		pTD->uiAltpTDPointer = 1 ;
		pTD->uipTDToken = (pDisk->bEndPointOutToggle << 31) | (uiCurWriteLen << 16) | (3 << 10) | (1 << 7) ;
		unsigned toggle = uiCurWriteLen / uiMaxPacketSize ;
		if(uiCurWriteLen % uiMaxPacketSize)
			toggle++ ;
		if(toggle % 2)
			pDisk->bEndPointOutToggle ^= 1 ;

    uintptr_t uiBufAddr = (uintptr_t)(pDisk->pRawAlignedBuffer + (iIndex * EHCI_MAX_BYTES_PER_TD)) ;
		if(SetupBuffer(pTD, uiBufAddr, uiCurWriteLen) != EHCIController_SUCCESS)
		{
			printf("\n EHCI Transfer Buffer setup failed") ;
			return false ;
		}

		if(iIndex > 0)
			_ppBulkWriteTDs[ iIndex - 1 ]->uiNextpTDPointer = (uintptr_t)pTD ;
	}

	_ppBulkWriteTDs[ iIndex - 1 ]->uiNextpTDPointer = 1 ;
  
	EHCITransaction aTransaction(_pBulkOutEndPt, _ppBulkWriteTDs[ 0 ]);

	if(!aTransaction.PollWait())
	{
		printf("\n Bulk Write Transaction Failed: ") ;
		DisplayTransactionState(_pBulkOutEndPt, _ppBulkWriteTDs[0]) ;
		_controller.DisplayStats() ;
		return false ;
	}
  
	EHCIDataHandler_CleanAysncQueueHead(_pBulkOutEndPt) ;
  return true;
}

bool EHCIDevice::SetAddress()
{
	_devAddr = USBController::Instance().GetNextDevNum();
	if(_devAddr <= 0)
    throw upan::exception(XLOC, "Invalid Next Dev Addr: %d", _devAddr);

	// Setup TDs
	EHCIQTransferDesc* pTDStart = EHCIDataHandler_CreateAsyncQTransferDesc() ;

	pTDStart->uiAltpTDPointer = 1 ;
	pTDStart->uipTDToken = (8 << 16) | (3 << 10) | (2 << 8) | (1 << 7) ;

	if(SetupAllocBuffer(pTDStart, sizeof(USBDevRequest)) != EHCIController_SUCCESS)
	{
		KernelDMM::Instance().free((uintptr_t)pTDStart) ;
		return EHCIController_FAILURE ;
	}

	USBDevRequest* pDevRequest = (USBDevRequest*)(pTDStart->uiBufferPointer[ 0 ]) ;
	pDevRequest->bRequestType = 0x00 ;
	pDevRequest->bRequest = 5 ;
	pDevRequest->usWValue = (_devAddr & 0xFF) ;
	pDevRequest->usWIndex = 0 ;
	pDevRequest->usWLength = 0 ;
	
	EHCIQTransferDesc* pTD1 = EHCIDataHandler_CreateAsyncQTransferDesc() ;
	pTDStart->uiNextpTDPointer = uint64_t(pTD1) ;
	pTD1->uiNextpTDPointer = 1 ;
	pTD1->uiAltpTDPointer = 1 ;
	pTD1->uipTDToken = (1 << 31) | (3 << 10) | (1 << 8) | (1 << 7) ;

	EHCITransaction aTransaction(_pControlQH, pTDStart);
	
	if(!aTransaction.PollWait())
	{
		printf("\n Transaction Failed: ") ;
		DisplayTransactionState(_pControlQH, pTDStart) ;
		_controller.DisplayStats() ;
    aTransaction.Clear();
		return false;
	}

  aTransaction.Clear();

	_pControlQH->uiEndPointCap_Part1 |= (_devAddr & 0x7F) ;

	if(!_controller.AsyncDoorBell())
	{
		printf("\n Async Door Bell failed while Setting Device Address") ;
		_controller.DisplayStats() ;
    return false;
	}
  return true;
}

bool EHCIDevice::GetDeviceDescriptor(USBStandardDeviceDesc* pDevDesc)
{
	USBDataHandler_InitDevDesc(pDevDesc) ;

  if(!GetDescriptor(0x100, 0, -1, pDevDesc))
    return false;

	byte len = pDevDesc->bLength ;

	if(len >= sizeof(USBStandardDeviceDesc))
		len = sizeof(USBStandardDeviceDesc) ;

  if(!GetDescriptor(0x100, 0, len, pDevDesc))
    return false;

  return true;
}

bool EHCIDevice::GetDescriptor(unsigned short usDescValue, unsigned short usIndex, int iLen, void* pDestDesc)
{
	// Setup TDs
	EHCIQTransferDesc* pTDStart = EHCIDataHandler_CreateAsyncQTransferDesc() ;

	pTDStart->uiAltpTDPointer = 1 ;
	pTDStart->uipTDToken = (8 << 16) | (3 << 10) | (2 << 8) | (1 << 7) ;

	if(SetupAllocBuffer(pTDStart, sizeof(USBDevRequest)) != EHCIController_SUCCESS)
	{
		KernelDMM::Instance().free((uintptr_t)pTDStart) ;
		return false;
	}

	if(iLen < 0)
		iLen = DEF_DESC_LEN ;

	USBDevRequest* pDevRequest = (USBDevRequest*)(pTDStart->uiBufferPointer[ 0 ]) ;
	pDevRequest->bRequestType = 0x80 ;
	pDevRequest->bRequest = 6 ;
	pDevRequest->usWValue = usDescValue ;
	pDevRequest->usWIndex = usIndex ;
	pDevRequest->usWLength = iLen ;

	EHCIQTransferDesc* pTD1 = EHCIDataHandler_CreateAsyncQTransferDesc() ;
	pTDStart->uiNextpTDPointer = uint64_t(pTD1) ;
	pTD1->uiAltpTDPointer = 1 ;
	pTD1->uipTDToken = (1 << 31) | (iLen << 16) | (3 << 10) | (1 << 8) | (1 << 7) ;
	
	if(SetupAllocBuffer(pTD1, iLen) != EHCIController_SUCCESS)
	{
		KernelDMM::Instance().free((uintptr_t)pTDStart) ;
		KernelDMM::Instance().free((uintptr_t)pTD1) ;
		return false;
	}

	// It is important to store the data buffer address now for later read as the data buffer address
	// in TD will be incremented by Host Controller with the number of bytes read
	uintptr_t uiDataBuffer = pTD1->uiBufferPointer[ 0 ] ;

	EHCIQTransferDesc* pTD2 = EHCIDataHandler_CreateAsyncQTransferDesc() ;
	pTD1->uiNextpTDPointer = uint64_t(pTD2) ;
	pTD2->uiNextpTDPointer = 1 ;
	pTD2->uiAltpTDPointer = 1 ;
	pTD2->uipTDToken = (3 << 10) | (1 << 7) ;

	EHCITransaction aTransaction(_pControlQH, pTDStart);

	if(!aTransaction.PollWait())
	{
		printf("\n Transaction Failed: ") ;
		DisplayTransactionState(_pControlQH, pTDStart) ;
		_controller.DisplayStats() ;
    aTransaction.Clear();
		return false;
	}

  memcpy(pDestDesc, (const void*)uiDataBuffer, iLen);

  aTransaction.Clear();

	return true;
}

bool EHCIDevice::GetConfigValue(byte& bConfigValue)
{
	EHCIQTransferDesc* pTDStart = EHCIDataHandler_CreateAsyncQTransferDesc() ;

	pTDStart->uiAltpTDPointer = 1 ;
	pTDStart->uipTDToken = (8 << 16) | (3 << 10) | (2 << 8) | (1 << 7) ;

	if(SetupAllocBuffer(pTDStart, sizeof(USBDevRequest)) != EHCIController_SUCCESS)
	{
		KernelDMM::Instance().free((uintptr_t)pTDStart) ;
		return false;
	}

	USBDevRequest* pDevRequest = (USBDevRequest*)(pTDStart->uiBufferPointer[ 0 ]) ;
	pDevRequest->bRequestType = 0x80 ;
	pDevRequest->bRequest = 8 ;
	pDevRequest->usWValue = 0 ;
	pDevRequest->usWIndex = 0 ;
	pDevRequest->usWLength = 1 ;
	
	EHCIQTransferDesc* pTD1 = EHCIDataHandler_CreateAsyncQTransferDesc() ;
	pTDStart->uiNextpTDPointer = uint64_t(pTD1) ;
	pTD1->uiAltpTDPointer = 1 ;
	pTD1->uipTDToken = (1 << 31) | (1 << 16) | (3 << 10) | (1 << 8) | (1 << 7) ;
	
	if(SetupAllocBuffer(pTD1, 1) != EHCIController_SUCCESS)
	{
		KernelDMM::Instance().free((uintptr_t)pTDStart) ;
		KernelDMM::Instance().free((uintptr_t)pTD1) ;
		return false;
	}

	EHCIQTransferDesc* pTD2 = EHCIDataHandler_CreateAsyncQTransferDesc() ;
	pTD1->uiNextpTDPointer = uint64_t(pTD2) ;
	pTD2->uiNextpTDPointer = 1 ;
	pTD2->uiAltpTDPointer = 1 ;
	pTD2->uipTDToken = (3 << 10) | (1 << 7) ;

	uintptr_t uiDataBuffer = pTD1->uiBufferPointer[ 0 ] ;

	EHCITransaction aTransaction(_pControlQH, pTDStart);

	if(!aTransaction.PollWait())
	{
		printf("\n Transaction Failed: ") ;
		DisplayTransactionState(_pControlQH, pTDStart) ;
		_controller.DisplayStats() ;
    aTransaction.Clear();
		return false;
	}
	
	bConfigValue = *reinterpret_cast<char*>(uiDataBuffer);
		
  aTransaction.Clear();

	return true;
}

byte EHCIDevice::SetupBuffer(EHCIQTransferDesc* pTD, uintptr_t uiAddress, unsigned uiSize)
{
	unsigned uiFirstPagePart = PAGE_SIZE - (uiAddress % PAGE_SIZE) ;

	pTD->uiBufferPointer[ 0 ] = uiAddress ;

	if(uiFirstPagePart >= uiSize)
		return EHCIController_SUCCESS ;

	pTD->uiBufferPointer[ 1 ] = pTD->uiBufferPointer[ 0 ] + uiFirstPagePart ;

	uiSize -= uiFirstPagePart ;

	unsigned i ;
	for(i = 2; uiSize > PAGE_SIZE && i < 5; i++, uiSize -= PAGE_SIZE)
	{
		pTD->uiBufferPointer[ i ] = pTD->uiBufferPointer[ i - 1 ] + PAGE_SIZE ;
	}

	if(uiSize > PAGE_SIZE)
	{
		printf("\n EHCI TD Buffer Allocation Math is incorrect in page boundary calculation !") ;
		KernelDMM::Instance().free(uiAddress) ;
		return EHCIController_FAILURE ;
	}

	return EHCIController_SUCCESS ;
}

byte EHCIDevice::SetupAllocBuffer(EHCIQTransferDesc* pTD, unsigned uiSize)
{
	static const unsigned MAX_LEN = PAGE_SIZE * 4 ;

	if(uiSize > MAX_LEN)
	{
		printf("\n EHCI QTD Buffer Size: %d greater than Limit (PAGE_SIZE * 4): %d", uiSize, MAX_LEN) ;
		return EHCIController_FAILURE ;
	}

	unsigned uiAddress = KernelDMM::Instance().allocate(uiSize) ;

	return SetupBuffer(pTD, uiAddress, uiSize) ;
}

void EHCIDevice::DisplayTransactionState(EHCIQueueHead* pQH, EHCIQTransferDesc* pTDStart)
{
	printf("\n QH Details:- ECap1: %x, ECap2: %x", pQH->uiEndPointCap_Part1, pQH->uiEndPointCap_Part2) ;
	printf("\n QTD Token: %x, Current: %x, Next: %x", pQH->uipQHToken, pQH->uiCurrrentpTDPointer, pQH->uiNextpTDPointer) ;
	printf("\n Alt_NAK: %x, BufferPtr0: %x", pQH->uiAltpTDPointer_NAKCnt, pQH->uiBufferPointer[ 0 ]) ;

	EHCIQTransferDesc* pTDCur = pTDStart ;
	unsigned i = 1 ;
	for(;(uintptr_t)pTDCur > 1;)
	{
		printf("\n TD %d Token: %x", i++, pTDCur->uipTDToken) ;
		pTDCur = (EHCIQTransferDesc*)pTDCur->uiNextpTDPointer ;
	}
}

