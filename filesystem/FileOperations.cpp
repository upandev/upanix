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
# include <FileOperations.h>
# include <FileDescriptor.h>
# include <Directory.h>
# include <FileSystem.h>
# include <ProcessManager.h>
# include <MountManager.h>
# include <SystemUtil.h>
# include <DMM.h>
# include <StringUtil.h>
# include <stdio.h>
# include <list.h>
# include <try.h>

/************************************************************************************************************/
static upan::result<unsigned short> FileOperations_ValidateAndGetFileAttr(unsigned short usFileType, unsigned short usMode)
{
	usMode = FILE_PERM(usMode) ;
	usFileType = FILE_TYPE(usFileType) ;

	if(!(usFileType == ATTR_TYPE_FILE || usFileType == ATTR_TYPE_DIRECTORY))
    return upan::result<unsigned short>::bad("invalid file attribute: %x", usFileType);

  return upan::good((unsigned short)(usFileType | usMode));
}

static void FileOperations_ParseFilePathWithDrive(const char* szFileNameWithDrive, char* szFileName, unsigned* pDriveID)
{
	int i = String_Chr(szFileNameWithDrive, '@') ;
	
	if(i == -1)
	{
		*pDriveID = ProcessManager::Instance().GetCurrentPAS().driveID() ;
		strcpy(szFileName, szFileNameWithDrive) ;
		return ;
	}

	if(i > 32)
	{
		strcpy(szFileName, szFileNameWithDrive) ;
		return ;
	}

	char szDriveName[33] ;

  memcpy(szDriveName, szFileNameWithDrive, i);
	szDriveName[i] = '\0' ;

	strcpy(szFileName, szFileNameWithDrive + i + 1) ;

	if(strcmp(szDriveName, ROOT_DRIVE_SYN) == 0)
	{
		*pDriveID = ROOT_DRIVE_ID ;
	}
	else
	{
    DiskDrive* pDiskDrive = DiskDriveManager::Instance().GetByDriveName(szDriveName, false).goodValueOrElse(nullptr);
		if(pDiskDrive == NULL)
			*pDriveID = ROOT_DRIVE_ID ;
		else
			*pDriveID = pDiskDrive->Id();
	}
}

static void __FileOperations_Create(const char* szFile, unsigned short usFileType, unsigned short usMode, Process& pas, int driveId);
static bool __FileOperations_Exists(const char* szFile, unsigned short usFileType, Process& pas, int driveId);


/************************************************************************************************************/
FileDescriptor& FileOperations_Open(const char* szFileName, const byte mode) {
	int iDriveID ;
	char szFile[100] ;
	FileOperations_ParseFilePathWithDrive(szFileName, szFile, (unsigned*)&iDriveID) ;

	Process* pPAS = &ProcessManager::Instance().GetCurrentPAS() ;

  if ( (mode & O_APPEND) || (mode & O_CREAT) ) {
    if(!__FileOperations_Exists(szFileName, ATTR_TYPE_FILE, *pPAS, iDriveID)) {
      __FileOperations_Create(szFileName, ATTR_TYPE_FILE, ATTR_FILE_DEFAULT, *pPAS, iDriveID);
    }
  }

  DiskDrive* pDiskDrive = DiskDriveManager::Instance().GetByID(iDriveID, true).goodValueOrThrow(XLOC);

  FileSystem::Node dirEntry = Directory_GetDirEntry(szFile, pPAS, iDriveID);

  if(!dirEntry.IsFile())
    throw upan::exception(XLOC, "%s file doesn't exist", szFileName);

  if(!pPAS->hasFilePermission(dirEntry, mode))
    throw upan::exception(XLOC, "insufficient permission to open file %s", szFileName);

	if( (mode & O_TRUNC) ) {
    FileOperations_Delete(szFileName);
    FileOperations_Create(szFileName, FILE_TYPE(dirEntry.Attribute()), FILE_PERM(dirEntry.Attribute()));
    dirEntry = Directory_GetDirEntry(szFile, pPAS, iDriveID);
	}

  char nodeIdBuf[64];
  sprintf(nodeIdBuf, "%d:%d:%d", pDiskDrive->Id(), dirEntry.ParentSectorID(), dirEntry.ParentSectorPos());
  const upan::string nodeId(nodeIdBuf);

  return dynamic_cast<FileDescriptor&>(pPAS->iodTable().allocate([&](int fd) {
    return new FileDescriptor(pPAS->processID(), fd, mode,
                              nodeId, dirEntry.FullPath(*pDiskDrive),
                              *pDiskDrive, dirEntry.StartSectorID());
  }));
}

byte FileOperations_Close(int fd) {
  try {
    ProcessManager::Instance().GetCurrentPAS().iodTable().free(fd);
  } catch(upan::exception& e) {
    e.Print();
    return FileOperations_FAILURE;
  }
	return FileOperations_SUCCESS ;
}

bool FileOperations_ReadLine(int fd, upan::string& line)
{
  line = "";
  const int CHUNK_SIZE = 64;
  char buffer[CHUNK_SIZE + 1];
  upan::list<upan::string> buffers;
  auto& file = ProcessManager::Instance().GetCurrentPAS().iodTable().getRealNonDupped(fd);
  while(true)
  {
    int readLen = file.read(buffer, CHUNK_SIZE);
    
    if(readLen == 0)
      break;

    //find new line
    int i = 0;
    for(i = 0; i < readLen; ++i)
      if(buffer[i] == '\n')
        break;
    buffer[i] = '\0';

    buffers.push_back(buffer);

    int offset = i - readLen + 1;
    if(offset < 0)
    {
      file.seek(SEEK_CUR, offset);
      break;
    }
  }

  if(buffers.empty())
    return false;
  for(auto chunk : buffers)
    line += chunk;
  return true;
}

void FileOperations_Create(const char* szFilePath, unsigned short usFileType, unsigned short usMode) {
  int iDriveID;
  char szFile[100];
  FileOperations_ParseFilePathWithDrive(szFilePath, szFile, (unsigned *) &iDriveID);
  __FileOperations_Create(szFile, usFileType, usMode, ProcessManager::Instance().GetCurrentPAS(), iDriveID);
}

void __FileOperations_Create(const char* szFile, unsigned short usFileType, unsigned short usMode, Process& pas, int driveId) {
  const unsigned short usFileAttr = FileOperations_ValidateAndGetFileAttr(usFileType, usMode).goodValueOrThrow(XLOC);

	byte bParentDirectoryBuffer[512] ;
	char szDirName[33] ;
  FileSystem::CWD CWD ;
	unsigned uiParentSectorNo ;
	byte bParentSectorPos ;

  Directory_GetDirEntryForCreateDelete(&pas, driveId, szFile, szDirName, uiParentSectorNo, bParentSectorPos, bParentDirectoryBuffer);

  CWD.pDirEntry = ((FileSystem::Node*)bParentDirectoryBuffer) + bParentSectorPos ;
	CWD.uiSectorNo = uiParentSectorNo ;
	CWD.bSectorEntryPosition = bParentSectorPos ;

  if(!pas.hasFilePermission(*CWD.pDirEntry, O_RDWR))
    throw upan::exception(XLOC, "insufficient permission to create file: %s", szFile);

  Directory_Create(&pas, driveId, bParentDirectoryBuffer, &CWD, szDirName, usFileAttr);
}

void FileOperations_Delete(const char* szFilePath)
{
	int iDriveID ;
	char szFile[100] ;
	FileOperations_ParseFilePathWithDrive(szFilePath, szFile, (unsigned*)&iDriveID) ;

	Process* pPAS = &ProcessManager::Instance().GetCurrentPAS() ;

	byte bParentDirectoryBuffer[512] ;
	char szDirName[33] ;
  FileSystem::CWD CWD ;
	unsigned uiParentSectorNo ;
	byte bParentSectorPos ;

  Directory_GetDirEntryForCreateDelete(pPAS, iDriveID, szFile, szDirName, uiParentSectorNo, bParentSectorPos, bParentDirectoryBuffer);

  CWD.pDirEntry = ((FileSystem::Node*)bParentDirectoryBuffer) + bParentSectorPos ;
	CWD.uiSectorNo = uiParentSectorNo ;
	CWD.bSectorEntryPosition = bParentSectorPos ;

  if(!pPAS->hasFilePermission(*CWD.pDirEntry, O_RDWR))
    throw upan::exception(XLOC, "insufficient permission to delete file: %s", szFilePath);

  const FileSystem::Node& fileDirEntry = FileOperations_GetDirEntry(szFilePath);

  if(pPAS->fileUserType(fileDirEntry) != USER_OWNER)
    throw upan::exception(XLOC, "insufficient permission to delete file: %s", szFilePath);

  Directory_Delete(pPAS, iDriveID, bParentDirectoryBuffer, &CWD, szDirName);
}

bool FileOperations_Exists(const char* szFileName, unsigned short usFileType) {
  int iDriveID;
  char szFile[100];
  FileOperations_ParseFilePathWithDrive(szFileName, szFile, (unsigned *) &iDriveID);
  return __FileOperations_Exists(szFile, usFileType, ProcessManager::Instance().GetCurrentPAS(), iDriveID);
}

bool __FileOperations_Exists(const char* szFile, unsigned short usFileType, Process& pas, int driveId) {
  try {
    const FileSystem::Node dirEntry = Directory_GetDirEntry(szFile, &pas, driveId);
    if((dirEntry.Attribute() & usFileType) != usFileType)
      return false;
  } catch(const upan::exception& ex) {
    ex.Print();
    return false;
  }
  return true;
}

uint32_t FileOperations_GetOffset(int fd) {
  return ProcessManager::Instance().GetCurrentPAS().iodTable().getRealNonDupped(fd).getOffset();
}

void FileOperations_GetCWD(char* szPathBuf, int iBufSize) {
  FileSystem::PresentWorkingDirectory& pwd = ProcessManager::Instance().GetCurrentPAS().processPWD();
	int iDriveID = ProcessManager::Instance().GetCurrentPAS().driveID() ;

  DiskDrive* pDiskDrive = DiskDriveManager::Instance().GetByID(iDriveID, true).goodValueOrThrow(XLOC);

  const upan::string& fullPath = pwd.DirEntry.FullPath(*pDiskDrive);

  if(fullPath.length() > iBufSize)
    throw upan::exception(XLOC, "%d buf-size is smaller than path size %d", iBufSize, fullPath.length());

  strcpy(szPathBuf, fullPath.c_str());
}

const FileSystem::Node FileOperations_GetDirEntry(const char* szFileName)
{
	int iDriveID ;
	char szFile[100] ;
	FileOperations_ParseFilePathWithDrive(szFileName, szFile, (unsigned*)&iDriveID) ;

  DiskDrive* pDiskDrive = DiskDriveManager::Instance().GetByID(iDriveID, true).goodValueOrThrow(XLOC);

  FileSystem::CWD CWD ;
	if(iDriveID == ProcessManager::Instance().GetCurrentPAS().driveID())
	{
    FileSystem::PresentWorkingDirectory& pwd = ProcessManager::Instance().GetCurrentPAS().processPWD();
		CWD.pDirEntry = &pwd.DirEntry ;
		CWD.uiSectorNo = pwd.uiSectorNo ;
		CWD.bSectorEntryPosition = pwd.bSectorEntryPosition ;
	}
	else
	{
		CWD.pDirEntry = &(pDiskDrive->_fileSystem.FSpwd.DirEntry) ;
		CWD.uiSectorNo = pDiskDrive->_fileSystem.FSpwd.uiSectorNo ;
		CWD.bSectorEntryPosition = pDiskDrive->_fileSystem.FSpwd.bSectorEntryPosition ;
	}

	unsigned uiSectorNo ;
	byte bSectorPos ;
	byte bDirectoryBuffer[512] ;
	
  Directory_ReadDirEntryInfo(*pDiskDrive, CWD, szFile, uiSectorNo, bSectorPos, bDirectoryBuffer);

  return ((FileSystem::Node*)bDirectoryBuffer)[bSectorPos] ;
}

FileSystem_FileStat FileOperations_GetStat(const char* szFileName, int iDriveID) {
  char szFile[100];
  if (iDriveID == FROM_FILE) {
    FileOperations_ParseFilePathWithDrive(szFileName, szFile, (unsigned *) &iDriveID);
  } else {
    strcpy(szFile, szFileName);
  }

  DiskDrive *pDiskDrive = DiskDriveManager::Instance().GetByID(iDriveID, true).goodValueOrThrow(XLOC);

  FileSystem::CWD CWD;

  if (iDriveID == ProcessManager::Instance().GetCurrentPAS().driveID()) {
    FileSystem::PresentWorkingDirectory &pwd = ProcessManager::Instance().GetCurrentPAS().processPWD();
    CWD.pDirEntry = &pwd.DirEntry;
    CWD.uiSectorNo = pwd.uiSectorNo;
    CWD.bSectorEntryPosition = pwd.bSectorEntryPosition;
  } else {
    CWD.pDirEntry = &(pDiskDrive->_fileSystem.FSpwd.DirEntry);
    CWD.uiSectorNo = pDiskDrive->_fileSystem.FSpwd.uiSectorNo;
    CWD.bSectorEntryPosition = pDiskDrive->_fileSystem.FSpwd.bSectorEntryPosition;
  }
  return FileOperations_GetStat(*pDiskDrive, CWD, szFile);
}

FileSystem_FileStat FileOperations_GetStat(DiskDrive& diskDrive, const FileSystem::CWD& cwd, const char* szFileName) {
	unsigned uiSectorNo ;
	byte bSectorPos ;
	byte bDirectoryBuffer[512] ;
	
  Directory_ReadDirEntryInfo(diskDrive, cwd, szFileName, uiSectorNo, bSectorPos, bDirectoryBuffer);

  FileSystem::Node* pSrcDirEntry = &((FileSystem::Node*)bDirectoryBuffer)[bSectorPos] ;

  FileSystem_FileStat fileStat;

  fileStat.st_dev = diskDrive.DriveNumber();
  fileStat.st_mode = pSrcDirEntry->Attribute() ;
  fileStat.st_uid = pSrcDirEntry->UserID() ;
  fileStat.st_size = pSrcDirEntry->Size() ;
  fileStat.st_atime = pSrcDirEntry->AccessedTime() ;
  fileStat.st_mtime = pSrcDirEntry->ModifiedTime() ;
  fileStat.st_ctime = pSrcDirEntry->CreatedTime() ;

  fileStat.st_blksize = 512 ;
  fileStat.st_blocks = (pSrcDirEntry->Size() / 512) + ((pSrcDirEntry->Size() % 512) ? 1 : 0 ) ;

  fileStat.st_rdev = 0 ;
  fileStat.st_gid = 1 ;
  fileStat.st_nlink = 1 ;
  fileStat.st_ino = 0 ;

  return fileStat;
}

void FileOperations_UpdateTime(DiskDrive& diskDrive, const FileSystem::CWD& cwd, const char* szFileName, byte bTimeType) {
	unsigned uiSectorNo ;
	byte bSectorPos ;
	byte bDirectoryBuffer[512] ;
	
  Directory_ReadDirEntryInfo(diskDrive, cwd, szFileName, uiSectorNo, bSectorPos, bDirectoryBuffer);

  FileSystem::Node* pSrcDirEntry = &((FileSystem::Node*)bDirectoryBuffer)[bSectorPos] ;
	if(bTimeType & DIR_ACCESS_TIME)
    pSrcDirEntry->AccessedTime(SystemUtil_GetTimeOfDay());

	if(bTimeType & DIR_MODIFIED_TIME)
    pSrcDirEntry->ModifiedTime(SystemUtil_GetTimeOfDay());

  diskDrive.xWrite(bDirectoryBuffer, uiSectorNo, 1);
}

byte FileOperations_GetFileOpenMode(int fd) {
  return ProcessManager::Instance().GetCurrentPAS().iodTable().getRealNonDupped(fd).getMode();
}

void FileOperations_SyncPWD()
{
  Directory_SyncPWD(&ProcessManager::Instance().GetCurrentPAS());
}

void FileOperations_ChangeDir(const char* szFileName)
{
	int iDriveID ;
	char szFile[100] ;
	FileOperations_ParseFilePathWithDrive(szFileName, szFile, (unsigned*)&iDriveID) ;

	Process* pPAS = &ProcessManager::Instance().GetCurrentPAS() ;
	
  Directory_Change(szFile, iDriveID, pPAS);
}

void FileOperations_GetDirectoryContent(const char* szPathAddress, FileSystem::Node** pDirList, int* iListSize)
{
	int iDriveID ;
	char szPath[100] ;
	FileOperations_ParseFilePathWithDrive(szPathAddress, szPath, (unsigned*)&iDriveID) ;

	Process* pPAS = &ProcessManager::Instance().GetCurrentPAS() ;

  Directory_GetDirectoryContent(szPath, pPAS, iDriveID, pDirList, iListSize);
}

bool FileOperations_FileAccess(const char* szFileName, int iDriveID, int mode)
{
  try
  {
    char szFile[100] ;
    if(iDriveID == FROM_FILE)
    {
      FileOperations_ParseFilePathWithDrive(szFileName, szFile, (unsigned*)&iDriveID) ;
    }
    else
    {
      strcpy(szFile, szFileName) ;
    }

    Process* pPAS = &ProcessManager::Instance().GetCurrentPAS() ;

    const FileSystem::Node& dirEntry = Directory_GetDirEntry(szFile, pPAS, iDriveID);

    if(!dirEntry.IsFile())
      return false;

    if(!pPAS->hasFilePermission(dirEntry, mode))
      return false;
  }
  catch(const upan::exception& ex)
  {
    ex.Print();
    return false;
  }

  return true;
}

void FileOperations_Dup2(int oldFD, int newFD) {
  ProcessManager::Instance().GetCurrentPAS().iodTable().dup2(oldFD, newFD);
}