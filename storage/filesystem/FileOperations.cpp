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
# include <StorageDriveManager.h>

/************************************************************************************************************/
static upan::result<unsigned short> FileOperations_ValidateAndGetFileAttr(unsigned short usFileType, unsigned short usMode)
{
	usMode = FILE_PERM(usMode) ;
	usFileType = FILE_TYPE(usFileType) ;

	if(!(usFileType == ATTR_TYPE_FILE || usFileType == ATTR_TYPE_DIRECTORY))
    return upan::result<unsigned short>::bad("invalid file attribute: %x", usFileType);

  return upan::good((unsigned short)(usFileType | usMode));
}

static void FileOperations_ParseFilePathWithDrive(const char* szFileNameWithDrive, char* szFileName, unsigned* pDriveID) {
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
    auto r = StorageDriveManager::Instance().GetByDriveName(szDriveName, false);
    *pDriveID = r.isGood() ? r.goodValue().Id() : ROOT_DRIVE_ID;
	}
}

/************************************************************************************************************/
FileDescriptor& FileOperations::open(const char* szFileName, const byte mode) {
  upan::mutex_guard g(_fileOpMutex);

	int iDriveID ;
	char szFile[100] ;
	FileOperations_ParseFilePathWithDrive(szFileName, szFile, (unsigned*)&iDriveID) ;

	auto& pas = ProcessManager::Instance().GetCurrentPAS() ;

  if ( (mode & O_APPEND) || (mode & O_CREAT) ) {
    if(!_exists(szFileName, ATTR_TYPE_FILE, pas, iDriveID)) {
      _create(szFileName, ATTR_TYPE_FILE, ATTR_FILE_DEFAULT, pas, iDriveID);
    }
  }

  StorageDrive& diskDrive = StorageDriveManager::Instance().GetByID(iDriveID, true).goodValueOrThrow(XLOC);

  FileNode dirEntry = Directory_GetDirEntry(szFile, pas, iDriveID);

  if(!dirEntry.IsFile())
    throw upan::exception(XLOC, "%s file doesn't exist", szFileName);

  if(!pas.hasFilePermission(dirEntry, mode))
    throw upan::exception(XLOC, "insufficient permission to open file %s", szFileName);

	if( (mode & O_TRUNC) ) {
    FileOperations::Instance().remove(szFileName);
    FileOperations::Instance().create(szFileName, FILE_TYPE(dirEntry.Attribute()), FILE_PERM(dirEntry.Attribute()));
    dirEntry = Directory_GetDirEntry(szFile, pas, iDriveID);
	}

  char nodeIdBuf[64];
  sprintf(nodeIdBuf, "%d:%d:%d", diskDrive.Id(), dirEntry.ParentSectorID(), dirEntry.ParentSectorPos());
  const upan::string nodeId(nodeIdBuf);

  return dynamic_cast<FileDescriptor&>(pas.iodTable().allocate([&](int fd) {
    return new FileDescriptor(pas.processID(), fd, mode,
                              nodeId, dirEntry.FullPath(diskDrive),
                              diskDrive, dirEntry.StartSectorID());
  }));
}

bool FileOperations::close(int fd) {
  try {
    ProcessManager::Instance().GetCurrentPAS().iodTable().free(fd);
  } catch(upan::exception& e) {
    e.Print();
    return false;
  }
	return true;
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

StorageDrive& FileOperations::parseFilePath(const upan::string& fullFilePath, FileSystem::DirectoryRef& cwd, upan::string &filePath) {
  auto& process = ProcessManager::Instance().GetCurrentPAS();
  const upan::string driveMark("@/");

  const int drivePos = fullFilePath.find(driveMark);
  if (drivePos >= 0) {
    const upan::string& driveName = fullFilePath.substr(0, drivePos);
    if (driveName.find('/') < 0) {
      filePath = fullFilePath.substr(drivePos + driveMark.length());
      auto& storageDrive = StorageDriveManager::Instance().GetByDriveName(driveName, true).goodValueOrThrow(XLOC);
      cwd = storageDrive.fileSystem().root();
      return storageDrive;
    }
  }

  auto& storageDrive = StorageDriveManager::Instance().GetByID(process.driveID(), true).goodValueOrThrow(XLOC);
  if (fullFilePath[0] == '/') {
    cwd = storageDrive.fileSystem().root();
  } else {
    cwd = process.pwd();
  }
  filePath = fullFilePath;
  return storageDrive;
}

void FileOperations::create(const upan::string& filePath, uint16_t fileType, uint16_t mode) {
  FileSystem::DirectoryRef cwd;
  upan::string parsedFilePath;
  auto& storageDrive = parseFilePath(filePath, cwd, parsedFilePath);
  //storageDrive.fileSystem().create(parsedFilePath, fileType, mode);
}

void FileOperations::_create(const char* szFile, unsigned short usFileType, unsigned short usMode, Process& pas, int driveId) {
  const unsigned short usFileAttr = FileOperations_ValidateAndGetFileAttr(usFileType, usMode).goodValueOrThrow(XLOC);

	byte bParentDirectoryBuffer[512] ;
	char szDirName[33] ;
	unsigned uiParentSectorNo ;
	byte bParentSectorPos ;

  StorageDrive& diskDrive = StorageDriveManager::Instance().GetByID(driveId, true).goodValueOrThrow(XLOC);
  Directory_GetDirEntryForCreateDelete(pas, diskDrive, szFile, szDirName, uiParentSectorNo, bParentSectorPos, bParentDirectoryBuffer);

  FileSystem::WorkingDirectory cwd(((FileNode*)bParentDirectoryBuffer) + bParentSectorPos, uiParentSectorNo, bParentSectorPos);

  if(!pas.hasFilePermission(*cwd.getNode(), O_RDWR)) {
    throw upan::exception(XLOC, "insufficient permission to create file: %s", szFile);
  }

  Directory_Create(&pas, diskDrive, bParentDirectoryBuffer, cwd, szDirName, usFileAttr);
}

void FileOperations::remove(const char* szFilePath) {
  upan::mutex_guard g(_fileOpMutex);

	int iDriveID ;
	char szFile[100] ;

	FileOperations_ParseFilePathWithDrive(szFilePath, szFile, (unsigned*)&iDriveID) ;

	auto& pas = ProcessManager::Instance().GetCurrentPAS() ;

	byte bParentDirectoryBuffer[512] ;
	char szDirName[33] ;
	unsigned uiParentSectorNo ;
	byte bParentSectorPos ;

  StorageDrive& diskDrive = StorageDriveManager::Instance().GetByID(iDriveID, true).goodValueOrThrow(XLOC);
  Directory_GetDirEntryForCreateDelete(pas, diskDrive, szFile, szDirName, uiParentSectorNo, bParentSectorPos, bParentDirectoryBuffer);

  FileSystem::WorkingDirectory cwd(((FileNode*)bParentDirectoryBuffer) + bParentSectorPos, uiParentSectorNo, bParentSectorPos);

  if(!pas.hasFilePermission(*cwd.getNode(), O_RDWR))
    throw upan::exception(XLOC, "insufficient permission to delete file: %s", szFilePath);

  const FileNode& fileDirEntry = FileOperations_GetDirEntry(szFilePath);

  if(pas.fileUserType(fileDirEntry) != USER_OWNER)
    throw upan::exception(XLOC, "insufficient permission to delete file: %s", szFilePath);

  Directory_Delete(pas, diskDrive, bParentDirectoryBuffer, cwd, szDirName);
}

bool FileOperations::exists(const char* szFileName, unsigned short usFileType) {
  upan::mutex_guard g(_fileOpMutex);

  int iDriveID;
  char szFile[100];
  FileOperations_ParseFilePathWithDrive(szFileName, szFile, (unsigned *) &iDriveID);
  return _exists(szFile, usFileType, ProcessManager::Instance().GetCurrentPAS(), iDriveID);
}

bool FileOperations::_exists(const char* szFile, unsigned short usFileType, Process& pas, int driveId) {
  try {
    const FileNode dirEntry = Directory_GetDirEntry(szFile, pas, driveId);
    if((dirEntry.Attribute() & usFileType) != usFileType)
      return false;
  } catch(const upan::exception& ex) {
    ex.Print();
    return false;
  }
  return true;
}

void FileOperations_GetCWD(char* szPathBuf, int iBufSize) {
  FileSystem::PresentWorkingDirectory& pwd = ProcessManager::Instance().GetCurrentPAS().processPWD();
	int iDriveID = ProcessManager::Instance().GetCurrentPAS().driveID() ;

  StorageDrive& diskDrive = StorageDriveManager::Instance().GetByID(iDriveID, true).goodValueOrThrow(XLOC);

  const upan::string& fullPath = pwd.getNode().FullPath(diskDrive);

  if(fullPath.length() > iBufSize)
    throw upan::exception(XLOC, "%d buf-size is smaller than path size %d", iBufSize, fullPath.length());

  strcpy(szPathBuf, fullPath.c_str());
}

FileNode FileOperations_GetDirEntry(const char* szFileName) {
	int iDriveID ;
	char szFile[100] ;
	FileOperations_ParseFilePathWithDrive(szFileName, szFile, (unsigned*)&iDriveID) ;

  StorageDrive& diskDrive = StorageDriveManager::Instance().GetByID(iDriveID, true).goodValueOrThrow(XLOC);
  auto& pas = ProcessManager::Instance().GetCurrentPAS();

  FileSystem::WorkingDirectory cwd = (iDriveID == pas.driveID()) ? pas.processPWD() : diskDrive.fileSystem().pwd();

	unsigned uiSectorNo ;
	byte bSectorPos ;
	byte bDirectoryBuffer[512] ;
	
  Directory_ReadDirEntryInfo(diskDrive, cwd, szFile, uiSectorNo, bSectorPos, bDirectoryBuffer);

  return ((FileNode*)bDirectoryBuffer)[bSectorPos] ;
}

struct stat FileOperations_GetStat(const char* szFileName, int iDriveID) {
  char szFile[100];
  if (iDriveID == FROM_FILE) {
    FileOperations_ParseFilePathWithDrive(szFileName, szFile, (unsigned *) &iDriveID);
  } else {
    strcpy(szFile, szFileName);
  }

  StorageDrive& diskDrive = StorageDriveManager::Instance().GetByID(iDriveID, true).goodValueOrThrow(XLOC);
  auto& pas = ProcessManager::Instance().GetCurrentPAS();

  FileSystem::WorkingDirectory cwd = iDriveID == pas.driveID() ? pas.processPWD() : diskDrive.fileSystem().pwd();
  return FileOperations_GetStat(diskDrive, cwd, szFile);
}

struct stat FileOperations_GetStat(StorageDrive& diskDrive, const FileSystem::WorkingDirectory& cwd, const char* szFileName) {
	unsigned uiSectorNo ;
	byte bSectorPos ;
	byte bDirectoryBuffer[512] ;
	
  Directory_ReadDirEntryInfo(diskDrive, cwd, szFileName, uiSectorNo, bSectorPos, bDirectoryBuffer);

  FileNode* pSrcDirEntry = &((FileNode*)bDirectoryBuffer)[bSectorPos] ;

  struct stat fileStat;

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

void FileOperations_UpdateTime(StorageDrive& diskDrive, const FileSystem::WorkingDirectory& cwd, const char* szFileName, byte bTimeType) {
	unsigned uiSectorNo ;
	byte bSectorPos ;
	byte bDirectoryBuffer[512] ;
	
  Directory_ReadDirEntryInfo(diskDrive, cwd, szFileName, uiSectorNo, bSectorPos, bDirectoryBuffer);

  FileNode* pSrcDirEntry = &((FileNode*)bDirectoryBuffer)[bSectorPos] ;
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
  Directory_SyncPWD(ProcessManager::Instance().GetCurrentPAS());
}

void FileOperations_ChangeDir(const char* szFileName)
{
	int iDriveID ;
	char szFile[100] ;
	FileOperations_ParseFilePathWithDrive(szFileName, szFile, (unsigned*)&iDriveID) ;
  Directory_Change(szFile, iDriveID, ProcessManager::Instance().GetCurrentPAS());
}

void FileOperations_GetDirectoryContent(const char* szPathAddress, FileNode** pDirList, int* iListSize)
{
	int iDriveID ;
	char szPath[100] ;
	FileOperations_ParseFilePathWithDrive(szPathAddress, szPath, (unsigned*)&iDriveID) ;
  Directory_GetDirectoryContent(szPath, ProcessManager::Instance().GetCurrentPAS(), iDriveID, pDirList, iListSize);
}

bool FileOperations_FileAccess(const char* szFileName, int iDriveID, int mode) {
  try {
    char szFile[100] ;
    if(iDriveID == FROM_FILE) {
      FileOperations_ParseFilePathWithDrive(szFileName, szFile, (unsigned*)&iDriveID) ;
    } else {
      strcpy(szFile, szFileName) ;
    }

    auto& pas = ProcessManager::Instance().GetCurrentPAS() ;
    const FileNode& dirEntry = Directory_GetDirEntry(szFile, pas, iDriveID);

    if(!dirEntry.IsFile()) {
      return false;
    }

    if(!pas.hasFilePermission(dirEntry, mode)) {
      return false;
    }
  } catch(const upan::exception& ex) {
    ex.Print();
    return false;
  }

  return true;
}

void FileOperations_Dup2(int oldFD, int newFD) {
  ProcessManager::Instance().GetCurrentPAS().iodTable().dup2(oldFD, newFD);
}