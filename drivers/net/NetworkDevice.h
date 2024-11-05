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
#include "RawNetPacket.h"

class PCIEntry;
class SocketBuffer;
class EthernetHandler;
class ARPHandler;
class IPV4Handler;
class UDP4Handler;
class DHCPHandler;

class NetworkDevice {
public:
  NetworkDevice(const PCIEntry& pciEntry);
  virtual ~NetworkDevice() = 0;
  
  virtual void Initialize() = 0;
  virtual void NotifyEvent() = 0;
  virtual void SendPacket(const RawNetPacket& packet) = 0;
  virtual EthernetHandler& GetEthernetHandler() = 0;
  virtual upan::option<ARPHandler&> GetARPHandler() = 0;
  virtual upan::option<IPV4Handler&> GetIPV4Handler() = 0;
  virtual upan::option<UDP4Handler&> GetUDP4Handler() = 0;
  virtual upan::option<DHCPHandler&> GetDHCPHandler() = 0;
  virtual const MACAddress& GetMACAddress() const = 0;

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
  const PCIEntry& _pciEntry;
};
