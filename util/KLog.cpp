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

# include <syslog.h>
# include <KLog.h>

void KLog::enable(const upan::string& priority) {
  enable_log_priority(str_to_log_priority(priority.c_str()));
}

void KLog::disable(const upan::string& priority) {
  disable_log_priority(str_to_log_priority(priority.c_str()));
}

void KLog::debug(const char* __restrict fmsg, ...) {
  va_list arg;
  va_start(arg, fmsg);
  syslog_arg(LOG_DEBUG, fmsg, arg);
  va_end(arg);
}

void KLog::info(const char* __restrict fmsg, ...) {
  va_list arg;
  va_start(arg, fmsg);
  syslog_arg(LOG_INFO, fmsg, arg);
  va_end(arg);
}

void KLog::notice(const char* __restrict fmsg, ...) {
  va_list arg;
  va_start(arg, fmsg);
  syslog_arg(LOG_NOTICE, fmsg, arg);
  va_end(arg);
}

void KLog::warn(const char* __restrict fmsg, ...) {
  va_list arg;
  va_start(arg, fmsg);
  syslog_arg(LOG_WARNING, fmsg, arg);
  va_end(arg);
}

void KLog::error(const char* __restrict fmsg, ...) {
  va_list arg;
  va_start(arg, fmsg);
  syslog_arg(LOG_ERR, fmsg, arg);
  va_end(arg);
}

void KLog::critical(const char* __restrict fmsg, ...) {
  va_list arg;
  va_start(arg, fmsg);
  syslog_arg(LOG_CRIT, fmsg, arg);
  va_end(arg);
}

void KLog::alert(const char* __restrict fmsg, ...) {
  va_list arg;
  va_start(arg, fmsg);
  syslog_arg(LOG_ALERT, fmsg, arg);
  va_end(arg);
}

void KLog::emergency(const char* fmsg, ...) {
  va_list arg;
  va_start(arg, fmsg);
  syslog_arg(LOG_EMERG, fmsg, arg);
  va_end(arg);
}

void KLog::exception(const upan::exception& e) {
  KLog::critical("%s", e.ErrorMsg().c_str());
}