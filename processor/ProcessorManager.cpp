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

#include <ProcessorManager.h>
#include <Acpi.h>
#include <Cpu.h>
#include <MemManager.h>
#include <TscClock.h>

extern "C" {
  extern const uint8_t AP_TRAMPOLINE_START[];

  extern uint32_t AP_PAGE_TABLE;
  extern uint32_t AP_EFER_BITS;
  extern uint64_t AP_STACK_TOP;

  static uint32_t AP_REPORTED_ID;
  static uint32_t AP_READY;
}

ProcessorManager* ProcessorManager::_instance = nullptr;

ProcessorManager& ProcessorManager::instance() {
  if (!_instance) {
    _instance = new ProcessorManager();
  }
  return *_instance;
}

ProcessorManager::ProcessorManager() : _apic(nullptr) {
  if (!Apic::IsAvailable()) {
    throw upan::exception(XLOC, "APIC not available");
  }

  _apic = new Apic();
  const auto& cpus = Acpi::Instance().GetMadt().GetLocalApics();
  const uint32_t bspApicId = _apic->GetLocalApicID();

  printf("\nCPU discovery: %u enabled logical CPUs", cpus.size());

  bool bspFound = false;
  for (const auto& cpu : cpus) {
    const bool isBsp = cpu.Id() == bspApicId;
    bspFound |= isBsp;
    printf("\nAPIC ID %u: %s", cpu.Id(), isBsp ? "bootstrap CPU, executing" : "discovered, not started by Upanix");
  }

  if (!bspFound) {
    printf("\nCPU discovery: executing APIC ID %u missing from MADT", bspApicId);
  }

  _processors.insert(ProcessorMap::value_type(bspApicId, new Processor(bspApicId)));
}

void ProcessorManager::initAPs() {
  AP_PAGE_TABLE = (uint32_t)(reinterpret_cast<uintptr_t>(MEM_PML4_TABLE));
  AP_EFER_BITS = (uint32_t)(Cpu::Instance().MSRread(IA32_EFER) & (1ULL << 11));

  const auto startupVector = uint8_t(uintptr_t(AP_TRAMPOLINE_START) >> 12);

  const auto& cpus = Acpi::Instance().GetMadt().GetLocalApics();
  const uint32_t bspApicId = _apic->GetLocalApicID();

  for (const auto& cpu : cpus) {
    if (cpu.Id() == bspApicId) {
      continue;
    }
    printf("\nInitializing AP with APIC ID %u", cpu.Id());

    const uintptr_t apStackBase = MemManager::Instance().AllocatePhysicalPage() * PAGE_SIZE;
    AP_STACK_TOP = apStackBase + PAGE_SIZE;

    __atomic_store_n(&AP_REPORTED_ID, 0u, __ATOMIC_RELAXED);
    __atomic_store_n(&AP_READY, 0u, __ATOMIC_RELAXED);

    _apic->initAP(cpu.Id());
    TscClock::instance().busyWait(10000);

    bool apReady = false;
    for (int i = 0; i < 200 && !apReady; ++i) {
      _apic->sipiAP(cpu.Id(), startupVector);

      TscClock::instance().busyWait(500);

      apReady = __atomic_load_n(&AP_READY, __ATOMIC_ACQUIRE) == 1;
    }

    if (!apReady) {
      printf("\n AP with APIC ID %u failed to initialize", cpu.Id());
    } else {
      const auto apId = __atomic_load_n(&AP_REPORTED_ID, __ATOMIC_ACQUIRE);
      printf("\n AP with APIC ID %u initialized, Reported ID: %u", cpu.Id(), apId);
    }
  }
}

extern "C"
void _ap_main() {
  ProcessorManager::instance().apMain();
}

void ProcessorManager::apMain() {
  const auto apicId = _apic->GetLocalApicID();

  //no need for mutex here because APs are initialized sequentially
  _processors.insert(ProcessorMap::value_type(apicId, new Processor(apicId)));

  __atomic_store_n(&AP_REPORTED_ID, apicId, __ATOMIC_RELAXED);
  __atomic_store_n(&AP_READY, 1u, __ATOMIC_RELEASE);
}

Processor& ProcessorManager::getProcessor() const {
  upan::mutex_guard g(_processorMutex);
  auto it = _processors.find(_apic->GetLocalApicID());
  if (it == _processors.end()) {
    throw upan::exception(XLOC, "Processor %d not found", _apic->GetLocalApicID());
  }
  return *it->second;
}