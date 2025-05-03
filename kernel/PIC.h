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
#ifndef _PIC_H_
#define _PIC_H_

#include <Global.h>
#include <IrqManager.h>

class PIC : public IrqManager
{
	private:
		PIC();

	private:
    void DisableForAPIC();
		void SendEOI(const IRQ&) override;
		void EnableIRQ(const IRQ&) override;
		void DisableIRQ(const IRQ&) override;
    bool IsIRQEnabled(const IRQ&) override;

		static const int SLAVE_IRQNO_START = 8;

		static const unsigned short MASTER_PORTA = 0x20;
		static const unsigned short MASTER_PORTB = 0x21;
		static const unsigned short MASTER_PORTC = 0x22;
		static const unsigned short MASTER_PORTD = 0x23;
		static const unsigned short SLAVE_PORTA = 0xA0;
		static const unsigned short SLAVE_PORTB = 0xA1;

		static const unsigned short MASTER_IRQ_BASE = 0x20;
		static const unsigned short SLAVE_IRQ_BASE = 0x28;

		unsigned short m_IRQMask;

    friend class IrqManager;
};

#endif
