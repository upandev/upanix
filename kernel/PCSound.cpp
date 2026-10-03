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

#include <PIT.h>
#include <PortCom.h>
#include <ProcessManager.h>
#include <PCSound.h>

PCSound::PCSound()
{
}

//Play sound using built in speaker
void PCSound::Play(unsigned freq)
{
  //Set the PIT to the desired frequency
  unsigned div = TIMECOUNTER_i8254_FREQU / freq;
  PortCom_SendByte(PIT_MODE_PORT, 0xb6);
  PortCom_SendByte(PIT_COUNTER_2_PORT, (byte)div);
  PortCom_SendByte(PIT_COUNTER_2_PORT, (byte)(div >> 8));
 
  //And play the sound using the PC speaker
  byte tmp = PortCom_ReceiveByte(0x61);
  if (tmp != (tmp | 3)) 
    PortCom_SendByte(0x61, tmp | 3);
}
 
//make it shutup
void PCSound::Stop()
{
  PortCom_SendByte(0x61, PortCom_ReceiveByte(0x61) & 0xFC);
}

void PCSound::Beep()
{
  Play(1000);
  ProcessManager::Instance().Sleep(1000000);
  Stop();
}
