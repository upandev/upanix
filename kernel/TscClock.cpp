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

#include <TscClock.h>
#include <Cpu.h>
#include <exception.h>
#include <PIT.h>
#include <SystemUtil.h>
#include <PortCom.h>
#include <Bit.h>

TscClock* TscClock::_instance = nullptr;

void TscClock::initialize() {
  if (_instance != nullptr) {
    throw upan::exception(XLOC, "TscClock already initialized");
  }
  _instance = new TscClock();
}

TscClock& TscClock::instance() {
  if (_instance == nullptr) {
    throw upan::exception(XLOC, "TscClock not initialized");
  }
  return *_instance;
}

TscClock::TscClock() {
  if (!Cpu::Instance().HasSupport(CF_RDTSC)) {
    throw upan::exception(XLOC, "CPU does not support RDTSC");
  }

  // we use PIT (counter 2) for determining correct TSC calibration
  const uint16_t partOfSecond = 20; // 50 ms
  const uint32_t divisor = TIMECOUNTER_i8254_FREQU / partOfSecond;

  // Disable speaker and hold channel-2 GATE low.
  const uint8_t originalCtrlPortValue = PortCom_ReceiveByte(PIT_COUNTER_2_CTRLPORT);
  PortCom_SendByte(PIT_COUNTER_2_CTRLPORT, originalCtrlPortValue & 0xFC);
  PortCom_SendByte(PIT_COUNTER_2_CTRLPORT, (PortCom_ReceiveByte(PIT_COUNTER_2_CTRLPORT) & 0xFD) | 1);

  PortCom_SendByte(PIT_MODE_PORT, PIT_COUNTER_2 | PIT_RW_LO_HI_MODE | PIT_MODE0 | SIXTEEN_BIT_BINARY); // Mode 0 is important!
  PortCom_SendByte(PIT_COUNTER_2_PORT, Bit::Byte1(divisor)); //LSB
  PortCom_SendByte(PIT_COUNTER_2_PORT, Bit::Byte2(divisor)); //MSB

  const uint64_t startTsc = rdtsc();
  // start PIT counting and APIC timer
  PortCom_SendByte(PIT_COUNTER_2_CTRLPORT, (originalCtrlPortValue & 0xFC) | 1u);

  // PIT timer at zero?
  while (!(PortCom_ReceiveByte(PIT_COUNTER_2_CTRLPORT) & (1U << 5))) {
    __asm__ __volatile__("pause");
  }

  const uint64_t endTsc = rdtsc();
  PortCom_SendByte(PIT_COUNTER_2_CTRLPORT, originalCtrlPortValue);

  const uint64_t elapsedCycles = endTsc - startTsc;
  _bootTime = SystemUtil_GetTimeOfDay() * 1000000;
  _bootTsc = endTsc;

  _frequencyHz = (elapsedCycles * static_cast<uint64_t>(TIMECOUNTER_i8254_FREQU) / divisor);


  printf("\nTSC frequency: %lu Hz", _frequencyHz);
}

time_t TscClock::currentTime() const {
  return _bootTime + duration(_bootTsc);
}

uint64_t TscClock::rdtsc() const {
  uint32_t lo, hi;
  __asm__ __volatile__("lfence\n\t"
                       "rdtsc" : "=a"(lo), "=d"(hi) : : "memory");
  return ((uint64_t)hi << 32) | lo;
}

uint64_t TscClock::rdtsc(uint64_t duration) const {
  return rdtsc() + durationToCycles(duration);
}

time_t TscClock::duration(uint64_t start) const {
  return duration(start, rdtsc());
}

time_t TscClock::duration(uint64_t start, uint64_t end) const {
  if (end < start) {
    return 0;
  }
  return static_cast<time_t>((end - start) * 1000000 / _frequencyHz);
}

uint64_t TscClock::durationToCycles(uint64_t duration) const {
  return static_cast<uint64_t>(duration * _frequencyHz / 1000000);
}