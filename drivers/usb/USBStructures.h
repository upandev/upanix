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
#ifndef _USB_STRUCTURES_H_
#define _USB_STRUCTURES_H_

#include <Global.h>
#include <USBConstants.h>
#include <SCSIHandler.h>
#include <exception.h>
#include <ustring.h>

class USBMassBulkStorageDisk;

typedef enum
{
	UHCI_CONTROLLER = 100,
	EHCI_CONTROLLER = 101,
} USB_CONTROLLER_TYPE;

typedef struct
{
	byte bRequestType;
	byte bRequest;
	unsigned short usWValue;
	unsigned short usWIndex;
	unsigned short usWLength;
} PACKED USBDevRequest;

class USBStandardDeviceDesc
{
  public:
  USBStandardDeviceDesc();
  void DebugPrint() const;

	byte bLength;
	byte bDescType;
	unsigned short bcdUSB;
	byte bDeviceClass;
	byte bDeviceSubClass;
	byte bDeviceProtocol;
	byte bMaxPacketSize0;

	short sVendorID;
	short sProductID;
	short bcdDevice;
	char indexManufacturer;
	char indexProduct;
	char indexSerialNum;
	char bNumConfigs;
} PACKED;

class USBStandardEndPt
{
  public:
  enum DirectionTypes { IN, OUT, BI };
  enum Types { CONTROL, ISOCHRONOUS, BULK, INTERRUPT };

  USBStandardEndPt();
  void DebugPrint();

  DirectionTypes Direction() const { return bEndpointAddress & 0x80 ? IN : OUT; }
  uint32_t Address() const { return bEndpointAddress & 0xF; }
  Types Type() const
  {
    int type = bmAttributes & 0x03;
    switch(type)
    {
    case 0: return Types::CONTROL;
    case 1: return Types::ISOCHRONOUS;
    case 2: return Types::BULK;
    case 3: return Types::INTERRUPT;
    }
    throw upan::exception(XLOC, "Invalid endpoint type: %d", type);
  }

	char bLength;
	char bDescriptorType;
	byte bEndpointAddress;
	char bmAttributes;
	unsigned short wMaxPacketSize;
	char bInterval;
} PACKED;

class USBStandardInterface
{
  public:
  USBStandardInterface();
  void DebugPrint();

	char bLength;
	char bDescriptorType;
	char bInterfaceNumber;
	char bAlternateSetting;
	char bNumEndpoints;
	byte bInterfaceClass;
	byte bInterfaceSubClass;
	byte bInterfaceProtocol;
	char iInterface;
	USBStandardEndPt* pEndPoints;
} PACKED;

typedef struct
{
	byte bLength;
	byte bDescriptorType;
	unsigned short* usLangID;
} USBStringDescZero;

class USBStandardConfigDesc
{
  public:
  USBStandardConfigDesc();
  void DebugPrint();

	char bLength;
	char bDescriptorType;
	unsigned short wTotalLength;
	char bNumInterfaces;
	char bConfigurationValue;
	char iConfiguration;
	byte bmAttributes;
	char bMaxPower;
	USBStandardInterface* pInterfaces;
} PACKED;

class USBDevice;

class USBulkDisk
{
  public:
    USBulkDisk(USBDevice*, int interfaceIndex, byte maxLun, const upan::string& protoName);
    ~USBulkDisk();
    void Initialize();
    byte MaxLun() const { return bMaxLun; }

  public:
    USBDevice* _usbDevice;
    byte bEndPointInToggle;
    byte bEndPointOutToggle;
    unsigned uiEndPointIn;
    unsigned uiEndPointOut;
    unsigned short usInMaxPacketSize;
    unsigned short usOutMaxPacketSize;
    USBStandardEndPt* pEndPointInt;
    byte* pRawAlignedBuffer;
    SCSIDevice** pSCSIDeviceList;

  private:
    const byte bMaxLun;
    const upan::string szProtocolName;
    USBMassBulkStorageDisk* pHostDevice;
};

class USBDriver
{
public:
  USBDriver(const upan::string& name) : _name(name) {}
  const upan::string& Name() const { return _name; }
  virtual bool AddDevice(USBDevice* d) = 0;
  virtual void RemoveDevice(USBDevice* d) = 0;

protected:
  const upan::string _name;
};

#endif
