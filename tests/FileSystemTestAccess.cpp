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

#include <fs.h>
#include <exception.h>
#include <FileSystemTest.h>

static void testFileAccessChange(const char* srcFileContent) {
  if (access("sfile.txt", 0666) == 0) {
    throw upan::exception(XLOC, "access didn't fail for a file sfile.txt that doesn't exists");
  }

  if (chmod("sfile.txt", 0466) == 0) {
    throw upan::exception(XLOC, "chmod didn't fail for a file sfile.txt that doesn't exists");
  }

  if (create("sfile.txt", ATTR_FILE_DEFAULT)) {
    throw upan::exception(XLOC, "failed to create file: sfile.txt");
  }

  int fd = open("sfile.txt", O_RDWR);
  if (fd < 0) {
    throw upan::exception(XLOC, "failed to open file: sfile.txt");
  }

  int w = strlen(srcFileContent);
  int n = write(fd, srcFileContent, w);
  if (n < w) {
    upan::exception(XLOC, "failed to write to sfile.txt");
  }

  close(fd);

  if (access("sfile.txt", 0666)) {
    throw upan::exception(XLOC, "access failed for file sfile.txt");
  }

  if (chmod("sfile.txt", 0466)) {
    throw upan::exception(XLOC, "chmod failed for file sfile.txt");
  }

  if (access("sfile.txt", 0666) != 0) {
    throw upan::exception(XLOC, "access didn't fail for file sfile.txt that doesn't permission to write");
  }

  if (access("sfile.txt", 0466)) {
    throw upan::exception(XLOC, "access failed for file sfile.txt that has permission to read");
  }

  fd = open("sfile.txt", O_RDWR | O_APPEND);
  if (fd >= 0) {
    throw upan::exception(XLOC, "open for read-write didn't fail for file sfile.txt that doesn't write permission");
  }

  if (chmod("sfile.txt", 0666)) {
    throw upan::exception(XLOC, "chmod failed for file sfile.txt");
  }

  fd = open("sfile.txt", O_RDWR | O_APPEND);
  if (fd < 0) {
    throw upan::exception(XLOC, "failed to open file: sfile.txt");
  }

  w = strlen(srcFileContent);
  n = write(fd, srcFileContent, w);
  if (n < w) {
    upan::exception(XLOC, "failed to write to sfile.txt");
  }

  close(fd);

  if (access("sfile.txt", 0666)) {
    throw upan::exception(XLOC, "access failed for file sfile.txt");
  }

  if(unlink("sfile.txt")) {
    throw upan::exception(XLOC, "failed to delete file sfile.txt");
  }
}

void FileSystemTest::testAccess() {
  try {
    testFileAccessChange("");
  } catch(const upan::exception& e) {
    printf("\n file access test failed... : %s", e.ErrorMsg().c_str());
    unlink("sfile.txt");
  }
}