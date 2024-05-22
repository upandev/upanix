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
# include <StorageDrive.h>
# include <try.h>
# include <FileDescriptor.h>

byte SysCallFile_IsPresent(uint64_t sysCallId)
{
	return (sysCallId > SYS_CALL_FILE_START && sysCallId < SYS_CALL_FILE_END) ;
}

void SysCallFile_Handle(uint64_t *retVal, uint64_t sysCallId, bool doAddrTranslation, uint64_t p1, uint64_t p2, uint64_t p3,
                   uint64_t p4, uint64_t p5)
{
	
	switch(sysCallId)
	{
		case SYS_CALL_CHANGE_DIR : //Change Directory
			//P1 => Directory Path
			{
				char* szPathAddress = ( char*) p1;

				*retVal = 0 ;
        try {
          FileOperations::Instance().changeDir(szPathAddress);
        } catch(const upan::exception& ex) {
          ex.Print();
					*retVal = -1 ;
        }
			}
			break ;

		case SYS_CALL_CWD : //Get CWD
			//P1 => Return Dir Name Pointer
			//P2 => Buf Length
			{
				char* szPathAddress = ( char*) p1;
				*retVal = 0;

        try {
          const upan::string& fullPath = FileOperations::Instance().getcwd();
          if ((int)p2 < fullPath.length()) {
            throw upan::exception(XLOC, "insufficient path buffer size");
          }
          strcpy(szPathAddress, fullPath.c_str());
        }
        catch(upan::exception& ex)
        {
          ex.Print();
          *retVal = -1;
        }
			}
			break ;

		case SYS_CALL_MKDIR : //Create Dir / File
		case SYS_CALL_FILE_CREATE:
			//P1 => Dir Path
			//P2 => Dir Attr
			{
				char* szPathAddress = ( char*) p1;
				unsigned short usType = (sysCallId == SYS_CALL_MKDIR) ? ATTR_TYPE_DIRECTORY : ATTR_TYPE_FILE ;
				*retVal = 0 ;

        try
        {
          FileOperations::Instance().create(szPathAddress, usType, (uint16_t)(p2));
        }
        catch(const upan::exception& ex)
				{
          ex.Print();
					*retVal = -1 ;
				}
			}
			break ;

		case SYS_CALL_RMDIR : //Delete Dir / File
			//P1 => Dir Path
			{
				char* szPathAddress = ( char*) p1;
				*retVal = 0 ;
        try
        {
          FileOperations::Instance().remove(szPathAddress);
        }
        catch(const upan::exception& ex)
        {
          ex.Print();
          *retVal = -1 ;
        }
			}
			break ;

		case SYS_CALL_GET_DIR_LIST :
			// P1 => Dir Path
			// P2 => Ret Dir Content List Address
			// P3 => Ret Dir Content List Size Address
			{
				const char* dirPath = (char*) p1;
				auto** retStats = (struct stat_ex**) p2;
				int* retSize = ( int*) p3;
				*retVal = 0 ;

        try {
          FileOperations::Instance().listDir(dirPath, retStats, retSize);
        } catch(const upan::exception& ex) {
          ex.Print();
          *retVal = -1 ;
        }
			}
			break ;
		
		case SYS_CALL_FILE_OPEN:
			// P1 => File Name / Path
			// P2 => Mode
			{
				const char* szFileNameAddr = ( const char*) p1;
				byte mode = p2 ;

				*retVal = 0 ;

        try {
          *retVal = FileOperations::Instance().open(szFileNameAddr, mode).id();
        } catch(const upan::exception& ex) {
          ex.Print();
          *retVal = -1 ;
        }
			}
			break ;

		case SYS_CALL_FILE_CLOSE:
			// P1 => File Desc
			{
				*retVal = FileOperations::Instance().close((int)p1) ? 0 : -1;
			}
			break ;

		case SYS_CALL_FILE_READ:
			// P1 => File Desc
			// P2 => Read buffer address
			// P3 => Byte to read 
			// Return Value = Bytes Read
			{
				char* szBufferAddr = ( char*) p2;

				*retVal = 0 ;
        try
        {
          auto& file = ProcessManager::Instance().GetCurrentPAS().iodTable().getRealNonDupped((int)p1);
          *retVal = file.read(szBufferAddr, (int)p3);
        }
        catch(...)
        {
          *retVal = -1 ;
        }
			}
			break ;

		case SYS_CALL_FILE_WRITE:
			// P1 => File Desc
			// P2 => Write buffer address
			// P3 => Byte to write 
			{
				const char* szBufferAddr = ( const char*) p2;

				*retVal = 0 ;
        try
        {
          auto& file = ProcessManager::Instance().GetCurrentPAS().iodTable().getRealNonDupped((int)p1);
          *retVal =  file.write(szBufferAddr, (int)p3);
        }
        catch(const upan::exception& ex)
				{
          ex.Print();
					*retVal = -1 ;
				}
			}
			break ;

	  case SYS_CALL_FILE_SELECT:
	    // P1 => Input IO Descriptors to wait on
	    // P2 => Output IO Descriptors that are ready
	    {
	      io_descriptor* in_waitIODescriptors = ( io_descriptor*) p1;
	      io_descriptor* out_readyIODescriptors = ( io_descriptor*) p2;

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
				*retVal = 0 ;
        try
        {
          auto& file = ProcessManager::Instance().GetCurrentPAS().iodTable().getRealNonDupped((int)p1);
          file.seek((int)p3, (int)p2);
        }
        catch(upan::exception& ex)
        {
          ex.Print();
          *retVal = -1;
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
          *retVal = ProcessManager::Instance().GetCurrentPAS().iodTable().getRealNonDupped((int)p1).getOffset();
        }
        catch(const upan::exception& ex)
        {
          ex.Print();
          *retVal = -1;
        }
			}
			break ;

		case SYS_CALL_FILE_MODE:
			// P1 => File Desc
			// P2 => Seek Type
			// P3 => Offset
			{
        try {
          *retVal = ProcessManager::Instance().GetCurrentPAS().iodTable().getRealNonDupped((int)p1).getMode();
        } catch(const upan::exception& ex) {
					*retVal = -1 ;
				}
			}
			break ;

		case SYS_CALL_FILE_STAT:
			// P1 => File Name
			// P2 => FileStat
			{
				*retVal = 0 ;
        try {
          const char* filePath = (const char*) p1;
          auto fileStat = (struct stat*) p2;
          *fileStat = FileOperations::Instance().stats(filePath);
        } catch(const upan::exception& ex) {
          ex.Print();
					*retVal = -1 ;
				}
			}
			break ;

		case SYS_CALL_FILE_STAT_FD:
			// P1 => File Name
			// P2 => FileStat
			{
				*retVal = 0 ;
        try
        {
          auto& file = dynamic_cast<FileDescriptor&>(ProcessManager::Instance().GetCurrentPAS().iodTable().getRealNonDupped((int)p1));
          auto pFileStat = (struct stat*)p2;
          *pFileStat = file.getStat();
        }
        catch(const upan::exception& ex)
				{
          ex.Print();
					*retVal = -1 ;
				}
			}
			break ;

		case SYS_CALL_FILE_ACCESS:
			// P1 => File Name
			// P2 => Mode
			{
        const char* szPathAddress = ( const char*) p1;
        try {
          *retVal = FileOperations::Instance().fileAccess(szPathAddress, (uint8_t)p2) ? 0 : -1;
        } catch(const upan::exception& ex) {
          ex.Print();
          *retVal = -1;
        }
			}
			break ;

		case SYS_CALL_FILE_DUP2:
			// P1 => Old FD
			// P2 => New FD
			{
				*retVal = 0 ;
				try {
          FileOperations::Instance().dup2(p1, p2);
				} catch(upan::exception& e) {
				  e.Print();
          *retVal = -1 ;
				}
			}
			break ;
	}
}
