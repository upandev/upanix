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
#include <PartitionManager.h>
#include <ATADrive.h>
#include <DMM.h>
#include <RawDiskDrive.h>

#define SI_EXT	5
#define SI_EMTY	0x01
#define SI_GPT  0xEE
#define DEF_START_SEC 63

void GPTHeader::CheckSignature() const
{
  static const char* EFI_SIG = "EFI PART";
  char signature[9];
  memcpy(signature, _signature, 8);
  signature[8] = '\0';
  if(strcmp(signature, EFI_SIG) != 0)
    throw upan::exception(XLOC, "GPT Header with unsupported Signature: %s", signature);
}

bool GPTPartitionEntry::IsEmpty() const
{
  for(int i = 0; i < 16; ++i)
    if(_partitionTypeGUID[i] != 0)
      return false;
  return true;
}

MBRPartitionInfo::MBRPartitionInfo(unsigned lbaStart, unsigned size, MBRPartitionInfo::Types type) :
  BootIndicator(type == ACTIVE ? 0x80 : 0x00),
  _StartHead(255),
  _StartSector(255),
  _StartCylinder(254),
  SystemIndicator(type == EXTENEDED ? SI_EXT : SI_EMTY),
  _EndHead(255),
  _EndSector(255),
  _EndCylinder(254),
	LBAStartSector(lbaStart),
	LBANoOfSectors(size)
{
	
/*
		case ATA_HARD_DISK:
				iSectorsPerTrack = ((ATAPort*)_disk.Device())->id.usSectors ;
				iHeadsPerCynlider = ((ATAPort*)_disk.Device())->id.usHead ;

				StartCylinder = (uiLBAStartSector / iSectorsPerTrack) / iHeadsPerCynlider ;
				StartHead = (uiLBAStartSector / iSectorsPerTrack) % iHeadsPerCynlider ;
				StartSector = (uiLBAStartSector % iSectorsPerTrack) + 1 ;
				
				EndCylinder = (uiLBAEndSector / iSectorsPerTrack) / iHeadsPerCynlider ;
				EndHead = (uiLBAEndSector / iSectorsPerTrack) % iHeadsPerCynlider ;
				EndSector = (uiLBAEndSector % iSectorsPerTrack) + 1 ;
				break ;
*/
}

PartitionTable::PartitionTable(RawDiskDrive& disk) 
  : _bIsGPTPartition(false), _bIsMBRPartition(false), 
    _bIsExtPartitionPresent(false), _disk(disk)
{
	byte bBootSectorBuffer[512] ;
	_disk.Read(0, 1, bBootSectorBuffer);
	MBRPartitionInfo* pPartitionInfo = ((MBRPartitionInfo*)(bBootSectorBuffer + 0x1BE));
  VerifyMBR(pPartitionInfo);

  if(_bIsMBRPartition)
    ReadMBRPartition(pPartitionInfo);
  else if(_bIsGPTPartition)
    ReadGPT();
}

PartitionTable::~PartitionTable()
{
  for(auto pp : _primaryPartitions)
    delete pp;
  for(auto ep : _extPartitions)
    delete ep;
}

void PartitionTable::ClearPartitionTable()
{
  while(_partitions.size())
    DeletePrimaryPartition();
}

void PartitionTable::VerifyMBR(const MBRPartitionInfo* pPartitionInfo)
{
	unsigned uiActivePartitionCount = 0 ;
	byte bPartitioned = false ;
	unsigned uiPrevEndLBASector = 0 ;
	unsigned uiSectorCount = 0 ;

	for(unsigned i = 0; i < MAX_NO_OF_PRIMARY_PARTITIONS; i++)
	{
		if(pPartitionInfo[i].BootIndicator == 0x80)
			uiActivePartitionCount++ ;

		if(!pPartitionInfo[i].IsEmpty())
		{
			if(pPartitionInfo[i].LBAStartSector > uiPrevEndLBASector)
			{
				uiPrevEndLBASector = pPartitionInfo[i].LBAStartSector + pPartitionInfo[i].LBANoOfSectors - 1 ;
				uiSectorCount += pPartitionInfo[i].LBANoOfSectors ;
				bPartitioned = true ;
        if(i == 0 && pPartitionInfo[i].SystemIndicator == SI_GPT)
        {
          _bIsGPTPartition = true;
          return;
        }
			}
			else
			{
				bPartitioned = false ;
				break ;
			}
		}
		else
			break ; 
	}

	unsigned uiSectorLimit = _disk.SizeInSectors();
  _bIsMBRPartition = !(bPartitioned == false || uiActivePartitionCount != 1 || uiSectorCount > uiSectorLimit);
}

void PartitionTable::ReadGPT()
{
  byte bGPTHeaderBuffer[ 512 ];
  _disk.Read(1, 1, bGPTHeaderBuffer);
  GPTHeader* gptHeader = reinterpret_cast<GPTHeader*>(bGPTHeaderBuffer);
  gptHeader->CheckSignature();
  byte bPartitionEntry[ 512 ];
  // 128 bytes per partition entry --> 4 entries per sector
  for(unsigned s = gptHeader->PartArrStartLBA(); ; ++s)
  {
    _disk.Read(s, 1, bPartitionEntry);
    GPTPartitionEntry* pe = reinterpret_cast<GPTPartitionEntry*>(bPartitionEntry);
    for(unsigned i = 0; i < 4; ++i)
    {
      if(pe[i].IsEmpty())
        return;
      //TODO: SI_EMPTY to valid partition type
      _partitions.push_back(PartitionEntry(pe[i].FirstLBA(), pe[i].Size(), SI_EMTY));
    }
  }
}

void PartitionTable::ReadMBRPartition(const MBRPartitionInfo* pPartitionInfo)
{
	byte bBootSectorBuffer[512] ;
  
	for(unsigned i = 0; i < MAX_NO_OF_PRIMARY_PARTITIONS; i++)
	{
		if(pPartitionInfo[i].IsEmpty())
			break ;

		if(pPartitionInfo[i].SystemIndicator == SI_EXT)
		{
			_bIsExtPartitionPresent = true ;
      _extPartitionEntry = pPartitionInfo[i];
		}
    else
    {
      _primaryPartitions.push_back(new MBRPartitionInfo(pPartitionInfo[i]));
      _partitions.push_back(PartitionEntry(pPartitionInfo[i].LBAStartSector, pPartitionInfo[i].LBANoOfSectors, pPartitionInfo[i].SystemIndicator));
    }
	}

  if(!_bIsExtPartitionPresent)
    return;
	unsigned uiExtSectorID, uiStartExtSectorID;
	uiExtSectorID = uiStartExtSectorID = _extPartitionEntry.LBAStartSector ;
	for(;;)
	{
		_disk.Read(uiExtSectorID, 1, bBootSectorBuffer);

		pPartitionInfo = ((MBRPartitionInfo*)(bBootSectorBuffer + 0x1BE)) ;
	
		if(pPartitionInfo[0].IsEmpty())
			break ;

    _extPartitions.push_back(new ExtPartitionTable(pPartitionInfo[0], uiExtSectorID));
    _partitions.push_back(PartitionEntry(pPartitionInfo[0].LBAStartSector + uiExtSectorID, pPartitionInfo[0].LBANoOfSectors, pPartitionInfo[0].SystemIndicator));

		if(pPartitionInfo[1].IsEmpty())
			break;

		uiExtSectorID = uiStartExtSectorID + pPartitionInfo[1].LBAStartSector ;
	}
}

void PartitionTable::CreatePrimaryPartitionEntry(unsigned uiSizeInSectors, MBRPartitionInfo::Types type)
{
	if(_primaryPartitions.size() == MAX_NO_OF_PRIMARY_PARTITIONS)
    throw upan::exception(XLOC, "primary partition is full");

	if(_bIsExtPartitionPresent)
    throw upan::exception(XLOC, "extended parition exists - can't create primary partition");

	if(_primaryPartitions.empty())
	{
		if(type == MBRPartitionInfo::EXTENEDED)
      throw upan::exception(XLOC, "no primary paritions - can't create extended partition");
	
    type = MBRPartitionInfo::ACTIVE;
	}

	unsigned uiUsedSizeInSectors = DEF_START_SEC ;

	if(!_primaryPartitions.empty())
  {
    auto lastPP = _primaryPartitions.rbegin();
		uiUsedSizeInSectors = lastPP->LBAStartSector + lastPP->LBANoOfSectors;
  }
		
	unsigned uiTotalSizeInSectors = _disk.SizeInSectors();

	if(uiTotalSizeInSectors >= uiUsedSizeInSectors)
		if((uiTotalSizeInSectors - uiUsedSizeInSectors) < uiSizeInSectors)
      throw upan::exception(XLOC, "insufficient space to create partition");

	unsigned uiLBAStartSector = uiUsedSizeInSectors ;

	byte bBootSectorBuffer[512] ;

	_disk.Read(0, 1, bBootSectorBuffer);

	bBootSectorBuffer[510] = 0x55 ;
	bBootSectorBuffer[511] = 0xAA ;

	MBRPartitionInfo* pRealPartitionTableEntry = ((MBRPartitionInfo*)(bBootSectorBuffer + 0x1BE)) ;

	if(type == MBRPartitionInfo::ACTIVE)
	{
    unsigned i = 0;
		for(auto pp : _primaryPartitions)
		{
      pp->BootIndicator = 0;
			pRealPartitionTableEntry[i++].BootIndicator = 0;
		}
	}

  MBRPartitionInfo* pi = new MBRPartitionInfo(uiLBAStartSector, uiSizeInSectors, type);
  pRealPartitionTableEntry[_primaryPartitions.size()] = *pi;
  _primaryPartitions.push_back(pi);

	_disk.Write(0, 1, bBootSectorBuffer);
}

void PartitionTable::CreateExtPartitionEntry(unsigned uiSizeInSectors)
{
	if(!_bIsExtPartitionPresent)
    throw upan::exception(XLOC, "extended partition doesn't exists");

  auto lastExtPartition = _extPartitions.rbegin();
	unsigned uiUsedSizeInSectors = 0 ;
	if(lastExtPartition != _extPartitions.rend())
	{
		uiUsedSizeInSectors = lastExtPartition->ActualStartSector()
      + lastExtPartition->CurrentPartition().LBAStartSector 
      + lastExtPartition->CurrentPartition().LBANoOfSectors 
      - _extPartitionEntry.LBAStartSector ;
	}

	const unsigned uiTotalSizeInSectors = _extPartitionEntry.LBANoOfSectors ;

	if(uiTotalSizeInSectors >= uiUsedSizeInSectors)
		if((uiTotalSizeInSectors - uiUsedSizeInSectors) < uiSizeInSectors)
      throw upan::exception(XLOC, "insufficient space to create partition");
	
	const unsigned uiLBAStartSector = DEF_START_SEC ;
	const unsigned uiLBANoOfSectors = uiSizeInSectors - DEF_START_SEC ;
	const unsigned uiNewPartitionSector = uiUsedSizeInSectors + _extPartitionEntry.LBAStartSector ;

	byte bBootSectorBuffer[512] ;
  const MBRPartitionInfo extPartitionInfo(uiLBAStartSector, uiLBANoOfSectors, MBRPartitionInfo::NORMAL);

  _extPartitions.push_back(new ExtPartitionTable(extPartitionInfo, uiNewPartitionSector));

	_disk.Read(uiNewPartitionSector, 1, bBootSectorBuffer);

	bBootSectorBuffer[510] = 0x55 ;
	bBootSectorBuffer[511] = 0xAA ;

	MBRPartitionInfo* pRealPartitionTableEntry = ((MBRPartitionInfo*)(bBootSectorBuffer + 0x1BE)) ;

	pRealPartitionTableEntry[0] = extPartitionInfo;
  memset(&(pRealPartitionTableEntry[1]), 0, sizeof(MBRPartitionInfo));

	_disk.Write(uiNewPartitionSector, 1, bBootSectorBuffer);

	if(lastExtPartition != _extPartitions.rend())
	{
		const auto uiPreviousExtPartitionStartSector = lastExtPartition->ActualStartSector();

		_disk.Read(uiPreviousExtPartitionStartSector, 1, bBootSectorBuffer);

		pRealPartitionTableEntry = ((MBRPartitionInfo*)(bBootSectorBuffer + 0x1BE)) ;

    const MBRPartitionInfo nextPartitionInfo(uiUsedSizeInSectors, uiSizeInSectors, MBRPartitionInfo::EXTENEDED);
		pRealPartitionTableEntry[1] = nextPartitionInfo;

		_disk.Write(uiPreviousExtPartitionStartSector, 1, bBootSectorBuffer);
	}
}

void PartitionTable::DeletePrimaryPartition()
{
	if(_primaryPartitions.empty())
    throw upan::exception(XLOC, "partition table is empty");
	
	byte bBootSectorBuffer[512] ;
	_disk.Read(0, 1, bBootSectorBuffer);

	MBRPartitionInfo* pRealPartitionTableEntry ;

	if(_bIsExtPartitionPresent)
	{
		pRealPartitionTableEntry = &(((MBRPartitionInfo*)(bBootSectorBuffer + 0x1BE))[_primaryPartitions.size()]);
		unsigned uiExtSector ;
		byte bExtBootSectorBuffer[512] ;
		MBRPartitionInfo* pExtRealPartitionTable ;

    for(auto ep : _extPartitions)
		{
			uiExtSector = ep->ActualStartSector();
			_disk.Read(uiExtSector, 1, bExtBootSectorBuffer);
			pExtRealPartitionTable = ((MBRPartitionInfo*)(bExtBootSectorBuffer + 0x1BE)) ;

			pExtRealPartitionTable[0] = MBRPartitionInfo();
			pExtRealPartitionTable[1] = MBRPartitionInfo();

			_disk.Write(uiExtSector, 1, bExtBootSectorBuffer);
      delete ep;
      _partitions.pop_back();
		}
    _extPartitions.clear();
    _bIsExtPartitionPresent = false;
    _extPartitionEntry = MBRPartitionInfo();
	}
	else
	{
    _primaryPartitions.pop_back();
    _partitions.pop_back();
		pRealPartitionTableEntry = &(((MBRPartitionInfo*)(bBootSectorBuffer + 0x1BE))[_primaryPartitions.size()]);
	}

  *pRealPartitionTableEntry = MBRPartitionInfo();

	_disk.Write(0, 1, bBootSectorBuffer);
}

void PartitionTable::DeleteExtPartition()
{
	if(_extPartitions.empty() || _bIsExtPartitionPresent == false)
    throw upan::exception(XLOC, "extended partition table is empty");
	
	byte bExtBootSectorBuffer[512] ;

  auto lastExtPartition = _extPartitions.rbegin();
	const unsigned uiCurrentExtPartitionStartSector = lastExtPartition->ActualStartSector();
  ++lastExtPartition;
	if(lastExtPartition != _extPartitions.rend())
	{
		const unsigned uiPreviousExtPartitionStartSector = lastExtPartition->ActualStartSector();

		_disk.Read(uiPreviousExtPartitionStartSector, 1, bExtBootSectorBuffer);

		MBRPartitionInfo* pRealPartitionEntry = &((MBRPartitionInfo*)(bExtBootSectorBuffer + 0x1BE))[1] ;
    *pRealPartitionEntry = MBRPartitionInfo();

		_disk.Write(uiPreviousExtPartitionStartSector, 1, bExtBootSectorBuffer);
	}

	_disk.Read(uiCurrentExtPartitionStartSector, 1, bExtBootSectorBuffer);

  memset((byte*)(bExtBootSectorBuffer + 0x1BE), 0, sizeof(MBRPartitionInfo) * 2);

	_disk.Write(uiCurrentExtPartitionStartSector, 1, bExtBootSectorBuffer);

  _extPartitions.pop_back();
  _partitions.pop_back();
}

//TODO: should work for GPT partitions
void PartitionTable::VerbosePrint() const
{
  printf("\n%-4s %-10s %-10s %-10s", "Boot", "Start", "End", "SysId");
  auto print = [](const MBRPartitionInfo& pi) {
    const char* sysId = "EMPTY";
    if(pi.SystemIndicator == SI_EXT)
      sysId = "Extended";
    else if(pi.SystemIndicator == 0x93)
      sysId = "MOSFS";
    printf("\n%-4d %-10d %-10d %-10s", pi.BootIndicator, pi.LBAStartSector, pi.LBANoOfSectors, sysId);
  };

  for(const auto pp : _primaryPartitions)
    print(*pp);
  if(_bIsExtPartitionPresent)
  {
    print(_extPartitionEntry);
    for(const auto ep : _extPartitions)
      print(ep->CurrentPartition());
  }
}
