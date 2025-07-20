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
# include <KLog.h>
# include <logger.h>

void KLog::enable(uint32_t levels) {
  upan::logger::instance().enable(levels);
}

void KLog::disable(uint32_t levels) {
  upan::logger::instance().disable(levels);
}

void KLog::trace(const char* __restrict fmsg, ...) {
  va_list arg;
  va_start(arg, fmsg);
  upan::logger::instance().logarg(upan::logger::LOG_TRACE, fmsg, arg);
  va_end(arg);
}

void KLog::debug(const char* __restrict fmsg, ...) {
  va_list arg;
  va_start(arg, fmsg);
  upan::logger::instance().logarg(upan::logger::LOG_DEBUG, fmsg, arg);
  va_end(arg);
}

void KLog::info(const char* __restrict fmsg, ...) {
  va_list arg;
  va_start(arg, fmsg);
  upan::logger::instance().logarg(upan::logger::LOG_INFO, fmsg, arg);
  va_end(arg);
}

void KLog::warn(const char* __restrict fmsg, ...) {
  va_list arg;
  va_start(arg, fmsg);
  upan::logger::instance().logarg(upan::logger::LOG_WARN, fmsg, arg);
  va_end(arg);
}

void KLog::error(const char* __restrict fmsg, ...) {
  va_list arg;
  va_start(arg, fmsg);
  upan::logger::instance().logarg(upan::logger::LOG_ERROR, fmsg, arg);
  va_end(arg);
}

void KLog::exception(const upan::exception& e) {
  KLog::error("%s", e.ErrorMsg().c_str());
}