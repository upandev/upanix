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

#include <BootBlock.h>
#include <StorageDrive.h>

void BootBlock::initialize(StorageDrive& storageDrive) {
  BPB_jmpBoot[0] = 0xEB ; /****************/
  BPB_jmpBoot[1] = 0xFE ; /* JMP $ -- ARR */
  BPB_jmpBoot[2] = 0x90 ; /****************/

  BPB_BytesPerSec = 0x200; // 512 ;
  BPB_RsvdSecCnt = 2 ;

  if(storageDrive.DeviceType() == DEV_FLOPPY)
    BPB_Media  = MEDIA_REMOVABLE ;
  else
    BPB_Media  = MEDIA_FIXED ;

  BPB_SecPerTrk = storageDrive.SectorsPerTrack();
  BPB_NumHeads = storageDrive.NoOfHeads();
  BPB_HiddSec  = 0 ;
  BPB_TotSec32 = storageDrive.SizeInSectors();

/*	pFSBootBlock->BPB_FSTableSize ; ---> Calculated */
  BPB_ExtFlags  = 0x0080 ;
  BPB_FSVer = 0x0100 ;  //version 1.0
  BPB_FSInfo  = 1 ;  //Typical Value for FSInfo Sector

  BPB_BootSig = 0x29 ;
  BPB_VolID = 0x01 ;  //TODO: Required to be set to current Date/Time of system ---- Not Mandatory
  strcpy((char*)BPB_VolLab, "No Name   ") ;  //10 + 1(\0) characters only -- ARR

  _usedSectors = 1 ;

  BPB_FSTableSize = (BPB_TotSec32 - BPB_RsvdSecCnt - 1) / (ENTRIES_PER_TABLE_SECTOR + 1) ;
}

void BootBlock::load(StorageDrive &storageDrive) {
  byte bArrFSBootBlock[512];
  storageDrive.Read(1, 1, bArrFSBootBlock);

  if(bArrFSBootBlock[510] != 0x55 || bArrFSBootBlock[511] != 0xAA) {
    throw upan::exception(XLOC, "invalid BPB signature - %x, %x", bArrFSBootBlock[510], bArrFSBootBlock[511]);
  }

  *this = *reinterpret_cast<BootBlock*>(bArrFSBootBlock);
  
  if(BPB_BootSig != 0x29)
    throw upan::exception(XLOC, "invalid BOOT signature: %x", BPB_BootSig);

  // TODO: A write to HD image file from mos fs util is changing the CHS value !!
  // Needs to be fixed. So, this check is skipped for the time being

  /*
  if(fsBootBlock.BPB_SecPerTrk != pDiskDrive->uiSectorsPerTrack)
    return FileSystem_ERR_INVALID_SECTORS_PER_TRACK;

  if(fsBootBlock.BPB_NumHeads != pDiskDrive->uiNoOfHeads)
    return FileSystem_ERR_INVALID_NO_OF_HEADS;
  */

  if(BPB_TotSec32 != storageDrive.SizeInSectors())
    throw upan::exception(XLOC, "invalid BPB_TotSec32: %d", BPB_TotSec32);

  if(BPB_FSTableSize == 0)
    throw upan::exception(XLOC, "invalid BPB_FSTableSize: %d", BPB_FSTableSize);

  if(BPB_BytesPerSec != 0x200)
    throw upan::exception(XLOC, "invalid BPB_BytesPerSec: %d", BPB_BytesPerSec);

  if(BPB_jmpBoot[0] != 0xEB || BPB_jmpBoot[1] != 0xFE || BPB_jmpBoot[2] != 0x90)
    throw upan::exception(XLOC, "invalid BPB_jmpBoot");

  if(storageDrive.DeviceType() == DEV_FLOPPY)
    if(BPB_Media != 0xF0)
      throw upan::exception(XLOC, "invalid BPB_Media: %x", BPB_Media);

  if(BPB_ExtFlags != 0x0080)
    throw upan::exception(XLOC, "invalid BPB_ExtFlags: %x", BPB_ExtFlags);

  if(BPB_FSVer != 0x0100)
    throw upan::exception(XLOC, "invalid BPB_FSVer: %x", BPB_FSVer);

  if(BPB_FSInfo != 1)
    throw upan::exception(XLOC, "invalid BPB_FSInfo: %d", BPB_FSInfo);

  if(BPB_VolID != 0x01)
    throw upan::exception(XLOC, "invalid BPB_VolID: %x", BPB_VolID);
}

void BootBlock::store(StorageDrive &storageDrive) {
  byte bSectorBuffer[512];

  bSectorBuffer[510] = 0x55; /* BootSector Signature */
  bSectorBuffer[511] = 0xAA;

  memcpy(bSectorBuffer, this, sizeof(BootBlock));
  storageDrive.Write(1, 1, bSectorBuffer);
}
