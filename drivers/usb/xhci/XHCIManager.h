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
#ifndef _XHCI_MANAGER_H_
#define _XHCI_MANAGER_H_

# include <Global.h>
# include <list.h>

class XHCIController;

class XHCIManager
{
  private:
    XHCIManager();
  public:
    enum EventMode { Poll, Interrupt };

    static XHCIManager& Instance()
    {
      static XHCIManager instance;
      return instance;
    }
    static void Handler();
    void Initialize();
    void SetEventMode(EventMode e) { _eventMode = e; }
    EventMode GetEventMode() const { return _eventMode; }
    bool Initialized() const { return _initialized; }
    void ProbeDevice();
    const upan::list<XHCIController*>& Controllers() { return _controllers; }
    const IRQ* irq() { return XHCI_IRQ; }

  private:
    bool _initialized;
    EventMode _eventMode;
    const IRQ* XHCI_IRQ;
    upan::list<XHCIController*> _controllers;    
};

#endif
