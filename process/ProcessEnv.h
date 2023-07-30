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
#ifndef _PROCESS_ENV_H_
#define _PROCESS_ENV_H_

#define ProcessEnv_SUCCESS 0
#define ProcessEnv_ERR_FULL 1
#define ProcessEnv_FAILURE 2

#include <ProcessManager.h>

#define ENV_VAR_LENGTH 256
#define NO_OF_ENVS (PAGE_SIZE / ENV_VAR_LENGTH)


typedef struct
{
	char Var[56] ;
	char Val[200] ;
} ProcessEnvEntry ;

void ProcessEnv_Initialize(__volatile__ unsigned uiPDEAddress, __volatile__ int iParentProcessID);
void ProcessEnv_InitializeForKernelProcess() ;
void ProcessEnv_UnInitialize(SchedulableProcess&);
char* ProcessEnv_Get(const char* szEnvVar) ;
byte ProcessEnv_Set(const char* szEnvVar, const char* szEnvValue) ;

#endif
