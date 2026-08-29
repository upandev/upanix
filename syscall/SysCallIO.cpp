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
# include <KernelUtil.h>
# include <FSDeviceManager.h>
# include <TerminalMasterDescriptor.h>
# include <TerminalDescriptor.h>
# include <PipeReadDescriptor.h>
# include <PipeWriteDescriptor.h>

bool SysCallIO_IsPresent(uint64_t sysCallId) {
	return (sysCallId > SYS_CALL_IO_START && sysCallId < SYS_CALL_IO_END) ;
}

void SysCallIO_Handle(uint64_t *retVal, uint64_t sysCallId, bool doAddrTranslation, uint64_t p1, uint64_t p2, uint64_t p3, uint64_t p4, uint64_t p5) {
	switch (sysCallId) {
		case SYS_CALL_IO_CLOSE:
			// P1 => descriptor
			{
        *retVal = 0;
        try {
          ProcessManager::Instance().GetCurrentPAS().iodTable().free((int)p1);
        } catch(upan::exception& e) {
          KLog::exception(e);
          *retVal = -1;
        }
			}
			break;

    case SYS_CALL_IO_CTL:
      {
        *retVal = 0;
        try {
          KernelUtil::IOCtl((int)p1, p2, p3);
        } catch(upan::exception& e) {
          e.Print();
          *retVal = -1;
        }
      }
      break;

    case SYS_CALL_IO_OPENPT:
    {
      *retVal = 0;
      auto flags = (int)p1;
      try {
        *retVal = FSDeviceManager::Instance().createTerminalDevice(flags);
      } catch(upan::exception& e) {
        KLog::exception(e);
        *retVal = -1;
      }
    }
    break;

    case SYS_CALL_IO_PTS_NAME:
    {
      *retVal = 0;
      auto fd = (int)p1;
      auto name = (char*)p2;
      auto len = (int)p3;
      try {
        strncpy(name,
                ProcessManager::Instance().GetCurrentPAS().iodTable().getRealNonDupped(fd).cast<TerminalMasterDescriptor>()->name().c_str(),
                len);
      } catch(upan::exception& e) {
        KLog::exception(e);
        *retVal = -1;
      }
    }
    break;

    case SYS_CALL_IO_TC_GET_ATTR:
    {
      *retVal = 0;
      auto fd = (int)p1;
      auto termios_p = (struct termios*)p2;
      try {
        if (termios_p) {
          auto& process = ProcessManager::Instance().GetCurrentPAS();
          auto terminalDevice = process.iodTable().getRealNonDupped(fd).cast<TerminalDescriptor>()->terminalDevice();
          *termios_p = terminalDevice->getTermios();
        }
      } catch(upan::exception& e) {
        KLog::exception(e);
        *retVal = -1;
      }
    }
    break;

    case SYS_CALL_IO_TC_SET_ATTR:
    {
      *retVal = 0;
      auto fd = (int)p1;
      auto termios_p = (struct termios*)p3;
      try {
        if (termios_p) {
          auto& process = ProcessManager::Instance().GetCurrentPAS();
          auto terminalDevice = process.iodTable().getRealNonDupped(fd).cast<TerminalDescriptor>()->terminalDevice();
          terminalDevice->setTermios(*termios_p);
        }
      } catch(upan::exception& e) {
        KLog::exception(e);
        *retVal = -1;
      }
    }
    break;

    case SYS_CALL_IO_IS_TTY:
    {
      *retVal = 0;
      auto fd = (int)p1;
      try {
        auto& process = ProcessManager::Instance().GetCurrentPAS();
        auto terminalDevice = process.iodTable().getRealNonDupped(fd).cast<TerminalDescriptor>();
        *retVal = terminalDevice.isEmpty() ? 0 : 1;
      } catch(upan::exception& e) {
        KLog::exception(e);
        *retVal = -1;
      }
    }
    break;

    case SYS_CALL_IO_CREATE_PIPE:
    {
      *retVal = 0;
      auto fd = (int*)p1;
      upan::string path((const char*)p2);
      try {
        if (path.empty()) {
          path = FSDeviceManager::Instance().createPipeDevice();
        } else {
          FSDeviceManager::Instance().createPipeDevice(path);
        }
        auto& process = ProcessManager::Instance().GetCurrentPAS();
        fd[0] = process.iodTable().allocate([&](int fd) {
          return new PipeReadDescriptor(process.processID(), fd, O_RDONLY, path);
        })->id();
        fd[1] = process.iodTable().allocate([&](int fd) {
          return new PipeWriteDescriptor(process.processID(), fd, O_WRONLY, path);
        })->id();
      } catch(const upan::exception& e) {
        KLog::exception(e);
        *retVal = -1;
      }
    }
    break;
  }
}
