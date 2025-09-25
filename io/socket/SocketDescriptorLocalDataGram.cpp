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

#include <sys/un.h>
#include <SocketDescriptorLocalDataGram.h>
#include <LocalDataGramResolver.h>
#include <ProcessManager.h>

static constexpr int MAX_MESSAGE_QUEUE_SIZE = 256;
static constexpr int MAX_MESSAGE_SIZE = 2048;
static upan::string EPHEMERAL_ADDRESS_PATH_PREFIX("@");
static int EPHEMERAL_ADDRESS_ID = 0;

SocketDescriptorLocalDataGram::SocketDescriptorLocalDataGram(int pid, int fd, SA_FAMILY_TYPE family)
  : SocketDescriptor(pid, fd, family, 0),
    _isConnected(false), _isBound(false), _errorCode(0) {
  _srcPath = EPHEMERAL_ADDRESS_PATH_PREFIX + upan::string::to_string(EPHEMERAL_ADDRESS_ID++);
  LocalDataGramResolver::Instance().setup(*this, _srcPath);
}

void SocketDescriptorLocalDataGram::_close() {
}

static upan::string extractPath(const struct sockaddr& address, socklen_t len) {
  if (len != sizeof(struct sockaddr_un)) {
    throw upan::exception(XLOC, "invalid socket len: %d", len);
  }

  const auto& localAddr = reinterpret_cast<const struct sockaddr_un&>(address);

  const upan::string path = localAddr.sun_path;
  if (!path.empty() && !(
          (IsKernel() || IsKernelProcess(ProcessManager::GetCurrentProcessID()))
          && path == SYS_LOG_PATH)) {
    if (access(path.c_str(), O_WRONLY) != 0) {
      throw upan::exception(XLOC, "Permission denied to open socket on file: %s", path.c_str());
    }
  }

  return path;
}

void SocketDescriptorLocalDataGram::_bind(const struct sockaddr& address, socklen_t len) {
  if (_isBound) {
    throw upan::exception(XLOC, "bind failed - socket %d is already bound", id());
  }
  const upan::string& path = extractPath(address, len);
  LocalDataGramResolver::Instance().release(*this);
  LocalDataGramResolver::Instance().setup(*this, path);
  _srcPath = path;
  _isBound = true;
}

void SocketDescriptorLocalDataGram::_connect(const struct sockaddr& address, socklen_t len) {
  if (_isConnected) {
    throw upan::exception(XLOC, "connect failed - socket %d is already connected", id());
  }

  const upan::string& path = extractPath(address, len);
  const auto& dest = LocalDataGramResolver::Instance().resolve(path);
  if (dest.isEmpty()) {
    throw upan::exception(XLOC, "connect failed - socket %d can't find the destination %s", id(), path.c_str());
  }
  _destPath = path;
  _isConnected = true;
}

int SocketDescriptorLocalDataGram::_read(void* buffer, int len) {
  return _recvFrom(buffer, len, 0, nullptr, nullptr);
}

bool SocketDescriptorLocalDataGram::_canRead_1() {
  upan::mutex_guard g(_ioSync);
  return !_messages.empty();
}

int SocketDescriptorLocalDataGram::_write(const void* buffer, int len) {
  return _sendTo(buffer, len, 0, nullptr, 0);
}

bool SocketDescriptorLocalDataGram::_canWrite_1() {
  upan::mutex_guard g(_ioSync);
  if (_destPath.empty()) {
    return true;
  } else {
    const auto& dest = LocalDataGramResolver::Instance().resolve(_destPath);
    if (dest.isEmpty()) {
      return false;
    }
    return dest.value().canWrite();
  }
}

void SocketDescriptorLocalDataGram::_shutdown(SOCKET_SHUTDOWN_TYPE type) {
  upan::mutex_guard g(_ioSync);
  if (type == SHUT_RDWR || type == SHUT_RD) {
    _messages.clear();
  }
}

ssize_t SocketDescriptorLocalDataGram::sendMessage(const void* buf, size_t n, const upan::string& srcPath) {
  upan::mutex_guard g(_ioSync);
  if (shutdownStatus() == SHUT_RDWR || shutdownStatus() == SHUT_RD) {
    //ignore the message
    return 0;
  }

  if (_messages.size() < MAX_MESSAGE_QUEUE_SIZE) {
    n = upan::min((size_t) MAX_MESSAGE_SIZE, n);
    upan::string msg((const char*) buf, n);
    _messages.push_back({ msg, srcPath} );
    return n;
  }

  return -1;
}

ssize_t SocketDescriptorLocalDataGram::_sendTo(const void* buf, size_t n, int flags, const struct sockaddr* addr, socklen_t len) {
  if (shutdownStatus() == SHUT_RDWR || shutdownStatus() == SHUT_WR) {
    throw upan::exception(XLOC, "socket is shutdown for write - can't send data");
  }

  if (!addr) {
    if (!_isConnected) {
      throw upan::exception(XLOC, "sendPacket failed - socket %d is not connected", id());
    }
  } else {
    const auto& destPath = extractPath(*addr, len);
    if (_isConnected) {
      if (destPath != _destPath) {
        throw upan::exception(XLOC, "sendPacket failed - socket %d is connected to %s, can't send to %s", id(), _destPath.c_str(), destPath.c_str());
      }
    } else {
      _destPath = destPath;
    }
  }

  auto dest = LocalDataGramResolver::Instance().resolve(_destPath);
  if (dest.isEmpty()) {
    throw upan::exception(XLOC, "sendPacket failed - socket %d can't find the destination %s", id(), _destPath.c_str());
  }

  while (true) {
    ssize_t r = dest.value().sendMessage(buf, n, _srcPath);
    if (r < 0) {
      if (getMode() & O_WR_NONBLOCK || getMode() & O_NONBLOCK) {
        return 0;
      }
      ProcessManager::Instance().WaitOnIODescriptor(id(), IO_OP_TYPES::IO_Write, 0);
      const auto err = ProcessManager::Instance().GetCurrentPAS().stateInfo().getError();
      if (err == ProcessStateInfo::INTERRUPTED) {
        throw upan::exception(XLOC, "local DGRAM write interrupted");
      }
    } else {
      return r;
    }
  }
}

ssize_t SocketDescriptorLocalDataGram::_recvFrom(void* buf, size_t n, int flags, struct sockaddr* addr, socklen_t* len) {
  if (shutdownStatus() == SHUT_RDWR || shutdownStatus() == SHUT_RD) {
    return 0;
  }

  if (_srcPath.empty()) {
    throw upan::exception(XLOC, "recvFrom failed - socket %d doesn't have an address", id());
  }

  while(true) {
    {
      upan::mutex_guard g(_ioSync);
      if (!_messages.empty()) {
        const Message& msg = _messages.front();
        _messages.pop_front();

        n = upan::min((size_t)msg._msg.length(), n);
        memcpy(buf, msg._msg.c_str(), n);

        if (addr && len && *len == sizeof(struct sockaddr_un)) {
          struct sockaddr_un* addrUn = (struct sockaddr_un*) addr;
          addrUn->sun_family = AF_LOCAL;

          memcpy(addrUn->sun_path, msg._srcPath.c_str(), msg._srcPath.length());
          addrUn->sun_path[msg._srcPath.length()] = '\0';

          *len = sizeof(struct sockaddr_un);
        }

        return n;
      }
    }

    if (getMode() & O_RD_NONBLOCK || getMode() & O_NONBLOCK) {
      return 0;
    }
    ProcessManager::Instance().WaitOnIODescriptor(id(), IO_OP_TYPES::IO_Read, 0);
    const auto err = ProcessManager::Instance().GetCurrentPAS().stateInfo().getError();
    if (err == ProcessStateInfo::INTERRUPTED) {
      throw upan::exception(XLOC, "local DGRAM read interrupted");
    }
  }
}