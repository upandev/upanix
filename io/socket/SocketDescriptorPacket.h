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

#include <SocketDescriptor.h>

class SocketDescriptorPacket : public SocketDescriptor {
protected:
  SocketDescriptorPacket(int pid, int fd, SA_FAMILY_TYPE family, int protocol);

protected:
  int _read(void* buffer, int len) override;
  bool _canRead_1() override;

  int _write(const void* buffer, int len) override;
  bool _canWrite_1() override { return true; }

  void _seek(int seekType, int offset) override { }
  uint32_t _getOffset() const override { return 0; }

  void _listen(int backlog) override {
    throw upan::exception(XLOC, "listen not supported for packet sockets");
  }

  int _accept(struct sockaddr* sockaddr, socklen_t* len) override {
    throw upan::exception(XLOC, "accept not supported for packet sockets");
  }

  void _shutdown(SOCKET_SHUTDOWN_TYPE type) override;

  virtual bool filterPacket(const upan::shared_ptr<RawNetPacket>& packet) = 0;
  struct sockaddr_in& srcAddr() { return _srcAddr; }
  upan::shared_ptr<RawNetPacket> recvPacket();
  void recvNotify(const upan::shared_ptr<RawNetPacket>& packet);
  const struct sockaddr_in& extractDestAddr(const struct sockaddr& addr, socklen_t len, bool portRequired);
  const struct sockaddr_in& resolveDestAddr(const struct sockaddr_in& connectedAddr, const struct sockaddr* sendAddr, socklen_t len, bool portRequired);

private:
  struct sockaddr_in _srcAddr;
  upan::mutex _ioSync;
  upan::queue<upan::shared_ptr<RawNetPacket>> _packetQueue;

  friend class ARPSocketResolver;
  friend class ICMPSocketResolver;
  friend class UDPSocketResolver;
};
