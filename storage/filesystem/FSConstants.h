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
#pragma once

#include <fs.h>
#include <vector.h>

#define MEDIA_REMOVABLE	0xF0
#define MEDIA_FIXED		0xF8

#define EOC		0x0FFFFFFF
#define EOC_B	0xFF

#define FS_ROOT_DIR "/"
#define DIR_SPECIAL_CURRENT		"."
#define DIR_SPECIAL_PARENT		".."

#define FILE_STDOUT "STDOUT"
#define FILE_STDIN  "STDIN"
#define FILE_STDERR "STDERR"

typedef enum {
  DIR_ACCESS_TIME = 0x01,
  DIR_MODIFIED_TIME = 0x02
} TIME_TYPE;

typedef enum
{
  USER_OWNER,
  USER_GROUP,
  USER_OTHERS
} FILE_USER_TYPE;