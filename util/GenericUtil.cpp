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
# include <GenericUtil.h>
# include <KeyboardHandler.h>
# include <StringUtil.h>
# include <FileOperations.h>
# include <DMM.h>
#include <ProcessManager.h>

class StrPathTokenizer : public StringTokenizer
{
	public:
		StrPathTokenizer(const char* szFileName, char* szFullFilePath) : 
			m_szFileName(szFileName), 
			m_szFullFilePath(szFullFilePath), 
			m_bFound(false) 
	{}
	
	void operator()(int index, const char* src, int len)
	{
		if(!m_bFound)
		{
			byte szTemp[128] ;

      memcpy(szTemp, src, len);
			szTemp[len] = '\0' ;
			
			if(szTemp[len - 1] != '/')
				strcat((char*)szTemp, "/") ;
			
			strcat((char*)szTemp, m_szFileName) ;

      if(FileOperations::Instance().exists((const char*)szTemp, ATTR_TYPE_FILE))
			{
				m_bFound = true ;

        memcpy(m_szFullFilePath, src, len);
				m_szFullFilePath[len] = '\0' ;
				
				if(m_szFullFilePath[len - 1] != '/')
					strcat(m_szFullFilePath, "/") ;
			}
		}
	}

	bool IsFound() { return m_bFound ; }

	private:
	const char* m_szFileName ;
	char* m_szFullFilePath ;
	bool m_bFound ;
} ;

void GenericUtil_ReadInput(char* szInputBuffer, const int iMaxReadLength, byte bEcho)
{
	int iCurrentReadPos = 0 ;

	while(SUCCESS)
	{
    const auto& data = KeyboardHandler::Instance().GetCharInBlockMode();
    uint8_t ch = data.getCh();

		switch(ch)
		{
			case Keyboard_LEFT_ALT:
			case Keyboard_LEFT_CTRL:
				break ;
			case Keyboard_F1:
			case Keyboard_F2:
			case Keyboard_F3:
			case Keyboard_F4:
			case Keyboard_F5:
			case Keyboard_F6:
			case Keyboard_F7:
			case Keyboard_F8:
//				SessionManager_SwitchToSession(SessionManager_KeyToSessionIDMap(ch)) ;
				break ;
			case Keyboard_F9:
			case Keyboard_F10:
				break ;
				
			case Keyboard_CAPS_LOCK:
				break ;
			case Keyboard_BACKSPACE:
				if(iCurrentReadPos > 0)
				{
					if(bEcho)
					{
            KC::MConsole().MoveCursor(-1) ;
            KC::MConsole().ClearLine(upanui::ConsoleBuffer::START_CURSOR_POS) ;
					}
					iCurrentReadPos-- ;
				}
				break ;
				
			case Keyboard_LEFT_SHIFT:
			case Keyboard_RIGHT_SHIFT:
				break ;
				
			case Keyboard_ESC:
				break ;

			case Keyboard_ENTER:
				szInputBuffer[iCurrentReadPos] = '\0' ;
				return ;

			default:

				if(iCurrentReadPos != iMaxReadLength)
				{
					szInputBuffer[iCurrentReadPos++] = ch ;
					if(bEcho)
            printf("%c", ch);
				}
		}
	}
}

bool GenericUtil_GetFullFilePathFromEnv(const char* szPathEnvVar, const char* szPathEnvDefVal, const char* szFileName, char* szFullFilePath)
{
  auto& pas = ProcessManager::Instance().GetCurrentPAS();
  char szEnvValue[MAX_ENV_VAL_LEN] ;
  const auto& val = pas.getEnv(szPathEnvVar);
	if(val.isEmpty())
	{
		//Default Env Path
		strcpy(szEnvValue, szPathEnvDefVal) ;
	}
	else
	{
		//Deafult Env Path
		strcpy(szEnvValue, val.value().c_str()) ;
		strcat(szEnvValue, szPathEnvDefVal) ;
	}

	// If no path env, then its from root "/"
	if(strlen(szEnvValue) == 0)
	{
		strcpy(szFullFilePath, "/") ;
		return true ;
	}

	int iListSize ;
	StrPathTokenizer tokenizer(szFileName, szFullFilePath) ;
	String_Tokenize(szEnvValue, ':', &iListSize, tokenizer) ;

  return tokenizer.IsFound() ;
}

void debug_step(const char* msg)
{
  printf("\n%s...", msg);
  KeyboardHandler::Instance().Getch();
  printf("\n%s - done!");
}
