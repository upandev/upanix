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

#include <MemUtil.h>
#include <UHCIController.h>
#include <ProcessManager.h>
#include <UHCIDevice.h>
#include <USBDataHandler.h>
#include <USBStructures.h>
#include <DMM.h>
#include <PortCom.h>
#include <UHCIManager.h>

UHCIDevice::UHCIDevice(UHCIController& c, unsigned portAddr) 
  : _controller(c), _bFirstBulkRead(true), _bFirstBulkWrite(true), _portAddr(portAddr)
{
	SetAddress();

	GetDeviceDescriptor(&_deviceDesc);
  _deviceDesc.DebugPrint();

	byte bConfigValue = 0;
	GetConfiguration(&bConfigValue);

  CheckConfiguration(&bConfigValue);

  GetConfigDescriptor(&_pArrConfigDesc);

  GetStringDescriptorZero(&_pStrDescZero);

	_eControllerType = UHCI_CONTROLLER ;

	SetLangId();
	
  GetDeviceStringDesc(_manufacturer, _deviceDesc.indexManufacturer);
  GetDeviceStringDesc(_product, _deviceDesc.indexProduct);
  GetDeviceStringDesc(_serialNum, _deviceDesc.indexSerialNum);
  PrintDeviceStringDetails();
}

void UHCIDevice::GetDeviceStringDesc(upan::string& desc, int descIndex)
{
  desc = "Unknown";
  if(_usLangID == 0)
    return;

	unsigned short usDescValue = (0x3 << 8);
	char szPart[ 8 ];

  GetDescriptor(usDescValue | descIndex, _usLangID, -1, szPart);

  int iLen = ((USBStringDescZero*)&szPart)->bLength;
  printf("\n String Index: %u, String Desc Size: %d", descIndex, iLen);
  if(iLen == 0)
    return;

  char* strDesc = new char[iLen];
  GetDescriptor(usDescValue | descIndex, _usLangID, iLen, strDesc);

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

byte UHCIDevice::GetMaxLun()
{
	unsigned uiFrameNumber = _controller.GetNextFreeFrame();

	UHCIQueueHead* pQH = UHCIQueueHead::Create();
	pQH->SetQHAttribute(UHCI_ATTR_QH_HEAD_LINK_TERMINATE, 1);

	UHCITransferDesc* pTD1 = UHCITransferDesc::Create();

	//pTD1->SetTDAttribute(UHCI_ATTR_TD_CONTROL_SPD, 1);
	//pTD1->SetTDAttribute(UHCI_ATTR_TD_CONTROL_IOC, 1);
	pTD1->SetTDAttribute(UHCI_ATTR_TD_CONTROL_ERR_LEVEL, TD_CONTROL_ERR3);
	pTD1->SetTDAttribute(UHCI_ATTR_TD_STATUS_ACTIVE, 1);
	pTD1->SetTDAttribute(UHCI_ATTR_TD_DEVICE_ADDR, _devAddr);

	pTD1->SetTDAttribute(UHCI_ATTR_TD_MAXLEN, 7);
	pTD1->SetTDAttribute(UHCI_ATTR_TD_PID, TOKEN_PID_SETUP);
	
	USBDevRequest* pDevRequest = (USBDevRequest*)DMM_AllocateForKernel(sizeof(USBDevRequest), 16);
	pDevRequest->bRequestType = USB_TYPE_CLASS | USB_RECIP_INTERFACE | USB_DIR_IN ;
	pDevRequest->bRequest = 0xFE ;
	pDevRequest->usWValue = 0 ;
	pDevRequest->usWIndex = _bInterfaceNumber ;
	pDevRequest->usWLength = 1 ;

	pTD1->SetTDAttribute(UHCI_ATTR_BUF_PTR, (unsigned)pDevRequest);
	
	pQH->SetQHAttribute(UHCI_ATTR_QH_TO_TD_ELEM_LINK, (unsigned)pTD1);
	
	UHCITransferDesc* pTD2 = UHCITransferDesc::Create();
	pTD1->SetTDAttribute(UHCI_ATTR_TD_VERTICAL_LINK, (unsigned)pTD2);
	
	byte* pBufPtr = (byte*)DMM_AllocateForKernel(1, 16);

	//pTD2->SetTDAttribute(UHCI_ATTR_TD_CONTROL_SPD, 1);
	pTD2->SetTDAttribute(UHCI_ATTR_BUF_PTR, (unsigned)pBufPtr);
	pTD2->SetTDAttribute(UHCI_ATTR_TD_CONTROL_ERR_LEVEL, TD_CONTROL_ERR3);
	pTD2->SetTDAttribute(UHCI_ATTR_TD_STATUS_ACTIVE, 1);
	pTD2->SetTDAttribute(UHCI_ATTR_TD_DATA_TOGGLE, 1);
	pTD2->SetTDAttribute(UHCI_ATTR_TD_DEVICE_ADDR, _devAddr);
	
	pTD2->SetTDAttribute(UHCI_ATTR_TD_MAXLEN, 0);
	pTD2->SetTDAttribute(UHCI_ATTR_TD_PID, TOKEN_PID_IN);
	
	UHCITransferDesc* pTD3 = UHCITransferDesc::Create();
	pTD2->SetTDAttribute(UHCI_ATTR_TD_VERTICAL_LINK, (unsigned)pTD3);
	
	//pTD3->SetTDAttribute(UHCI_ATTR_TD_CONTROL_SPD, 1);
	pTD3->SetTDAttribute(UHCI_ATTR_TD_CONTROL_IOC, 1);
	pTD3->SetTDAttribute(UHCI_ATTR_TD_CONTROL_ERR_LEVEL, TD_CONTROL_ERR3);
	pTD3->SetTDAttribute(UHCI_ATTR_TD_STATUS_ACTIVE, 1);
	pTD3->SetTDAttribute(UHCI_ATTR_TD_DEVICE_ADDR, _devAddr);
	
	pTD3->SetTDAttribute(UHCI_ATTR_TD_MAXLEN, TD_MAXLEN_BLK);
	pTD3->SetTDAttribute(UHCI_ATTR_TD_PID, TOKEN_PID_OUT);
	pTD3->SetTDAttribute(UHCI_ATTR_TD_TERMINATE, 1);

  byte bLUN = 0;
	if(UpanixMain_IsKernelDebugOn())
	{
		printf("\n QH Element Pointer = %x", pQH->ElementLinkPointer());
		printf("\n TD1 Status = %x/%x/%x", (unsigned)pTD1, pTD1->ControlnStatus(), pTD1->Token());
		printf("\n TD2 Status = %x/%x/%x", (unsigned)pTD2, pTD2->ControlnStatus(), pTD2->Token());
		printf("\n TD3 Status = %x/%x/%x", (unsigned)pTD3, pTD3->ControlnStatus(), pTD3->Token());

		printf("\nCurrent Frame Number = %d", PortCom_ReceiveWord(_controller.IOBase() + FRNUM_REG));
		printf("\nFrame Number = %d", uiFrameNumber);

		_controller.SetFrameListEntry(uiFrameNumber, ((unsigned)pQH + GLOBAL_DATA_SEGMENT_BASE) | 0x2, true, true);
		printf("\nFrame Reg = %x", _controller.GetFrameListEntry(uiFrameNumber));

		ProcessManager::Instance().Sleep(5000);
		printf("\n QH Element Pointer = %x", pQH->ElementLinkPointer());
		printf("\n QH Head Pointer = %x", pQH->HeadLinkPointer());
		printf("\n TD1 Status = %x/%x/%x\n TD2 Status = %x/%x/%x\n",
				(unsigned)pTD1, pTD1->ControlnStatus(), pTD1->Token(),
				(unsigned)pTD2, pTD2->ControlnStatus(), pTD2->Token());
		printf("\nFrame Reg = %x", _controller.GetFrameListEntry(uiFrameNumber));

		printf("\n Status = %x", PortCom_ReceiveWord(_controller.IOBase() + USBSTS_REG));
		printf("\n Port Status = %x", PortCom_ReceiveWord(_portAddr + 2));
		printf("\nFrame Number = %d", PortCom_ReceiveWord(_controller.IOBase() + FRNUM_REG));

		bLUN = pBufPtr[0] ;
		printf("\n\n Scheduling Frame Clean %d: %d", uiFrameNumber, _controller.CleanFrame(uiFrameNumber));
		ProcessManager::Instance().Sleep(2000);
	}
	else
	{
		_controller.SetFrameListEntry(uiFrameNumber, ((unsigned)pQH + GLOBAL_DATA_SEGMENT_BASE) | 0x2, true, true);

		if(!pQH->PollWait())
		{
			printf("\nFrame Number = %d", uiFrameNumber);
			printf("\n QH Element Pointer = %x", pQH->ElementLinkPointer());
			printf("\n QH Head Pointer = %x", pQH->HeadLinkPointer());
			printf("\n TD1 Status = %x/%x/%x", (unsigned)pTD1, pTD1->ControlnStatus(), pTD1->Token());
			printf("\n TD2 Status = %x/%x/%x", (unsigned)pTD2, pTD2->ControlnStatus(), pTD2->Token());
			printf("\n TD3 Status = %x/%x/%x", (unsigned)pTD3, pTD3->ControlnStatus(), pTD3->Token());
			printf("\n Status = %x", PortCom_ReceiveWord(_controller.IOBase() + USBSTS_REG));
			printf("\n Port Status = %x", PortCom_ReceiveWord(_portAddr + 2));
			printf("\nFrame Number = %d", PortCom_ReceiveWord(_controller.IOBase() + FRNUM_REG));
		}
		else
			bLUN = pBufPtr[0] ;

		_controller.CleanFrame(uiFrameNumber);
	}
  return bLUN;
}

bool UHCIDevice::CommandReset()
{
	unsigned uiFrameNumber = _controller.GetNextFreeFrame();

	UHCIQueueHead* pQH = UHCIQueueHead::Create();
	pQH->SetQHAttribute(UHCI_ATTR_QH_HEAD_LINK_TERMINATE, 1);

	UHCITransferDesc* pTD1 = UHCITransferDesc::Create();

	//pTD1->SetTDAttribute(UHCI_ATTR_TD_CONTROL_SPD, 1);
	//pTD1->SetTDAttribute(UHCI_ATTR_TD_CONTROL_IOC, 1);
	pTD1->SetTDAttribute(UHCI_ATTR_TD_CONTROL_ERR_LEVEL, TD_CONTROL_ERR3);
	pTD1->SetTDAttribute(UHCI_ATTR_TD_STATUS_ACTIVE, 1);
	pTD1->SetTDAttribute(UHCI_ATTR_TD_DEVICE_ADDR, _devAddr);

	pTD1->SetTDAttribute(UHCI_ATTR_TD_MAXLEN, 7);
	pTD1->SetTDAttribute(UHCI_ATTR_TD_PID, TOKEN_PID_SETUP);
	
	USBDevRequest* pDevRequest = (USBDevRequest*)DMM_AllocateForKernel(sizeof(USBDevRequest), 16);
	pDevRequest->bRequestType = USB_TYPE_CLASS | USB_RECIP_INTERFACE ;
	pDevRequest->bRequest = 0xFF ;
	pDevRequest->usWValue = 0 ;
	pDevRequest->usWIndex = _bInterfaceNumber ;
	pDevRequest->usWLength = 0 ;

	pTD1->SetTDAttribute(UHCI_ATTR_BUF_PTR, (unsigned)pDevRequest);
	
	pQH->SetQHAttribute(UHCI_ATTR_QH_TO_TD_ELEM_LINK, (unsigned)pTD1);
	
	UHCITransferDesc* pTD2 = UHCITransferDesc::Create();
	pTD1->SetTDAttribute(UHCI_ATTR_TD_VERTICAL_LINK, (unsigned)pTD2);

	pTD2->SetTDAttribute(UHCI_ATTR_TD_TERMINATE, 1);
	
	//pTD2->SetTDAttribute(UHCI_ATTR_TD_CONTROL_SPD, 1);
	pTD2->SetTDAttribute(UHCI_ATTR_TD_CONTROL_IOC, 1);
	pTD2->SetTDAttribute(UHCI_ATTR_TD_CONTROL_ERR_LEVEL, TD_CONTROL_ERR3);
	pTD2->SetTDAttribute(UHCI_ATTR_TD_STATUS_ACTIVE, 1);
	pTD2->SetTDAttribute(UHCI_ATTR_TD_DATA_TOGGLE, 1);
	pTD2->SetTDAttribute(UHCI_ATTR_TD_DEVICE_ADDR, _devAddr);
	
	pTD2->SetTDAttribute(UHCI_ATTR_TD_MAXLEN, TD_MAXLEN_BLK);
	pTD2->SetTDAttribute(UHCI_ATTR_TD_PID, TOKEN_PID_OUT);

	if(UpanixMain_IsKernelDebugOn())
	{
		printf("\n QH Element Pointer = %x", pQH->ElementLinkPointer());
		printf("\n TD1 Status = %x/%x/%x\n TD2 Status = %x/%x/%x\n",
				(unsigned)pTD1, pTD1->ControlnStatus(), pTD1->Token(),
				(unsigned)pTD2, pTD2->ControlnStatus(), pTD2->Token());

		printf("\nCurrent Frame Number = %d", PortCom_ReceiveWord(_controller.IOBase() + FRNUM_REG));
		printf("\nFrame Number = %d", uiFrameNumber);

		_controller.SetFrameListEntry(uiFrameNumber, ((unsigned)pQH + GLOBAL_DATA_SEGMENT_BASE) | 0x2, true, true);
		printf("\nFrame Reg = %x", _controller.GetFrameListEntry(uiFrameNumber));

		ProcessManager::Instance().Sleep(5000);
		printf("\n QH Element Pointer = %x", pQH->ElementLinkPointer());
		printf("\n QH Head Pointer = %x", pQH->HeadLinkPointer());
		printf("\n TD1 Status = %x/%x/%x\n TD2 Status = %x/%x/%x\n",
				(unsigned)pTD1, pTD1->ControlnStatus(), pTD1->Token(),
				(unsigned)pTD2, pTD2->ControlnStatus(), pTD2->Token());
		printf("\nFrame Reg = %x", _controller.GetFrameListEntry(uiFrameNumber));

		printf("\n Status = %x", PortCom_ReceiveWord(_controller.IOBase() + USBSTS_REG));
		printf("\n Port Status = %x", PortCom_ReceiveWord(_portAddr + 2));
		printf("\nFrame Number = %d", PortCom_ReceiveWord(_controller.IOBase() + FRNUM_REG));

		printf("\n\n Scheduling Frame Clean %d: %d", uiFrameNumber, _controller.CleanFrame(uiFrameNumber));
		ProcessManager::Instance().Sleep(2000);
	}
	else
	{
		_controller.SetFrameListEntry(uiFrameNumber, ((unsigned)pQH + GLOBAL_DATA_SEGMENT_BASE) | 0x2, true, true);

		if(!pQH->PollWait())
		{
			printf("\nFrame Number = %d", uiFrameNumber);
			printf("\n QH Element Pointer = %x", pQH->ElementLinkPointer());
			printf("\n QH Head Pointer = %x", pQH->HeadLinkPointer());
			printf("\n TD1 Status = %x/%x/%x\n TD2 Status = %x/%x/%x\n",
					(unsigned)pTD1, pTD1->ControlnStatus(), pTD1->Token(),
					(unsigned)pTD2, pTD2->ControlnStatus(), pTD2->Token());
			printf("\n Status = %x", PortCom_ReceiveWord(_controller.IOBase() + USBSTS_REG));
			printf("\n Port Status = %x", PortCom_ReceiveWord(_portAddr + 2));
			printf("\nFrame Number = %d", PortCom_ReceiveWord(_controller.IOBase() + FRNUM_REG));
		}

		_controller.CleanFrame(uiFrameNumber);
	}

	return true ;
}

bool UHCIDevice::ClearHaltEndPoint(USBulkDisk* pDisk, bool bIn)
{
	unsigned uiEndPoint = (bIn) ? pDisk->uiEndPointIn : pDisk->uiEndPointOut ;

	unsigned uiFrameNumber = _controller.GetNextFreeFrame();

	UHCIQueueHead* pQH = UHCIQueueHead::Create();
	pQH->SetQHAttribute(UHCI_ATTR_QH_HEAD_LINK_TERMINATE, 1);

	UHCITransferDesc* pTD1 = UHCITransferDesc::Create();

	//pTD1->SetTDAttribute(UHCI_ATTR_TD_CONTROL_SPD, 1);
	//pTD1->SetTDAttribute(UHCI_ATTR_TD_CONTROL_IOC, 1);
	pTD1->SetTDAttribute(UHCI_ATTR_TD_CONTROL_ERR_LEVEL, TD_CONTROL_ERR3);
	pTD1->SetTDAttribute(UHCI_ATTR_TD_STATUS_ACTIVE, 1);
	pTD1->SetTDAttribute(UHCI_ATTR_TD_DEVICE_ADDR, _devAddr);

	pTD1->SetTDAttribute(UHCI_ATTR_TD_MAXLEN, 7);
	pTD1->SetTDAttribute(UHCI_ATTR_TD_PID, TOKEN_PID_SETUP);
	
	USBDevRequest* pDevRequest = (USBDevRequest*)DMM_AllocateForKernel(sizeof(USBDevRequest), 16);
	pDevRequest->bRequestType = USB_RECIP_ENDPOINT ;
	pDevRequest->bRequest = 1 ;
	pDevRequest->usWValue = 0 ;
	pDevRequest->usWIndex = (uiEndPoint & 0xF) | ((bIn) ? 0x80 : 0x00);
	pDevRequest->usWLength = 0 ;

	pTD1->SetTDAttribute(UHCI_ATTR_BUF_PTR, (unsigned)pDevRequest);
	
	pQH->SetQHAttribute(UHCI_ATTR_QH_TO_TD_ELEM_LINK, (unsigned)pTD1);
	
	UHCITransferDesc* pTD2 = UHCITransferDesc::Create();
	pTD1->SetTDAttribute(UHCI_ATTR_TD_VERTICAL_LINK, (unsigned)pTD2);

	pTD2->SetTDAttribute(UHCI_ATTR_TD_TERMINATE, 1);
	
	//pTD3->SetTDAttribute(UHCI_ATTR_TD_CONTROL_SPD, 1);
	pTD2->SetTDAttribute(UHCI_ATTR_TD_CONTROL_IOC, 1);
	pTD2->SetTDAttribute(UHCI_ATTR_TD_CONTROL_ERR_LEVEL, TD_CONTROL_ERR3);
	pTD2->SetTDAttribute(UHCI_ATTR_TD_STATUS_ACTIVE, 1);
	pTD2->SetTDAttribute(UHCI_ATTR_TD_DATA_TOGGLE, 1);
	pTD2->SetTDAttribute(UHCI_ATTR_TD_DEVICE_ADDR, _devAddr);
	
	pTD2->SetTDAttribute(UHCI_ATTR_TD_MAXLEN, TD_MAXLEN_BLK);
	pTD2->SetTDAttribute(UHCI_ATTR_TD_PID, (bIn) ? TOKEN_PID_IN : TOKEN_PID_OUT);

	if(UpanixMain_IsKernelDebugOn())
	{
		printf("\n QH Element Pointer = %x", pQH->ElementLinkPointer());
		printf("\n TD1 Status = %x/%x/%x\n TD2 Status = %x/%x/%x\n",
				(unsigned)pTD1, pTD1->ControlnStatus(), pTD1->Token(),
				(unsigned)pTD2, pTD2->ControlnStatus(), pTD2->Token());

		printf("\nCurrent Frame Number = %d", PortCom_ReceiveWord(_controller.IOBase() + FRNUM_REG));
		printf("\nFrame Number = %d", uiFrameNumber);

		_controller.SetFrameListEntry(uiFrameNumber, ((unsigned)pQH + GLOBAL_DATA_SEGMENT_BASE) | 0x2, true, true);
		printf("\nFrame Reg = %x", _controller.GetFrameListEntry(uiFrameNumber));

		ProcessManager::Instance().Sleep(5000);
		printf("\n QH Element Pointer = %x", pQH->ElementLinkPointer());
		printf("\n QH Head Pointer = %x", pQH->HeadLinkPointer());
		printf("\n TD1 Status = %x/%x/%x\n TD2 Status = %x/%x/%x\n",
				(unsigned)pTD1, pTD1->ControlnStatus(), pTD1->Token(),
				(unsigned)pTD2, pTD2->ControlnStatus(), pTD2->Token());
		printf("\nFrame Reg = %x", _controller.GetFrameListEntry(uiFrameNumber));

		printf("\n Status = %x", PortCom_ReceiveWord(_controller.IOBase() + USBSTS_REG));
		printf("\n Port Status = %x", PortCom_ReceiveWord(_portAddr));
		printf("\nFrame Number = %d", PortCom_ReceiveWord(_controller.IOBase() + FRNUM_REG));

		printf("\n\n Scheduling Frame Clean %d: %d", uiFrameNumber, _controller.CleanFrame(uiFrameNumber));
		ProcessManager::Instance().Sleep(2000);
	}
	else
	{
		_controller.SetFrameListEntry(uiFrameNumber, ((unsigned)pQH + GLOBAL_DATA_SEGMENT_BASE) | 0x2, true, true);

		if(!pQH->PollWait())
		{
			printf("\nFrame Number = %d", uiFrameNumber);
			printf("\n QH Element Pointer = %x", pQH->ElementLinkPointer());
			printf("\n QH Head Pointer = %x", pQH->HeadLinkPointer());
			printf("\n TD1 Status = %x/%x/%x\n TD2 Status = %x/%x/%x\n",
					(unsigned)pTD1, pTD1->ControlnStatus(), pTD1->Token(),
					(unsigned)pTD2, pTD2->ControlnStatus(), pTD2->Token());
			printf("\n Status = %x", PortCom_ReceiveWord(_controller.IOBase() + USBSTS_REG));
			printf("\n Port Status = %x", PortCom_ReceiveWord(_portAddr));
			printf("\nFrame Number = %d", PortCom_ReceiveWord(_controller.IOBase() + FRNUM_REG));
		}

		if(bIn) pDisk->bEndPointInToggle = 0 ;
		else pDisk->bEndPointOutToggle = 0 ;

		_controller.CleanFrame(uiFrameNumber);
	}

	return true ;
}

bool UHCIDevice::BulkRead(USBulkDisk* pDisk, void* pDataBuf, unsigned uiLen)
{
	if(uiLen == 0)
		return true ;

	unsigned uiMaxLen = pDisk->usInMaxPacketSize * MAX_UHCI_TD_PER_BULK_RW ;
	if(uiLen > uiMaxLen)
	{
		printf("\n Max Bulk Transfer per Frame is %d bytes", uiMaxLen);
		return false ;
	}

	unsigned uiMaxPacketSize = pDisk->usInMaxPacketSize ;
	int iNoOfTDs = uiLen / uiMaxPacketSize ;
	iNoOfTDs += ((uiLen % uiMaxPacketSize) != 0);

	if(iNoOfTDs > MAX_UHCI_TD_PER_BULK_RW)
	{
		printf("\n No. of TDs per Bulk Read/Wrtie exceeded limit %d !!", MAX_UHCI_TD_PER_BULK_RW);
		return false ;
	}

	if(_bFirstBulkRead)
	{
		_bFirstBulkRead = false ;

		int i ;
		for(i = 0; i < MAX_UHCI_TD_PER_BULK_RW; i++)
			_ppBulkReadTDs[ i ] = UHCITransferDesc::Create(); 
	}
	else
	{
		int i ;
		for(i = 0; i < MAX_UHCI_TD_PER_BULK_RW; i++)
			_ppBulkReadTDs[ i ]->Clear();
	}

	int iIndex ;

	unsigned uiFrameNumber = _controller.GetNextFreeFrame();
	UHCIQueueHead* pQH = UHCIQueueHead::Create();
	pQH->SetQHAttribute(UHCI_ATTR_QH_HEAD_LINK_TERMINATE, 1);

	unsigned uiYetToRead = uiLen ;
	unsigned uiCurReadLen ;

	for(iIndex = 0; iIndex < iNoOfTDs; iIndex++)
	{
		uiCurReadLen = (uiYetToRead > uiMaxPacketSize) ? uiMaxPacketSize : uiYetToRead ;
		uiYetToRead -= uiCurReadLen ;

		UHCITransferDesc* pTD = _ppBulkReadTDs[ iIndex ] ;

		//pTD->SetTDAttribute(UHCI_ATTR_TD_CONTROL_SPD, 1);
		pTD->SetTDAttribute(UHCI_ATTR_TD_CONTROL_ERR_LEVEL, TD_CONTROL_ERR3);
		pTD->SetTDAttribute(UHCI_ATTR_TD_STATUS_ACTIVE, 1);
		pTD->SetTDAttribute(UHCI_ATTR_TD_DEVICE_ADDR, _devAddr);
		pTD->SetTDAttribute(UHCI_ATTR_TD_ENDPT_ADDR, pDisk->uiEndPointIn);
		pTD->SetTDAttribute(UHCI_ATTR_TD_DATA_TOGGLE, pDisk->bEndPointInToggle);
		pDisk->bEndPointInToggle ^= 1 ;

		pTD->SetTDAttribute(UHCI_ATTR_TD_MAXLEN, uiCurReadLen - 1);
		pTD->SetTDAttribute(UHCI_ATTR_TD_PID, TOKEN_PID_IN);
		
		pTD->SetTDAttribute(UHCI_ATTR_BUF_PTR, (unsigned)(pDisk->pRawAlignedBuffer + (iIndex * uiMaxPacketSize)));

		if(iIndex > 0)
			_ppBulkReadTDs[ iIndex - 1 ]->SetTDAttribute(UHCI_ATTR_TD_VERTICAL_LINK, (unsigned)pTD);
	}

	_ppBulkReadTDs[ iIndex - 1 ]->SetTDAttribute(UHCI_ATTR_TD_TERMINATE, 1);
	_ppBulkReadTDs[ iIndex - 1 ]->SetTDAttribute(UHCI_ATTR_TD_CONTROL_IOC, 1);
	pQH->SetQHAttribute(UHCI_ATTR_QH_TO_TD_ELEM_LINK, (unsigned)(_ppBulkReadTDs[ 0 ]));

	_controller.SetFrameListEntry(uiFrameNumber, ((unsigned)pQH + GLOBAL_DATA_SEGMENT_BASE) | 0x2, false, false);

	if(!pQH->PollWait())
	{
		printf("\nFrame Number = %d", uiFrameNumber);
		printf("\n QH Element Pointer = %x", pQH->ElementLinkPointer());
		printf("\n QH Head Pointer = %x", pQH->HeadLinkPointer());

		for(iIndex = 0; iIndex < iNoOfTDs; iIndex++)
      printf("\n TD%d Status = %x/%x/%x", iIndex+1, (unsigned)(_ppBulkReadTDs[ iIndex ]), _ppBulkReadTDs[ iIndex ]->ControlnStatus(), _ppBulkReadTDs[ iIndex ]->Token());

     printf("\n Status = %x", PortCom_ReceiveWord(_controller.IOBase() + USBSTS_REG));
     printf("\n Port Status = %x", PortCom_ReceiveWord(_portAddr));
     printf("\nFrame Number = %d", PortCom_ReceiveWord(_controller.IOBase() + FRNUM_REG));
   }

   memcpy(pDataBuf, pDisk->pRawAlignedBuffer, uiLen);
   _controller.CleanFrame(uiFrameNumber);

   return true ;
}

bool UHCIDevice::BulkWrite(USBulkDisk* pDisk, void* pDataBuf, unsigned uiLen)
{
  if(uiLen == 0)
    return true ;

  unsigned uiMaxLen = pDisk->usOutMaxPacketSize * MAX_UHCI_TD_PER_BULK_RW ;
  if(uiLen > uiMaxLen)
  {
     printf("\n Max Bulk Transfer per Frame is %d bytes", uiMaxLen);
     return false ;
  }

  unsigned uiMaxPacketSize = pDisk->usOutMaxPacketSize ;
  int iNoOfTDs = uiLen / uiMaxPacketSize ;
  iNoOfTDs += ((uiLen % uiMaxPacketSize) != 0);

  if(iNoOfTDs > MAX_UHCI_TD_PER_BULK_RW)
  {
     printf("\n No. of TDs per Bulk Read/Wrtie exceeded limit %d !!", MAX_UHCI_TD_PER_BULK_RW);
     return false ;
  }

  if(_bFirstBulkWrite)
  {
    _bFirstBulkWrite = false ;
    for(int i = 0; i < MAX_UHCI_TD_PER_BULK_RW; i++)
      _ppBulkWriteTDs[ i ] = UHCITransferDesc::Create(); 
  }
  else
  {
    for(int i = 0; i < MAX_UHCI_TD_PER_BULK_RW; i++)
      _ppBulkWriteTDs[ i ]->Clear();
  }

  int iIndex ;

  unsigned uiFrameNumber = _controller.GetNextFreeFrame();
  UHCIQueueHead* pQH = UHCIQueueHead::Create();

  pQH->SetQHAttribute(UHCI_ATTR_QH_HEAD_LINK_TERMINATE, 1);

  memcpy(pDisk->pRawAlignedBuffer, pDataBuf, uiLen);

  unsigned uiYetToWrite = uiLen ;
  unsigned uiCurWriteLen ;

  for(iIndex = 0; iIndex < iNoOfTDs; iIndex++)
	{
		uiCurWriteLen = (uiYetToWrite > uiMaxPacketSize) ? uiMaxPacketSize : uiYetToWrite ;
		uiYetToWrite -= uiCurWriteLen ;

		UHCITransferDesc* pTD = _ppBulkWriteTDs[ iIndex ] ;

		//pTD->SetTDAttribute(UHCI_ATTR_TD_CONTROL_SPD, 1);
		pTD->SetTDAttribute(UHCI_ATTR_TD_CONTROL_ERR_LEVEL, TD_CONTROL_ERR3);
		pTD->SetTDAttribute(UHCI_ATTR_TD_STATUS_ACTIVE, 1);
		pTD->SetTDAttribute(UHCI_ATTR_TD_DEVICE_ADDR, _devAddr);
		pTD->SetTDAttribute(UHCI_ATTR_TD_ENDPT_ADDR, pDisk->uiEndPointOut);
		pTD->SetTDAttribute(UHCI_ATTR_TD_DATA_TOGGLE, pDisk->bEndPointOutToggle);
		pDisk->bEndPointOutToggle ^= 1 ;

		pTD->SetTDAttribute(UHCI_ATTR_TD_MAXLEN, uiCurWriteLen - 1);
		pTD->SetTDAttribute(UHCI_ATTR_TD_PID, TOKEN_PID_OUT);
		
		pTD->SetTDAttribute(UHCI_ATTR_BUF_PTR, (unsigned)(pDisk->pRawAlignedBuffer + (iIndex * uiMaxPacketSize)));

		if(iIndex > 0)
			_ppBulkWriteTDs[ iIndex - 1 ]->SetTDAttribute(UHCI_ATTR_TD_VERTICAL_LINK, (unsigned)pTD);
	}

	_ppBulkWriteTDs[ iIndex - 1 ]->SetTDAttribute(UHCI_ATTR_TD_TERMINATE, 1);
	_ppBulkWriteTDs[ iIndex - 1 ]->SetTDAttribute(UHCI_ATTR_TD_CONTROL_IOC, 1);
	pQH->SetQHAttribute(UHCI_ATTR_QH_TO_TD_ELEM_LINK, (unsigned)(_ppBulkWriteTDs[ 0 ]));
	
	_controller.SetFrameListEntry(uiFrameNumber, ((unsigned)pQH + GLOBAL_DATA_SEGMENT_BASE) | 0x2, false, false);

	if(!pQH->PollWait())
	{
		printf("\nFrame Number = %d", uiFrameNumber);
		printf("\n QH Element Pointer = %x", pQH->ElementLinkPointer());
		printf("\n QH Head Pointer = %x", pQH->HeadLinkPointer());

		for(iIndex = 0; iIndex < iNoOfTDs; iIndex++)
			printf("\n TD%d Status = %x/%x/%x", iIndex+1, (unsigned)(_ppBulkWriteTDs[ iIndex ]), 
					_ppBulkWriteTDs[ iIndex ]->ControlnStatus(), _ppBulkWriteTDs[ iIndex ]->Token());

		printf("\n Status = %x", PortCom_ReceiveWord(_controller.IOBase() + USBSTS_REG));
		printf("\n Port Status = %x", PortCom_ReceiveWord(_portAddr));
		printf("\nFrame Number = %d", PortCom_ReceiveWord(_controller.IOBase() + FRNUM_REG));
	}

	_controller.CleanFrame(uiFrameNumber);

	return true ;
}

void UHCIDevice::GetDescriptor(unsigned short usDescValue, unsigned short usIndex, int iLen, void* pDestDesc)
{
	unsigned uiFrameNumber = _controller.GetNextFreeFrame();

	UHCIQueueHead* pQH = UHCIQueueHead::Create();
	pQH->SetQHAttribute(UHCI_ATTR_QH_HEAD_LINK_TERMINATE, 1);

	UHCITransferDesc* pTD1 = UHCITransferDesc::Create();

	//pTD1->SetTDAttribute(UHCI_ATTR_TD_CONTROL_SPD, 1);
	//pTD1->SetTDAttribute(UHCI_ATTR_TD_CONTROL_IOC, 1);
	pTD1->SetTDAttribute(UHCI_ATTR_TD_CONTROL_ERR_LEVEL, TD_CONTROL_ERR3);
	pTD1->SetTDAttribute(UHCI_ATTR_TD_STATUS_ACTIVE, 1);
	pTD1->SetTDAttribute(UHCI_ATTR_TD_DEVICE_ADDR, _devAddr);

	pTD1->SetTDAttribute(UHCI_ATTR_TD_MAXLEN, 7);
	pTD1->SetTDAttribute(UHCI_ATTR_TD_PID, TOKEN_PID_SETUP);
	
	if(iLen < 0)
		iLen = DEF_DESC_LEN ;

	USBDevRequest* pDevRequest = (USBDevRequest*)DMM_AllocateForKernel(sizeof(USBDevRequest), 16);
	pDevRequest->bRequestType = 0x80 ;
	pDevRequest->bRequest = 6 ;
	pDevRequest->usWValue = usDescValue ;
	pDevRequest->usWIndex = usIndex ;
	pDevRequest->usWLength = iLen + 1 ;

	pTD1->SetTDAttribute(UHCI_ATTR_BUF_PTR, (unsigned)pDevRequest);
	
	pQH->SetQHAttribute(UHCI_ATTR_QH_TO_TD_ELEM_LINK, (unsigned)pTD1);
	
	UHCITransferDesc* pTD2 = UHCITransferDesc::Create();
	pTD1->SetTDAttribute(UHCI_ATTR_TD_VERTICAL_LINK, (unsigned)pTD2);

	unsigned uiBufPtr = DMM_AllocateForKernel(iLen, 16);

	//pTD2->SetTDAttribute(UHCI_ATTR_TD_CONTROL_SPD, 1);
	//pTD2->SetTDAttribute(UHCI_ATTR_TD_CONTROL_IOC, 1);
	pTD2->SetTDAttribute(UHCI_ATTR_TD_CONTROL_ERR_LEVEL, TD_CONTROL_ERR3);
	pTD2->SetTDAttribute(UHCI_ATTR_TD_STATUS_ACTIVE, 1);
	pTD2->SetTDAttribute(UHCI_ATTR_TD_DEVICE_ADDR, _devAddr);
	
	pTD2->SetTDAttribute(UHCI_ATTR_TD_MAXLEN, iLen - 1);
	pTD2->SetTDAttribute(UHCI_ATTR_TD_DATA_TOGGLE, 1);
	pTD2->SetTDAttribute(UHCI_ATTR_TD_PID, TOKEN_PID_IN);

	pTD2->SetTDAttribute(UHCI_ATTR_BUF_PTR, uiBufPtr);

	UHCITransferDesc* pTD3 = UHCITransferDesc::Create();
	pTD2->SetTDAttribute(UHCI_ATTR_TD_VERTICAL_LINK, (unsigned)pTD3);

	pTD3->SetTDAttribute(UHCI_ATTR_TD_TERMINATE, 1);
	//pTD3->uiLinkPointer = 5 ;
	
	//pTD3->SetTDAttribute(UHCI_ATTR_TD_CONTROL_SPD, 1);
	pTD3->SetTDAttribute(UHCI_ATTR_TD_CONTROL_IOC, 1);
	pTD3->SetTDAttribute(UHCI_ATTR_TD_CONTROL_ERR_LEVEL, TD_CONTROL_ERR3);
	pTD3->SetTDAttribute(UHCI_ATTR_TD_STATUS_ACTIVE, 1);
	pTD3->SetTDAttribute(UHCI_ATTR_TD_DEVICE_ADDR, _devAddr);
	
	pTD3->SetTDAttribute(UHCI_ATTR_TD_MAXLEN, TD_MAXLEN_BLK);
	pTD3->SetTDAttribute(UHCI_ATTR_TD_PID, TOKEN_PID_OUT);

	if(UpanixMain_IsKernelDebugOn())
	{
		printf("\n QH Element Pointer = %x", pQH->ElementLinkPointer());
		printf("\n TD1 Status = %x/%x/%x\n TD2 Status = %x/%x/%x\n TD3 Status = %x/%x/%x", 
				(unsigned)pTD1, pTD1->ControlnStatus(), pTD1->Token(),
				(unsigned)pTD2, pTD2->ControlnStatus(), pTD2->Token(),
				(unsigned)pTD3, pTD3->ControlnStatus(), pTD3->Token());

		printf("\nCurrent Frame Number = %d", PortCom_ReceiveWord(_controller.IOBase() + FRNUM_REG));
		printf("\nFrame Number = %d", uiFrameNumber);

		_controller.SetFrameListEntry(uiFrameNumber, ((unsigned)pQH + GLOBAL_DATA_SEGMENT_BASE) | 0x2, true, true);
		printf("\nFrame Reg = %x", _controller.GetFrameListEntry(uiFrameNumber));

		ProcessManager::Instance().Sleep(5000);
		printf("\n QH Element Pointer = %x", pQH->ElementLinkPointer());
		printf("\n QH Head Pointer = %x", pQH->HeadLinkPointer());
		printf("\n TD1 Status = %x/%x/%x\n TD2 Status = %x/%x/%x\n TD3 Status = %x/%x/%x", 
				(unsigned)pTD1, pTD1->ControlnStatus(), pTD1->Token(),
				(unsigned)pTD2, pTD2->ControlnStatus(), pTD2->Token(),
				(unsigned)pTD3, pTD3->ControlnStatus(), pTD3->Token());
		printf("\nFrame Reg = %x", _controller.GetFrameListEntry(uiFrameNumber));

		printf("\n Status = %x", PortCom_ReceiveWord(_controller.IOBase() + USBSTS_REG));
		printf("\n Port Status = %x", PortCom_ReceiveWord(_portAddr));
		printf("\nFrame Number = %d", PortCom_ReceiveWord(_controller.IOBase() + FRNUM_REG));

		void* pDesc = (void*)pTD2->GetTDAttribute(UHCI_ATTR_BUF_PTR);
		
		MemUtil_CopyMemory(MemUtil_GetDS(), (unsigned)pDesc, MemUtil_GetDS(), (unsigned) pDestDesc, iLen);

		printf("\n\n Scheduling Frame Clean %d: %d", uiFrameNumber, _controller.CleanFrame(uiFrameNumber));
		ProcessManager::Instance().Sleep(2000);
	}
	else
	{
		_controller.SetFrameListEntry(uiFrameNumber, ((unsigned)pQH + GLOBAL_DATA_SEGMENT_BASE) | 0x2, true, true);

		if(!pQH->PollWait())
		{
			printf("\nFrame Number = %d", uiFrameNumber);
			printf("\n QH Element Pointer = %x", pQH->ElementLinkPointer());
			printf("\n QH Head Pointer = %x", pQH->HeadLinkPointer());
			printf("\n TD1 Status = %x/%x/%x\n TD2 Status = %x/%x/%x\n TD3 Status = %x/%x/%x", 
					(unsigned)pTD1, pTD1->ControlnStatus(), pTD1->Token(),
					(unsigned)pTD2, pTD2->ControlnStatus(), pTD2->Token(),
					(unsigned)pTD3, pTD3->ControlnStatus(), pTD3->Token());
			printf("\n Status = %x", PortCom_ReceiveWord(_controller.IOBase() + USBSTS_REG));
			printf("\n Port Status = %x", PortCom_ReceiveWord(_portAddr));
			printf("\nFrame Number = %d", PortCom_ReceiveWord(_controller.IOBase() + FRNUM_REG));
		}

		void* pDesc = (void*)pTD2->GetTDAttribute(UHCI_ATTR_BUF_PTR);
		
		MemUtil_CopyMemory(MemUtil_GetDS(), (unsigned)pDesc, MemUtil_GetDS(), (unsigned) pDestDesc, iLen);

		_controller.CleanFrame(uiFrameNumber);
	}
}

void UHCIDevice::SetAddress()
{
	_devAddr = USBController::Instance().GetNextDevNum();
	if(_devAddr < -1)
    throw upan::exception(XLOC, "Maximum USB Device Limit reached. New Device Allocation Failed!");
	
	unsigned uiFrameNumber = _controller.GetNextFreeFrame();

	UHCIQueueHead* pQH = UHCIQueueHead::Create();
	pQH->SetQHAttribute(UHCI_ATTR_QH_HEAD_LINK_TERMINATE, 1);

	UHCITransferDesc* pTD1 = UHCITransferDesc::Create();

	pTD1->SetTDAttribute(UHCI_ATTR_TD_CONTROL_IOC, 1);
	pTD1->SetTDAttribute(UHCI_ATTR_TD_CONTROL_ERR_LEVEL, TD_CONTROL_ERR3);
	pTD1->SetTDAttribute(UHCI_ATTR_TD_STATUS_ACTIVE, 1);

	pTD1->SetTDAttribute(UHCI_ATTR_TD_MAXLEN, 7);
	pTD1->SetTDAttribute(UHCI_ATTR_TD_PID, TOKEN_PID_SETUP);
	
	USBDevRequest* pDevRequest = (USBDevRequest*)DMM_AllocateForKernel(sizeof(USBDevRequest), 16);
	pDevRequest->bRequestType = 0x00 ;
	pDevRequest->bRequest = 5 ;
	pDevRequest->usWValue = _devAddr ;
	pDevRequest->usWIndex = 0 ;
	pDevRequest->usWLength = 0 ;

	pTD1->SetTDAttribute(UHCI_ATTR_BUF_PTR, (unsigned)pDevRequest);
	
	pQH->SetQHAttribute(UHCI_ATTR_QH_TO_TD_ELEM_LINK, (unsigned)pTD1);
	
	UHCITransferDesc* pTD2 = UHCITransferDesc::Create();
	pTD1->SetTDAttribute(UHCI_ATTR_TD_VERTICAL_LINK, (unsigned)pTD2);

	pTD2->SetTDAttribute(UHCI_ATTR_TD_TERMINATE, 1);

	pTD2->SetTDAttribute(UHCI_ATTR_TD_CONTROL_IOC, 1);
	pTD2->SetTDAttribute(UHCI_ATTR_TD_CONTROL_ERR_LEVEL, TD_CONTROL_ERR3);
	pTD2->SetTDAttribute(UHCI_ATTR_TD_STATUS_ACTIVE, 1);
	
	pTD2->SetTDAttribute(UHCI_ATTR_TD_MAXLEN, TD_MAXLEN_BLK);
	pTD2->SetTDAttribute(UHCI_ATTR_TD_DATA_TOGGLE, 1);
	pTD2->SetTDAttribute(UHCI_ATTR_TD_PID, TOKEN_PID_IN);

	if(UpanixMain_IsKernelDebugOn())
	{
		printf("\n QH Element Pointer = %x", pQH->ElementLinkPointer());
		printf("\n TD1 Status = %x/%x/%x\n TD2 Status = %x/%x/%x\n",
				(unsigned)pTD1, pTD1->ControlnStatus(), pTD1->Token(),
				(unsigned)pTD2, pTD2->ControlnStatus(), pTD2->Token());

		printf("\nCurrent Frame Number = %d", PortCom_ReceiveWord(_controller.IOBase() + FRNUM_REG));
		printf("\nFrame Number = %d", uiFrameNumber);

		_controller.SetFrameListEntry(uiFrameNumber, ((unsigned)pQH + GLOBAL_DATA_SEGMENT_BASE) | 0x2, true, true);
		printf("\nFrame Reg = %x", _controller.GetFrameListEntry(uiFrameNumber));

		ProcessManager::Instance().Sleep(5000);
		printf("\n QH Element Pointer = %x", pQH->ElementLinkPointer());
		printf("\n QH Head Pointer = %x", pQH->HeadLinkPointer());
		printf("\n TD1 Status = %x/%x/%x\n TD2 Status = %x/%x/%x\n",
				(unsigned)pTD1, pTD1->ControlnStatus(), pTD1->Token(),
				(unsigned)pTD2, pTD2->ControlnStatus(), pTD2->Token());
		printf("\nFrame Reg = %x", _controller.GetFrameListEntry(uiFrameNumber));

		printf("\n Status = %x", PortCom_ReceiveWord(_controller.IOBase() + USBSTS_REG));
		printf("\n Port Status = %x", PortCom_ReceiveWord(_portAddr));
		printf("\nFrame Number = %d", PortCom_ReceiveWord(_controller.IOBase() + FRNUM_REG));

		printf("\n\n Scheduling Frame Clean %d: %d", uiFrameNumber, _controller.CleanFrame(uiFrameNumber));
		ProcessManager::Instance().Sleep(2000);
	}
	else
	{
		_controller.SetFrameListEntry(uiFrameNumber, ((unsigned)pQH + GLOBAL_DATA_SEGMENT_BASE) | 0x2, true, true);

		if(!pQH->PollWait())
		{
			printf("\nFrame Number = %d", uiFrameNumber);
			printf("\n QH Element Pointer = %x", pQH->ElementLinkPointer());
			printf("\n QH Head Pointer = %x", pQH->HeadLinkPointer());
			printf("\n TD1 Status = %x/%x/%x\n TD2 Status = %x/%x/%x\n",
					(unsigned)pTD1, pTD1->ControlnStatus(), pTD1->Token(),
					(unsigned)pTD2, pTD2->ControlnStatus(), pTD2->Token());

			printf("\n Status = %x", PortCom_ReceiveWord(_controller.IOBase() + USBSTS_REG));
			printf("\n Port Status = %x", PortCom_ReceiveWord(_portAddr));
			printf("\nFrame Number = %d", PortCom_ReceiveWord(_controller.IOBase() + FRNUM_REG));
		}

		_controller.CleanFrame(uiFrameNumber);
	}
}

void UHCIDevice::GetDeviceDescriptor(USBStandardDeviceDesc* pDevDesc)
{
	USBDataHandler_InitDevDesc(pDevDesc);
	GetDescriptor(0x100, 0, -1, pDevDesc);

	byte iLen = pDevDesc->bLength ;

	if(iLen >= sizeof(USBStandardDeviceDesc))
		iLen = sizeof(USBStandardDeviceDesc);

	GetDescriptor(0x100, 0, iLen, pDevDesc);
}

void UHCIDevice::CheckConfiguration(byte* bConfigValue)
{
	if(_deviceDesc.bNumConfigs <= 0)
    throw upan::exception(XLOC, "\n Invalid NumConfigs: %d", _deviceDesc.bNumConfigs);

	if(*bConfigValue <= 0 || *bConfigValue > _deviceDesc.bNumConfigs)
	{
		printf("\n Current Configuration %d -> Invalid. Setting to Configuration 1", *bConfigValue);
		SetConfiguration(1);
		*bConfigValue = 1 ;
	}

  SetConfigIndex(*bConfigValue);
}

void UHCIDevice::GetConfigDescriptor(USBStandardConfigDesc** pConfigDesc)
{
	int index ;

	USBStandardConfigDesc* pCD = (USBStandardConfigDesc*)DMM_AllocateForKernel(sizeof(USBStandardConfigDesc) * _deviceDesc.bNumConfigs);

	for(index = 0; index < (int)_deviceDesc.bNumConfigs; index++)
		USBDataHandler_InitConfigDesc(&pCD[index]);

	for(index = 0; index < (int)_deviceDesc.bNumConfigs; index++)
	{
		unsigned short usDescValue = (0x2 << 8) | (index & 0xFF);

		GetDescriptor(usDescValue, 0, -1, &(pCD[index]));

		int iLen = pCD[index].wTotalLength;

		void* pBuffer = (void*)DMM_AllocateForKernel(iLen);

		GetDescriptor(usDescValue, 0, iLen, pBuffer);

		USBDataHandler_CopyConfigDesc(&pCD[index], pBuffer, pCD[index].bLength);

		pCD[index].DebugPrint();

		printf("\n Parsing Interface information for Configuration: %d", index);
		printf("\n Number of Interfaces: %d", (int)pCD[ index ].bNumInterfaces);
		ProcessManager::Instance().Sleep(10000);

		void* pInterfaceBuffer = (char*)pBuffer + pCD[index].bLength ;

		pCD[index].pInterfaces = (USBStandardInterface*)DMM_AllocateForKernel(sizeof(USBStandardInterface) * pCD[index].bNumInterfaces);
		int iI ;

		for(iI = 0; iI < (int)pCD[index].bNumInterfaces; iI++)
		{
			USBDataHandler_InitInterfaceDesc(&(pCD[index].pInterfaces[iI]));
		}

		for(iI = 0; iI < (int)pCD[index].bNumInterfaces; iI++)
		{
			USBStandardInterface* pInt = (USBStandardInterface*)(pInterfaceBuffer);

			int iIntLen = sizeof(USBStandardInterface) - sizeof(USBStandardEndPt*);
			int iCopyLen = pInt->bLength ;

			if(iCopyLen > iIntLen)
				iCopyLen = iIntLen ;

			MemUtil_CopyMemory(MemUtil_GetDS(), (unsigned)pInt, 
								MemUtil_GetDS(), (unsigned)&(pCD[index].pInterfaces[iI]),
								iCopyLen);

			pCD[index].pInterfaces[iI].DebugPrint();

			USBStandardEndPt* pEndPtBuffer = (USBStandardEndPt*)((char*)pInterfaceBuffer + pInt->bLength);

			int iNumEndPoints = pCD[index].pInterfaces[iI].bNumEndpoints ;

			pCD[index].pInterfaces[iI].pEndPoints = (USBStandardEndPt*)DMM_AllocateForKernel(
														sizeof(USBStandardEndPt) * iNumEndPoints);

			printf("\n Parsing EndPoints for Interface: %d of Configuration: %d", iI, index);
			ProcessManager::Instance().Sleep(10000);

			int iE ;
			for(iE = 0; iE < iNumEndPoints; iE++)
				USBDataHandler_InitEndPtDesc(&(pCD[index].pInterfaces[iI].pEndPoints[iE]));

			for(iE = 0; iE < iNumEndPoints; iE++)
			{
				USBStandardEndPt* pEndPt = &pEndPtBuffer[iE] ;

				int iELen = sizeof(USBStandardEndPt);
				int iECopyLen = pEndPt->bLength ;
				
				if(iECopyLen > iELen)
					iECopyLen = iELen ;

				MemUtil_CopyMemory(MemUtil_GetDS(), (unsigned)pEndPt, 
									MemUtil_GetDS(), (unsigned)&(pCD[index].pInterfaces[iI].pEndPoints[iE]),
									iECopyLen);

				pCD[index].pInterfaces[iI].pEndPoints[iE].DebugPrint();
			}

			pInterfaceBuffer = (USBStandardInterface*)((char*)pEndPtBuffer + iNumEndPoints * sizeof(USBStandardEndPt));
		}

		DMM_DeAllocateForKernel((uintptr_t)pBuffer);
		pBuffer = NULL ;
	}

	*pConfigDesc = pCD ;
}

void UHCIDevice::GetStringDescriptorZero(USBStringDescZero** ppStrDescZero)
{
	unsigned short usDescValue = (0x3 << 8);
	char szPart[ 8 ];

	GetDescriptor(usDescValue, 0, -1, szPart);

	int iLen = ((USBStringDescZero*)&szPart)->bLength ;
	printf("\n String Desc Zero Len: %d", iLen);

	byte* pStringDescZero = (byte*)DMM_AllocateForKernel(iLen);

	GetDescriptor(usDescValue, 0, iLen, pStringDescZero);

	*ppStrDescZero = (USBStringDescZero*)DMM_AllocateForKernel(sizeof(USBStringDescZero));

	USBDataHandler_CopyStrDescZero(*ppStrDescZero, pStringDescZero);
	USBDataHandler_DisplayStrDescZero(*ppStrDescZero);

	DMM_DeAllocateForKernel((uintptr_t)pStringDescZero);
}

void UHCIDevice::GetConfiguration(byte* bConfigValue)
{
	unsigned uiFrameNumber = _controller.GetNextFreeFrame();

	UHCIQueueHead* pQH = UHCIQueueHead::Create();
	pQH->SetQHAttribute(UHCI_ATTR_QH_HEAD_LINK_TERMINATE, 1);

	UHCITransferDesc* pTD1 = UHCITransferDesc::Create();

	pTD1->SetTDAttribute(UHCI_ATTR_TD_CONTROL_IOC, 1);
	pTD1->SetTDAttribute(UHCI_ATTR_TD_CONTROL_ERR_LEVEL, TD_CONTROL_ERR3);
	pTD1->SetTDAttribute(UHCI_ATTR_TD_STATUS_ACTIVE, 1);
	pTD1->SetTDAttribute(UHCI_ATTR_TD_DEVICE_ADDR, _devAddr);

	pTD1->SetTDAttribute(UHCI_ATTR_TD_MAXLEN, 7);
	pTD1->SetTDAttribute(UHCI_ATTR_TD_PID, TOKEN_PID_SETUP);
	
	USBDevRequest* pDevRequest = (USBDevRequest*)DMM_AllocateForKernel(sizeof(USBDevRequest), 16);
	pDevRequest->bRequestType = 0x80 ;
	pDevRequest->bRequest = 8 ;
	pDevRequest->usWValue = 0 ;
	pDevRequest->usWIndex = 0 ;
	pDevRequest->usWLength = 1 ;

	pTD1->SetTDAttribute(UHCI_ATTR_BUF_PTR, (unsigned)pDevRequest);
	
	pQH->SetQHAttribute(UHCI_ATTR_QH_TO_TD_ELEM_LINK, (unsigned)pTD1);
	
	UHCITransferDesc* pTD2 = UHCITransferDesc::Create();
	pTD1->SetTDAttribute(UHCI_ATTR_TD_VERTICAL_LINK, (unsigned)pTD2);

	unsigned uiBufPtr = DMM_AllocateForKernel(1, 16);

	//pTD2->SetTDAttribute(UHCI_ATTR_TD_CONTROL_SPD, 1);
	//pTD2->SetTDAttribute(UHCI_ATTR_TD_CONTROL_IOC, 1);
	pTD2->SetTDAttribute(UHCI_ATTR_TD_CONTROL_ERR_LEVEL, TD_CONTROL_ERR3);
	pTD2->SetTDAttribute(UHCI_ATTR_TD_STATUS_ACTIVE, 1);
	pTD2->SetTDAttribute(UHCI_ATTR_TD_DEVICE_ADDR, _devAddr);
	
	pTD2->SetTDAttribute(UHCI_ATTR_TD_MAXLEN, 0);
	pTD2->SetTDAttribute(UHCI_ATTR_TD_DATA_TOGGLE, 1);
	pTD2->SetTDAttribute(UHCI_ATTR_TD_PID, TOKEN_PID_IN);

	pTD2->SetTDAttribute(UHCI_ATTR_BUF_PTR, uiBufPtr);

	UHCITransferDesc* pTD3 = UHCITransferDesc::Create();
	pTD2->SetTDAttribute(UHCI_ATTR_TD_VERTICAL_LINK, (unsigned)pTD3);

	pTD3->SetTDAttribute(UHCI_ATTR_TD_TERMINATE, 1);
	
	//pTD3->SetTDAttribute(UHCI_ATTR_TD_CONTROL_SPD, 1);
	pTD3->SetTDAttribute(UHCI_ATTR_TD_CONTROL_IOC, 1);
	pTD3->SetTDAttribute(UHCI_ATTR_TD_CONTROL_ERR_LEVEL, TD_CONTROL_ERR3);
	pTD3->SetTDAttribute(UHCI_ATTR_TD_STATUS_ACTIVE, 1);
	pTD3->SetTDAttribute(UHCI_ATTR_TD_DEVICE_ADDR, _devAddr);
	
	pTD3->SetTDAttribute(UHCI_ATTR_TD_MAXLEN, TD_MAXLEN_BLK);
	pTD3->SetTDAttribute(UHCI_ATTR_TD_PID, TOKEN_PID_OUT);

	if(UpanixMain_IsKernelDebugOn())
	{
		printf("\n QH Element Pointer = %x", pQH->ElementLinkPointer());
		printf("\n TD1 Status = %x/%x/%x\n TD2 Status = %x/%x/%x\n TD3 Status = %x/%x/%x", 
				(unsigned)pTD1, pTD1->ControlnStatus(), pTD1->Token(),
				(unsigned)pTD2, pTD2->ControlnStatus(), pTD2->Token(),
				(unsigned)pTD3, pTD3->ControlnStatus(), pTD3->Token());

		printf("\nCurrent Frame Number = %d", PortCom_ReceiveWord(_controller.IOBase() + FRNUM_REG));
		printf("\nFrame Number = %d", uiFrameNumber);

		_controller.SetFrameListEntry(uiFrameNumber, ((unsigned)pQH + GLOBAL_DATA_SEGMENT_BASE) | 0x2, true, true);
		printf("\nFrame Reg = %x", _controller.GetFrameListEntry(uiFrameNumber));

		ProcessManager::Instance().Sleep(5000);
		printf("\n QH Element Pointer = %x", pQH->ElementLinkPointer());
		printf("\n QH Head Pointer = %x", pQH->HeadLinkPointer());
		printf("\n TD1 Status = %x/%x/%x\n TD2 Status = %x/%x/%x\n TD3 Status = %x/%x/%x", 
				(unsigned)pTD1, pTD1->ControlnStatus(), pTD1->Token(),
				(unsigned)pTD2, pTD2->ControlnStatus(), pTD2->Token(),
				(unsigned)pTD3, pTD3->ControlnStatus(), pTD3->Token());
		printf("\nFrame Reg = %x", _controller.GetFrameListEntry(uiFrameNumber));

		printf("\n Status = %x", PortCom_ReceiveWord(_controller.IOBase() + USBSTS_REG));
		printf("\n Port Status = %x", PortCom_ReceiveWord(_portAddr));
		printf("\nFrame Number = %d", PortCom_ReceiveWord(_controller.IOBase() + FRNUM_REG));

		*bConfigValue = *((char*)pTD2->GetTDAttribute(UHCI_ATTR_BUF_PTR));

		printf("\n\n Scheduling Frame Clean %d: %d", uiFrameNumber, _controller.CleanFrame(uiFrameNumber));
		ProcessManager::Instance().Sleep(2000);
	}
	else
	{
		_controller.SetFrameListEntry(uiFrameNumber, ((unsigned)pQH + GLOBAL_DATA_SEGMENT_BASE) | 0x2, true, true);

		if(!pQH->PollWait())
		{
			printf("\nFrame Number = %d", uiFrameNumber);
			printf("\n QH Element Pointer = %x", pQH->ElementLinkPointer());
			printf("\n QH Head Pointer = %x", pQH->HeadLinkPointer());
			printf("\n TD1 Status = %x/%x/%x\n TD2 Status = %x/%x/%x\n TD3 Status = %x/%x/%x", 
					(unsigned)pTD1, pTD1->ControlnStatus(), pTD1->Token(),
					(unsigned)pTD2, pTD2->ControlnStatus(), pTD2->Token(),
					(unsigned)pTD3, pTD3->ControlnStatus(), pTD3->Token());

			printf("\n Status = %x", PortCom_ReceiveWord(_controller.IOBase() + USBSTS_REG));
			printf("\n Port Status = %x", PortCom_ReceiveWord(_portAddr));
			printf("\nFrame Number = %d", PortCom_ReceiveWord(_controller.IOBase() + FRNUM_REG));
		}

		*bConfigValue = *((char*)pTD2->GetTDAttribute(UHCI_ATTR_BUF_PTR));

		_controller.CleanFrame(uiFrameNumber);
	}
}

void UHCIDevice::SetConfiguration(byte bConfigValue)
{
	unsigned uiFrameNumber = _controller.GetNextFreeFrame();

	UHCIQueueHead* pQH = UHCIQueueHead::Create();
	pQH->SetQHAttribute(UHCI_ATTR_QH_HEAD_LINK_TERMINATE, 1);

	UHCITransferDesc* pTD1 = UHCITransferDesc::Create();

	pTD1->SetTDAttribute(UHCI_ATTR_TD_CONTROL_IOC, 1);
	pTD1->SetTDAttribute(UHCI_ATTR_TD_CONTROL_ERR_LEVEL, TD_CONTROL_ERR3);
	pTD1->SetTDAttribute(UHCI_ATTR_TD_STATUS_ACTIVE, 1);
	pTD1->SetTDAttribute(UHCI_ATTR_TD_DEVICE_ADDR, _devAddr);

	pTD1->SetTDAttribute(UHCI_ATTR_TD_MAXLEN, 7);
	pTD1->SetTDAttribute(UHCI_ATTR_TD_PID, TOKEN_PID_SETUP);
	
	USBDevRequest* pDevRequest = (USBDevRequest*)DMM_AllocateForKernel(sizeof(USBDevRequest), 16);
	pDevRequest->bRequestType = 0x00 ;
	pDevRequest->bRequest = 9 ;
	pDevRequest->usWValue = bConfigValue ;
	pDevRequest->usWIndex = 0 ;
	pDevRequest->usWLength = 0 ;

	pTD1->SetTDAttribute(UHCI_ATTR_BUF_PTR, (unsigned)pDevRequest);
	
	pQH->SetQHAttribute(UHCI_ATTR_QH_TO_TD_ELEM_LINK, (unsigned)pTD1);
	
	UHCITransferDesc* pTD2 = UHCITransferDesc::Create();
	pTD1->SetTDAttribute(UHCI_ATTR_TD_VERTICAL_LINK, (unsigned)pTD2);

	pTD2->SetTDAttribute(UHCI_ATTR_TD_TERMINATE, 1);

	pTD2->SetTDAttribute(UHCI_ATTR_TD_CONTROL_IOC, 1);
	pTD2->SetTDAttribute(UHCI_ATTR_TD_CONTROL_ERR_LEVEL, TD_CONTROL_ERR3);
	pTD2->SetTDAttribute(UHCI_ATTR_TD_STATUS_ACTIVE, 1);
	pTD2->SetTDAttribute(UHCI_ATTR_TD_DEVICE_ADDR, _devAddr);
	
	pTD2->SetTDAttribute(UHCI_ATTR_TD_MAXLEN, TD_MAXLEN_BLK);
	pTD2->SetTDAttribute(UHCI_ATTR_TD_DATA_TOGGLE, 1);
	pTD2->SetTDAttribute(UHCI_ATTR_TD_PID, TOKEN_PID_IN);

	if(UpanixMain_IsKernelDebugOn())
	{
		printf("\n QH Element Pointer = %x", pQH->ElementLinkPointer());
		printf("\n TD1 Status = %x/%x/%x\n TD2 Status = %x/%x/%x\n",
				(unsigned)pTD1, pTD1->ControlnStatus(), pTD1->Token(),
				(unsigned)pTD2, pTD2->ControlnStatus(), pTD2->Token());

		printf("\nCurrent Frame Number = %d", PortCom_ReceiveWord(_controller.IOBase() + FRNUM_REG));
		printf("\nFrame Number = %d", uiFrameNumber);

		_controller.SetFrameListEntry(uiFrameNumber, ((unsigned)pQH + GLOBAL_DATA_SEGMENT_BASE) | 0x2, true, true);
		printf("\nFrame Reg = %x", _controller.GetFrameListEntry(uiFrameNumber));

		ProcessManager::Instance().Sleep(5000);
		printf("\n QH Element Pointer = %x", pQH->ElementLinkPointer());
		printf("\n QH Head Pointer = %x", pQH->HeadLinkPointer());
		printf("\n TD1 Status = %x/%x/%x\n TD2 Status = %x/%x/%x\n",
				(unsigned)pTD1, pTD1->ControlnStatus(), pTD1->Token(),
				(unsigned)pTD2, pTD2->ControlnStatus(), pTD2->Token());
		printf("\nFrame Reg = %x", _controller.GetFrameListEntry(uiFrameNumber));

		printf("\n Status = %x", PortCom_ReceiveWord(_controller.IOBase() + USBSTS_REG));
		printf("\n Port Status = %x", PortCom_ReceiveWord(_portAddr));
		printf("\nFrame Number = %d", PortCom_ReceiveWord(_controller.IOBase() + FRNUM_REG));

		printf("\n\n Scheduling Frame Clean %d: %d", uiFrameNumber, _controller.CleanFrame(uiFrameNumber));
		ProcessManager::Instance().Sleep(2000);
	}
	else
	{
		_controller.SetFrameListEntry(uiFrameNumber, ((unsigned)pQH + GLOBAL_DATA_SEGMENT_BASE) | 0x2, true, true);

		if(!pQH->PollWait())
		{
			printf("\nFrame Number = %d", uiFrameNumber);
			printf("\n QH Element Pointer = %x", pQH->ElementLinkPointer());
			printf("\n QH Head Pointer = %x", pQH->HeadLinkPointer());
			printf("\n TD1 Status = %x/%x/%x\n TD2 Status = %x/%x/%x\n",
					(unsigned)pTD1, pTD1->ControlnStatus(), pTD1->Token(),
					(unsigned)pTD2, pTD2->ControlnStatus(), pTD2->Token());

			printf("\n Status = %x", PortCom_ReceiveWord(_controller.IOBase() + USBSTS_REG));
			printf("\n Port Status = %x", PortCom_ReceiveWord(_portAddr));
			printf("\nFrame Number = %d", PortCom_ReceiveWord(_controller.IOBase() + FRNUM_REG));
		}

		_controller.CleanFrame(uiFrameNumber);
	}
}

