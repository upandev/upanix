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

#include <stdlib.h>
#include <ARPHeaders.h>
#include <EthernetHeaders.h>
#include <IPV4Headers.h>
#include <UDPHeaders.h>

class RawNetPacket {
public:
  RawNetPacket();
  RawNetPacket(const uint8_t* addr, int len);
  explicit RawNetPacket(int len);
  ~RawNetPacket();

  RawNetPacket(const RawNetPacket& o) = delete;
  RawNetPacket& operator=(const RawNetPacket& o) = delete;

  RawNetPacket(RawNetPacket&& o) noexcept;
  RawNetPacket& operator=(RawNetPacket&& o) noexcept;

  uint8_t* buf() const { return _buf; }
  int len() const { return _len; }

  NetworkPacket::Ethernet::Header& getEthernetHeader();
  const NetworkPacket::Ethernet::Header& getEthernetHeader() const;
  uint8_t* getEthernetData();
  const uint8_t* getEthernetData() const;

  NetworkPacket::IPV4::Header& getIPV4Header();
  const NetworkPacket::IPV4::Header& getIPV4Header() const;
  uint8_t* getIPV4Data();
  const uint8_t* getIPV4Data() const;

  NetworkPacket::ARP::Header& getARPHeader();
  const NetworkPacket::ARP::Header& getARPHeader() const;

  NetworkPacket::UDP::Header& getUDP4Header();
  const NetworkPacket::UDP::Header& getUDP4Header() const;
  uint8_t* getUDP4Data();
  const uint8_t* getUDP4Data() const;

private:
  void move(RawNetPacket& o);

  uint8_t* _buf;
  int _len;
};