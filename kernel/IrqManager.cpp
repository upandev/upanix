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

#include <stdio.h>
#include <IrqManager.h>
#include <PIC.h>
#include <Apic.h>
#include <IDT.h>

IrqManager* IrqManager::_instance = nullptr;

void IRQ::Signal() const {
  _interruptCount.inc();
}

bool IRQ::Consume() const
{
  upan::mutex_guard mg(_consumeMutex);
  if(_interruptCount.get() > 0) {
    _interruptCount.dec();
    return true;
  }
  return false;
}

StdIRQ::StdIRQ() : NO_IRQ(999),
	TIMER_IRQ(0),
	KEYBOARD_IRQ(1),
	FLOPPY_IRQ(6),
	RTC_IRQ(8),
	MOUSE_IRQ(12)
{
}

StdIRQ& StdIRQ::Instance()
{
  static StdIRQ instance;
  return instance;
}

void IrqManager::Initialize()
{
  static PIC pic;
  if(Apic::IsAvailable())
  {
    pic.DisableForAPIC();
    static Apic apic;
    _instance = &apic;
    _instance->_isApic = true;
    apic.Initialize();
  }
  else
  {
    _instance = &pic;
    _instance->_isApic = false;
  }
}

IrqManager& IrqManager::Instance()
{
  if(!_instance)
    throw upan::exception(XLOC, "IRQ Manager is not initialized yet!");
  return *_instance;
}

IrqManager::IrqManager() : _isApic(false)
{
}

const IRQ* IrqManager::RegisterIRQ(const int& iIRQNo, unsigned pHandler) {
	if(iIRQNo < 0 && iIRQNo >= MAX_INTERRUPT)
		return nullptr;

	IRQ* pIRQ = new IRQ(iIRQNo);
	if(!RegisterIRQ(*pIRQ, pHandler))	{
		delete pIRQ;
		pIRQ = nullptr;
	}
	return pIRQ;
}

bool IrqManager::RegisterIRQ(const IRQ& irq, uintptr_t pHandler)
{
	if(GetIRQ(irq))
	{
		printf("\nIRQ '%d' is already registered!", irq.GetIRQNo());
		return false;
	}

	_irqs.push_back(&irq);

	IDT::Instance().LoadEntry(IRQ_BASE + irq.GetIRQNo(), pHandler, SYS_CODE_SELECTOR, 0x8E) ;

	return true;
}

bool IrqManager::UnregisterIRQ(const IRQ& irq)
{
  auto it = upan::find_if(_irqs.begin(), _irqs.end(), [&irq](const IRQ* i) { return i->GetIRQNo() == irq.GetIRQNo(); });
  if (it == _irqs.end()) {
    return false;
  }
  DisableIRQ(irq);
  _irqs.erase(it);
	return true;
}

const IRQ* IrqManager::GetIRQ(const IRQ& irq)
{
	return GetIRQ(irq.GetIRQNo());
}

const IRQ* IrqManager::GetIRQ(const int& iIRQNo)
{
	for(auto i : _irqs)
    if(i->GetIRQNo() == iIRQNo)
      return i;
  return NULL;
}

void IrqManager::DisplayIRQList()
{
	for(auto i : _irqs)
		printf("\n IRQ = %d", i->GetIRQNo()) ;
}

bool IrqManager::IsInterruptEnabled() {
  uint64_t flag;
  __asm__ __volatile__("pushfq;"
                       "popq %0;" : "=m"(flag) : : "memory");
  return flag & 0x200;
}