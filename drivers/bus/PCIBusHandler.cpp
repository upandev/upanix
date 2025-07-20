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
#include <PCIBusHandler.h>
#include <PortCom.h>
#include <DMM.h>
#include <KernelService.h>
#include <Bit.h>
#include <IrqManager.h>
#include <Apic.h>
#include <exception.h>

PCIBusHandler::PCIBusHandler() : _type(PCI_TYPE_ONE), _uiNoOfPCIBuses(0)
{
}

void PCIBusHandler::Initialize()
{
  for(auto p : _pciEntries)
    delete p;
  _uiNoOfPCIBuses = 0;
  ReturnCode result = Success;

  try
  {
    Find();

    printf("\n\tPCI Configuration Mechanism# %d", _type);
    printf("\n\tScanning PCI Bus for devices...");

    ScanBus(0);

    printf("\n\tPCI Bus Scan Successfull");
    printf("\n\tFollowing devices are found: ");

    int i = 0;
    for(auto p : _pciEntries)
      printf("\n%d) Vendor = %u Dev = %u If = %u CCode = %u SubCCode = %u", ++i, p->usVendorID, p->usDeviceID, p->bInterface, p->bClassCode, p->bSubClass);
  }
  catch(const upan::exception& e)
  {
    result = Failure;
    e.Print();
  }

  KC::MConsole().LoadMessage("PCI Bus Initialization", result) ;
}

void PCIBusHandler::Find()
{
	unsigned uiData ;

    /* Check if Configuration 1 is Supported */
	PortCom_SendByte(PCI_REG_1, 0x01) ;

	uiData = PortCom_ReceiveDoubleWord(PCI_REG_2) ;
	PortCom_SendDoubleWord(PCI_REG_2, 0x80000000) ;

	if(PortCom_ReceiveDoubleWord(PCI_REG_2) == 0x80000000)
	{
		PortCom_SendDoubleWord(PCI_REG_2, uiData) ;
		_type = PCI_TYPE_ONE ;
    return;
	}

	PortCom_SendDoubleWord(PCI_REG_2, uiData) ;

    /* Check if Configuration 2 is Supported */
	PortCom_SendByte(PCI_REG_1, 0x00) ;
	PortCom_SendByte(PCI_REG_2, 0x00) ;
	PortCom_SendByte(PCI_REG_3, 0x00) ;

	if(PortCom_ReceiveByte(PCI_REG_2) == 0x00 && PortCom_ReceiveByte(PCI_REG_3) == 0x00)
	{
		_type = PCI_TYPE_TWO ;
    return;
	}

  throw upan::exception(XLOC, "No PCI Installed");
}

void PCIBusHandler::ScanBus(unsigned uiBusNumber)
{
	unsigned uiDevicePerBus = (_type == PCI_TYPE_ONE) ? 32 : 16 ;
	unsigned uiDeviceNumber, uiFunction ;
	unsigned short usCommand ;
	unsigned short usVendorID ;
	byte bHeaderType = 0 ;

	_uiNoOfPCIBuses++ ;

	for(uiDeviceNumber = 0; uiDeviceNumber < uiDevicePerBus; uiDeviceNumber++)
	{
		for(uiFunction = 0; uiFunction < 8; uiFunction++)
		{
      ReadPCIConfig(uiBusNumber, uiDeviceNumber, uiFunction, PCI_VENDOR_ID, 2, &usVendorID);
		
			if(usVendorID != 0xFFFF && usVendorID != 0x0000) 
			{
				if(uiFunction == 0)
				{
          ReadPCIConfig(uiBusNumber, uiDeviceNumber, uiFunction, PCI_HEADER_TYPE, 1, &bHeaderType);
				}
				else
				{
					if((bHeaderType & PCI_MULTIFUNCTION) == 0)
					{
						continue ;
					}
				}

				if(bHeaderType & PCI_HEADER_BRIDGE)
				{
          ReadPCIConfig(uiBusNumber, uiDeviceNumber, uiFunction, PCI_COMMAND, 2, &usCommand);

          WritePCIConfig(uiBusNumber, uiDeviceNumber, uiFunction, PCI_COMMAND, 2, (usCommand & ~(PCI_COMMAND_IO | PCI_COMMAND_MMIO)));

          WritePCIConfig(uiBusNumber, uiDeviceNumber, uiFunction, PCI_BUS_PRIMARY, 1, uiBusNumber);

          WritePCIConfig(uiBusNumber, uiDeviceNumber, uiFunction, PCI_BUS_SECONDARY, 1, _uiNoOfPCIBuses);

          WritePCIConfig(uiBusNumber, uiDeviceNumber, uiFunction, PCI_BUS_SUBORDINATE, 1, 0xFF);

          ScanBus(_uiNoOfPCIBuses);

          WritePCIConfig(uiBusNumber, uiDeviceNumber, uiFunction, PCI_BUS_SUBORDINATE, 1, _uiNoOfPCIBuses - 1);

          ReadPCIConfig(uiBusNumber, uiDeviceNumber, uiFunction, PCI_COMMAND, 2, &usCommand);

          WritePCIConfig(uiBusNumber, uiDeviceNumber, uiFunction, PCI_COMMAND, 2, (usCommand | PCI_COMMAND_IO | PCI_COMMAND_MMIO));
				}
        else
        {
          byte bInterface, bClassCode, bSubClass;
          ReadPCIConfig(uiBusNumber, uiDeviceNumber, uiFunction, PCI_INTERFACE, 1, &bInterface);
          ReadPCIConfig(uiBusNumber, uiDeviceNumber, uiFunction, PCI_CLASS_CODE, 1, &bClassCode);
          ReadPCIConfig(uiBusNumber, uiDeviceNumber, uiFunction, PCI_SUB_CLASS, 1, &bSubClass);
          
          if(bClassCode == 0x06 && (bSubClass == 0x01 || bSubClass == 0x02) && bInterface == 0x00)
          {
            uint32_t irqABCD;
            ReadPCIConfig(uiBusNumber, uiDeviceNumber, uiFunction, PCI_IRQ_ABCD, 4, &irqABCD);

            _irqABCD.push_back(Bit::Byte1(irqABCD));
            _irqABCD.push_back(Bit::Byte2(irqABCD));
            _irqABCD.push_back(Bit::Byte3(irqABCD));
            _irqABCD.push_back(Bit::Byte4(irqABCD));

            uint32_t irqEFGH;
            ReadPCIConfig(uiBusNumber, uiDeviceNumber, uiFunction, PCI_IRQ_EFGH, 4, &irqEFGH);
            _irqEFGH.push_back(Bit::Byte1(irqEFGH));
            _irqEFGH.push_back(Bit::Byte2(irqEFGH));
            _irqEFGH.push_back(Bit::Byte3(irqEFGH));
            _irqEFGH.push_back(Bit::Byte4(irqEFGH));
          }
        }

        if(_pciEntries.size() < MAX_PCI_DEVICES)
          _pciEntries.push_back(new PCIEntry(uiBusNumber, uiDeviceNumber, uiFunction, bHeaderType));
        else
          printf("\n Too many PCI devices!!!");
	   	}
		}
  }
}

void PCIBusHandler::ReadPCIConfig(unsigned uiBusNumber, unsigned uiDeviceNumber, unsigned uiFunction, unsigned uiPCIEntryOffset, unsigned uiPCIEntrySize, void* pValue)
{
//TODO: PCI TYPE = 0 => Using BIOS PCI Interface
	if(_type == PCI_TYPE_ONE)
	{
		PortCom_SendDoubleWord(PCI_REG_2, (0x80000000 
			| (uiBusNumber << 16) 
			| (uiDeviceNumber << 11)
			| (uiFunction << 8)
			| (uiPCIEntryOffset & ~3))) ;

		switch(uiPCIEntrySize)
		{
			case 1:
				*((byte*)pValue) = PortCom_ReceiveByte(PCI_REG_4 + (uiPCIEntryOffset & 0x3)) ;
				break ;

			case 2:
				*((unsigned short*)pValue) = PortCom_ReceiveWord(PCI_REG_4 + (uiPCIEntryOffset & 0x2)) ;
				break ;
		
			case 4:
				*((unsigned*)pValue) = PortCom_ReceiveDoubleWord(PCI_REG_4) ;
				break ;

			default:
        throw upan::exception(XLOC, "Invalid PCIEntrySize: %d for READ from Type 1 PCI", uiPCIEntrySize);
		}
	}
	else if(_type == PCI_TYPE_TWO)
	{
		PortCom_SendByte(PCI_REG_2, 0xF0 | (uiFunction << 1)) ;
		PortCom_SendByte(PCI_REG_3, uiBusNumber) ;

		switch(uiPCIEntrySize)
		{
			case 1:
				*((byte*)pValue) = PortCom_ReceiveByte(0xC000 | (uiDeviceNumber << 8) | uiPCIEntryOffset) ;
				break ;

			case 2:
				*((unsigned short*)pValue) = PortCom_ReceiveWord(0xC000 | (uiDeviceNumber << 8) | uiPCIEntryOffset) ;
				break ;

			case 4:
				*((unsigned*)pValue) = PortCom_ReceiveDoubleWord(0xC000 | (uiDeviceNumber << 8) | uiPCIEntryOffset) ;
				break ;

			default:
        throw upan::exception(XLOC, "Invalid PCIEntrySize: %d for READ from Type 2 PCI", uiPCIEntrySize);
		}

		PortCom_SendByte(PCI_REG_2, 0x00) ;
	}
  else
    throw upan::exception(XLOC, "Unsupport PCI Type: %d for READ", _type);
}

void PCIBusHandler::WritePCIConfig(unsigned uiBusNumber, unsigned uiDeviceNumber, unsigned uiFunction, unsigned uiPCIEntryOffset, unsigned uiPCIEntrySize, unsigned uiValue)
{
//TODO: PCI TYPE = 0 => Using BIOS PCI Interface

	if(_type == PCI_TYPE_ONE) 
	{
		PortCom_SendDoubleWord(PCI_REG_2, (0x80000000
			| (uiBusNumber << 16)
			| (uiDeviceNumber << 11)
			| (uiFunction << 8)
			| (uiPCIEntryOffset & ~0x3))) ;
	
		switch(uiPCIEntrySize)
		{
			case 1:
				PortCom_SendByte(PCI_REG_4 + (uiPCIEntryOffset & 0x3), uiValue) ;
				break ;

			case 2:
				PortCom_SendWord(PCI_REG_4 + (uiPCIEntryOffset & 0x2), uiValue) ;
				break ;

			case 4:
				PortCom_SendDoubleWord(PCI_REG_4, uiValue) ;
				break ;
			
			default:
        throw upan::exception(XLOC, "Invalid PCIEntrySize: %d for WRITE from Type 1 PCI", uiPCIEntrySize);
		}
	}
	else if(_type == PCI_TYPE_TWO)
	{
		PortCom_SendByte(PCI_REG_2, 0xF0 | (uiFunction << 1)) ;
		PortCom_SendByte(PCI_REG_3, uiBusNumber) ;

		switch(uiPCIEntrySize)
		{
			case 1:
				PortCom_SendByte(0xC000 | (uiDeviceNumber << 8) | uiPCIEntryOffset, uiValue) ;
				break ;
			
			case 2:
				PortCom_SendWord(0xC000 | (uiDeviceNumber << 8) | uiPCIEntryOffset, uiValue) ;
				break ;

			case 4:
				PortCom_SendDoubleWord(0xC000 | (uiDeviceNumber << 8) | uiPCIEntryOffset, uiValue) ;
				break ;
	
			default:
        throw upan::exception(XLOC, "Invalid PCIEntrySize: %d for WRITE from Type 2 PCI", uiPCIEntrySize);
		}

		PortCom_SendByte(PCI_REG_2, 0x00) ;
	}
  else
    throw upan::exception(XLOC, "Unsupport PCI Type: %d for WRITE", _type);
}

PCIEntry::PCIEntry(unsigned uiBusNo, unsigned uiDeviceNo, unsigned uiFunc, byte bHeaderType)
  : uiBusNumber(uiBusNo), uiDeviceNumber(uiDeviceNo), uiFunction(uiFunc)
{
  try
  {
    if(bHeaderType & PCI_HEADER_BRIDGE)
      ReadBridgePCIHeader();
    else
      ReadNonBridgePCIHeader();
  }
  catch(const upan::exception& e)
  {
    throw upan::exception(XLOC, "failed to read PCI configuration for Bus:%u, Device:%u, Function:%u, HeaderType:%u - Reason:%s",
                          uiBusNumber, uiDeviceNumber, uiFunction, bHeaderType, e.ErrorMsg().c_str());
  }
}

uint64_t PCIEntry::GetIOMapAddress() const {
  uint64_t bar0 = BusEntity.NonBridge.uiBaseAddress0;
  uint32_t addrType = (bar0 >> 1 & 0x3);
  printf("\n PCI BaseAddr Type: %d", addrType);
  if (addrType == 2) {
    uint64_t bar1 = BusEntity.NonBridge.uiBaseAddress1;
    return ((bar1 & PCI_ADDRESS_MEMORY_32_MASK) << 32) | (bar0 & PCI_ADDRESS_MEMORY_32_MASK);
  } else {
    return bar0 & PCI_ADDRESS_MEMORY_32_MASK;
  }
}

void PCIEntry::ReadPCIConfig(unsigned uiPCIEntryOffset, unsigned uiPCIEntrySize, void* pValue) const
{
  PCIBusHandler::Instance().ReadPCIConfig(uiBusNumber, uiDeviceNumber, uiFunction, uiPCIEntryOffset, uiPCIEntrySize, pValue);
}

void PCIEntry::WritePCIConfig(unsigned uiPCIEntryOffset, unsigned uiPCIEntrySize, unsigned uiValue) const
{
  PCIBusHandler::Instance().WritePCIConfig(uiBusNumber, uiDeviceNumber, uiFunction, uiPCIEntryOffset, uiPCIEntrySize, uiValue);
}

void PCIEntry::ReadNonBridgePCIHeader()
{
  ReadPCIConfig(PCI_VENDOR_ID, 2, &usVendorID);

  ReadPCIConfig(PCI_DEVICE_ID, 2, &usDeviceID);

  ReadPCIConfig(PCI_COMMAND, 2, &usCommand);

  ReadPCIConfig(PCI_STATUS, 2, &usStatus);

  ReadPCIConfig(PCI_REVISION, 1, &bRevisionID);

  ReadPCIConfig(PCI_INTERFACE, 1, &bInterface);

  ReadPCIConfig(PCI_CLASS_CODE, 1, &bClassCode);

  ReadPCIConfig(PCI_SUB_CLASS, 1, &bSubClass);

  ReadPCIConfig(PCI_HEADER_TYPE, 1, &bHeaderType);

  ReadPCIConfig(PCI_BASE_REGISTERS + 0, 4, &BusEntity.NonBridge.uiBaseAddress0);

  ReadPCIConfig(PCI_BASE_REGISTERS + 4, 4, &BusEntity.NonBridge.uiBaseAddress1);

  ReadPCIConfig(PCI_BASE_REGISTERS + 8, 4, &BusEntity.NonBridge.uiBaseAddress2);

  ReadPCIConfig(PCI_BASE_REGISTERS + 12, 4, &BusEntity.NonBridge.uiBaseAddress3);

  ReadPCIConfig(PCI_BASE_REGISTERS + 16, 4, &BusEntity.NonBridge.uiBaseAddress4);

  ReadPCIConfig(PCI_BASE_REGISTERS + 20, 4, &BusEntity.NonBridge.uiBaseAddress5);

  ReadPCIConfig(PCI_SUBSYS_VENDOR_ID, 2, &BusEntity.NonBridge.usSubsystemVendorID);

  ReadPCIConfig(PCI_SUBSYS_DEVICE_ID, 2, &BusEntity.NonBridge.usSubsystemDeviceID);
	
  ReadPCIConfig(PCI_INTERRUPT_LINE, 1, &BusEntity.NonBridge.bInterruptLine);

  ReadPCIConfig(PCI_INTERRUPT_PIN, 1, &BusEntity.NonBridge.bInterruptPin);

  ReadPCIConfig(PCI_MIN_GRANT, 1, &BusEntity.NonBridge.bMinDMAGrant);

  ReadPCIConfig(PCI_MAX_LATENCY, 1, &BusEntity.NonBridge.bMaxDMALatency);

  if(usStatus & PCI_STS_CAPABILITIESLIST)
  {
    uint8_t capBase = 0;
    ReadPCIConfig(PCI_CAPLIST, 1, &capBase);
    while(capBase)
    {
      uint8_t capId = 0;
      ReadPCIConfig(capBase, 1, &capId);
      _extendCapabilities.push_back(ExtendedCapability(capBase, capId));
      ReadPCIConfig(capBase + 1, 1, &capBase);
    }
  }
}

const PCIEntry::ExtendedCapability* PCIEntry::GetExtendedCapability(uint32_t capId) const
{
  for(const auto& i : _extendCapabilities)
    if(i.CapId() == capId)
      return &i;
  return nullptr;
}

void PCIEntry::ReadBridgePCIHeader()
{
  ReadPCIConfig(PCI_VENDOR_ID, 2, &usVendorID);

  ReadPCIConfig(PCI_DEVICE_ID, 2, &usDeviceID);

  ReadPCIConfig(PCI_COMMAND, 2, &usCommand);

  ReadPCIConfig(PCI_STATUS, 2, &usStatus);

  ReadPCIConfig(PCI_REVISION, 1, &bRevisionID);

  ReadPCIConfig(PCI_INTERFACE, 1, &bInterface);

  ReadPCIConfig(PCI_CLASS_CODE, 1, &bClassCode);

  ReadPCIConfig(PCI_SUB_CLASS, 1, &bSubClass);

  ReadPCIConfig(PCI_HEADER_TYPE, 1, &bHeaderType);

  ReadPCIConfig(PCI_BASE_REGISTERS + 0, 4, &BusEntity.Bridge.uiBaseAddress0);

  ReadPCIConfig(PCI_BASE_REGISTERS + 4, 4, &BusEntity.Bridge.uiBaseAddress1);

  ReadPCIConfig(PCI_BUS_PRIMARY, 1, &BusEntity.Bridge.bPrimaryBus);

  ReadPCIConfig(PCI_BUS_SECONDARY, 1, &BusEntity.Bridge.bSecondaryBus);

  ReadPCIConfig(PCI_BUS_SUBORDINATE, 1, &BusEntity.Bridge.bSubordinateBus);
}

unsigned PCIEntry::GetPCIMemSize(int iAddressIndex) const
{
  unsigned uiOffset = PCI_BASE_REGISTERS + iAddressIndex * 4;
	unsigned uiBase, uiSize ;

	ReadPCIConfig(uiOffset, 4, &uiBase);
	WritePCIConfig(uiOffset, 4, 0xFFFFFFFF);

	ReadPCIConfig(uiOffset, 4, &uiSize);
	WritePCIConfig(uiOffset, 4, uiBase);

  if(!uiSize || uiSize == 0xFFFFFFFF)
    return 0 ;

  // Why is this required !!
  if(uiBase == 0xFFFFFFFF)
    uiBase = 0;

  if(!(uiSize & PCI_IO_ADDRESS_SPACE))
    return PCISize(uiSize, PCI_ADDRESS_MEMORY_32_MASK) ;
  else
    return PCISize(uiSize, PCI_ADDRESS_IO_MASK & 0xFFFF) ;
}

unsigned PCIEntry::PCISize(unsigned uiBase, unsigned uiMask) const
{
  unsigned uiSize = uiBase & uiMask ;
  return uiSize & ~( uiSize - 1 );
}

void PCIEntry::EnableBusMaster() const
{
	unsigned short usCmd ;
	ReadPCIConfig(PCI_COMMAND, 2, &usCmd);
	if(!(usCmd & PCI_COMMAND_MASTER))
	{
		printf("\nPCI: Enabling Bus Master for Device: %x", uiDeviceNumber) ;
		usCmd |= PCI_COMMAND_MASTER ;
		WritePCIConfig(PCI_COMMAND, 2, usCmd);
	}
	// SetBios Bus Master
	byte bLat ;
	ReadPCIConfig(PCI_LATENCY, 1, &bLat);
	if(bLat > 255)
		bLat = 255 ;
	else if(bLat < 16)
		bLat = 64;
	else
		return;
	printf("\nPCI: Setting latency timer of device: %x to %u", uiDeviceNumber, bLat);
	WritePCIConfig(PCI_LATENCY, 1, bLat);
}

bool PCIEntry::SetupMsiInterrupt(const PCI_IRQ irqNo) const
{
  if(!IrqManager::Instance().IsApic())
    return false;

  auto msiCap = GetExtendedCapability(MSI_CAP_ID);
  if(!msiCap)
  {
    printf("\n MSI Capability not found");
    return false;
  }
  const uint32_t msiBase = msiCap->CapBase();

  uint8_t MESSAGEADDRLO;
  uint8_t MESSAGEADDRHI;
  uint8_t MESSAGEDATA;
  uint8_t MESSAGECONTROL;
  MESSAGECONTROL = msiBase + 2; // Message Control (bit0: MSI on/off, bit7: 0: 32-bit-, 1: 64-bit-addresses)
  MESSAGEADDRLO = msiBase + 4;  // Message Address

  uint16_t msiMessageControl;
  ReadPCIConfig(MESSAGECONTROL, 2, &msiMessageControl); // xHCI spec, Figure 49 (5.2.6.1 MSI configuration)

  if (msiMessageControl & MSI_64BITADDR)
  {
    MESSAGEADDRHI = msiBase + 8; // Message Upper Address
    MESSAGEDATA = msiBase + 12;  // Message Data
  }
  else
  {
    MESSAGEADDRHI = 0;     // Does not exist
    MESSAGEDATA = msiBase + 8; // Message Data
  }

  uint16_t data;
  ReadPCIConfig(MESSAGEDATA, 2, &data);
  data = (data & 0x3800) | (irqNo + IrqManager::IRQ_BASE);// "Reserved fields are not assumed to be any value. Software must preserve their contents on writes."

  /* Intel 3A, 10.11.1 Message Address Register Format:
     Bits 31-20 <97> These bits contain a fixed value for interrupt messages (0FEEH).
     This value locates interrupts at the 1-MByte area with a base address of 4G <96> 18M.
     All accesses to this region are directed as interrupt messages.
     Care must be taken to ensure that no other device claims the region as I/O space. */
  const Apic& apic = dynamic_cast<const Apic&>(IrqManager::Instance());
  const uint32_t address = apic.PhyApicBase() | apic.GetLocalApicID() << 12;

  WritePCIConfig(MESSAGEADDRLO, 4, address);
  if(MESSAGEADDRHI)
    WritePCIConfig(MESSAGEADDRHI, 4, 0);

  WritePCIConfig(MESSAGEDATA, 2, data);
  return true;
}

void PCIEntry::SwitchToMsi() const
{
  // Get PCI capability for MSI
  auto msiCap = GetExtendedCapability(MSI_CAP_ID);
  if(!msiCap)
    throw upan::exception(XLOC, "MSI extended capability not found!");
  const uint32_t msiBase = msiCap->CapBase();

  // Disable legacy interrupt
  uint16_t cmd;
  ReadPCIConfig(PCI_COMMAND, 2, &cmd);
  WritePCIConfig(PCI_COMMAND, 2, cmd | PCI_INTERRUPTDISABLE);

  uint8_t MESSAGECONTROL = msiBase + 2;

  // Enable the MSI interrupt mechanism by setting the MSI Enable flag in the MSI Capability Structure Message Control register
  uint16_t control;
  ReadPCIConfig(MESSAGECONTROL, 2, &control);
  WritePCIConfig(MESSAGECONTROL, 2, control | MSI_ENABLED);
}

