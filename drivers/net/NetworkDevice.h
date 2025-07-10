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
#pragma once

#include <ustring.h>
#include <option.h>
#include <NetworkUtil.h>
#include <MACAddress.h>
#include <RawNetPacket.h>
#include <EthernetHandler.h>
#include <IPV4Handler.h>
#include <UDP4Handler.h>
#include <ARPHandler.h>
#include <ICMPHandler.h>
#include <uniq_ptr.h>

class PCIEntry;
class SocketBuffer;

class NetworkDevice {
private:
  static constexpr uint16_t DEFAULT_MTU = 1500;
  static constexpr char DEFAULT_HOST_NAME[] = "Upanix";

public:

  explicit NetworkDevice(const PCIEntry& pciEntry);
  virtual ~NetworkDevice() = 0;

  const upan::string& name() const { return _name; }
  int id() const { return _id; }

  virtual void Initialize() = 0;
  virtual void NotifyEvent() = 0;
  virtual void SendPacket(const RawNetPacket& packet) = 0;

  virtual uint16_t mtu() const { return DEFAULT_MTU; }
  virtual const char* hostName() const { return DEFAULT_HOST_NAME; }

  const MACAddress& GetMACAddress() const { return _macAddress; }
  in_addr_t GetIPAddress() const { return _ipAddress; }
  in_addr_t GetGatewayAddress() const { return _gatewayAddress; }
  in_addr_t GetSubnetMask() const { return _subnetMask; }
  in_addr_t GetBroadcastAddress() const { return _broadcastAddress; }
  in_addr_t GetDNSAddress() const { return _dnsAddress; }

  EthernetHandler& getEthernetHandler() { return _ethernetHandler; }
  const EthernetHandler& getEthernetHandler() const { return _ethernetHandler; }

  ARPHandler& getARPHandler() { return _arpHandler; }
  const ARPHandler& getARPHandler() const { return _arpHandler; }

  IPV4Handler& getIPV4Handler() { return _ipv4Handler; }
  const IPV4Handler& getIPV4Handler() const { return _ipv4Handler; }

  UDP4Handler& getUDP4Handler() { return _udp4Handler; }
  const UDP4Handler& getUDP4Handler() const { return _udp4Handler; }

  ICMPHandler& getICMPHandler() { return _icmpHandler; }
  const ICMPHandler& getICMPHandler() const { return _icmpHandler; }

  // virtual int Configure() = 0;
  // virtual void Tx(SocketBuffer& socketBuffer) = 0;

  // virtual int Start() = 0;

  // virtual void Stop() = 0;

  // virtual int AddInterface() = 0;

  // virtual int ChangeInterface() = 0;

  // virtual void RemoveInterface() = 0;

  // virtual void ConfigureFilter() = 0;

  // virtual int STAState() = 0;

  // virtual void STANotify() = 0;

  // virtual void ConfigureTx() = 0;

  // virtual void BSSInfoChanged() = 0;

  // virtual int SetKey() = 0;

	// virtual uint64_t GetTSF() = 0;

  // virtual void SetTFS() = 0;

	// virtual void ResetTSF() = 0;

	// virtual int AMPDUAction() = 0;

	// virtual int GetSurvey() = 0;

	// virtual void SetCoverageClass() = 0;

	// virtual void Flush() = 0;

	// virtual bool TxFramesPending() = 0;

	// virtual int TxLastBeacon() = 0;

	// virtual int GetStats() = 0;

	// virtual int GetAntenna() = 0;

	// virtual void ReleaseBufferedFrames() = 0;

	// virtual void SWScanStart() = 0;

	// virtual void SWScanComplete() = 0;

	// virtual void WakeTxQueue() = 0;

protected:
  void setName(const upan::string& name) { _name = name; }
  void setId(int id) { _id = id; }

  void setMACAddress(const MACAddress& macAddress) { _macAddress = macAddress; }
  void setIPAddress(in_addr_t ip) { _ipAddress = ip; }
  void setGatewayAddress(in_addr_t ip) { _gatewayAddress = ip; }
  void setSubnetMask(in_addr_t ip) { _subnetMask = ip; }
  void setBroadcastAddress(in_addr_t ip) { _broadcastAddress = ip; }
  void setDNSAddress(in_addr_t ip) { _dnsAddress = ip; }

  friend class DHCPClient;
  friend class NetworkManager;
protected:
  const PCIEntry& _pciEntry;

  upan::string _name;
  int _id;
  MACAddress _macAddress;
  in_addr_t _ipAddress;
  in_addr_t _gatewayAddress;
  in_addr_t _subnetMask;
  in_addr_t _broadcastAddress;
  in_addr_t _dnsAddress;

  EthernetHandler _ethernetHandler;
  IPV4Handler _ipv4Handler;
  UDP4Handler _udp4Handler;
  ICMPHandler _icmpHandler;
  ARPHandler _arpHandler;
};
