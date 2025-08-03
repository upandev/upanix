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
#include <TCPStreamWorker.h>
#include "NetworkManager.h"

TCPStreamWorker::TCPStreamWorker() : upan::timer_thread(10) {
}

void TCPStreamWorker::addConnection(upan::shared_ptr<TCPConnection>& connection) {
  upan::mutex_guard g(_mutex);
  NetworkManager::Instance().getTCPSocketResolver().setup(connection);
  _tcpConnections.insert(connection);
}

void TCPStreamWorker::on_timer_trigger() {
  upan::mutex_guard g(_mutex);
  for (auto it = _tcpConnections.begin(); it != _tcpConnections.end();) {
    auto conn = *it;
    if (conn->state() == TCPConnection::TCP_CLOSED) {
      _tcpConnections.erase(it++);
      NetworkManager::Instance().getTCPSocketResolver().releaseConnection(conn);
      KLog::info("TCP connection closed for %s", conn->str().c_str());
    } else {
      ++it;
    }
  }

  for (auto connection : _tcpConnections) {
    connection->processStream();
  }
}