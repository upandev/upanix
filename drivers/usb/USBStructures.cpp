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
#include <DMM.h>
#include <stdio.h>
#include <ustring.h>
#include <algorithm.h>
#include <exception.h>
#include <StorageDrive.h>
#include <USBStructures.h>
#include <USBMassBulkStorageDisk.h>
#include <USBDevice.h>
#include <StorageDriveManager.h>

/***********************************************************************************************/

USBStandardDeviceDesc::USBStandardDeviceDesc() :
  bLength(0), bDescType(0), bcdUSB(0),
  bDeviceClass(0), bDeviceSubClass(0), bDeviceProtocol(0),
  bMaxPacketSize0(0), sVendorID(0), sProductID(0),
  bcdDevice(0), indexManufacturer(0), indexProduct(0),
  indexSerialNum(0), bNumConfigs(0)
{
}

void USBStandardDeviceDesc::DebugPrint() const
{
  printf("\n Len=%d,DType=%d,bcd=%d",  (int)bLength,  (int)bDescType,  (int)bcdUSB);

  printf(",Dev C=%d,SubC=%d,Proto=%d,MaxPkSize=%d", (int)bDeviceClass, (int)bDeviceSubClass, (int)bDeviceProtocol, (int)bMaxPacketSize0);

  printf(",VenID=%d,ProdID=%d,bcdDev=%d,iManufac=%d,iProd=%d,iSlNo=%d,nConfigs=%d",
    (int)sVendorID,
    (int)sProductID,
    (int)bcdDevice,
    (int)indexManufacturer,
    (int)indexProduct,
    (int)indexSerialNum,
    (int)bNumConfigs);
}

USBStandardConfigDesc::USBStandardConfigDesc() : 
  bLength(0), 
  bDescriptorType(0),
  wTotalLength(0),
  bNumInterfaces(0),
  bConfigurationValue(0),
  iConfiguration(0),
  bmAttributes(0),
  bMaxPower(0),
  pInterfaces(nullptr)
{
}

void USBStandardConfigDesc::DebugPrint()
{
  printf("\n Len=%d,DType=%d,TotLen=%d,nInterfs=%d,ConfVal=%d,Conf=%d,Attrs=%x,MaxPower=%d", 
    bLength,
    bDescriptorType,
    wTotalLength,
    bNumInterfaces,
    bConfigurationValue,
    iConfiguration,
    bmAttributes,
    bMaxPower);
}

USBStandardInterface::USBStandardInterface() :
	bLength(0),
	bDescriptorType(0),
	bInterfaceNumber(0),
	bAlternateSetting(0),
	bNumEndpoints(0),
	bInterfaceClass(0),
	bInterfaceSubClass(0),
	bInterfaceProtocol(0),
	iInterface(0),
	pEndPoints(nullptr)
{
}

void USBStandardInterface::DebugPrint()
{
  printf("\n Len=%d,DType=%d,IntrfceNo=%d,AltSetting=%d,nEpts=%d,IC=%d,ISubC=%d,IProto=%d,Intrfce=%d", 
    bLength,
    bDescriptorType,
    bInterfaceNumber,
    bAlternateSetting,
    bNumEndpoints,
    bInterfaceClass,
    bInterfaceSubClass,
    bInterfaceProtocol,
    iInterface);
}

USBStandardEndPt::USBStandardEndPt() :
	bLength(0),
	bDescriptorType(0),
	bEndpointAddress(0),
	bmAttributes(0),
	wMaxPacketSize(0),
	bInterval(0)
{
}

void USBStandardEndPt::DebugPrint()
{
  printf("\n Len=%d,DType=%d,EpAddr=%x,Attrs=%d,MaxPktSize=%d,Intrvl=%u",
    bLength, bDescriptorType, bEndpointAddress, bmAttributes, wMaxPacketSize, bInterval);
}

USBulkDisk::USBulkDisk(USBDevice* device, 
  int interfaceIndex,
  byte maxLun, 
  const upan::string& protoName) :
        _usbDevice(device), bEndPointInToggle(0), bEndPointOutToggle(0), pRawAlignedBuffer(nullptr),
        pSCSIDeviceList(nullptr), bMaxLun(maxLun), szProtocolName(protoName), pHostDevice(nullptr)
{
	const USBStandardInterface& interface = _usbDevice->_pArrConfigDesc[ _usbDevice->_iConfigIndex ].pInterfaces[ interfaceIndex ];
  _usbDevice->_iInterfaceIndex = interfaceIndex;
  _usbDevice->_bInterfaceNumber = interface.bInterfaceNumber;
  _usbDevice->_pPrivate = this;

	/* Find the endpoints we need
	 * We are expecting a minimum of 2 endpoints - in and out (bulk).
	 * An optional interrupt is OK (necessary for CBI protocol).
	 * We will ignore any others.
	 */
	USBStandardEndPt *pEndPointIn = nullptr, *pEndPointOut = nullptr;
	for(int i = 0; i < interface.bNumEndpoints; ++i)
	{
		USBStandardEndPt* pEndPoint = &(interface.pEndPoints[ i ]) ;

		// Bulk Endpoint
		if( (pEndPoint->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_BULK )
		{
			// Bulk in/out
			if( pEndPoint->bEndpointAddress & USB_DIR_IN )
				pEndPointIn = pEndPoint ;
			else
				pEndPointOut = pEndPoint ;
		}
		else if( (pEndPoint->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_INT )
			pEndPointInt = pEndPoint ;
	}

	/*  Do some basic sanity checks, and bail if we find a problem */
	if( !pEndPointIn || !pEndPointOut || ( interface.bInterfaceProtocol == USB_PR_CB && !pEndPointInt ) )
    throw upan::exception(XLOC, "USBMassBulkStorageDisk: Invalid End Point configuration. In:%x, Out:%x, Int:%x", pEndPointIn, pEndPointOut, pEndPointInt);

	/* Copy over the endpoint data */
	if(pEndPointIn)
	{
		uiEndPointIn = pEndPointIn->bEndpointAddress & USB_ENDPOINT_NUMBER_MASK ;
		usInMaxPacketSize = pEndPointIn->wMaxPacketSize ;
		printf("\n Max Bulk Read Size: %d", usInMaxPacketSize) ;
	}

	if(pEndPointOut)
	{
		uiEndPointOut = pEndPointOut->bEndpointAddress & USB_ENDPOINT_NUMBER_MASK ;
		usOutMaxPacketSize = pEndPointOut->wMaxPacketSize ;
		printf("\n Max Bulk Write Size: %d", usOutMaxPacketSize) ;
	}

  pRawAlignedBuffer = (byte*)KernelDMM::Instance().allocate(US_BULK_MAX_TRANSFER_SIZE, upan::max(usInMaxPacketSize, usOutMaxPacketSize)) ;
	pHostDevice = new USBMassBulkStorageDisk(this);
}

USBulkDisk::~USBulkDisk()
{
  if(pRawAlignedBuffer)
		KernelDMM::Instance().free((uintptr_t)pRawAlignedBuffer);

  if(pSCSIDeviceList)
    KernelDMM::Instance().free((uintptr_t)pSCSIDeviceList);

  delete pHostDevice;
}

void USBulkDisk::Initialize()
{
  byte bStatus = pHostDevice->DoReset() ;
  if(bStatus != USBMassBulkStorageDisk_SUCCESS)
    throw upan::exception(XLOC, "Failed to reset");

  pSCSIDeviceList = (SCSIDevice**)KernelDMM::Instance().allocate(sizeof(SCSIDevice**) * (bMaxLun + 1)) ;
  int iLun ;
  for(iLun = 0; iLun <= bMaxLun; iLun++)
    pSCSIDeviceList[ iLun ] = nullptr ;

  char szName[10];
  bool bDeviceFound = false;

  //TODO: For the time configuring only LUN 0
  for(iLun = 0; iLun <= 0/*bMaxLun*/; iLun++)
  {
    SCSIDevice* pSCSIDevice = SCSIHandler_ScanDevice(pHostDevice, 0, 0, iLun) ;
    pSCSIDeviceList[ iLun ] = pSCSIDevice ;

    if(pSCSIDevice == NULL)
    {
      printf("\n No SCSI USB Mass Bulk Storage Device @ LUN: %d", iLun) ;
      continue ;
    }

    sprintf(szName, "usbdisk%d", USBDiskDriver::NextDeviceId());
    pHostDevice->AddDeviceDrive(StorageDriveManager::Instance().CreateRawDisk(szName, RawStorageDrive::USB_SCSI_DISK, pSCSIDevice));
    bDeviceFound = true ;
  }

  if(!bDeviceFound)
    throw upan::exception(XLOC, "No USB Bulk Storage Device Found. Driver setup failed");
}
