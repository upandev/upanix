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

#include <NetworkDevice.h>

class LoopbackNetworkDevice : public NetworkDevice {
private:
  static constexpr char DEFAULT_HOST_NAME[] = "localhost";

public:
  explicit LoopbackNetworkDevice();
  ~LoopbackNetworkDevice() override = default;

  void send(RawNetPacket& packet, ETH_PROTO_TYPE eType) override;
  uint32_t deviceLayerHeaderLen() const override { return 0; };
  const char* hostName() const override { return DEFAULT_HOST_NAME; }
};
