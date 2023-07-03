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
#include <USBStructures.h>
#include <USBDataHandler.h>
#include <stdio.h>

/***********************************************************************************************/

void USBDataHandler_InitDevDesc(USBStandardDeviceDesc* pDesc)
{
	pDesc->bLength = 0 ;
	pDesc->bDescType = 0 ;
	pDesc->bcdUSB = 0 ;
	pDesc->bDeviceClass = 0 ;
	pDesc->bDeviceSubClass = 0 ;
	pDesc->bDeviceProtocol = 0 ;
	pDesc->bMaxPacketSize0 = 0 ;

	pDesc->sVendorID = 0 ;
	pDesc->sProductID = 0 ;
	pDesc->bcdDevice = 0 ;
	pDesc->indexManufacturer = 0 ;
	pDesc->indexProduct = 0 ;
	pDesc->indexSerialNum = 0 ;
	pDesc->bNumConfigs = 0 ;
}

void USBDataHandler_CopyDevDesc(void* pDestv, const void* pSrcv, size_t iLen)
{
	USBStandardDeviceDesc* pDest = (USBStandardDeviceDesc*)pDestv ;
	USBStandardDeviceDesc* pSrc = (USBStandardDeviceDesc*)pSrcv ;

	pDest->bLength = pSrc->bLength ;
	pDest->bDescType = pSrc->bDescType ;
	pDest->bcdUSB = pSrc->bcdUSB ;
	pDest->bDeviceClass = pSrc->bDeviceClass ;
	pDest->bDeviceSubClass = pSrc->bDeviceSubClass ;
	pDest->bDeviceProtocol = pSrc->bDeviceProtocol ;
	pDest->bMaxPacketSize0 = pSrc->bMaxPacketSize0 ;

	if(iLen >= sizeof(USBStandardDeviceDesc))
	{
		pDest->sVendorID = pSrc->sVendorID ;
		pDest->sProductID = pSrc->sProductID ;
		pDest->bcdDevice = pSrc->bcdDevice ;
		pDest->indexManufacturer = pSrc->indexManufacturer ;
		pDest->indexProduct = pSrc->indexProduct ;
		pDest->indexSerialNum = pSrc->indexSerialNum ;
		pDest->bNumConfigs = pSrc->bNumConfigs ;
	}
	else
	{
		size_t iNextLen = DEF_DESC_LEN ;

		pDest->sVendorID = -1 ;
		pDest->sProductID = -1 ;
		pDest->bcdDevice = -1 ;
		pDest->indexManufacturer = -1 ;
		pDest->indexProduct = -1 ;
		pDest->indexSerialNum = -1 ;
		pDest->bNumConfigs = -1 ;

		while(true)
		{
			iNextLen += sizeof(pDest->sVendorID) ;
			if(iLen >= iNextLen) pDest->sVendorID = pSrc->sVendorID ; 
			else break ;

			iNextLen += sizeof(pDest->sProductID) ;
			if(iLen >= iNextLen) pDest->sProductID = pSrc->sProductID ;
			else break ;

			iNextLen += sizeof(pDest->bcdDevice) ;
			if(iLen >= iNextLen) pDest->bcdDevice = pSrc->bcdDevice ;
			else break ;

			iNextLen += sizeof(pDest->indexManufacturer) ;
			if(iLen >= iNextLen) pDest->indexManufacturer = pSrc->indexManufacturer ;
			else break ;

			iNextLen += sizeof(pDest->indexProduct) ;
			if(iLen >= iNextLen) pDest->indexProduct = pSrc->indexProduct ;
			else break ;

			iNextLen += sizeof(pDest->indexSerialNum) ;
			if(iLen >= iNextLen) pDest->indexSerialNum = pSrc->indexSerialNum ;
			else break ;

			iNextLen += sizeof(pDest->bNumConfigs) ;
			if(iLen >= iNextLen) pDest->bNumConfigs = pSrc->bNumConfigs ;
			else break ;

			break ;
		}
	}
}

void USBDataHandler_InitConfigDesc(USBStandardConfigDesc* pDesc)
{
	pDesc->bLength = 0 ;
	pDesc->bDescriptorType = 0 ;
	pDesc->wTotalLength = 0 ;
	pDesc->bNumInterfaces = 0 ;
	pDesc->bConfigurationValue = 0 ;
	pDesc->iConfiguration = 0 ;
	pDesc->bmAttributes = 0 ;

	pDesc->bMaxPower = 0 ;
	pDesc->pInterfaces = NULL ;
}

void USBDataHandler_CopyConfigDesc(void* pDestv, const void* pSrcv, int iLen)
{
	USBStandardConfigDesc* pDest = (USBStandardConfigDesc*)pDestv ;
	USBStandardConfigDesc* pSrc = (USBStandardConfigDesc*)pSrcv ;

	pDest->bLength = pSrc->bLength ;
	pDest->bDescriptorType = pSrc->bDescriptorType ;
	pDest->wTotalLength = pSrc->wTotalLength ;
	pDest->bNumInterfaces = pSrc->bNumInterfaces ;
	pDest->bConfigurationValue = pSrc->bConfigurationValue ;
	pDest->iConfiguration = pSrc->iConfiguration ;
	pDest->bmAttributes = pSrc->bmAttributes ;

	int iDescLen = sizeof(USBStandardConfigDesc) - sizeof(USBStandardInterface*) ;

	if(iLen >= iDescLen)
	{
		pDest->bMaxPower = pSrc->bMaxPower ;
	}
	else
	{
		int iNextLen = DEF_DESC_LEN ;

		iNextLen += sizeof(pDest->bMaxPower) ;
		pDest->bMaxPower = (iLen >= iNextLen) ? pDest->bMaxPower : -1 ;
	}
}

void USBDataHandler_CopyStrDescZero(USBStringDescZero* pDest, const void* pSrcv)
{
	USBStringDescZero* pSrc = (USBStringDescZero*)(pSrcv) ;

	pDest->bLength = pSrc->bLength ;
	pDest->bDescriptorType = pSrc->bDescriptorType ;

	pDest->usLangID = (unsigned short*)DMM_AllocateForKernel(pSrc->bLength - 2) ;

	pSrc->usLangID = (unsigned short*)((byte*)pSrc +  2) ;

	for(int i = 0; i < (pSrc->bLength - 2 ) / 2; ++i)
		pDest->usLangID[i] = pSrc->usLangID[ i ] ;
}

void USBDataHandler_DisplayStrDescZero(USBStringDescZero* pStringDescZero)
{
  printf("\n Len: %d,DType: %d", pStringDescZero->bLength, pStringDescZero->bDescriptorType);
  for(int i = 0; i < (pStringDescZero->bLength - 2) / 2; i++)
    printf(",LangId[ %d ]=0x%x", i, pStringDescZero->usLangID[i]) ;
}

void USBDataHandler_InitInterfaceDesc(USBStandardInterface* pInt)
{
	pInt->bLength = 0 ;
	pInt->bDescriptorType = 0 ;
	pInt->bInterfaceNumber = 0 ;
	pInt->bAlternateSetting = 0 ;
	pInt->bNumEndpoints = 0 ;
	pInt->bInterfaceClass = 0 ;
	pInt->bInterfaceSubClass = 0 ;
	pInt->bInterfaceProtocol = 0 ;
	pInt->iInterface = 0 ;
	pInt->pEndPoints = NULL ;
}

void USBDataHandler_InitEndPtDesc(USBStandardEndPt* pDesc)
{
	pDesc->bLength = 0 ;
	pDesc->bDescriptorType = 0 ;
	pDesc->bEndpointAddress = 0 ;
	pDesc->bmAttributes = 0 ;
	pDesc->wMaxPacketSize = 0 ;
	pDesc->bInterval = 0 ;
}

void USBDataHandler_DeAllocConfigDesc(USBStandardConfigDesc* pCD, char bNumConfigs)
{
	int index ;
	for(index = 0; index < (int)bNumConfigs; index++)
	{
		if(pCD[index].pInterfaces != NULL)
		{
			int iI ;
			for(iI = 0; iI < pCD[index].bNumInterfaces; iI++)
				DMM_DeAllocateForKernel((uintptr_t)pCD[index].pInterfaces[iI].pEndPoints) ;

			DMM_DeAllocateForKernel((uintptr_t)pCD[index].pInterfaces) ;
		}
	}
	
	DMM_DeAllocateForKernel((uintptr_t)pCD) ;
}

