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

#include <TestRunner.h>
#include <FileSystemTest.h>
#include <PipeTest.h>

void TestRunner::run() {
  printf("\n-> Running tests...");

  printf("\n-> Running FileSystemTest...");

  printf("\n--> Running testRename...\n");
  FileSystemTest::testRename();

  printf("\n--> Running testSymlink...\n");
  FileSystemTest::testSymlink();

  printf("\n--> Running testAccess...\n");
  FileSystemTest::testAccess();

  printf("\n-> Running PipeTest...");

  printf("\n--> Running testUnamedPipe...\n");
  PipeTest::testUnamedPipe();

  printf("\n--> Running testNamedPipe...\n");
  PipeTest::testNamedPipe();
}