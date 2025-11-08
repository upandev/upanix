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
# include <CommandLineParser.h>
# include <string.h>
# include <ctype.h>
# include <stdlib.h>

bool CommandLineParser_TokenCompare(char ch)
{
	return iswhitespace(ch) ;
}

bool CommandLineParser_GroupToken(char ch)
{
	return (ch == '"') ;
}

const upan::string Expand(const upan::string& param)
{
  if(param.length() > 1 && param[0] == '$' && !iswhitespace(param[1]))
  {
    int j;
    for(j = 0; param[j] != '\0' && !iswhitespace(param[j]) && param[j] != '/'; ++j) ;

    upan::string var(param.c_str() + 1, j - 1);
    const char* val = getenv(var.c_str());
    upan::string temp(param.c_str() + j);
    return upan::string(val) + temp;
  }
  else
    return param;
}

void CommandLineParser_Copy(int index, const char* src, int len, void* np)
{
  upan::string token(src, len);
  if (index == 0)
    CommandLineParser::Instance()._command = token;
  else if (len > 0)
  {
    //TODO: options are not handled properly yet
    if (src[0] == '-' && (len == 1 || !isdigit(src[1])))
      CommandLineParser::Instance()._options.insert(token);
    else
      CommandLineParser::Instance()._params.push_back(Expand(token));
  }
}

void CommandLineParser::Parse(const upan::string& commandLine) {
  _command = "";
  _options.clear();
  _params.clear();
  int d = 0;

  strtok_c(commandLine.c_str(),
           &CommandLineParser_TokenCompare,
           &CommandLineParser_GroupToken,
           &CommandLineParser_Copy,
           &d, nullptr);
}

const char* CommandLineParser::GetCommand() const
{
  if(_command.length() == 0)
    return nullptr;

  return _command.c_str();
}

const char* CommandLineParser::GetParameterAt(const int pos) const
{
  if(pos < 0 || pos >= GetNoOfParameters())
    return nullptr;
  return _params[pos].c_str();
}

bool CommandLineParser::IsOptPresent(const upan::string& opt) const
{
  return _options.find(opt) != _options.end();
}