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
#include <FileNode.h>
#include <SystemUtil.h>
#include <FileSystem.h>
#include <UserManager.h>
#include <StorageDrive.h>

void FileNode::Init(const char* szDirName, unsigned short usDirAttribute, int iUserID, unsigned uiParentSecNo, byte bParentSecPos) {
  strcpy((char*)_name, szDirName) ;

  _attribute = usDirAttribute ;

  _createdTime = SystemUtil_GetTimeOfDay();
  _accessedTime = _createdTime;
  _modifiedTime = _createdTime;

  _startSectorID = EOC ;
  _size = 0 ;

  _parentSectorID = uiParentSecNo ;
  _parentSectorPos = bParentSecPos ;

  _userID = iUserID ;
}

void FileNode::InitAsRoot() {
  Init(FS_ROOT_DIR, ATTR_DIR_DEFAULT, ROOT_USER_ID, 0, 0);
}

upan::string FileNode::FullPath(StorageDrive& diskDrive) {
  byte bSectorBuffer[512] ;

  const FileNode* pParseDirEntry = this;

  upan::string fullPath = "";
  upan::string temp = "";

  bool bFirst = true ;

  while(true)
  {
    if(strcmp(pParseDirEntry->Name(), FS_ROOT_DIR) == 0)
    {
      return upan::string(FS_ROOT_DIR) + fullPath;
    }
    else
    {
      upan::string curDir = pParseDirEntry->Name();
      if(!bFirst)
      {
        fullPath = curDir + FS_ROOT_DIR + fullPath;
      }
      else
      {
        fullPath = curDir;
        bFirst = false ;
      }
    }

    unsigned uiParSectorNo = pParseDirEntry->ParentSectorID() ;
    byte bParSectorPos = pParseDirEntry->ParentSectorPos() ;

    diskDrive.xRead(bSectorBuffer, uiParSectorNo, 1);

    pParseDirEntry = &((const FileNode*)bSectorBuffer)[bParSectorPos] ;
  }

  throw upan::exception(XLOC, "failed to find full path for directory/file %s", _name);
}

