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
# include <SysCall.h>
# include <SysCallFile.h>
# include <DeviceDrive.h>
# include <try.h>

byte SysCallFile_IsPresent(unsigned uiSysCallID)
{
	return (uiSysCallID > SYS_CALL_FILE_START && uiSysCallID < SYS_CALL_FILE_END) ;
}

__volatile__ bool bDoAddrTranslation = true ;

void SysCallFile_Handle(
        __volatile__ int* piRetVal,
        __volatile__ unsigned uiSysCallID,
        __volatile__ bool bDoAddrTranslation,
        volatile uint64_t P1,
        volatile uint64_t P2,
        volatile uint64_t P3,
        volatile uint64_t P4,
        volatile uint64_t P5,
        volatile uint64_t P6,
        volatile uint64_t P7,
        volatile uint64_t P8,
        volatile uint64_t P9)
{
	
	switch(uiSysCallID)
	{
		case SYS_CALL_CHANGE_DIR : //Change Directory
			//P1 => Directory Path
			{
				char* szPathAddress = KERNEL_ADDR(bDoAddrTranslation, char*, P1) ;

				*piRetVal = 0 ;
        try
        {
          FileOperations_ChangeDir(szPathAddress);
        }
        catch(const upan::exception& ex)
        {
          ex.Print();
					*piRetVal = -1 ;
        }
			}
			break ;

		case SYS_CALL_PWD : //Get PWD
			//P1 => Return Dir Name Pointer
			{
				char** szPathAddress = KERNEL_ADDR(bDoAddrTranslation, char**, P1) ;
        *piRetVal = 0;
        try
        {
          Directory_PresentWorkingDirectory( &ProcessManager::Instance().GetCurrentPAS(), szPathAddress);
        }
        catch(const upan::exception& ex)
        {
          ex.Print();
          *piRetVal = -1;
        }
			}
			break ;

		case SYS_CALL_CWD : //Get CWD
			//P1 => Return Dir Name Pointer
			//P2 => Buf Length
			{
				char* szPathAddress = KERNEL_ADDR(bDoAddrTranslation, char*, P1) ;
				*piRetVal = 0;

        try
        {
          FileOperations_GetCWD(szPathAddress, P2);
        }
        catch(upan::exception& ex)
        {
          ex.Print();
          *piRetVal = -1;
        }
			}
			break ;

		case SYS_CALL_MKDIR : //Create Dir / File
		case SYS_CALL_FILE_CREATE:
			//P1 => Dir Path
			//P2 => Dir Attr
			{
				char* szPathAddress = KERNEL_ADDR(bDoAddrTranslation, char*, P1) ;
				unsigned short usType = (uiSysCallID == SYS_CALL_MKDIR) ? ATTR_TYPE_DIRECTORY : ATTR_TYPE_FILE ; 
				*piRetVal = 0 ;

        try
        {
          FileOperations_SyncPWD();
          FileOperations_Create(szPathAddress, usType, (unsigned short)(P2));
        }
        catch(const upan::exception& ex)
				{
          ex.Print();
					*piRetVal = -1 ;
				}
			}
			break ;

		case SYS_CALL_RMDIR : //Delete Dir / File
			//P1 => Dir Path
			{
				char* szPathAddress = KERNEL_ADDR(bDoAddrTranslation, char*, P1) ;
				*piRetVal = 0 ;
        try
        {
          FileOperations_SyncPWD();
          FileOperations_Delete(szPathAddress);
        }
        catch(const upan::exception& ex)
        {
          ex.Print();
          *piRetVal = -1 ;
        }
			}
			break ;

		case SYS_CALL_GET_DIR_LIST :
			// P1 => Dir Path
			// P2 => Ret Dir Content List Address
			// P3 => Ret Dir Content List Size Address
			{
				char* szPathAddress = KERNEL_ADDR(bDoAddrTranslation, char*, P1) ;
				FileSystem::Node** pRetDirContentList = KERNEL_ADDR(bDoAddrTranslation, FileSystem::Node**, P2) ;
				int* pRetDirContentListSize = KERNEL_ADDR(bDoAddrTranslation, int*, P3) ;
				*piRetVal = 0 ;

        try
        {
          FileOperations_SyncPWD();
          FileOperations_GetDirectoryContent(szPathAddress, pRetDirContentList, pRetDirContentListSize);
        }
        catch(const upan::exception& ex)
        {
          ex.Print();
          *piRetVal = -1 ;
        }
			}
			break ;
		
		case SYS_CALL_FILE_OPEN:
			// P1 => File Name / Path
			// P2 => Mode
			{
				const char* szFileNameAddr = KERNEL_ADDR(bDoAddrTranslation, const char*, P1) ;
				byte mode = P2 ;

				*piRetVal = 0 ;

        try
        {
          FileOperations_SyncPWD();
          *piRetVal = FileOperations_Open(szFileNameAddr, mode).id();
        }
        catch(const upan::exception& ex)
        {
          ex.Print();
          *piRetVal = -1 ;
        }
			}
			break ;

		case SYS_CALL_FILE_CLOSE:
			// P1 => File Desc
			{
				*piRetVal = 0 ;
				if(FileOperations_Close((int)P1) != FileOperations_SUCCESS)
					*piRetVal = -1 ;
			}
			break ;

		case SYS_CALL_FILE_READ:
			// P1 => File Desc
			// P2 => Read buffer address
			// P3 => Byte to read 
			// Return Value = Bytes Read
			{
				char* szBufferAddr = KERNEL_ADDR(bDoAddrTranslation, char*, P2) ;

				*piRetVal = 0 ;
        try
        {
          auto& file = ProcessManager::Instance().GetCurrentPAS().iodTable().getRealNonDupped((int)P1);
          *piRetVal = file.read(szBufferAddr, (int)P3);
        }
        catch(...)
        {
          *piRetVal = -1 ;
        }
			}
			break ;

		case SYS_CALL_FILE_WRITE:
			// P1 => File Desc
			// P2 => Write buffer address
			// P3 => Byte to write 
			{
				const char* szBufferAddr = KERNEL_ADDR(bDoAddrTranslation, const char*, P2);

				*piRetVal = 0 ;
        try
        {
          auto& file = ProcessManager::Instance().GetCurrentPAS().iodTable().getRealNonDupped((int)P1);
          *piRetVal =  file.write(szBufferAddr, (int)P3);
        }
        catch(const upan::exception& ex)
				{
          ex.Print();
					*piRetVal = -1 ;
				}
			}
			break ;

	  case SYS_CALL_FILE_SELECT:
	    // P1 => Input IO Descriptors to wait on
	    // P2 => Output IO Descriptors that are ready
	    {
	      io_descriptor* in_waitIODescriptors = KERNEL_ADDR(bDoAddrTranslation, io_descriptor*, P1);
	      io_descriptor* out_readyIODescriptors = KERNEL_ADDR(bDoAddrTranslation, io_descriptor*, P2);

	      upan::vector<io_descriptor> waitIODescriptors;
	      for(int i = 0; in_waitIODescriptors[i]._fd >= 0; ++i) {
	        waitIODescriptors.push_back(in_waitIODescriptors[i]);
	      }

	      const auto& readyIODescriptors = ProcessManager::Instance().GetCurrentPAS().iodTable().select(waitIODescriptors);
	      int i;
	      for(i = 0; i < readyIODescriptors.size(); ++i) {
	        out_readyIODescriptors[i] = readyIODescriptors[i];
	      }
	      out_readyIODescriptors[i]._fd = -1;
	    }
	    break;
		case SYS_CALL_FILE_SEEK:
			// P1 => File Desc
			// P2 => Offset
			// P3 => Seek Type
			{
				*piRetVal = 0 ;
        try
        {
          auto& file = ProcessManager::Instance().GetCurrentPAS().iodTable().getRealNonDupped((int)P1);
          file.seek((int)P3, (int)P2);
        }
        catch(upan::exception& ex)
        {
          ex.Print();
          *piRetVal = -1;
        }
      }
			break ;

		case SYS_CALL_FILE_TELL:
			// P1 => File Desc
			// P2 => Seek Type
			// P3 => Offset
			{
        try
        {
          *piRetVal = FileOperations_GetOffset((int)P1);
        }
        catch(const upan::exception& ex)
        {
          ex.Print();
          *piRetVal = -1;
        }
			}
			break ;

		case SYS_CALL_FILE_MODE:
			// P1 => File Desc
			// P2 => Seek Type
			// P3 => Offset
			{
        try
        {
          *piRetVal = FileOperations_GetFileOpenMode((int)P1);
        }
        catch(const upan::exception& ex)
				{
					*piRetVal = -1 ;
				}
			}
			break ;

		case SYS_CALL_FILE_STAT:
			// P1 => File Name
			// P2 => FileStat
			{
				*piRetVal = 0 ;
				const char* szPathAddress = KERNEL_ADDR(bDoAddrTranslation, const char*, P1) ;
        try
        {
          FileSystem_FileStat* pFileStat = KERNEL_ADDR(bDoAddrTranslation, FileSystem_FileStat*, P2);
          *pFileStat = FileOperations_GetStat(szPathAddress, FROM_FILE);
        }
        catch(const upan::exception& ex)
				{
          ex.Print();
					*piRetVal = -1 ;
				}
			}
			break ;

		case SYS_CALL_FILE_STAT_FD:
			// P1 => File Name
			// P2 => FileStat
			{
				*piRetVal = 0 ;
				int iFD = P1 ;
        try
        {
          FileSystem_FileStat* pFileStat = KERNEL_ADDR(bDoAddrTranslation, FileSystem_FileStat*, P2) ;
          *pFileStat = FileOperations_GetStatFD(iFD);
        }
        catch(const upan::exception& ex)
				{
          ex.Print();
					*piRetVal = -1 ;
				}
			}
			break ;

		case SYS_CALL_FILE_ACCESS:
			// P1 => File Name
			// P2 => Mode
			{
				*piRetVal = 0 ;
				const char* szPathAddress = KERNEL_ADDR(bDoAddrTranslation, const char*, P1) ;
        if(!FileOperations_FileAccess(szPathAddress, FROM_FILE, (int)P2))
				{
					*piRetVal = -1 ;
				}
			}
			break ;

		case SYS_CALL_FILE_DUP2:
			// P1 => Old FD
			// P2 => New FD
			{
				*piRetVal = 0 ;
				try {
          FileOperations_Dup2(P1, P2);
				} catch(upan::exception& e) {
				  e.Print();
          *piRetVal = -1 ;
				}
			}
			break ;
	}
}
