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

#include <PIC.h>
#include <PortCom.h>

PIC::PIC()
{
	// Sending ICW1 to master and slave PIC 
	PortCom_SendByte(MASTER_PORTA, 0x11) ;
	PortCom_SendByte(SLAVE_PORTA, 0x11) ;

	// Sending base vector number of master and slave PIC 
	PortCom_SendByte(MASTER_PORTB, MASTER_IRQ_BASE) ; // base vector number of master PIC
	PortCom_SendByte(SLAVE_PORTB, SLAVE_IRQ_BASE) ; // base vector number of slave PIC

	// Sending connection parameters between the PICs to the PICs 
	PortCom_SendByte(MASTER_PORTB, 0x04) ; // IRQ2 is connected to slave PIC
	PortCom_SendByte(SLAVE_PORTB, 0x02) ; // IRQ2 of master PIC is used for the slave PIC

	// Intel environment, manual EOI
	PortCom_SendByte(MASTER_PORTB, 0x01) ;
	PortCom_SendByte(SLAVE_PORTB, 0x01) ;

	// Disable All Interrupts
	PortCom_SendByte(MASTER_PORTB, 0xff) ;
	PortCom_SendByte(SLAVE_PORTB, 0xff) ;

	m_IRQMask = 0xFFFF ;
}

void PIC::DisableForAPIC()
{
	PortCom_SendByte(SLAVE_PORTB, 0xFF); 
	PortCom_SendByte(MASTER_PORTB, 0xFF);
	PortCom_SendByte(MASTER_PORTC, 0x70);
	PortCom_SendByte(MASTER_PORTD, 0x01);
}

void PIC::SendEOI(const IRQ& irq)
{
  PortCom_SendByte(MASTER_PORTA, 0x20) ;
  if(irq.GetIRQNo() >= SLAVE_IRQNO_START)
    PortCom_SendByte(SLAVE_PORTA, 0x20) ;
}

void PIC::EnableIRQ(const IRQ& irq)
{
  IrqGuard g;

	int iIRQNo = irq.GetIRQNo();
	m_IRQMask &= ~(1 << iIRQNo) ;
	if(iIRQNo >= SLAVE_IRQNO_START)
		m_IRQMask &= ~(1 << 2) ;
	
	PortCom_SendByte(MASTER_PORTB, (m_IRQMask & 0xFF)) ;
	PortCom_SendByte(SLAVE_PORTB, ((m_IRQMask >> 8) & 0xFF)) ;
}

void PIC::DisableIRQ(const IRQ& irq)
{
  IrqGuard g;

	int iIRQNo = irq.GetIRQNo();
	m_IRQMask |= (1 << iIRQNo) ;
	if((m_IRQMask & 0xFF00) == 0xFF00) // All Slave Ints disabled
		m_IRQMask |= (1 << 2) ;

	PortCom_SendByte(MASTER_PORTB, (m_IRQMask & 0xFF)) ;
	PortCom_SendByte(SLAVE_PORTB, ((m_IRQMask >> 8) & 0xFF)) ;
}

bool PIC::IsIRQEnabled(const IRQ& irq) {
  IrqGuard g;
  int iIRQNo = irq.GetIRQNo();
  return (m_IRQMask & (1 << iIRQNo)) == 0;
}
