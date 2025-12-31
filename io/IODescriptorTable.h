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

class StorageDrive;

class IODescriptorTable {
public:
  typedef enum {
    STDIN = 0,
    STDOUT = 1,
    STDERR = 2,
    TERMINAL_MASTER = 3,
    KSYSLOG = 4,
  } STD_DESCRIPTORS;

  typedef enum {
    IO_Read,
    IO_Write
  } IO_OP_TYPES ;

  typedef struct {
    int _fd;
    IO_OP_TYPES _ioType;
  } io_descriptor;

  typedef upan::map<int, IODescriptor::Ptr> IODMap;

  explicit IODescriptorTable(int pid);
  ~IODescriptorTable() noexcept;

  IODescriptor::Ptr allocate(const upan::function<IODescriptor::Ptr, int>& descriptorBuilder);
  void free(int fd);
  void updateRedirections(int srcFD, IODescriptor::Ptr targetDesc);
  int dup(int oldFD);
  void dup2(int oldFD, int newFD);
  IODescriptor::Ptr getRealNonDupped(int fd);
  IODescriptor::Ptr get(int fd);
  void setupStreamedStdio();
  void setupNullStdio();
  upan::vector<io_descriptor> select(const upan::vector<io_descriptor>& ioDescriptors, struct timeval* timeout, int& retCode);
  upan::vector<io_descriptor> selectCheck(const upan::vector<io_descriptor>& ioDescriptors);
  void closeAllFiles(StorageDrive&);

private:
  IODMap::iterator getItr(int fd);

  int _pid;
  int _descIdCounter;
  upan::mutex _ioMutex;
  IODMap _iodMap;
};