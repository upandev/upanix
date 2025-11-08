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

# include <Global.h>
# include <ustring.h>
# include <ithread.h>

void Console_StartUpanixConsole();

class Console {
private:
  Console();
public:
  Console(const Console&) = delete;
  Console& operator=(const Console&) = delete;

public:
  static Console& Instance() {
    static Console _instance;
    return _instance;
  }
  void Start();

  class ConsoleOutHandler : public upan::thread {
  public:
    explicit ConsoleOutHandler(Console& console);
  private:
    void run() override;
    void ProcessInput(const uint8_t* input, int len);
  private:
    Console& _console;
  };
private:
  void OnKeyboardInput(const uint8_t* buffer, int len);
  void DisplayCommandLine();
  void ExecuteCommand();

  const upan::string _prompt;
  upan::string _commandLine;
  ConsoleOutHandler _ioHandler;
};
