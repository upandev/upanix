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

#include <FileSystemTest.h>
#include <dirent.h>
#include <fs.h>
#include <exception.h>

void FileSystemTest::recursiveDirectoryCleanup(const upan::string& dirPath) {
  auto sd = opendir(dirPath.c_str());

  struct dirent* sde;
  while ((sde = readdir(sd))) {
    upan::string filePath(dirPath);
    filePath.concat("/", sde->d_name);

    struct stat st;
    if (stat(filePath.c_str(), &st)) {
      throw upan::exception(XLOC, "failed to get stat for file %s", filePath.c_str());
    }

    if (S_ISDIR(st.st_mode)) {
      recursiveDirectoryCleanup(filePath.c_str());
    } else {
      //printf("\n deleting %s", filePath.c_str());
      if (unlink(filePath.c_str())) {
        //printf("\n failed to delete %s", filePath.c_str());
      }
    }
  }

  closedir(sd);
  //printf("\n deleting %s", dirPath.c_str());
  if (unlink(dirPath.c_str())) {
    //printf("\n failed to delete %s", dirPath.c_str());
  }
}