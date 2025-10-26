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

#include <map.h>
#include <shared_ptr.h>
#include <FSDevice.h>

class FSSocketDevice;
class FSTerminalDevice;
class FSDeviceManager {
private:
  FSDeviceManager();

public:
  static FSDeviceManager& Instance() {
    static FSDeviceManager instance;
    return instance;
  }

  upan::shared_ptr<FSDevice> getDevice(const upan::string& path);
  upan::shared_ptr<FSSocketDevice> getSocketDevice(const upan::string& path);
  upan::shared_ptr<FSTerminalDevice> getTerminalDevice(const upan::string& path);
  void createSocketDevice(const upan::string& path);
  int createTerminalDevice(int flags);
  void removeDevice(const upan::string& path);

private:
  const upan::string _rootPrefix;
  upan::atomic::integral<int> _nextTerminalId;
  upan::map<upan::string, upan::shared_ptr<FSDevice>> _devices;
};