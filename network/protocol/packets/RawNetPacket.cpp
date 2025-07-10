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


#include <RawNetPacket.h>
#include <string.h>
#include <newalloc.h>
#include <DMM.h>

RawNetPacket::RawNetPacket() : _buf(nullptr), _len(0) {
}

RawNetPacket::RawNetPacket(const uint8_t* addr, const int len) :
  _buf(new uint8_t[len]), _len(len) {
  memcpy(_buf, addr, _len);
}

RawNetPacket::RawNetPacket(const int len) :
  _buf(new uint8_t[len]), _len(len) {
}

RawNetPacket::~RawNetPacket() {
  if (_buf) {
    delete[] _buf;
    _buf = nullptr;
  }
}

RawNetPacket::RawNetPacket(RawNetPacket&& o) noexcept : RawNetPacket() {
  move(o);
}

RawNetPacket& RawNetPacket::operator=(RawNetPacket&& o) noexcept {
  move(o);
  return *this;
}

void RawNetPacket::move(RawNetPacket& o) {
  this->_buf = o._buf;
  this->_len = o._len;
  o._buf = nullptr;
}

NetworkPacket::Ethernet::Header& RawNetPacket::getEthernetHeader() {
  return *reinterpret_cast<NetworkPacket::Ethernet::Header*>(_buf);
}

const NetworkPacket::Ethernet::Header& RawNetPacket::getEthernetHeader() const {
  return const_cast<RawNetPacket*>(this)->getEthernetHeader();
}

uint8_t* RawNetPacket::getEthernetData() {
  return _buf + NetworkPacket::Ethernet::HEADER_SIZE;
}

const uint8_t* RawNetPacket::getEthernetData() const {
  return const_cast<RawNetPacket*>(this)->getEthernetData();
}

NetworkPacket::IPV4::Header& RawNetPacket::getIPV4Header() {
  return *reinterpret_cast<NetworkPacket::IPV4::Header*>(getEthernetData());
}

const NetworkPacket::IPV4::Header& RawNetPacket::getIPV4Header() const {
  return const_cast<RawNetPacket*>(this)->getIPV4Header();
}

uint8_t* RawNetPacket::getIPV4Data() {
  return getEthernetData() + getIPV4Header().headerLen();
}

const uint8_t* RawNetPacket::getIPV4Data() const {
  return const_cast<RawNetPacket*>(this)->getIPV4Data();
}

NetworkPacket::ARP::Header& RawNetPacket::getARPHeader() {
  return *reinterpret_cast<NetworkPacket::ARP::Header*>(getEthernetData());
}

const NetworkPacket::ARP::Header& RawNetPacket::getARPHeader() const {
  return const_cast<RawNetPacket*>(this)->getARPHeader();
}

NetworkPacket::UDP::Header& RawNetPacket::getUDP4Header() {
  return *reinterpret_cast<NetworkPacket::UDP::Header*>(getIPV4Data());
}

const NetworkPacket::UDP::Header& RawNetPacket::getUDP4Header() const {
  return const_cast<RawNetPacket*>(this)->getUDP4Header();
}

uint8_t* RawNetPacket::getUDP4Data() {
  return getIPV4Data() + NetworkPacket::UDP::HEADER_SIZE;
}

const uint8_t* RawNetPacket::getUDP4Data() const {
  return const_cast<RawNetPacket*>(this)->getUDP4Data();
}