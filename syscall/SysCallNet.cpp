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

#include <SysCall.h>
#include <SysCallNet.h>
#include <NetworkOperations.h>
#include <NetworkManager.h>

bool SysCallNet_IsPresent(uint64_t sysCallId) {
	return (sysCallId > SYS_CALL_NETWORK_START && sysCallId < SYS_CALL_NETWORK_END);
}

void SysCallNet_Handle(uint64_t *retVal, uint64_t sysCallId, bool doAddrTranslation, uint64_t p1, uint64_t p2, uint64_t p3, uint64_t p4, uint64_t p5) {
	switch(sysCallId) {
		case SYS_CALL_SOCKET_CREATE:
			{
        const auto family = (SA_FAMILY_TYPE)p1;
        const auto type = (SOCKET_TYPE)p2;
        const auto protocol = (int)p3;

        try {
          *retVal = NetworkOperations::Instance().createSocket(family, type, protocol);
        } catch(const upan::exception& e) {
          KLog::exception(e);
          *retVal = -1;
        }
			}
			break;

    case SYS_CALL_SOCKET_BIND:
      {
        *retVal = 0;
        const auto fd = (sock_t)p1;
        const auto& address = *(struct sockaddr*)p2;
        const auto len = (socklen_t)p3;
        try {
          NetworkOperations::Instance().bind(fd, address, len);
        } catch(const upan::exception& e) {
          KLog::exception(e);
          *retVal = -1;
        }
      }
      break;

    case SYS_CALL_SOCKET_SET_OPT:
      {
        *retVal = 0;
        try {
          NetworkOperations::Instance().setSockOpt((sock_t)p1, (int)p2, (SOCKET_OPTION)p3, (const void*)p4, (socklen_t)p5);
        } catch(const upan::exception& e) {
          KLog::exception(e);
          *retVal = -1;
        }
      }
      break;

    case SYS_CALL_SOCKET_GET_OPT:
      {
        *retVal = 0;
        try {
          NetworkOperations::Instance().getSockOpt((sock_t)p1, (int)p2, (SOCKET_OPTION)p3, (void*)p4, (socklen_t*)p5);
        } catch(const upan::exception& e) {
          KLog::exception(e);
          *retVal = -1;
        }
      }
      break;

    case SYS_CALL_SOCKET_GET_NAME:
    {
      *retVal = 0;
      const auto fd = (sock_t)p1;
      auto addr = (struct sockaddr*)p2;
      auto len = (socklen_t*)p3;
      try {
        if (addr == nullptr || len == nullptr) {
          throw upan::exception(XLOC, "getsockname: invalid (null) parameters");
        }
        NetworkOperations::Instance().getSockName(fd, *addr, *len);
      } catch(const upan::exception& e) {
        KLog::exception(e);
        *retVal = -1;
      }
    }
    break;

    case SYS_CALL_SOCKET_PEER_NAME:
    {
      *retVal = 0;
      const auto fd = (sock_t)p1;
      auto addr = (struct sockaddr*)p2;
      auto len = (socklen_t*)p3;
      try {
        if (addr == nullptr || len == nullptr) {
          throw upan::exception(XLOC, "getpeername: invalid (null) parameters");
        }
        NetworkOperations::Instance().getPeerName(fd, *addr, *len);
      } catch(const upan::exception& e) {
        KLog::exception(e);
        *retVal = -1;
      }
    }
    break;

    case SYS_CALL_SOCKET_SEND_TO:
    {
      *retVal = 0;
      const auto fd = (sock_t)p1;
      const auto buf = (void*)p2;
      const auto n = (size_t)p3;
      const auto flags = (int)p4;
      auto ext_param = (uint64_t*)p5;
      const auto address = (struct sockaddr*)ext_param[0];
      const auto len = (socklen_t)ext_param[1];
      try {
        *retVal = NetworkOperations::Instance().sendTo(fd, buf, n, flags, address, len);
      } catch(const upan::exception& e) {
        KLog::exception(e);
        *retVal = -1;
      }
    }
    break;

    case SYS_CALL_SOCKET_RECV_FROM:
    {
      *retVal = 0;
      const auto fd = (sock_t)p1;
      auto buf = (void*)p2;
      const auto n = (size_t)p3;
      const auto flags = (int)p4;
      auto ext_param = (uint64_t*)p5;
      const auto address = (struct sockaddr*)ext_param[0];
      const auto len = (socklen_t*)ext_param[1];
      try {
        *retVal = NetworkOperations::Instance().recvFrom(fd, buf, n, flags, address, len);
      } catch(const upan::exception& e) {
        KLog::exception(e);
        *retVal = -1;
      }
    }
    break;

    case SYS_CALL_SOCKET_CONNECT:
    {
      *retVal = 0;
      const auto fd = (sock_t)p1;
      auto& addr = *(struct sockaddr*)p2;
      auto len = (size_t)p3;
      try {
        NetworkOperations::Instance().connect(fd, addr, len);
      } catch(const upan::exception& e) {
        KLog::exception(e);
        *retVal = -1;
      }
    }
    break;

    case SYS_CALL_SOCKET_LISTEN:
    {
      *retVal = 0;
      const auto fd = (sock_t)p1;
      const auto backlog = (int)p2;
      try {
        NetworkOperations::Instance().listen(fd, backlog);
      } catch(const upan::exception& e) {
        KLog::exception(e);
        *retVal = -1;
      }
    }
    break;

    case SYS_CALL_SOCKET_ACCEPT:
    {
      *retVal = 0;
      const auto fd = (sock_t)p1;
      auto sockaddr = (struct sockaddr*)p2;
      auto socklen = (socklen_t*)p3;
      try {
        *retVal = NetworkOperations::Instance().accept(fd, sockaddr, socklen);
      } catch(const upan::exception& e) {
        KLog::exception(e);
        *retVal = -1;
      }
    }
    break;

    case SYS_CALL_SOCKET_SHUTDOWN:
    {
      *retVal = 0;
      const auto fd = (sock_t)p1;
      const auto type = (SOCKET_SHUTDOWN_TYPE)p2;
      try {
        NetworkOperations::Instance().shutdown(fd, type);
      } catch(const upan::exception& e) {
        KLog::exception(e);
        *retVal = -1;
      }
    }
    break;

    case SYS_CALL_GET_HOST_BY_NAME:
    {
      *retVal = 0;
      auto name = (const char*)p1;
      auto hostinfo = (struct hostent**)p2;
      try {
        *hostinfo = NetworkManager::Instance().getDNSClient().resolveHost(name);
      } catch(const upan::exception& e) {
        KLog::exception(e);
        *hostinfo = nullptr;
        *retVal = -1;
      }
    }
    break;

    case SYS_CALL_GET_HOST_BY_ADDR:
    {
      *retVal = 0;
      auto addr = (const void*)p1;
      auto len = (socklen_t)p2;
      int type = (int)p3;
      auto hostinfo = (struct hostent**)p4;
      try {
        *hostinfo = NetworkManager::Instance().getDNSClient().resolveReverseHost(addr, len, type);
      } catch(const upan::exception& e) {
        KLog::exception(e);
        *hostinfo = nullptr;
        *retVal = -1;
      }
    }
    break;

    case SYS_CALL_FREE_HOST_INFO:
    {
      *retVal = 0;
      auto hostinfo = (struct hostent*)p1;
      try {
        NetworkManager::Instance().getDNSClient().freeHostinfo(hostinfo);
      } catch(const upan::exception& e) {
        KLog::exception(e);
        *retVal = -1;
      }
    }
    break;

    case SYS_CALL_SOCKET_PAIR:
    {
      const auto family = (SA_FAMILY_TYPE)p1;
      const auto type = (SOCKET_TYPE)p2;
      const auto protocol = (int)p3;
      const auto sv = (int*)p4;

      try {
        *retVal = 0;
        NetworkOperations::Instance().createSocketPair(family, type, protocol, sv);
      } catch(const upan::exception& e) {
        KLog::exception(e);
        *retVal = -1;
      }
    }
    break;
	}
}
