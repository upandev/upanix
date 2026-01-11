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

int SysIO_Close(int fd) {
  uint64_t retStatus ;
  SysCallIO_Handle(&retStatus, SYS_CALL_IO_CLOSE, false, fd, 2, 3, 4, 5);
  return retStatus ;
}

int SysIO_Ctl(int fd, uint64_t cmd, uint64_t arg) {
  uint64_t retStatus;
  SysCallIO_Handle(&retStatus, SYS_CALL_IO_CTL, false, fd, cmd, arg, 4, 5);
  return retStatus;
}

int SysIO_OpenPT(int flags) {
  uint64_t retStatus;
  SysCallIO_Handle(&retStatus, SYS_CALL_IO_OPENPT, false, flags, 2, 3, 4, 5);
  return retStatus;
}

int SysIO_GetPTSName(int fd, char* name, int len) {
  uint64_t retStatus;
  SysCallIO_Handle(&retStatus, SYS_CALL_IO_PTS_NAME, false, (uint64_t)fd, (uint64_t)name, (uint64_t)len, 4, 5);
  return retStatus;
}

int SysIO_TCGetAttr(int fd, struct termios *termios_p) {
  uint64_t retStatus;
  SysCallIO_Handle(&retStatus, SYS_CALL_IO_TC_GET_ATTR, false, (uint64_t)fd, (uint64_t)termios_p, 3, 4 ,5);
  return retStatus;
}

int SysIO_TCSetAttr(int fd, termios_actions action, const struct termios *termios_p) {
  uint64_t retStatus;
  SysCallIO_Handle(&retStatus, SYS_CALL_IO_TC_SET_ATTR, false, (uint64_t)fd, (uint64_t)action, (uint64_t)termios_p, 4 ,5);
  return retStatus;
}

int SysIO_IsTTY(int fd) {
  uint64_t retStatus;
  SysCallIO_Handle(&retStatus, SYS_CALL_IO_IS_TTY, false, (uint64_t)fd, 2, 3, 4 ,5);
  return (int)retStatus;
}

int SysIO_CreatePipe(int fd[2]) {
  uint64_t retStatus;
  SysCallIO_Handle(&retStatus, SYS_CALL_IO_CREATE_PIPE, false, (uint64_t)fd, 2, 3, 4 ,5);
  return (int)retStatus;
}