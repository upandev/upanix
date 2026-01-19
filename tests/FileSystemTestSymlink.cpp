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
#include <dirent.h>
#include <Global.h>
#include <FileSystemTest.h>

static void testSymlinkWithAbsolutePath(const char* targetFileContent) {
  if(symlink("/wdir/ldir/target.txt", "wdir/tlink.txt")) {
    throw upan::exception(XLOC, "failed to create symlink wdir/tlink.txt");
  }

  char buf[128];

  int n = readlink("wdir/tlink.txt", buf, 128);
  if (n <= 0) {
    throw upan::exception(XLOC, "failed to readlink wdir/tlink.txt");
  }

  buf[n] = '\0';
  if (strcmp("/wdir/ldir/target.txt", buf) != 0) {
    throw upan::exception(XLOC, "invalid link value: %s", buf);
  }

  int fd = open("wdir/tlink.txt", O_RDONLY);
  if (fd < 0) {
    throw upan::exception(XLOC, "failed to open wdir/tlink.txt");
  }

  n = read(fd, buf, 128);
  if (n <= 0) {
    close (fd);
    throw upan::exception(XLOC, "failed to read wdir/tlink.txt");
  }

  close (fd);
  buf[n] = '\0';
  if(strcmp(buf, targetFileContent) != 0) {
    throw upan::exception(XLOC, "target file content mismatch: %s", buf);
  }
}

static void testSymlinkWithRelativePath(const char* targetFileContent) {
  if(symlink("wdir/ldir/target.txt", "wdir/trlink.txt")) {
    throw upan::exception(XLOC, "failed to create symlink wdir/trlink.txt");
  }

  char buf[128];

  int n = readlink("wdir/trlink.txt", buf, 128);
  if (n <= 0) {
    throw upan::exception(XLOC, "failed to readlink wdir/trlink.txt");
  }

  buf[n] = '\0';
  if (strcmp("wdir/ldir/target.txt", buf) != 0) {
    throw upan::exception(XLOC, "invalid link value: %s", buf);
  }

  if (chdir("/bin")) {
    throw upan::exception(XLOC, "failed to change directory to /bin");
  }

  int fd = open("/wdir/trlink.txt", O_RDONLY);
  if (fd < 0) {
    throw upan::exception(XLOC, "failed to open /wdir/trlink.txt");
  }

  n = read(fd, buf, 128);
  if (n <= 0) {
    close (fd);
    throw upan::exception(XLOC, "failed to read wdir/trlink.txt");
  }

  close (fd);
  buf[n] = '\0';
  if(strcmp(buf, targetFileContent) != 0) {
    throw upan::exception(XLOC, "target file content mismatch: %s", buf);
  }

  if (chdir("..")) {
    throw upan::exception(XLOC, "failed to change directory to ..");
  }
}

static void testSymlinkForNonExistentTarget() {
  if(symlink("/wdir/ldir/targetd.txt", "wdir/tdlink.txt")) {
    throw upan::exception(XLOC, "failed to create symlink wdir/tdlink.txt");
  }

  char buf[128];

  int n = readlink("wdir/tdlink.txt", buf, 128);
  if (n <= 0) {
    throw upan::exception(XLOC, "failed to readlink wdir/tdlink.txt");
  }

  buf[n] = '\0';
  if (strcmp("/wdir/ldir/targetd.txt", buf) != 0) {
    throw upan::exception(XLOC, "invalid link value: %s", buf);
  }

  int fd = open("wdir/tdlink.txt", O_RDONLY);
  if (fd >= 0) {
    throw upan::exception(XLOC, "open for non existent symlink wdir/tdlink.txt didn't fail");
  }

  char targetFileContent[] = "This is the link target (delayed) file";

  if (create("/wdir/ldir/targetd.txt", ATTR_FILE_DEFAULT)) {
    throw upan::exception(XLOC, "failed to create file: targetd.txt");
  }

  //printf("\n wdir/ldir/targetd.txt created");

  fd = open("/wdir/ldir/targetd.txt", O_RDWR);
  if (fd < 0) {
    throw upan::exception(XLOC, "failed to open file: targetd.txt");
  }

  int w = strlen(targetFileContent);
  n = write(fd, targetFileContent, w);
  if (n < w) {
    close(fd);
    upan::exception(XLOC, "failed to write to targetd.txt");
  }

  close(fd);

  fd = open("wdir/tdlink.txt", O_RDONLY);
  if (fd < 0) {
    throw upan::exception(XLOC, "failed to open wdir/tdlink.txt");
  }

  n = read(fd, buf, 128);
  if (n <= 0) {
    close (fd);
    throw upan::exception(XLOC, "failed to read wdir/tdlink.txt");
  }

  close (fd);
  buf[n] = '\0';
  if(strcmp(buf, targetFileContent) != 0) {
    throw upan::exception(XLOC, "target file content mismatch: %s", buf);
  }
}

static void testSymlinkOfSymlink(const char* targetFileContent) {
  if(symlink("/wdir/ldir/target.txt", "wdir/tlink1.txt")) {
    throw upan::exception(XLOC, "failed to create symlink wdir/tlink1.txt");
  }

  char buf[128];

  int n = readlink("wdir/tlink1.txt", buf, 128);
  if (n <= 0) {
    throw upan::exception(XLOC, "failed to readlink wdir/tlink1.txt");
  }

  buf[n] = '\0';
  if (strcmp("/wdir/ldir/target.txt", buf) != 0) {
    throw upan::exception(XLOC, "invalid link value: %s", buf);
  }

  if(symlink("wdir/tlink1.txt", "wdir/tlink2.txt")) {
    throw upan::exception(XLOC, "failed to create symlink wdir/tlink2.txt");
  }

  n = readlink("wdir/tlink2.txt", buf, 128);
  if (n <= 0) {
    throw upan::exception(XLOC, "failed to readlink wdir/tlink2.txt");
  }

  buf[n] = '\0';
  if (strcmp("wdir/tlink1.txt", buf) != 0) {
    throw upan::exception(XLOC, "invalid link value: %s", buf);
  }

  int fd = open("wdir/tlink1.txt", O_RDONLY);
  if (fd < 0) {
    throw upan::exception(XLOC, "failed to open wdir/tlink1.txt");
  }

  n = read(fd, buf, 128);
  if (n <= 0) {
    close (fd);
    throw upan::exception(XLOC, "failed to read wdir/tlink1.txt");
  }

  close (fd);
  buf[n] = '\0';
  if(strcmp(buf, targetFileContent) != 0) {
    throw upan::exception(XLOC, "target file content mismatch: %s", buf);
  }

  fd = open("wdir/tlink2.txt", O_RDONLY);
  if (fd < 0) {
    throw upan::exception(XLOC, "failed to open wdir/tlink2.txt");
  }

  n = read(fd, buf, 128);
  if (n <= 0) {
    close (fd);
    throw upan::exception(XLOC, "failed to read wdir/tlink2.txt");
  }

  close (fd);
  buf[n] = '\0';
  if(strcmp(buf, targetFileContent) != 0) {
    throw upan::exception(XLOC, "target file content mismatch: %s", buf);
  }

  if (unlink("wdir/tlink2.txt")) {
    throw upan::exception(XLOC, "failed to unlink wdir/tlink2.txt");
  }

  fd = open("wdir/tlink2.txt", O_RDONLY);
  if (fd >= 0) {
    throw upan::exception(XLOC, "open for non existent symlink wdir/tlink2.txt didn't fail");
  }

  fd = open("wdir/tlink1.txt", O_RDONLY);
  if (fd < 0) {
    throw upan::exception(XLOC, "failed to open wdir/tlink1.txt");
  }

  n = read(fd, buf, 128);
  if (n <= 0) {
    close (fd);
    throw upan::exception(XLOC, "failed to read wdir/tlink1.txt");
  }

  close (fd);
  buf[n] = '\0';
  if(strcmp(buf, targetFileContent) != 0) {
    throw upan::exception(XLOC, "target file content mismatch: %s", buf);
  }

  if (unlink("wdir/tlink1.txt")) {
    throw upan::exception(XLOC, "failed to unlink wdir/tlink1.txt");
  }

  fd = open("wdir/tlink1.txt", O_RDONLY);
  if (fd >= 0) {
    throw upan::exception(XLOC, "open for non existent symlink wdir/tlink1.txt didn't fail");
  }

  fd = open("wdir/ldir/target.txt", O_RDONLY);
  if (fd < 0) {
    throw upan::exception(XLOC, "failed to open wdir/ldir/target.txt");
  }

  n = read(fd, buf, 128);
  if (n <= 0) {
    close (fd);
    throw upan::exception(XLOC, "failed to read wdir/ldir/target.txt");
  }

  close (fd);
  buf[n] = '\0';
  if(strcmp(buf, targetFileContent) != 0) {
    throw upan::exception(XLOC, "target file content mismatch: %s", buf);
  }
}

void FileSystemTest::testSymlink() {
  try {
    if (mkdir("wdir", ATTR_DIR_DEFAULT)) {
      throw upan::exception(XLOC, "failed to create directory: wdir");
    }

    //printf("\n wdir created");

    if (mkdir("wdir/ldir", ATTR_DIR_DEFAULT)) {
      throw upan::exception(XLOC, "failed to create directory: ldir");
    }

    //printf("\n wdir/ldir created");

    char targetFileContent[] = "This is the link target file";

    if (create("wdir/ldir/target.txt", ATTR_FILE_DEFAULT)) {
      throw upan::exception(XLOC, "failed to create file: target.txt");
    }

    //printf("\n wdir/ldir/target.txt created");

    int fd = open("wdir/ldir/target.txt", O_RDWR);
    if (fd < 0) {
      throw upan::exception(XLOC, "failed to open file: target.txt");
    }

    int w = strlen(targetFileContent);
    int n = write(fd, targetFileContent, w);
    if (n < w) {
      close(fd);
      upan::exception(XLOC, "failed to write to target.txt");
    }

    close(fd);

    testSymlinkWithAbsolutePath(targetFileContent);
    testSymlinkWithRelativePath(targetFileContent);
    testSymlinkForNonExistentTarget();
    testSymlinkOfSymlink(targetFileContent);
  } catch(const upan::exception& e) {
    printf("\n symlink test failed... : %s", e.ErrorMsg().c_str());
  }

  recursiveDirectoryCleanup("wdir");
}