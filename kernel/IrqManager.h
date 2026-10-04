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

#include <Global.h>
#include <list.h>
#include <mutex.h>

class IrqManager;

class IRQ
{
	public:
		int GetIRQNo() const { return m_iIRQNo; }
		bool operator==(const IRQ& r) const { return m_iIRQNo == r.GetIRQNo() ; }
    void Signal() const;
    bool Consume() const;

	private:
		explicit IRQ(int iIRQNo) : m_iIRQNo(iIRQNo), _interruptCount(0) { }

	private:
    static const int MAX_PROC_ON_INT_QUEUE = 8 * 1024;
		const int m_iIRQNo;
    mutable upan::atomic::integral<uint32_t> _interruptCount;
    mutable upan::mutex _consumeMutex;

	friend class IrqManager;
  friend class StdIRQ;
};

class StdIRQ
{
  private:
    StdIRQ();
    StdIRQ(const StdIRQ&);

  public:
    static StdIRQ& Instance();

    //Some standard IRQs
    const IRQ NO_IRQ;
    const IRQ TIMER_IRQ;
    const IRQ KEYBOARD_IRQ;
    const IRQ FLOPPY_IRQ;
    const IRQ RTC_IRQ;
    const IRQ MOUSE_IRQ;
};

class IrqManager
{
  private:
    IrqManager(const IrqManager&);
    static IrqManager* _instance;

  protected:
    IrqManager();

  public:
    static const int IRQ_BASE = 32;
		static const int MAX_INTERRUPT = 24;

    static IrqManager& Instance();
    static void Initialize();

    bool IsApic() const { return _isApic; }

		const IRQ* RegisterIRQ(const int& iIRQNo, unsigned pHandler);
		bool RegisterIRQ(const IRQ& irq, uintptr_t pHandler);
    bool UnregisterIRQ(const IRQ& irq);
		const IRQ* GetIRQ(const IRQ& irq);
		const IRQ* GetIRQ(const int& iIRQNo);
		void DisplayIRQList();
    static bool IsInterruptEnabled();

		virtual void SendEOI(const IRQ&) = 0;
		virtual void EnableIRQ(const IRQ&) = 0;
		virtual void DisableIRQ(const IRQ&) = 0;
    virtual bool IsIRQEnabled(const IRQ&) = 0;

  private:
		upan::list<const IRQ*> _irqs;
    bool _isApic;
};

class IrqGuard
{
  public:
    IrqGuard(const IRQ& irq) : _irq(&irq)
    {
      IrqManager::Instance().DisableIRQ(*_irq);
    }
    IrqGuard() : _irq(nullptr) {
      __asm__ __volatile__("pushfq;"
                           "popq %0;" : "=m"(_allIntSyncFlag) : : "memory");
      if(_allIntSyncFlag & 0x200) {
        __asm__ __volatile__("cli");
      }
    }
    ~IrqGuard() {
      if(_irq) {
        IrqManager::Instance().EnableIRQ(*_irq);
      } else {
        if(_allIntSyncFlag & 0x200) {
          __asm__ __volatile__("sti");
        }
      }
    }
  private:
    const IRQ* _irq;
    __volatile__ uint64_t _allIntSyncFlag;
};