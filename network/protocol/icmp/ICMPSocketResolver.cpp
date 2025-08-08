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

#include <ICMPSocketResolver.h>

void ICMPSocketResolver::setup(SocketDescriptorICMP& socket, const struct icmp& header) {
  upan::mutex_guard g(_mutex);

  ICMP_PACKET_ID icmpId = header.icmp_id << 16 | header.icmp_seq;

  auto it = _icmpIdSocketMap.find(icmpId);
  if (it != _icmpIdSocketMap.end()) {
    throw upan::exception(XLOC, "ICMPSocketResolver::setupRoute: duplicate ICMP packet id: %d", icmpId);
  }

  _icmpIdSocketMap.insert(ICMP_ID_SOCKET_MAP::value_type(icmpId, &socket));
  _icmpIdSocketMapReverse[&socket].insert(icmpId);
}

void ICMPSocketResolver::release(SocketDescriptorICMP& socket) {
  upan::mutex_guard g(_mutex);

  auto it = _icmpIdSocketMapReverse.find(&socket);
  if (it != _icmpIdSocketMapReverse.end()) {
    for (auto id : it->second) {
      _icmpIdSocketMap.erase(id);
    }
    _icmpIdSocketMapReverse.erase(it);
  }
}

upan::option<SocketDescriptorICMP&> ICMPSocketResolver::resolve(const upan::shared_ptr<RawNetPacket>& packet) {
  upan::mutex_guard g(_mutex);

  if (packet->getIPV4Header().dataLen() < (int)sizeof(struct icmp)) {
    throw upan::exception(XLOC, "ICMPSocketResolver::resolve: packet len is too small");
  }

  auto* icmp = reinterpret_cast<const struct icmp*>(packet->getIPV4Data());
  ICMP_PACKET_ID icmpId = icmp->icmp_id << 16 | icmp->icmp_seq;

  auto it = _icmpIdSocketMap.find(icmpId);
  if (it != _icmpIdSocketMap.end()) {
    return upan::option<SocketDescriptorICMP&>(it->second);
  }

  return upan::option<SocketDescriptorICMP&>::empty();
}

void ICMPSocketResolver::recv(const upan::shared_ptr<RawNetPacket>& packet) {
  resolve(packet).ifPresent([&packet](SocketDescriptorICMP& socket) { socket.recvNotify(packet); });
}