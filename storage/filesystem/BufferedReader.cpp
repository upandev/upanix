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
# include <BufferedReader.h>
# include <DMM.h>
# include <FileOperations.h>
# include <IODescriptorTable.h>

/* This is used in case UnBuffered Data comes after Buffered Data. If the unbuffered data offset is beyond file size
file op will return error which causes buffered reader to return error... which shouldn't happen because the request
is not completely beyond file size. So, to avoid this, request to file op is sent with offset - OVERFLOW_ADJUST bytes
to ensure some non zero byte request is there within file size limit*/
#define OVERFLOW_ADJUST 2

BufferedReader::BufferedReader(const upan::string& szFileName, unsigned uiOffSet, unsigned uiBufferSize) : m_uiOffSet(uiOffSet), m_szBuffer(nullptr) {
  _file = FileOperations::Instance().open(szFileName.c_str(), O_RDONLY);
  if (_file.isEmpty()) {
    throw upan::exception(XLOC, "Failed to open file %s", szFileName.c_str());
  }

  try
  {
    _file->seek(SEEK_SET, uiOffSet);
    upan::uniq_ptr<char[]> buffer(new char[uiBufferSize]);
    m_uiBufferSize = _file->read(buffer.get(), uiBufferSize);
    m_szBuffer = buffer.release();
  }
  catch(...)
	{
    close(_file->id());
    throw;
	}
}

BufferedReader::~BufferedReader()
{
  delete[] m_szBuffer;
	close(_file->id()) ;
}

void BufferedReader::Seek(unsigned uiOffSet) {
  _file->seek(SEEK_SET, uiOffSet);
}

int BufferedReader::Read(uint8_t* szBuffer, int iLen)
{
  return DoRead(szBuffer, iLen);
}

int BufferedReader::DoRead(uint8_t* szBuffer, int iLen)
{
  unsigned uiCurrentOffset = _file->getOffset();

	if(uiCurrentOffset >= m_uiOffSet && (uiCurrentOffset + iLen) <= (m_uiOffSet + m_uiBufferSize))
	{
		// Case 1: Data Completely Buffered
		// Buffer:-		<----------------------->
		// Request:-		<--------->
		
		memcpy(szBuffer, m_szBuffer + (uiCurrentOffset - m_uiOffSet), iLen) ;
    return iLen ;
	}
	else if((uiCurrentOffset + iLen) < m_uiOffSet || uiCurrentOffset > (m_uiOffSet + m_uiBufferSize))
	{
		// Case 2: Data Completely UnBuffered
		// Buffer:-						<----------------------->
		// Request:-	<--------->
		// Request:-												<--------->

    return _file->read(szBuffer, iLen);
	}
	else
	{
		// Case 3: Partially Buffered
		// Case 3a: 
		// Buffer:-				<----------------------->
		// Request:-							<--------------->

		if(
		(uiCurrentOffset >= m_uiOffSet && uiCurrentOffset <= (m_uiOffSet + m_uiBufferSize - OVERFLOW_ADJUST))
			&&
		((uiCurrentOffset + iLen) > (m_uiOffSet + m_uiBufferSize))
		)
		{
			unsigned uiBufLen = m_uiBufferSize - (uiCurrentOffset - m_uiOffSet) - OVERFLOW_ADJUST ;

			memcpy(szBuffer, m_szBuffer + (uiCurrentOffset - m_uiOffSet), uiBufLen) ;

      _file->seek(SEEK_SET, uiCurrentOffset + uiBufLen);
      return _file->read(szBuffer + uiBufLen, iLen - uiBufLen) + uiBufLen;
		}
		// Case 3b: 
		// Buffer:-				<----------------------->
		// Request:-	<--------------->
		else if(
		((uiCurrentOffset + iLen) >= m_uiOffSet && (uiCurrentOffset + iLen) <= (m_uiOffSet + m_uiBufferSize))
			&&
		(uiCurrentOffset < m_uiOffSet)
		)
		{
			unsigned uiFileLen = (m_uiOffSet - uiCurrentOffset) ;
      int readLen = _file->read(szBuffer, uiFileLen);

			memcpy(szBuffer + uiFileLen, m_szBuffer, (iLen - uiFileLen)) ;	

      return readLen + (iLen - uiFileLen) ;
		}
		// Case 3c: 
		// Buffer:-				<----------------------->
		// Request:-	<---------------------------------------->
		else if(((uiCurrentOffset + iLen) > (m_uiOffSet + m_uiBufferSize) && uiCurrentOffset < m_uiOffSet))
		{
      unsigned uiFileLenLeft = (m_uiOffSet - uiCurrentOffset) ;
      int readLen = _file->read(szBuffer, uiFileLenLeft);

			unsigned uiBufLen = m_uiBufferSize - OVERFLOW_ADJUST ;
			memcpy(szBuffer + uiFileLenLeft, m_szBuffer, uiBufLen) ;

			//unsigned uiFileLenRight = (m_uiOffSet + m_uiBufferSize) - (uiCurrentOffset + iLen) + OVERFLOW_ADJUST ;
      readLen += uiBufLen ;
      unsigned uiFileLenRight = iLen - readLen ;
      _file->seek(SEEK_SET, uiCurrentOffset + readLen);
      return _file->read(szBuffer + readLen, uiFileLenRight) + readLen;
		}
		else
		{
      throw upan::exception(XLOC, "Bug in BufferedReader impl");
		}
	}
}

