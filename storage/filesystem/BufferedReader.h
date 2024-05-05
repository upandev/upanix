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
#ifndef _BUFFERED_READER_H_
#define _BUFFERED_READER_H_

#include <Global.h>
#include <ustring.h>
#include <IODescriptor.h>

class BufferedReader
{
	private:
		unsigned m_uiOffSet ;
		unsigned m_uiBufferSize ;
		char* m_szBuffer ;

	public:
		BufferedReader(const upan::string& szFileName, unsigned uiOffSet, unsigned uiBufferSize) ;
		~BufferedReader() ;

    void Seek(unsigned uiOffSet) ;
    int Read(char* szBuffer, int iLen) ;

  private:
    IODescriptor* _file;
    int DoRead(char* szBuffer, int iLen);
} ;

#endif
