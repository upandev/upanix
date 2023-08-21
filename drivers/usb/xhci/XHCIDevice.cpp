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

#include <uniq_ptr.h>
#include <XHCIDevice.h>
#include <XHCIController.h>
#include <XHCIContext.h>
#include <TRB.h>
#include <USBStructures.h>
#include <USBDataHandler.h>

XHCIDevice::XHCIDevice(XHCIController& controller,
                       XHCIPortRegister& port,
                       unsigned portId,
                       unsigned slotType) : _portId(portId), _port(port), _controller(controller),
                       _inputContext(nullptr), _devContext(nullptr)
{
  _slotID = _controller.EnableSlot(slotType);
  if(!_slotID)
    throw upan::exception(XLOC, "Failed to get SlotID");

  _inputContext = new InputContext(_controller, _slotID, port, portId, 0);
  _devContext = new DeviceContext(_controller.CapReg().IsContextSize64());

  _controller.SetDeviceContext(_slotID, _devContext->Slot());

//TODO: to deal with older devices - you may have to send AddressDevice twice
//first, with block bit set and then with block bit cleared
//With first request, address should be set and slot state is default (1)
//With second request, slot state should change to Addressed (2)
  _controller.AddressDevice(KERNEL_REAL_ADDRESS(&_inputContext->Control()), _slotID, false);

  if(_devContext->EP0().EPState() != EndPointContext::Running)
    throw upan::exception(XLOC, "After AddressDevice, EndPoint0 is in %d state", _devContext->EP0().EPState());

  if(_devContext->Slot().SlotState() != SlotContext::Addressed)
    throw upan::exception(XLOC, "After AddressDevice, Slot is in %d state", _devContext->Slot().SlotState());

  printf("\n USB Device Address: %x, PortMaxPkSz: %u, ControlEP MaxPktSz: %u", 
    _devContext->Slot().DevAddress(),
    port.MaxPacketSize(),
    _devContext->EP0().MaxPacketSize());

  GetDeviceDescriptor();

  GetStringDescriptorZero();
  SetLangId();
  GetDeviceStringDesc(_manufacturer, _deviceDesc.indexManufacturer);
  GetDeviceStringDesc(_product, _deviceDesc.indexProduct);
  GetDeviceStringDesc(_serialNum, _deviceDesc.indexSerialNum);

  PrintDeviceStringDetails();

  GetConfigDescriptor();

  ConfigureEndPoint();

  SetConfiguration();
}

XHCIDevice::~XHCIDevice()
{
  delete _devContext;
  delete _inputContext;
}

void XHCIDevice::ConfigureEndPoint()
{
  uint32_t lastEPId = 0;
  uint32_t addContextFlag = 1; //Only Slot is set to 1, A1 (EP0) is not required
  for(int index = 0; index < (int)_deviceDesc.bNumConfigs; ++index)
  {
    const auto& config = _pArrConfigDesc[index];
    for(int iI = 0; iI < (int)config.bNumInterfaces; ++iI)
    {
      const auto& interface = config.pInterfaces[iI];
      for(int iE = 0; iE < interface.bNumEndpoints; ++iE)
      {
        const auto& endpoint = interface.pEndPoints[iE];
        const auto epId = _inputContext->AddEndPoint(endpoint);
        if(lastEPId < epId)
          lastEPId = epId;
        addContextFlag |=  (1 << epId);
      }
    }
  }
  _inputContext->Slot().SetContextEntries(lastEPId);
  //TODO: is it requied to update MaxPacketSize of FS device ? if so then update and call Evaluate Context with A0 and A1 set
  _controller.EvaluateContext(KERNEL_REAL_ADDRESS(&_inputContext->Control()), _slotID);

  //set A1 -> Slot and other A bits corresponding to EPs being configured
  _inputContext->Control().SetAddContextFlag(addContextFlag);
  _inputContext->Control().SetDropContextFlag((~addContextFlag) & ~(0x3));

  _controller.ConfigureEndPoint(KERNEL_REAL_ADDRESS(&_inputContext->Control()), _slotID);

  if(_devContext->Slot().SlotState() != SlotContext::Configured)
    throw upan::exception(XLOC, "After ConfigureEndPoint, Slot is in %d state", _devContext->Slot().SlotState());
}

void XHCIDevice::GetDeviceDescriptor()
{
  //the buffer has to be on kernel heap - a mem area that is 1-1 mapped b/w virtual (page) and physical
  //as it's used by XHCI controller to transfer data
  GetDescriptor(0x100, 0, sizeof(USBStandardDeviceDesc), &_deviceDesc);
  _deviceDesc.DebugPrint();
}

void XHCIDevice::GetStringDescriptorZero()
{
  unsigned short usDescValue = (0x3 << 8);
  byte* buffer = new byte[8];

  GetDescriptor(usDescValue, 0, -1, buffer);

  int iLen = reinterpret_cast<USBStringDescZero*>(buffer)->bLength;
  printf("\n String Desc Zero Len: %d", iLen);

  delete[] buffer;
  buffer = new byte[iLen];

  GetDescriptor(usDescValue, 0, iLen, buffer);

  _pStrDescZero = new USBStringDescZero();

  USBDataHandler_CopyStrDescZero(_pStrDescZero, buffer);
  USBDataHandler_DisplayStrDescZero(_pStrDescZero);

  delete[] buffer;
}

void XHCIDevice::GetDeviceStringDesc(upan::string& desc, int descIndex)
{
  desc = "Unknown";
  if(_usLangID == 0)
    return;

  //index = 0 means there is no string descriptor
  if(descIndex == 0)
    return;

  unsigned short usDescValue = (0x3 << 8);
  char* buffer = new char[8];

  GetDescriptor(usDescValue | descIndex, _usLangID, -1, buffer);

  int iLen = reinterpret_cast<USBStringDescZero*>(buffer)->bLength;
  if(iLen == 0)
    return;

  delete buffer;
  buffer = new char[iLen];
  GetDescriptor(usDescValue | descIndex, _usLangID, iLen, buffer);

  int j, k;
  for(j = 0, k = 0; j < (iLen - 2); k++, j += 2)
  {
    //TODO: Ignoring Unicode 2nd byte for the time.
    buffer[k] = buffer[j + 2];
  }
  buffer[k] = '\0';

  desc = buffer;
  delete[] buffer;
}

void XHCIDevice::GetDescriptor(uint16_t descValue, uint16_t index, int len, void* dataBuffer)
{
  if(len < 0)
    len = DEF_DESC_LEN;
  _inputContext->SendCommand(0x80, 6, descValue, index, len, TransferType::IN_DATA_STAGE, dataBuffer);
}

byte XHCIDevice::GetConfigValue()
{
  upan::uniq_ptr<byte[]> buffer(new byte[8]);
  _inputContext->SendCommand(0x80, 8, 0, 0, 1, TransferType::IN_DATA_STAGE, buffer.get());
  return buffer[0];
}

void XHCIDevice::GetConfigDescriptor()
{
  _pArrConfigDesc = new USBStandardConfigDesc[_deviceDesc.bNumConfigs];

  for(int index = 0; index < (int)_deviceDesc.bNumConfigs; ++index)
  {
    unsigned short usDescValue = (0x2 << 8) | (index & 0xFF);
    GetDescriptor(usDescValue, 0, -1, &(_pArrConfigDesc[index]));

    int iLen = _pArrConfigDesc[index].wTotalLength;
    char* pBuffer = new char[iLen];
    GetDescriptor(usDescValue, 0, iLen, pBuffer);

    USBDataHandler_CopyConfigDesc(&_pArrConfigDesc[index], pBuffer, _pArrConfigDesc[index].bLength);

    _pArrConfigDesc[index].DebugPrint();

    printf("\n Parsing Interface information for Configuration: %d", index);
    printf("\n Number of Interfaces: %d", (int)_pArrConfigDesc[ index ].bNumInterfaces);

    void* pInterfaceBuffer = (char*)pBuffer + _pArrConfigDesc[index].bLength;

    _pArrConfigDesc[index].pInterfaces = new USBStandardInterface[_pArrConfigDesc[index].bNumInterfaces];

    for(int iI = 0; iI < (int)_pArrConfigDesc[index].bNumInterfaces;)
    {
      USBStandardInterface* pInt = (USBStandardInterface*)(pInterfaceBuffer);
      int iIntLen = sizeof(USBStandardInterface) - sizeof(USBStandardEndPt*);
      if(pInt->bLength != iIntLen || pInt->bDescriptorType != 4)
      {
        pInterfaceBuffer = (USBStandardInterface*)((char*)pInterfaceBuffer + pInt->bLength);
        continue;
      }

      memcpy(&(_pArrConfigDesc[index].pInterfaces[iI]), pInt, pInt->bLength);

      _pArrConfigDesc[index].pInterfaces[iI].DebugPrint();

      void* pEndPtBuffer = ((char*)pInterfaceBuffer + pInt->bLength);

      int iNumEndPoints = _pArrConfigDesc[index].pInterfaces[iI].bNumEndpoints;

      _pArrConfigDesc[index].pInterfaces[iI].pEndPoints = new USBStandardEndPt[iNumEndPoints];

      printf("\n Parsing EndPoints for Interface: %d of Configuration: %d", iI, index);

      for(int iE = 0; iE < iNumEndPoints;)
      {
        USBStandardEndPt* pEndPt = (USBStandardEndPt*)(pEndPtBuffer);
        if(pEndPt->bLength != sizeof(USBStandardEndPt) || pEndPt->bDescriptorType != 5)
        {
          pEndPtBuffer = (char*)pEndPtBuffer + pEndPt->bLength;
          continue;
        }
        memcpy(&(_pArrConfigDesc[index].pInterfaces[iI].pEndPoints[iE]), pEndPt, pEndPt->bLength);
        _pArrConfigDesc[index].pInterfaces[iI].pEndPoints[iE].DebugPrint();
        pEndPtBuffer = (char*)pEndPtBuffer + pEndPt->bLength;
        ++iE;
      }

      pInterfaceBuffer = (USBStandardInterface*)pEndPtBuffer;
      ++iI;
    }

    delete[] pBuffer;
  }
}

void XHCIDevice::SetConfiguration()
{
  auto configValue = GetConfigValue();
  printf("\n Current Config Value: %d", (int)configValue);

  if(configValue <= 0 || configValue > _deviceDesc.bNumConfigs)
  {
    configValue = 1;
    _inputContext->SendCommand(0x00, 9, configValue, 0, 0, TransferType::NO_DATA_STAGE, nullptr);
    configValue = GetConfigValue();
    printf("\n New Config Value: %d", (int)configValue);
  }
  SetConfigIndex(configValue);
}

byte XHCIDevice::GetMaxLun()
{
  upan::uniq_ptr<byte[]> buffer(new byte[8]);

  //Setup stage
  const uint32_t requestType = USB_TYPE_CLASS | USB_RECIP_INTERFACE | USB_DIR_IN;
  const int len = 1;
  _inputContext->SendCommand(requestType, 0xFE, 0, _bInterfaceNumber, len, TransferType::IN_DATA_STAGE, buffer.get());
	
  return buffer[0];
}

bool XHCIDevice::CommandReset()
{
  //Setup stage
  const uint32_t requestType = USB_TYPE_CLASS | USB_RECIP_INTERFACE;
  _inputContext->SendCommand(requestType, 0xFF, 0, _bInterfaceNumber, 0, TransferType::NO_DATA_STAGE, nullptr);
	return true;
}

bool XHCIDevice::ClearHaltEndPoint(USBulkDisk* pDisk, bool bIn)
{
	unsigned uiEndPoint = (bIn) ? pDisk->uiEndPointIn : pDisk->uiEndPointOut;
  const uint32_t index = (uiEndPoint & 0xF) | ((bIn) ? 0x80 : 0x00);
  _inputContext->SendCommand(USB_RECIP_ENDPOINT, 1, 0, index, 0, TransferType::NO_DATA_STAGE, nullptr);
	return true;
}

bool XHCIDevice::BulkRead(USBulkDisk* pDisk, void* pDataBuf, unsigned uiLen)
{
  if(uiLen == 0)
    return true ;

  unsigned uiMaxLen = pDisk->usInMaxPacketSize * 64;//MAX_EHCI_TD_PER_BULK_RW ;
  if(uiLen > uiMaxLen)
  {
    printf("\n Max Bulk Transfer per Frame is %d bytes", uiMaxLen) ;
    return false ;
  }
  //printf("\n Read: %d", uiLen);
  _inputContext->ReceiveData((unsigned)pDisk->pRawAlignedBuffer, uiLen);
  memcpy(pDataBuf, pDisk->pRawAlignedBuffer, uiLen) ;
  return true ;
}

bool XHCIDevice::BulkWrite(USBulkDisk* pDisk, void* pDataBuf, unsigned uiLen)
{
  if(uiLen == 0)
    return true;

  unsigned uiMaxLen = pDisk->usOutMaxPacketSize * 64;//MAX_EHCI_TD_PER_BULK_RW;
  if(uiLen > uiMaxLen)
  {
    printf("\n Max Bulk Transfer per Frame is %d bytes", uiMaxLen);
    return false;
  }
  //printf("\n Write: %d", uiLen);
  memcpy(pDisk->pRawAlignedBuffer, pDataBuf, uiLen) ;
  _inputContext->SendData((unsigned)pDisk->pRawAlignedBuffer, uiLen);
  return true;
}

void XHCIDevice::SetIdle()
{
  const uint32_t requestType = USB_TYPE_CLASS | USB_RECIP_INTERFACE;
  _inputContext->SendCommand(requestType, 0x0A, 0, _bInterfaceNumber, 0, TransferType::NO_DATA_STAGE, nullptr);
}

bool XHCIDevice::SetupInterruptReceiveData(uintptr_t bufferAddress, uint32_t len, USBInterruptDataHandler* handler)
{
  _inputContext->SetInterruptDataHandler(handler);
  return _inputContext->ReceiveInterruptData(bufferAddress, len);
}

int XHCIDevice::GetInterruptInInterval()
{
  return _inputContext->GetInterruptInEPInterval();
}

uint32_t XHCIDevice::GetInterruptQueueSize()
{
  return EndPoint::TRANSFER_RING_SIZE;
}
