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
  if(strcmp(buf, targetFileContent) != 0) {
    throw upan::exception(XLOC, "target file content mismatch: %s", buf);
  }
}

static void testRenameInSameDirectory(const char* srcFileContent) {
  if (create("sdir/sfile.txt", ATTR_FILE_DEFAULT)) {
    throw upan::exception(XLOC, "failed to create file: sfile.txt");
  }

  printf("\n sdir/sfile.txt created");

  int fd = open("sdir/sfile.txt", O_RDWR);
  if (fd < 0) {
    throw upan::exception(XLOC, "failed to open file: sdir/sfile.txt");
  }

  int w = strlen(srcFileContent);
  int n = write(fd, srcFileContent, w);
  if (n < w) {
    upan::exception(XLOC, "failed to write to sdir/sfile.txt");
  }

  printf("\n sdir content before renaming");
  auto sd = opendir("sdir");
  struct dirent* sde;
  while ((sde = readdir(sd))) {
    printf("\n   %s", sde->d_name);
  }

  closedir(sd);

  printf("\n renaming sdir/srfile.txt to sdir/srfile.txt");
  if (rename("sdir/sfile.txt", "sdir/srfile.txt")) {
    throw upan::exception(XLOC, "failed to rename file sdir/sfile.txt to sdir/srfile.txt");
  }

  printf("\n sdir content after renaming");
  sd = opendir("sdir");
  while ((sde = readdir(sd))) {
    printf("\n   %s", sde->d_name);
    if (strcmp(sde->d_name, "srfile.txt") != 0) {
      throw upan::exception(XLOC, "renamed file name doesn't match srfile.txt");
    }
  }

  closedir(sd);

  lseek(fd, 0, SEEK_SET);
  char rbuf[128];
  n = read(fd, rbuf, 128);
  if (n >= 0) {
    rbuf[n] = '\0';
    printf("\n Source file content: %s", rbuf);
  }

  close(fd);

  if (n != w) {
    throw upan::exception(XLOC, "failed to read %d bytes from sdir/srfile.txt", w);
  }

  if (strcmp(rbuf, srcFileContent) != 0) {
    throw upan::exception(XLOC, "mismatch in data from file write vs read");
  }
}

static void testRenameAcrossDirectory(const char* srcFileContent) {
  printf("\n renaming sdir/srfile.txt to ddir/drfile.txt");
  if (rename("sdir/srfile.txt", "ddir/drfile.txt")) {
    throw upan::exception(XLOC, "failed to rename file sdir/srfile.txt to ddir/drfile.txt");
  }

  auto fd = open("ddir/drfile.txt", O_RDONLY);
  if (fd < 0) {
    throw upan::exception(XLOC, "failed to open renamed file ddir/drfile.txt");
  }

  char rbuf[128];
  auto n = read(fd, rbuf, 128);
  if (n >= 0) {
    rbuf[n] = '\0';
    printf("\n Source file content: %s", rbuf);
  }

  int w = strlen(srcFileContent);
  if (n != w) {
    throw upan::exception(XLOC, "failed to read %d bytes from ddir/drfile.txt", w);
  }

  if (strcmp(rbuf, srcFileContent) != 0) {
    throw upan::exception(XLOC, "mismatch in data from file write vs read");
  }

  close(fd);

  printf("\n sdir content after renaming file under ddir");
  auto sd = opendir("sdir");
  struct dirent* sde;
  int count = 0;
  while ((sde = readdir(sd))) {
    printf("\n   %s", sde->d_name);
    ++count;
  }
  if (count != 0) {
    throw upan::exception(XLOC, "sdir is not empty");
  }

  closedir(sd);

  printf("\n ddir content after renaming");
  sd = opendir("ddir");
  while ((sde = readdir(sd))) {
    printf("\n   %s", sde->d_name);
    if (strcmp(sde->d_name, "drfile.txt") != 0) {
      throw upan::exception(XLOC, "renamed file name doesn't match drfile.txt");
    }
  }

  closedir(sd);
}

static void testRenameWithReplaceInSameDirectory(const char* srcFileContent) {
  if (create("ddir/dxfile.txt", ATTR_FILE_DEFAULT)) {
    throw upan::exception(XLOC, "failed to create file: dxfile.txt");
  }

  printf("\n ddir/dxfile.txt created");

  int fd = open("ddir/dxfile.txt", O_RDWR);
  if (fd < 0) {
    throw upan::exception(XLOC, "failed to open file: ddir/dxfile.txt");
  }

  char wbuf[] = "This is a dest rename replace file";
  int w = strlen(wbuf);
  int n = write(fd, wbuf, w);
  if (n < w) {
    upan::exception(XLOC, "failed to write to ddir/dxfile.txt");
  }

  close(fd);

  printf("\n ddir content before renaming");
  auto sd = opendir("ddir");
  struct dirent* sde;
  while ((sde = readdir(sd))) {
    printf("\n   %s", sde->d_name);
  }
  closedir(sd);

  printf("\n renaming ddir/drfile.txt to ddir/dxfile.txt");
  if (rename("ddir/drfile.txt", "ddir/dxfile.txt")) {
    throw upan::exception(XLOC, "failed to rename file ddir/drfile.txt to ddir/dxfile.txt");
  }

  printf("\n ddir content after renaming");
  sd = opendir("ddir");
  while ((sde = readdir(sd))) {
    printf("\n   %s", sde->d_name);
    if (strcmp(sde->d_name, "dxfile.txt") != 0) {
      throw upan::exception(XLOC, "renamed file name doesn't match dxfile.txt");
    }
  }

  closedir(sd);

  fd = open("ddir/dxfile.txt", O_RDONLY);
  if (fd < 0) {
    throw upan::exception(XLOC, "failed to open ddir/dxfile.txt");
  }

  char rbuf[128];
  n = read(fd, rbuf, 128);
  if (n >= 0) {
    rbuf[n] = '\0';
    printf("\n Source file content: %s", rbuf);
  }

  close(fd);

  w = strlen(srcFileContent);
  if (n != w) {
    throw upan::exception(XLOC, "failed to read %d bytes from ddir/dxfile.txt", w);
  }

  if (strcmp(rbuf, srcFileContent) != 0) {
    throw upan::exception(XLOC, "mismatch in data from file write vs read");
  }
}

static void testRenameWithReplaceAcrossDirectory(const char* srcFileContent) {
  int fd = open("sdir/sxfile.txt", O_RDWR | O_CREAT, ATTR_FILE_DEFAULT);
  if (fd < 0) {
    throw upan::exception(XLOC, "failed to open file: sdir/sxfile.txt");
  }

  printf("\n sdir/sxfile.txt created");

  char wbuf[] = "This is a src rename replace file";
  int w = strlen(wbuf);
  int n = write(fd, wbuf, w);
  if (n < w) {
    upan::exception(XLOC, "failed to write to sdir/sxfile.txt");
  }

  close(fd);

  printf("\n ddir content before renaming");
  auto sd = opendir("ddir");
  struct dirent* sde;
  while ((sde = readdir(sd))) {
    printf("\n   %s", sde->d_name);
  }
  closedir(sd);

  printf("\n sdir content before renaming");
  sd = opendir("sdir");
  while ((sde = readdir(sd))) {
    printf("\n   %s", sde->d_name);
  }
  closedir(sd);

  printf("\n renaming ddir/dxfile.txt to sdir/sxfile.txt");
  if (rename("ddir/dxfile.txt", "sdir/sxfile.txt")) {
    throw upan::exception(XLOC, "failed to rename file ddir/dxfile.txt to sdir/sxfile.txt");
  }

  printf("\n ddir content after renaming");
  sd = opendir("ddir");
  while ((sde = readdir(sd))) {
    printf("\n   %s", sde->d_name);
    closedir(sd);
    throw upan::exception(XLOC, "ddir is not empty after renaming the file dxfile.txt");
  }
  closedir(sd);

  printf("\n sdir content before renaming");
  sd = opendir("sdir");
  while ((sde = readdir(sd))) {
    printf("\n   %s", sde->d_name);
    if (strcmp(sde->d_name, "sxfile.txt") != 0) {
      throw upan::exception(XLOC, "renamed file name doesn't match sxfile.txt");
    }
  }
  closedir(sd);

  fd = open("sdir/sxfile.txt", O_RDONLY);
  if (fd < 0) {
    throw upan::exception(XLOC, "failed to open sdir/sxfile.txt");
  }

  char rbuf[128];
  n = read(fd, rbuf, 128);
  if (n >= 0) {
    rbuf[n] = '\0';
    printf("\n Source file content: %s", rbuf);
  }

  close(fd);

  w = strlen(srcFileContent);
  if (n != w) {
    throw upan::exception(XLOC, "failed to read %d bytes from sdir/sxfile.txt", w);
  }

  if (strcmp(rbuf, srcFileContent) != 0) {
    throw upan::exception(XLOC, "mismatch in data from file write vs read");
  }
}

void FileSystemTest::testSymlink() {
  try {
    if (mkdir("wdir", ATTR_DIR_DEFAULT)) {
      throw upan::exception(XLOC, "failed to create directory: wdir");
    }

    printf("\n wdir created");

    if (mkdir("wdir/ldir", ATTR_DIR_DEFAULT)) {
      throw upan::exception(XLOC, "failed to create directory: ldir");
    }

    printf("\n wdir/ldir created");

    char targetFileContent[] = "This is the link target file";

    if (create("wdir/ldir/target.txt", ATTR_FILE_DEFAULT)) {
      throw upan::exception(XLOC, "failed to create file: target.txt");
    }

    printf("\n wdir/ldir/target.txt created");

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
  } catch(const upan::exception& e) {
    printf("\n symlink test failed...");
    e.Print();
  }

  printf("\n cleaning up");
  recursiveDirectoryCleanup("wdir");
}