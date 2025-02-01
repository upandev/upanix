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

#include <Global.h>
#include <map.h>
#include <uniq_ptr.h>
#include <fs.h>
#include <mutex.h>
#include <IODescriptor.h>
#include <function.h>
#include <mosstd.h>
#include <vector.h>

class IODescriptorTable {
public:
  typedef enum {
    STDIN = 0,
    STDOUT = 1,
    STDERR = 2,
    SYSLOG = 3
  } STD_DESCRIPTORS;

  typedef upan::map<int, IODescriptor*> IODMap;

  IODescriptorTable(int pid, int parentPid);
  ~IODescriptorTable() noexcept;

  IODescriptor& allocate(const upan::function<IODescriptor*, int>& descriptorBuilder);
  void free(int fd);
  void dup2(int oldFD, int newFD);
  IODescriptor& getRealNonDupped(int fd);
  IODescriptor& get(int fd);
  void setupStreamedStdio();
  void setupNullStdio();
  upan::vector<io_descriptor> select(const upan::vector<io_descriptor>& ioDescriptors);
  upan::vector<io_descriptor> selectCheck(const upan::vector<io_descriptor>& ioDescriptors);

private:
  IODMap::iterator getItr(int fd);

  int _pid;
  int _descIdCounter;
  upan::mutex _ioMutex;
  IODMap _iodMap;
};