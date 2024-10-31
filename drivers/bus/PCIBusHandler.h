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
#ifndef _PCI_BUS_HANDLER_H_
#define _PCI_BUS_HANDLER_H_

#include <Global.h>
#include <list.h>

typedef enum {
  PCI_TYPE_ONE = 1,
  PCI_TYPE_TWO = 2,
} PCI_TYPE;

typedef enum {
  XHCI_IRQ_NO = 3,
  WIFINET_IRQ_NO = 4
} PCI_IRQ;

#define PCI_VENDOR_ID_FOXCONN 0x105b
#define PCI_VENDOR_ID_WNC 0x185F
#define PCI_VENDOR_ID_LITEON 0x11AD
#define PCI_VENDOR_ID_XAVI 0x1B9A
#define PCI_VENDOR_ID_AZWAVE 0x1A3B
#define PCI_VENDOR_ID_ATHEROS 0x168C
#define PCI_VENDOR_ID_HP 0x103C
#define PCI_VENDOR_ID_DELL 0x1028

#define PCI_REG_1	0xCFB
#define PCI_REG_2	0xCF8
#define PCI_REG_3	0xCFA
#define PCI_REG_4	0xCFC

#define PCI_VENDOR_ID		0x00
#define PCI_DEVICE_ID		0x02
#define PCI_COMMAND			0x04
#define PCI_STATUS			0x06
#define PCI_REVISION		0x08
#define PCI_INTERFACE		0x09
#define PCI_SUB_CLASS		0x0A
#define PCI_CLASS_CODE		0x0B
#define PCI_LATENCY			0x0D
#define PCI_HEADER_TYPE		0x0E
#define PCI_BASE_REGISTERS	0x10
#define PCI_SUBSYS_VENDOR_ID 0x2C
#define PCI_SUBSYS_DEVICE_ID 0x2E
#define PCI_CAPLIST         0x34
#define PCI_INTERRUPT_LINE	0x3C
#define PCI_INTERRUPT_PIN	0x3D
#define PCI_MIN_GRANT		0x3E
#define PCI_MAX_LATENCY		0x3F
#define PCI_IRQ_ABCD    0x60
#define PCI_IRQ_EFGH    0x68

#define PCI_STS_CAPABILITIESLIST 0x10

#define PCI_STS_CAPABILITIESLIST 0x10

#define PCI_COMMAND_IO		0x001
#define PCI_COMMAND_MMIO	0x002
#define PCI_COMMAND_MASTER	0x004

#define PCI_IO_ADDRESS_SPACE		0x01
#define PCI_ADDRESS_IO_MASK		0xFFFFFFFC
#define PCI_ADDRESS_MEMORY_32_MASK 	0xFFFFFFF0
#define PCI_RESOURCE_TYPE_IO	0x100

#define PCI_HEADER_BRIDGE   0x01
#define PCI_MULTIFUNCTION   0x80

#define PCI_BUS_PRIMARY		0x18
#define PCI_BUS_SECONDARY	0x19
#define PCI_BUS_SUBORDINATE	0x1A

#define MAX_PCI_DEVICES		255

#define PCI_MASS_STORAGE	0x01
#define PCI_IDE				0x01

#define PCI_SERIAL_BUS		0x0C
#define PCI_USB				0x03

// MSI
#define MSI_CAP_ID 5
#define MSI_ENABLED 1
#define MSI_64BITADDR 0x80
#define PCI_INTERRUPTDISABLE 0x400

#define PCI_CACHE_LINE_SIZE 0x0C
#define PCI_LATENCY_TIMER 0x0D

// Vendor IDs
#define INTEL_VENDOR_ID 0x8086

class PCIEntry {
  public:
	unsigned uiBusNumber;
	unsigned uiDeviceNumber;
	unsigned uiFunction;

	unsigned short usVendorID;
	unsigned short usDeviceID;
	unsigned short usCommand;
	unsigned short usStatus;

	byte bRevisionID;

	byte bInterface;
	byte bClassCode;
	byte bSubClass;

	byte bCacheLineSize;
	byte bLatency;
	byte bHeaderType;
	byte bSelfTestResult;

	union
	{
		struct 
		{
			unsigned uiBaseAddress0;
			unsigned uiBaseAddress1;
			unsigned uiBaseAddress2;
			unsigned uiBaseAddress3;
			unsigned uiBaseAddress4;
			unsigned uiBaseAddress5;

			unsigned uiCardBusCIS;

			unsigned short usSubsystemVendorID;
			unsigned short usSubsystemDeviceID;
		
			unsigned uiExpansionROM;
			
			byte bCapabilityPointer;
			
			byte bReserved1[3];
			unsigned uiReserved2[1];

			byte bInterruptLine;
			byte bInterruptPin;
			byte bMinDMAGrant;
			byte bMaxDMALatency;

			unsigned uiDeviceSpecific[48];
		} NonBridge;
		
		struct
		{
			unsigned uiBaseAddress0;
			unsigned uiBaseAddress1;
			
			byte bPrimaryBus;
			byte bSecondaryBus;
			byte bSubordinateBus;
			
			byte bSecondaryLatency;

			byte bIOBaseLow;
			byte bIOLimitLow;
	
			unsigned short usSecondaryStatus;

			unsigned short usMemoryBaseLow;
			unsigned short usMemoryLimitLow;
			unsigned short usPrefetchBaseLow;
			unsigned short usPrefetchLimitLow;		

			unsigned uiPrefetchBaseHigh;
			unsigned uiPrefetchLimitHigh;

			unsigned short usIOBaseHigh;
			unsigned short usIOLimitHigh;

			unsigned uiReserved[1];

			byte bInterruptLine;
			byte bInterruptPin;
			
			unsigned short usBridgeControl;
			unsigned uiDeviceSpecifix[48];
		} Bridge;
			
		struct
		{
			unsigned uiExCaBase;

			byte bCapabilityPointer;
			byte bReserved05;
			
			unsigned short usSecondaryStatus;
			
			byte bPCIBus;
			byte bCardBus;
			byte bSubordinateBus;

			byte bLatencyTimer;
			
			unsigned uiMemoryBase0;
			unsigned uiMemoryLimit0;
			unsigned uiMemoryBase1;
			unsigned uiMemoryLimit1;

			unsigned short usIOBase0Low;
			unsigned short usIOBase0High;
			unsigned short usIOLimit0Low;
			unsigned short usIOLimit0High;

			unsigned short usIOBase1Low;
			unsigned short usIOBase1High;
			unsigned short usIOLimit1Low;
			unsigned short usIOLimit1High;

			byte bInterruptLine;
			byte bInterruptPin;
	
			unsigned short usBridgeControl;
			unsigned short usSubsystemVendorID;
			unsigned short usSubsystemDeviceID;

			unsigned uiLegacyBaseAddr;
			unsigned uiCardBusReserved[14];
			unsigned uiVendorSpecific[32];
		} CardBus;
	} BusEntity;

  uint64_t GetIOMapAddress() const;
  void ReadPCIConfig(unsigned uiPCIEntryOffset, unsigned uiPCIEntrySize, void* pValue) const;
  void WritePCIConfig(unsigned uiPCIEntryOffset, unsigned uiPCIEntrySize, unsigned uiValue) const;
  unsigned GetPCIMemSize(int iAddressIndex) const;
  unsigned PCISize(unsigned uiBase, unsigned uiMask) const;
  void EnableBusMaster() const;
  bool SetupMsiInterrupt(const PCI_IRQ irqNo) const;
  void SwitchToMsi() const;

  PCIEntry(unsigned uiBusNumber, unsigned uiDeviceNumber, unsigned uiFunction, byte bHeaderType);

  class ExtendedCapability
  {
    public:
      ExtendedCapability(uint8_t capBase, uint8_t capId) : _capBase(capBase), _capId(capId) {}
      uint8_t CapBase() const { return _capBase; }
      uint8_t CapId() const { return _capId; }

    private:
      uint8_t _capBase;
      uint8_t _capId;
  };
  const ExtendedCapability* GetExtendedCapability(uint32_t capId) const;

  private:
    void ReadNonBridgePCIHeader();
    void ReadBridgePCIHeader();
    upan::list<ExtendedCapability> _extendCapabilities;
}; 

class PCIBusHandler
{
  private:
    PCIBusHandler();
  public:
    static PCIBusHandler& Instance()
    {
      static PCIBusHandler instance;
      return instance;
    }
    void Initialize();
    void ReadPCIConfig(unsigned uiBusNumber, unsigned uiDeviceNumber, unsigned uiFunction,
                       unsigned uiPCIEntryOffset, unsigned uiPCIEntrySize, void* pValue);
    void WritePCIConfig(unsigned uiBusNumber, unsigned uiDeviceNumber, unsigned uiFunction,
                        unsigned uiPCIEntryOffset, unsigned uiPCIEntrySize, unsigned uiValue);
    const upan::list<PCIEntry*>& PCIEntries() const { return _pciEntries; }
    const upan::list<uint8_t>& IrqABCD() const { return _irqABCD; }
    const upan::list<uint8_t>& IrqEFGH() const { return _irqEFGH; }
  private:
    void Find();
    void ScanBus(unsigned uiBusNumber);

    PCI_TYPE _type;
    unsigned _uiNoOfPCIBuses;
    upan::list<PCIEntry*> _pciEntries;
    upan::list<uint8_t>   _irqABCD;
    upan::list<uint8_t>   _irqEFGH;
};

#endif
