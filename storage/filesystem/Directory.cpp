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
#include <Directory.h>
#include <StringUtil.h>
#include <StorageDrive.h>
#include <DMM.h>
#include <FileOperations.h>
#include <FileDescriptor.h>
#include <StorageDriveManager.h>

#define MAX_SECTORS_PER_RW 8

/************************************* Static Functions ***********************************/
static void Directory_BufferedWrite(StorageDrive& diskDrive, unsigned uiSectorID, byte* bSectorBuffer, byte* bBuffer,
                                    unsigned& uiStartSectorID, unsigned& uiPrevSectorID, unsigned& iCount, bool bFlush)
{
  bool bNewBuffering = false ;

	if(bFlush)
	{
    if(iCount > 0)
      diskDrive.xWrite(bBuffer, uiStartSectorID, iCount);

    iCount = 0 ;
    return;
	}

  if(iCount == 0)
	{
    uiStartSectorID = uiPrevSectorID = uiSectorID ;
    memcpy(bBuffer, bSectorBuffer, 512);
    iCount = 1 ;
	}
  else if(uiPrevSectorID + 1 == uiSectorID)
	{
    uiPrevSectorID = uiSectorID ;
    memcpy(bBuffer + iCount * 512, bSectorBuffer, 512);
    ++iCount;
	}
	else
	{
		bNewBuffering = true ;
	}

  if(iCount == MAX_SECTORS_PER_RW || bNewBuffering)
	{
    diskDrive.xWrite(bBuffer, uiStartSectorID, iCount);

    iCount = 0 ;
		
		if(bNewBuffering)
		{	
      uiStartSectorID = uiPrevSectorID = uiSectorID ;
      memcpy(bBuffer, bSectorBuffer, 512);
      iCount = 1 ;
		}
	}
}

/**********************************************************************************************/

void Directory_Create(Process* processAddressSpace, StorageDrive &diskDrive, byte* bParentDirectoryBuffer,
                      FileSystem::WorkingDirectory &cwd, char* szDirName, unsigned short usDirAttribute) {
	byte bSectorBuffer[512] ;
	unsigned uiSectorNo ;
	byte bSectorPos ;
	unsigned uiFreeSectorID ;

  FileSystem::PresentWorkingDirectory& pwd = processAddressSpace->processPWD() ;

  if(cwd.getNode()->StartSectorID() == EOC) {
    uiFreeSectorID = diskDrive.fileSystem().AllocateSector();
		uiSectorNo = uiFreeSectorID ;
		bSectorPos = 0 ;
    cwd.getNode()->StartSectorID(uiFreeSectorID);
	}	else {
    if(Directory_FindDirectory(diskDrive, cwd, szDirName, uiSectorNo, bSectorPos, bSectorBuffer))
      throw upan::exception(XLOC, "directory %s already exists", szDirName);

		if(bSectorPos == EOC_B) {
      uiFreeSectorID = diskDrive.fileSystem().AllocateSector();
      diskDrive.fileSystem().SetSectorEntryValue(uiSectorNo, uiFreeSectorID);
			uiSectorNo = uiFreeSectorID ;
			bSectorPos = 0 ;
		}
	}

  ((FileNode*)bSectorBuffer)[bSectorPos].Init(szDirName, usDirAttribute, processAddressSpace->userID(),
                                                          cwd.getSectorId(), cwd.getSectorEntryPos());

  diskDrive.xWrite(bSectorBuffer, uiSectorNo, 1);

  cwd.getNode()->AddNode();

  diskDrive.xWrite(bParentDirectoryBuffer, cwd.getSectorId(), 1);

  if(diskDrive.Id() == processAddressSpace->driveID()
     && cwd.getSectorId() == pwd.getSectorId()
     && cwd.getSectorEntryPos() == pwd.getSectorEntryPos()) {
    pwd.setNode(*cwd.getNode());
  }

	//TODO: Required Only If "/" Dir Entry is Created
  if(strcmp((const char*)cwd.getNode()->Name(), FS_ROOT_DIR) == 0) {
    diskDrive.fileSystem().pwd().setNode(*cwd.getNode());
  }
}

void Directory_Delete(Process &pas, StorageDrive &diskDrive, byte* bParentDirectoryBuffer, FileSystem::WorkingDirectory &cwd, const char* szDirName) {
	byte bSectorBuffer[512] ;
	unsigned uiSectorNo ;
	byte bSectorPos ;

  if(cwd.getNode()->StartSectorID() == EOC) {
    throw upan::exception(XLOC, "directory %s doesn't exists to delete", szDirName);
	}	else {
    if(!Directory_FindDirectory(diskDrive, cwd, szDirName, uiSectorNo, bSectorPos, bSectorBuffer))
      throw upan::exception(XLOC, "directory %s doesn't exists to delete", szDirName);
	}

  FileNode* delDir = ((FileNode*)bSectorBuffer) + bSectorPos ;

  if(delDir->IsDirectory() && delDir->Size() != 0) {
    throw upan::exception(XLOC, "directory %s is not empty - can't delete", szDirName);
  }

  unsigned uiCurrentSectorID = delDir->StartSectorID();
	unsigned uiNextSectorID ;

	while(uiCurrentSectorID != EOC)
	{
    uiNextSectorID = diskDrive.fileSystem().DeallocateSector(uiCurrentSectorID);
		uiCurrentSectorID = uiNextSectorID ;
	}

  delDir->MarkAsDeleted();

  diskDrive.xWrite(bSectorBuffer, uiSectorNo, 1);

  cwd.getNode()->RemoveNode();

  diskDrive.xWrite(bParentDirectoryBuffer, cwd.getSectorId(), 1);

  FileSystem::PresentWorkingDirectory& pwd = pas.processPWD() ;
	if(diskDrive.Id() == pas.driveID()
     && cwd.getSectorId() == pwd.getSectorId()
     && cwd.getSectorEntryPos() == pwd.getSectorEntryPos()) {
    pwd.setNode(*cwd.getNode());
  }

	//TODO: Required Only If "/" Dir Entry is Created
  if(strcmp((const char*)cwd.getNode()->Name(), FS_ROOT_DIR) == 0) {
    diskDrive.fileSystem().pwd().setNode(*cwd.getNode());
  }
}

void Directory_GetDirEntryForCreateDelete(Process &pas, StorageDrive &diskDrive, const char* szDirPath, char* szDirName, unsigned& uiSectorNo, byte& bSectorPos, byte* bDirectoryBuffer) {
  FileSystem* pFSMountInfo = &diskDrive.fileSystem() ;

	if(strlen(szDirPath) == 0 ||	strcmp(FS_ROOT_DIR, szDirPath) == 0)
    throw upan::exception(XLOC, "can't create/delete current/root directory");

  FileSystem::WorkingDirectory cwd;

	if(szDirPath[0] == '/' || pas.driveID() != diskDrive.Id()) {
    cwd = pFSMountInfo->pwd();
	}	else {
    cwd = pas.processPWD();
	}

  uiSectorNo = cwd.getSectorId();
  bSectorPos = cwd.getSectorEntryPos();
  diskDrive.xRead(bDirectoryBuffer, uiSectorNo, 1);

  int iListSize;

	StringDefTokenizer tokenizer ;

	String_Tokenize(szDirPath, '/', &iListSize, tokenizer) ;

  FileNode tempDirEntry;
  for(int i = 0; i < iListSize - 1; i++) {
    if(!Directory_FindDirectory(diskDrive, cwd, tokenizer.szToken[i], uiSectorNo, bSectorPos, bDirectoryBuffer)) {
      throw upan::exception(XLOC, "directory %s is not found", tokenizer.szToken[i]);
    }
    tempDirEntry = *(((FileNode*)bDirectoryBuffer) + bSectorPos);
    cwd = FileSystem::WorkingDirectory(&tempDirEntry, uiSectorNo, bSectorPos);
	}

  strcpy(szDirName, tokenizer.szToken[iListSize - 1]) ;
	
  if(strcmp(DIR_SPECIAL_CURRENT, szDirName) == 0 || strcmp(DIR_SPECIAL_PARENT, szDirName) == 0) {
    throw upan::exception(XLOC, "%s is a special directory", szDirName);
  }
}

void Directory_GetDirectoryContent(const char* szFileName, Process &pas, int iDriveID, FileNode** pDirList, int* iListSize) {
	byte bDirectoryBuffer[512] ;

  StorageDrive& diskDrive = StorageDriveManager::Instance().GetByID(iDriveID, true).goodValueOrThrow(XLOC);
  FileSystem::WorkingDirectory cwd = (pas.driveID() == iDriveID) ? pas.processPWD() : diskDrive.fileSystem().pwd();

  FileNode* dirFile ;
  FileNode* pAddress ;

	if(strlen(szFileName) == 0) {
		dirFile = cwd.getNode();
	} else {
		unsigned uiSectorNo ;
		byte bSectorPos ;

    Directory_ReadDirEntryInfo(diskDrive, cwd, szFileName, uiSectorNo, bSectorPos, bDirectoryBuffer);

    dirFile = ((FileNode*)bDirectoryBuffer) + bSectorPos ;

    if(dirFile->IsDeleted())
      throw upan::exception(XLOC, "directory/file %s doesn't exists - it's deleted", szFileName);

    if(!dirFile->IsDirectory()) {
			*iListSize = 1 ;

      if(pas.isKernelProcess()) {
        *pDirList = (FileNode*)KernelDMM::Instance().allocate(sizeof(FileNode)) ;
				pAddress = *pDirList ;
			} else {
        *pDirList = (FileNode*)pas.dmm().allocate(sizeof(FileNode)) ;
        pAddress = (FileNode*)(*pDirList);
			}

      *pAddress = *dirFile;
      return;
		}
	}

	unsigned uiCurrentSectorID ;
	int iScanDirCount = 0;
	byte bSectorPosIndex ;

  FileNode* curDir ;

	iScanDirCount = 0 ;
  uiCurrentSectorID = dirFile->StartSectorID();
  *iListSize = dirFile->Size();

  if(pas.isKernelProcess())	{
    *pDirList = (FileNode*)KernelDMM::Instance().allocate(sizeof(FileNode) * (*iListSize)) ;
		pAddress = *pDirList ;
	}	else{
    *pDirList = (FileNode*)pas.dmm().allocate(sizeof(FileNode) * (*iListSize)) ;
    pAddress = (FileNode*)(*pDirList);
	}

	while(uiCurrentSectorID != EOC) {
    diskDrive.xRead(bDirectoryBuffer, uiCurrentSectorID, 1);

		for(bSectorPosIndex = 0; bSectorPosIndex < FileSystem::DIR_ENTRIES_PER_SECTOR; bSectorPosIndex++) {
      curDir = ((FileNode*)bDirectoryBuffer) + bSectorPosIndex ;

      if(!curDir->IsDeleted()) {
        ++iScanDirCount;
        if(iScanDirCount > *iListSize)
          return;
        pAddress[iScanDirCount - 1] = *curDir;
			}
		}

    uiCurrentSectorID = diskDrive.fileSystem().GetSectorEntryValue(uiCurrentSectorID);
	}
}

bool Directory_FindDirectory(StorageDrive& diskDrive, const FileSystem::WorkingDirectory& cwd, const char* szDirName, unsigned& uiSectorNo, byte& bSectorPos, byte* bDestSectorBuffer) {
  const auto dirNode = cwd.getNode();
  if(!dirNode->IsDirectory()) {
    throw upan::exception(XLOC, "%s is not a directory", szDirName);
  }

	byte bSectorBuffer[512] ;

	if(strcmp(szDirName, DIR_SPECIAL_CURRENT) == 0)	{
    uiSectorNo = cwd.getSectorId();
    bSectorPos = cwd.getSectorEntryPos();
    diskDrive.xRead(bDestSectorBuffer, uiSectorNo, 1);
    return true;
	}

	if(strcmp(szDirName, DIR_SPECIAL_PARENT) == 0) {
    if(strcmp((const char*)dirNode->Name(), FS_ROOT_DIR) == 0) {
      uiSectorNo = cwd.getSectorId() ;
      bSectorPos = cwd.getSectorEntryPos() ;
		}	else {
      uiSectorNo = dirNode->ParentSectorID() ;
      bSectorPos = dirNode->ParentSectorPos() ;
		}
	
    diskDrive.xRead(bDestSectorBuffer, uiSectorNo, 1);
			
    return true;
	}

	byte bDeletedEntryFound ;
	unsigned uiCurrentSectorID ;
	unsigned uiNextSectorID ;
	unsigned uiScanDirCount ;
	byte bSectorPosIndex ;

  FileNode* curDir ;

  uiSectorNo = EOC ;
  bSectorPos = EOC_B ;
	bDeletedEntryFound = false ;
	uiScanDirCount = 0 ;

  uiCurrentSectorID = dirNode->StartSectorID() ;

	while(uiCurrentSectorID != EOC) {
    diskDrive.xRead(bSectorBuffer, uiCurrentSectorID, 1);

		for(bSectorPosIndex = 0; bSectorPosIndex < FileSystem::DIR_ENTRIES_PER_SECTOR; bSectorPosIndex++) {
      curDir = ((FileNode*)bSectorBuffer) + bSectorPosIndex ;

      if(strcmp(szDirName, (const char*)curDir->Name()) == 0 && !curDir->IsDeleted())	{
        memcpy(bDestSectorBuffer, bSectorBuffer, 512);
        uiSectorNo = uiCurrentSectorID ;
        bSectorPos = bSectorPosIndex ;
        return true;
			}

      if(curDir->IsDeleted()) {
				if(bDeletedEntryFound == false) {
          memcpy(bDestSectorBuffer, bSectorBuffer, 512);
          uiSectorNo = uiCurrentSectorID ;
          bSectorPos = bSectorPosIndex ;
					bDeletedEntryFound = true ;
				}
			} else {
				uiScanDirCount++ ;
        if(uiScanDirCount >= dirNode->Size())
					break ;
			}
		}

    uiNextSectorID = diskDrive.fileSystem().GetSectorEntryValue(uiCurrentSectorID);

    if(uiScanDirCount >= dirNode->Size()) {
			if(bDeletedEntryFound == true)
        return false;

			if(bSectorPosIndex < FileSystem::DIR_ENTRIES_PER_SECTOR - 1) {
        memcpy(bDestSectorBuffer, bSectorBuffer, 512);
        uiSectorNo = uiCurrentSectorID ;
        bSectorPos = bSectorPosIndex + 1 ;
        return false;
			}

			if(bSectorPosIndex == FileSystem::DIR_ENTRIES_PER_SECTOR - 1) {
				if(uiNextSectorID != EOC) {
          uiSectorNo = uiNextSectorID ;
          bSectorPos = 0 ;
          return false;
				}
				
        uiSectorNo = uiCurrentSectorID ;
        bSectorPos = EOC_B ;
        return false;
			}
		}
		uiCurrentSectorID = uiNextSectorID ;
	}

  return false;
}

void Directory_FileWrite(StorageDrive* pDiskDrive, const FileSystem::WorkingDirectory &cwd, FileDescriptor& fdEntry, byte* bDataBuffer, unsigned uiDataSize)
{
	if(uiDataSize == 0)
    return throw upan::exception(XLOC, "zero byte file write");

	unsigned uiSectorNo ;
	byte bSectorPos ;
	byte bDirectoryBuffer[512] ;
	const char* szFileName = fdEntry.getFileName().c_str();

  Directory_ReadDirEntryInfo(*pDiskDrive, cwd, szFileName, uiSectorNo, bSectorPos, bDirectoryBuffer);

  FileNode* dirFile = ((FileNode*)bDirectoryBuffer) + bSectorPos ;

  if(dirFile->IsDirectory())
    throw upan::exception(XLOC, "%s is a directory - can't do file-write", szFileName);

  unsigned uiOffset = fdEntry.getMode() & O_APPEND ? dirFile->Size() : fdEntry.getOffset();

  Directory_ActualFileWrite(pDiskDrive, bDataBuffer, fdEntry, uiDataSize, dirFile);

  fdEntry.setOffset(uiOffset + uiDataSize);

  if(dirFile->Size() < fdEntry.getOffset())
	{
    dirFile->Size(fdEntry.getOffset()) ;
    pDiskDrive->xWrite(bDirectoryBuffer, uiSectorNo, 1);
	}
}

void Directory_ActualFileWrite(StorageDrive* pDiskDrive, byte* bDataBuffer, FileDescriptor& fdEntry, unsigned uiDataSize, FileNode* dirFile)
{
	unsigned uiCurrentSectorID, uiNextSectorID, uiPrevSectorID = EOC ;
	int iStartWriteSectorNo, iStartWriteSectorPos ;
	int iSectorIndex ;
	unsigned uiWriteRemainingCount, uiWrittenCount ;
	unsigned uiCurrentFileSize ;
	unsigned uiOffset = fdEntry.getMode() & O_APPEND ? dirFile->Size() : fdEntry.getOffset();

	byte bStartAllocation ;
	byte bSectorBuffer[512] ;

	iStartWriteSectorNo = uiOffset / 512 ;
	iStartWriteSectorPos = uiOffset % 512 ;

  uiCurrentFileSize = dirFile->Size();

  fdEntry.getLastReadSectorDetails(*dirFile, iSectorIndex, uiCurrentSectorID);

	if(iSectorIndex < 0 || iSectorIndex > iStartWriteSectorNo)
	{
		iSectorIndex = 0 ;
    uiCurrentSectorID = dirFile->StartSectorID() ;
	}
	
	while(iSectorIndex < iStartWriteSectorNo && uiCurrentSectorID != EOC)
	{
    uiNextSectorID = pDiskDrive->fileSystem().GetSectorEntryValue(uiCurrentSectorID);

		iSectorIndex++ ;
		uiPrevSectorID = uiCurrentSectorID ;
		uiCurrentSectorID = uiNextSectorID ;
	}

	if(uiCurrentSectorID == EOC)
	{
		memset((char*)bSectorBuffer, 0, 512) ;

		do
		{
      uiCurrentSectorID = pDiskDrive->fileSystem().AllocateSector();

      if(dirFile->StartSectorID() == EOC)
			{
        dirFile->StartSectorID(uiCurrentSectorID);
			}
			else
			{
        pDiskDrive->fileSystem().SetSectorEntryValue(uiPrevSectorID, uiCurrentSectorID);
			}
			
			uiPrevSectorID = uiCurrentSectorID ;

      pDiskDrive->xWrite(bSectorBuffer, uiCurrentSectorID, 1);
			
			fdEntry.setLastReadSectorDetails(iSectorIndex, uiCurrentSectorID) ;

			iSectorIndex++ ;

		} while(iSectorIndex <= iStartWriteSectorNo) ;
	}
	else
	{
    fdEntry.setLastReadSectorDetails(iSectorIndex, uiCurrentSectorID) ;
	}

	uiWrittenCount = 0 ;
	uiWriteRemainingCount = uiDataSize ;

	if(iStartWriteSectorPos != 0)
	{
    pDiskDrive->xRead(bSectorBuffer, uiCurrentSectorID, 1);

		uiWrittenCount = 512 - iStartWriteSectorPos ;
		if(uiDataSize <= uiWrittenCount)
			uiWrittenCount = uiDataSize ;

    memcpy(bSectorBuffer + iStartWriteSectorPos, bDataBuffer, uiWrittenCount);

    pDiskDrive->xWrite(bSectorBuffer, uiCurrentSectorID, 1);
			
		if(uiWrittenCount == uiDataSize)
      return;

    uiNextSectorID = pDiskDrive->fileSystem().GetSectorEntryValue(uiCurrentSectorID);

    uiPrevSectorID = uiCurrentSectorID ;
    uiCurrentSectorID = uiNextSectorID ;

    uiWriteRemainingCount -= uiWrittenCount ;
	}

	bStartAllocation = false ;
	
	unsigned uiBufStartSectorID, uiBufPrecSectorID ;
	byte bWriteBuffer[MAX_SECTORS_PER_RW * 512] ;
  uint32_t iBufCount = 0 ;

	while(true)
	{
		if(uiCurrentSectorID == EOC || bStartAllocation == true)
		{
			bStartAllocation = true ;
      uiCurrentSectorID = pDiskDrive->fileSystem().AllocateSector();
      pDiskDrive->fileSystem().SetSectorEntryValue(uiPrevSectorID, uiCurrentSectorID);
		}
		
		if(uiWriteRemainingCount < 512)
		{
			if(bStartAllocation == false && (uiOffset + uiDataSize) < uiCurrentFileSize)
			{
        pDiskDrive->xRead(bSectorBuffer, uiCurrentSectorID, 1);
			}

      memcpy(bSectorBuffer, (bDataBuffer + uiWrittenCount), uiWriteRemainingCount);

      Directory_BufferedWrite(*pDiskDrive, uiCurrentSectorID, bSectorBuffer, bWriteBuffer, uiBufStartSectorID,
                              uiBufPrecSectorID, iBufCount, false);

      Directory_BufferedWrite(*pDiskDrive, EOC, NULL, bWriteBuffer, uiBufStartSectorID, uiBufPrecSectorID, iBufCount, true);

      return;
		}

    Directory_BufferedWrite(*pDiskDrive, uiCurrentSectorID, bDataBuffer + uiWrittenCount, bWriteBuffer,
                            uiBufStartSectorID, uiBufPrecSectorID, iBufCount, false);
		
		uiWrittenCount += 512 ;
		uiWriteRemainingCount -= 512 ;

		if(uiWriteRemainingCount == 0)
		{
      Directory_BufferedWrite(*pDiskDrive, EOC, NULL, bWriteBuffer, uiBufStartSectorID, uiBufPrecSectorID, iBufCount, true);
      return;
		}

    uiNextSectorID = pDiskDrive->fileSystem().GetSectorEntryValue(uiCurrentSectorID);
		uiPrevSectorID = uiCurrentSectorID ;
		uiCurrentSectorID = uiNextSectorID ;
	}

  throw upan::exception(XLOC, "fs table is corrupted for drive:%s", pDiskDrive->DriveName().c_str());
}

int Directory_FileRead(StorageDrive* pDiskDrive, const FileSystem::WorkingDirectory &cwd, FileDescriptor& fdEntry, byte* bDataBuffer, unsigned uiDataSize) {
	const char* szFileName = fdEntry.getFileName().c_str();
	unsigned uiOffset = fdEntry.getOffset();

  FileNode* pDirFile ;
	byte bDirectoryBuffer[512] ;
	unsigned uiSectorNo ;
	byte bSectorPos ;

  Directory_ReadDirEntryInfo(*pDiskDrive, cwd, szFileName, uiSectorNo, bSectorPos, bDirectoryBuffer);
		
  pDirFile = ((FileNode*)bDirectoryBuffer) + bSectorPos ;
  if(pDirFile->IsDirectory()) {
    throw upan::exception(XLOC, "%s is a directory - can't file-read", szFileName);
  }

  if(uiOffset >= pDirFile->Size()) {
    return 0;
  }

  if(pDirFile->Size() == 0)	{
		bDataBuffer[0] = '\0' ;
    return 0;
	}

	unsigned uiCurrentSectorID, uiNextSectorID, uiStartSectorID ;
	int iStartReadSectorNo, iStartReadSectorPos ;
	int iSectorIndex ;
	unsigned uiCurrentFileSize ;
	int iReadRemainingCount, iReadCount, iCurrentReadSize, iSectorCount ;

	byte bSectorBuffer[MAX_SECTORS_PER_RW * 512] ;

	iStartReadSectorNo = uiOffset / 512 ;
	iStartReadSectorPos = uiOffset % 512 ;

  uiCurrentFileSize = pDirFile->Size() ;

  fdEntry.getLastReadSectorDetails(*pDirFile, iSectorIndex, uiCurrentSectorID);

	if(iSectorIndex < 0 || iSectorIndex > iStartReadSectorNo)	{
		iSectorIndex = 0 ;
    uiCurrentSectorID = pDirFile->StartSectorID() ;
	}

	while(iSectorIndex != iStartReadSectorNo)	{
    uiNextSectorID = pDiskDrive->fileSystem().GetSectorEntryValue(uiCurrentSectorID);
		iSectorIndex++ ;
		uiCurrentSectorID = uiNextSectorID ;
	}

  fdEntry.setLastReadSectorDetails(iSectorIndex, uiCurrentSectorID) ;
	unsigned uiLastReadSectorNumber = uiCurrentSectorID ;
	int iLastReadSectorIndex = iSectorIndex ;

	iReadCount = 0 ;
	iReadRemainingCount = (uiDataSize < (uiCurrentFileSize - uiOffset) && uiDataSize > 0) ? uiDataSize : (uiCurrentFileSize  - uiOffset) ;

	iSectorCount = 0 ;

	while(true)	{
		if(uiCurrentSectorID == EOC) {
      fdEntry.setLastReadSectorDetails(iLastReadSectorIndex, uiLastReadSectorNumber) ;
      return iReadCount;
		}
		
		uiStartSectorID = uiCurrentSectorID ;

		iLastReadSectorIndex += iSectorCount ;
		uiLastReadSectorNumber = uiCurrentSectorID ;

		iSectorCount = 1 ;

		for(;;) {
      uiNextSectorID = pDiskDrive->fileSystem().GetSectorEntryValue(uiCurrentSectorID);

			if(uiCurrentSectorID + 1 == uiNextSectorID) {
				uiCurrentSectorID = uiNextSectorID ;

				iSectorCount++ ;
				iCurrentReadSize = iSectorCount * 512 - iStartReadSectorPos ;

				if(iReadRemainingCount <= iCurrentReadSize) {
					if((iStartReadSectorPos + iReadRemainingCount) <= 512)
						iSectorCount-- ;

					iCurrentReadSize = iReadRemainingCount ;
					break ;
				}

				if(iSectorCount == MAX_SECTORS_PER_RW) {
          uiNextSectorID = pDiskDrive->fileSystem().GetSectorEntryValue(uiCurrentSectorID);
					uiCurrentSectorID = uiNextSectorID ;
					break ;	
				}
			} else {
				iCurrentReadSize = iSectorCount * 512 - iStartReadSectorPos ;

				if(iReadRemainingCount <= iCurrentReadSize)
					iCurrentReadSize = iReadRemainingCount ;

				uiCurrentSectorID = uiNextSectorID ;
				break ;
			}
		}

    pDiskDrive->xRead(bSectorBuffer, uiStartSectorID, iSectorCount);

    memcpy(bDataBuffer + iReadCount, bSectorBuffer + iStartReadSectorPos, iCurrentReadSize);

		iReadCount += iCurrentReadSize ;
		iReadRemainingCount -= iCurrentReadSize ;

		iStartReadSectorPos = 0 ;

		if(iReadRemainingCount <= 0) {
      fdEntry.setLastReadSectorDetails(iLastReadSectorIndex, uiLastReadSectorNumber) ;
      return iReadCount ;
		}
	}

  throw upan::exception(XLOC, "fs table is corrupted for drive:%s", pDiskDrive->DriveName().c_str());
}

void Directory_ReadDirEntryInfo(StorageDrive& diskDrive, const FileSystem::WorkingDirectory& cwd, const char* szFileName, unsigned& uiSectorNo, byte& bSectorPos, byte* bDirectoryBuffer) {

	if(strlen(szFileName) == 0) {
    throw upan::exception(XLOC, "file name can't be empty");
  }

  FileSystem::PresentWorkingDirectory& fsPwd = diskDrive.fileSystem().pwd();
  FileSystem::WorkingDirectory workingDirectory;

	if(szFileName[0] == '/') {
		if(strcmp(FS_ROOT_DIR, szFileName) == 0) {
      diskDrive.xRead(bDirectoryBuffer, fsPwd.getSectorId(), 1);
      uiSectorNo = fsPwd.getSectorId() ;
      bSectorPos = fsPwd.getSectorEntryPos() ;
      return;
		}
    workingDirectory = fsPwd;
	} else {
    workingDirectory = cwd;
	}

  int iListSize = 0;

	StringDefTokenizer tokenizer ;

	String_Tokenize(szFileName, '/', &iListSize, tokenizer) ;

  FileNode tempDirEntry;
  for(int i = 0; i < iListSize; i++) {
    if (!Directory_FindDirectory(diskDrive, workingDirectory, tokenizer.szToken[i], uiSectorNo, bSectorPos, bDirectoryBuffer)) {
      throw upan::exception(XLOC, "file/directory %s doesn't exist", szFileName);
    }

    tempDirEntry = *(((FileNode*)bDirectoryBuffer) + bSectorPos);
    workingDirectory = FileSystem::WorkingDirectory(&tempDirEntry, uiSectorNo, bSectorPos);
	}
}

void Directory_Change(const char* szFileName, int iDriveID, Process &pas) {
	unsigned uiSectorNo ;
	byte bSectorPos ;
	byte bDirectoryBuffer[512] ;

  StorageDrive& diskDrive = StorageDriveManager::Instance().GetByID(iDriveID, true).goodValueOrThrow(XLOC);

  FileSystem::WorkingDirectory cwd = iDriveID == pas.driveID() ? pas.processPWD() : diskDrive.fileSystem().pwd();

  Directory_ReadDirEntryInfo(diskDrive, cwd, szFileName, uiSectorNo, bSectorPos, bDirectoryBuffer);

  FileNode* dirFile = ((FileNode*)bDirectoryBuffer) + bSectorPos ;

  if(!dirFile->IsDirectory())
    throw upan::exception(XLOC, "%s is not a directory", szFileName);

	pas.setDriveID(iDriveID);

  pas.processPWD().Init(*(((FileNode*)bDirectoryBuffer) + bSectorPos), uiSectorNo, bSectorPos);
	
	unsigned uiSecNo ;
	byte bSecPos ;
	char szPWD[256] ;
	char szTempPwd[256] = "" ;

  if(strcmp((const char*)dirFile->Name(), FS_ROOT_DIR) == 0) {
    strcpy(szPWD, FS_ROOT_DIR);
  }	else {
    while (true) {
      strcpy(szPWD, FS_ROOT_DIR);

      strcat(szPWD, (const char *) dirFile->Name());
      strcat(szPWD, szTempPwd);

      uiSecNo = dirFile->ParentSectorID();
      bSecPos = dirFile->ParentSectorPos();

      diskDrive.xRead(bDirectoryBuffer, uiSecNo, 1);

      dirFile = ((FileNode *) bDirectoryBuffer) + bSecPos;

      if (strcmp((const char *) dirFile->Name(), FS_ROOT_DIR) == 0)
        break;

      strcpy(szTempPwd, szPWD);
    }
  }

	strcpy(szTempPwd, szPWD) ;
	strcpy(szPWD, diskDrive.DriveName().c_str());
	strcat(szPWD, "@") ;
	strcat(szPWD, szTempPwd) ;

	pas.setEnv("PWD", szPWD);
}

void Directory_PresentWorkingDirectory(Process* processAddressSpace, char** uiReturnDirPathAddress)
{
	char* pAddress ;
  const char* szPWD = processAddressSpace->getEnv("PWD").valueOrElse("").c_str();

	if(processAddressSpace->isKernelProcess())
	{
		*uiReturnDirPathAddress = (char*)KernelDMM::Instance().allocate(strlen(szPWD) + 1) ;
		pAddress = *uiReturnDirPathAddress ;
	}
	else
	{
		*uiReturnDirPathAddress = (char*)processAddressSpace->dmm().allocate(strlen(szPWD) + 1) ;
		pAddress = (char*)(*uiReturnDirPathAddress);
	}

	strcpy(pAddress, szPWD) ;
}

FileNode Directory_GetDirEntry(const char* szFileName, Process &pas, int iDriveID)
{
	byte bDirectoryBuffer[512] ;

  StorageDrive& diskDrive = StorageDriveManager::Instance().GetByID(iDriveID, true).goodValueOrThrow(XLOC);

  FileSystem::WorkingDirectory cwd = pas.driveID() == iDriveID ? pas.processPWD() : diskDrive.fileSystem().pwd();

  FileNode* dirFile ;

	unsigned uiSectorNo ;
	byte bSectorPos ;

  Directory_ReadDirEntryInfo(diskDrive, cwd, szFileName, uiSectorNo, bSectorPos, bDirectoryBuffer);

  dirFile = ((FileNode*)bDirectoryBuffer) + bSectorPos ;

  if(dirFile->IsDeleted()) {
    throw upan::exception(XLOC, "directory %s doesn't exists - it's deleted", szFileName);
  }

  return *dirFile;
}

void Directory_SyncPWD(Process &pas) {
  StorageDrive& diskDrive = StorageDriveManager::Instance().GetByID(pas.driveID(), true).goodValueOrThrow(XLOC);

	uint32_t uiSectorNo = pas.processPWD().getSectorId();
	uint8_t bSectorEntryPos = pas.processPWD().getSectorEntryPos();

	byte bSectorBuffer[512] ;
  diskDrive.xRead(bSectorBuffer, uiSectorNo, 1);

  pas.processPWD().setNode((((FileNode*)bSectorBuffer)[bSectorEntryPos]));
}

