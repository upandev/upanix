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
#ifndef _XHCI_STRUCTURES_H_
#define _XHCI_STRUCTURES_H_

#include <Global.h>
#include <Bit.h>
#include <exception.h>

enum USB_PROTOCOL
{
  UNKNOWN,
  USB1,
  USB2,
  USB3
};

enum DEVICE_SPEED
{
  UNDEFINED, 
  FULL_SPEED, // 12 MB/s
  LOW_SPEED, // 1.5 Mb/s
  HIGH_SPEED, // 480 Mb/s
  SUPER_SPEED // 5 Gb/s
};

class XHCICapRegister
{
  public:
    enum ISTFrameTypes { Microframes, Frames };

    void Print() const;
    uint8_t CapLength() const { return _capLength; }
    
    //HCSPARAMS1
    uint32_t MaxSlots() const { return _hcsParams1 & 0xFF; }
    uint32_t MaxIntrs() const { return (_hcsParams1 >> 8) & 0x7FF; }
    uint32_t MaxPorts() const { return (_hcsParams1 >> 24) & 0xFF; }

    //HCSPARAMS2
    ISTFrameTypes ISTFrameType() const { return _hcsParams2 & 0x8 ? Frames : Microframes; }
    uint32_t ISTValue() const { return _hcsParams2 & 0x7; }
    uint32_t ERSTMax() const { return (_hcsParams2 >> 4) & 0xF; }
    //in terms of pages
    uint32_t MaxScratchpadBufSize() const
    {
      uint32_t hi = (_hcsParams2 >> 21) & 0x1F;
      uint32_t lo = (_hcsParams2 >> 27) & 0x1F;
      return (hi << 5) | lo;
    }
    bool ScatchpadRestore() const { return Bit::IsSet(_hcsParams2, 0x04000000); }

    //HCSPARAMS3
    //U1 -> U0 in micro seconds
    uint32_t U1ExitLatency() const
    {
      uint32_t l = _hcsParams3 & 0xF;
      return l > 10 ? 10 : l;
    }
    //U2 -> U0 in micro seconds
    uint32_t U2ExitLatency() const
    {
      uint32_t l = (_hcsParams3 >> 16);
      return l > 0x7FF ? 0x7FF : l;
    }

    //HCCPARAMS1
    //64bit addressing capability
    bool IsAC64() const { return Bit::IsSet(_hccParams1, 0x1); }
    //Bandwidth negotiation capability
    bool IsBNC() const { return Bit::IsSet(_hccParams1, 0x2); }
    //64 or 32 uint8_t context data structure size
    bool IsContextSize64() const { return Bit::IsSet(_hccParams1, 0x4); }
    //Supports Port Power Control
    bool IsPPC() const { return Bit::IsSet(_hccParams1, 0x8); }
    //Supports Port Indicator
    bool IsPIND() const { return Bit::IsSet(_hccParams1, 0x10); }
    //Supports light HC reset
    bool IsLHRC() const { return Bit::IsSet(_hccParams1, 0x20); }
    //Supports latency tolerance messaging
    bool IsLTC() const { return Bit::IsSet(_hccParams1, 0x40); }
    //Supports Secondary SID
    bool IsSS() const { return !Bit::IsSet(_hccParams1, 0x80); }
    //Parses all event data TRBs after short packet while advancing to next TD
    bool IsPAE() const { return Bit::IsSet(_hccParams1, 0x100); }
    //Supports generation of Stopped - Short Packet completion code
    bool IsSPC() const { return Bit::IsSet(_hccParams1, 0x200); }
    //Supports Stopped EDTLA field
    bool IsSEC() const { return Bit::IsSet(_hccParams1, 0x400); }
    //Capable of matching Frame IDs of consecutive Isoch TDs
    bool IsCFC() const { return Bit::IsSet(_hccParams1, 0x800); }
    //Max Primary Stream Array Size
    uint32_t MaxPSASize() const { return (_hccParams1 >> 12) & 0xF; }
    //Extended capabilities pointer offset from Base
    uint32_t ECPOffset() const { return (_hccParams1 >> 14) & 0x3FFFC; }

    //DoorBell array base address from Base (cap base)
    uint32_t DoorBellOffset() const { return _doorBellOffset & ~(0x3); }

    //Runtime registers offset from Base (cap base)
    uint32_t RTSOffset() const { return _rtsOffset & ~(0x1F); }

    //HCCPARAMS2
    //U3 Entry Capability
    bool IsU3C() const { return Bit::IsSet(_hccParams2, 0x1); }
    //Configure Endpoint Command Max Exit Latency Too Large Capability
    bool IsCMC() const { return Bit::IsSet(_hccParams2, 0x2); }
    //Force Save Context Capability
    bool IsFSC() const { return Bit::IsSet(_hccParams2, 0x4); }
    //Compliance Transition Capability
    bool IsCTC() const { return Bit::IsSet(_hccParams2, 0x8); }
    //Large ESIT Payload Capability
    bool IsLEC() const { return Bit::IsSet(_hccParams2, 0x10); }
    //Configuration Information Capability
    bool IsCIC() const { return Bit::IsSet(_hccParams2, 0x20); }

  private:
    volatile uint8_t _capLength;
    volatile uint8_t _reserved;
    volatile uint16_t _hciVersion;
    volatile uint32_t _hcsParams1;
    volatile uint32_t _hcsParams2;
    volatile uint32_t _hcsParams3;
    volatile uint32_t _hccParams1;
    volatile uint32_t _doorBellOffset;
    volatile uint32_t _rtsOffset;
    volatile uint32_t _hccParams2;
} PACKED;

class XHCIPortRegister
{
  public:
    bool IsDeviceConnected() const { return Bit::IsSet(_sc, 0x1); }
    bool IsEnabled() const { return Bit::IsSet(_sc, 0x2); }
    void Reset();
    void WarmReset();
    bool IsPowerOn() const { return Bit::IsSet(_sc, 0x200); }
    void PowerOn();
    void PowerOff();
    void WakeOnConnect(bool enable)
    {
      _sc = Bit::Set(_sc, 0x2000000, enable);
    }
    void WakeOnDisconnect(bool enable)
    {
      _sc = Bit::Set(_sc, 0x4000000, enable);
    }
    void ClearPRC()
    {
      _sc |= 0x200000;
    }
    bool IsRemovableDevice() const { return Bit::IsSet(_sc, 0x40000000); }
    uint32_t PortSpeedID() const
    {
      return (_sc >> 10) & 0xF;
    }
    int32_t MaxPacketSize() const
    {
      switch(PortSpeedID())
      {
        //Low speed
        case LOW_SPEED: return 8;
        //High speed, Full speed
        case HIGH_SPEED:
        case FULL_SPEED: return 64;
        //Super speed
        case SUPER_SPEED: return 512;
      }
      return 8;
    }
    void Print();
  private:
    volatile uint32_t _sc;
    volatile uint32_t _pmsc;
    volatile uint32_t _li;
    volatile uint32_t _hlpmc;
} PACKED;

class XHCIOpRegister
{
  public:
    void Print() const;

    bool IsHCRunning() const { return Bit::IsSet(_usbCmd, 0x1); }
    void Run();
    void Stop();

    uint32_t Status() const { return _usbStatus; }
    bool StatusChanged() const { return _usbStatus & 0x41C ? true : false; }
    static bool StatusChanged(const uint32_t usbStatus) { return usbStatus & 0x41C ? true : false; }
    //write 1 to a given bit to clear
    void Clear() { _usbStatus = _usbStatus; }

    void HCReset();

    bool IsHCInterruptEnabled() const { return Bit::IsSet(_usbCmd, 0x4); }
    void EnableHCInterrupt()
    {
      if(!IsHCInterruptEnabled())
        _usbCmd = Bit::Set(_usbCmd, 0x4, true);
    }
    void DisableHCInterrupt()
    {
      if(IsHCInterruptEnabled())
        _usbCmd = Bit::Set(_usbCmd, 0x4, false);
    }

    bool IsHSEEnabled() const { return Bit::IsSet(_usbCmd, 0x8); }
    void EnableHSE()
    {
      if(!IsHSEEnabled())
        _usbCmd = Bit::Set(_usbCmd, 0x8, true);
    }
    void DisableHSE()
    {
      if(IsHSEEnabled())
        _usbCmd = Bit::Set(_usbCmd, 0x8, false);
    }

    void LightHCReset(const XHCICapRegister* cr);
    bool Saving() const { return Bit::IsSet(_usbStatus, 0x100); }
    static bool Saving(const uint32_t usbStatus) { return Bit::IsSet(usbStatus, 0x100); }

    bool Restoring() const { return Bit::IsSet(_usbStatus, 0x200); }
    static bool Restoring(const uint32_t usbStatus) { return Bit::IsSet(usbStatus, 0x200); }

    void HCSave();
    void HCRestore();

    bool IsWrapEventEnabled() { return Bit::IsSet(_usbCmd, 0x400); }
    void EnableWrapEvent()
    {
      if(!IsWrapEventEnabled())
        _usbCmd |= 0x400;
    }
    void DisableWrapEvent()
    {
      if(IsWrapEventEnabled())
        _usbCmd &= ~(0x400);
    }

    //U3 MFINDEX STOP
    bool IsU3SEnabled() const { return Bit::IsSet(_usbCmd, 0x800); }
    void EnableU3S()
    {
      if(!IsU3SEnabled())
        _usbCmd = Bit::Set(_usbCmd, 0x800, true);
    }
    void DisableU3S()
    {
      if(IsU3SEnabled())
        _usbCmd = Bit::Set(_usbCmd, 0x800, false);
    }

    bool IsShortPacketEnabled(const XHCICapRegister* cr) const;
    void EnableShortPacket(const XHCICapRegister* cr);
    void DisableShortPacket(const XHCICapRegister* cr);

    //Max Exit Latency Too Large Capability Error
    bool IsCEMEnable() const { return Bit::IsSet(_usbCmd, 0x2000); }
    void EnableCEM()
    {
      if(!IsCEMEnable())
        _usbCmd = Bit::Set(_usbCmd, 0x2000, true);
    }
    void DisableCEM()
    {
      if(IsCEMEnable())
        _usbCmd = Bit::Set(_usbCmd, 0x2000, false);
    }

    void SetDNCTRL(uint32_t val) { _dnCtrl |= val; }
    //Status

    bool IsHCHalted() const { return Bit::IsSet(_usbStatus, 0x1); }
    static bool IsHCHalted(const uint32_t usbStatus) { return Bit::IsSet(usbStatus, 0x1); }

    //Host System Error
    bool IsHSError() const { return Bit::IsSet(_usbStatus, 0x4); }
    static bool IsHSError(const uint32_t usbStatus) { return Bit::IsSet(usbStatus, 0x4); }

    bool IsAnyEventPending() const { return Bit::IsSet(_usbStatus, 0x8); }
    static bool IsAnyEventPending(const uint32_t usbStatus) { return Bit::IsSet(usbStatus, 0x8); }

    //Save/Restore Error
    bool IsSRError() const { return Bit::IsSet(_usbStatus, 0x400); }
    static bool IsSRError(const uint32_t usbStatus) { return Bit::IsSet(usbStatus, 0x400); }

    bool IsPortChanged() const { return Bit::IsSet(_usbStatus, 0x10); }
    static bool IsPortChanged(const uint32_t usbStatus) { return Bit::IsSet(usbStatus, 0x10); }

    bool IsHCNotReady() const { return Bit::IsSet(_usbStatus, 0x800); }
    static bool IsHCNotReady(const uint32_t usbStatus) { return Bit::IsSet(usbStatus, 0x800); }

    //Host Controller Error
    bool IsHCError() const { return Bit::IsSet(_usbStatus, 0x1000); }
    static bool IsHCError(const uint32_t usbStatus) { return Bit::IsSet(usbStatus, 0x1000); }

    void ClearHSError() { _usbStatus = Bit::Set(_usbStatus, 0x4, true); }
    void ClearEventInterrupt() { _usbStatus = Bit::Set(_usbStatus, 0x8, true); }
    void ClearPortChanged() { _usbStatus = Bit::Set(_usbStatus, 0x10, true); }
    void ClearSRError() { _usbStatus = Bit::Set(_usbStatus, 0x400, true); }

    bool IsHCReady() const;

    //in KB
    uint32_t SupportedPageSize() const;

    bool IsCommandRingRunning() const { return Bit::IsSet(_crcr, 0x8); }
    //crcr is 64 bit
    //void StopCommandRing() { _crcr = Bit::Set(_crcr, 0x2, true); }
    //void AbortCommandRing() { _crcr = Bit::Set(_crcr, 0x4, true); }

    void SetCommandRingPointer(uint64_t ptr)
    {
      //Set ring cycle state to 1 @ start up - as command ring pointer is set is only once
      _crcr = (_crcr & (uint64_t)0x3F) | (ptr & ~((uint64_t)0x3F)) | 0x1;
    }

    void SetDCBaap(uint64_t ptr)
    {
      //require LSB 6 bits to be zero
      if(ptr & 0x3F)
        throw upan::exception(XLOC, "invalid DC_BAAP '%llx' not aligned to 64 uint8_t", ptr);
      _dcBaap = ptr;
    }

    uint32_t MaxSlotsEnabled() const { return _config & 0xFF; }
    void MaxSlotsEnabled(uint32_t val);

    bool IsU3EntryEnabled() const { return Bit::IsSet(_config, 0x100); }
    void EnableU3Entry()
    {
      if(!IsU3EntryEnabled())
        _config = Bit::Set(_config, 0x100, true);
    }
    void DisableU3Entry()
    {
      if(IsU3EntryEnabled())
        _config = Bit::Set(_config, 0x100, false);
    }
    //Configuration Information Enable
    bool IsCIE() const { return Bit::IsSet(_config, 0x200); }

    //Port Registers is an array starting at 0x400
    //So, it's address is same as address of _portRegs;
    XHCIPortRegister* PortRegisters() { return &_portRegs; }
  private:
    volatile uint32_t _usbCmd;
    volatile uint32_t _usbStatus;
    volatile uint32_t _pageSize;
    volatile uint64_t _reserved1;
    volatile uint32_t _dnCtrl; //Device Notification Control
    volatile uint64_t _crcr; //Command Ring Control
    volatile uint64_t _reserved2;
    volatile uint64_t _reserved3;
    volatile uint64_t _dcBaap; //Device Context Base Address Array Pointer
    volatile uint32_t _config;
    volatile uint8_t  _reserved4[964];
    XHCIPortRegister _portRegs;
} PACKED;

class LegSupXCap
{
  public:
    bool IsBiosOwned() const { return Bit::IsSet(_usbLegSup, 0x10000); }
    bool IsOSOwned() const { return Bit::IsSet(_usbLegSup, 0x1000000); }
    void BiosToOSHandoff();
  private:
    uint32_t _usbLegSup;
    uint32_t _usbLegCtlSts; 
} PACKED;

class PortSpeed
{
  public:
    enum PSI_RATE { BITS_PER_SECOND, Kb_PER_SECOND, Mb_PER_SECOND, Gb_PER_SECOND, UNKNOWN };
    PortSpeed() : _mantissa(0), _psiRate(PSI_RATE::UNKNOWN) {}
    PortSpeed(unsigned m, PSI_RATE r) : _mantissa(m), _psiRate(r) {}
    unsigned Mantissa() const { return _mantissa; }
    PSI_RATE BitRate() const { return _psiRate; }
    const char* BitRateS() const
    {
      switch(_psiRate)
      {
        case PSI_RATE::BITS_PER_SECOND: return "b/s";
        case PSI_RATE::Kb_PER_SECOND: return "Kb/s";
        case PSI_RATE::Mb_PER_SECOND: return "Mb/s";
        case PSI_RATE::Gb_PER_SECOND: return "Gb/s";
        default: return "unknown";
      }
    }
  private:
    unsigned _mantissa;
    PSI_RATE _psiRate;
};

class SupProtocolXCap
{
  public:
    uint32_t MajorVersion() const { return (_revision >> 24) & 0xFF; }
    uint32_t MinorVersion() const { return (_revision >> 16) & 0xFF; }
    
    USB_PROTOCOL Protocol() const
    {
      switch(MajorVersion())
      {
        case 3: return USB_PROTOCOL::USB3;
        case 2: return USB_PROTOCOL::USB2;
        case 1: return USB_PROTOCOL::USB1;
      }
      return USB_PROTOCOL::UNKNOWN;
    }

    bool HasPort(uint32_t portId) const
    {
      uint32_t startPortNo = _portDetails & 0xFF;
      uint32_t endPortNo = startPortNo + ((_portDetails >> 8) & 0xFF);
      return portId >= startPortNo && portId < endPortNo;
    }

    uint32_t SlotType() const
    {
      return _slotType & 0x1F;
    }

    PortSpeed PortSpeedInfo(unsigned psiv) const
    {
      unsigned psiCount = (_portDetails >> 28) & 0xF;
      printf("\n PSIC: %d", psiCount);
      auto psi = (unsigned*)((uintptr_t)this + sizeof(SupProtocolXCap));
      for(unsigned i = 0; i < psiCount; ++i)
      {
        if(psiv != (psi[i] & 0xF))
          continue;
        return PortSpeed(psi[i] >> 16, (PortSpeed::PSI_RATE)((psi[i] >> 4) & 0x3));
      }
      //use default speed id mapping
      switch(psiv)
      {
        //Full speed
        case FULL_SPEED: return PortSpeed(96, PortSpeed::Mb_PER_SECOND);
        //Low speed
        case LOW_SPEED: return PortSpeed(1.5, PortSpeed::Mb_PER_SECOND);
        //High speed
        case HIGH_SPEED: return PortSpeed(480, PortSpeed::Mb_PER_SECOND);
        //Super speed
        case SUPER_SPEED: return PortSpeed(5, PortSpeed::Gb_PER_SECOND);
      }
      return PortSpeed();
    }

    void Print() const
    {
      printf("\n B1: %x, B2: %x, B3: %x", _revision, _name, _portDetails);
    }
  private:
    uint32_t _revision;
    uint32_t _name;
    uint32_t _portDetails;
    uint32_t _slotType;
} PACKED;

#endif
