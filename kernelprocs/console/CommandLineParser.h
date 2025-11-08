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
#ifndef _COMMAND_LINE_PARSER_H_
#define _COMMAND_LINE_PARSER_H_

#include <vector.h>
#include <set.h>
#include <ustring.h>

class CommandLineParser
{
private:
  CommandLineParser() {}
public:
  static CommandLineParser& Instance()
  {
    static CommandLineParser _instance;
    return _instance;
  }

  void Parse(const upan::string& commandLine);

  const char* GetCommand() const;
  int GetNoOfParameters() const { return _params.size(); }
  int GetNoOfOptions() const { return _options.size(); }
  const char* GetParameterAt(const int iPos) const;
  bool IsOptPresent(const upan::string& opt) const;

public:
  upan::string _command;
  upan::vector<upan::string> _params;
  upan::set<upan::string> _options;
};

#endif

