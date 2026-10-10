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
  const uint32_t bspApicId = _apic->GetLocalApicID();

  const auto& cpus = Acpi::Instance().GetMadt().GetLocalApics();
  printf("\nCPU discovery: %u enabled logical CPUs", cpus.size());

  bool bspFound = false;
  int seqId = 0;
  for (const auto& cpu : cpus) {
    const bool isBsp = cpu.Id() == bspApicId;
    bspFound |= isBsp;

    auto processor = isBsp ? (Processor*)(new BootstrapProcessor(seqId, cpu.Id())) : new ApplicationProcessor(seqId, cpu.Id(), *_apic);
    seqId++;
    _processors.insert(ProcessorMap::value_type(cpu.Id(), processor));

    printf("\nAPIC ID %u: %s", cpu.Id(), isBsp ? "bootstrap CPU, executing" : "discovered, not started by Upanix");
  }

  if (!bspFound) {
    printf("\nCPU discovery: executing APIC ID %u missing from MADT", bspApicId);
  }
}

void ProcessorManager::initAPs() {
  const auto& cpus = Acpi::Instance().GetMadt().GetLocalApics();
  const uint32_t bspApicId = _apic->GetLocalApicID();

  for (const auto& cpu : cpus) {
    if (cpu.Id() == bspApicId) {
      continue;
    }
    getProcessor(cpu.Id()).init();
  }
}

extern "C"
void _ap_main() {
  ProcessorManager::instance().apMain();
}

void ProcessorManager::apMain() {
  getCurrentProcessor().main();
}

Processor& ProcessorManager::getProcessor(uint32_t id) const {
  upan::mutex_guard g(_processorMutex);
  auto it = _processors.find(id);
  if (it == _processors.end()) {
    throw upan::exception(XLOC, "Processor %d not found", _apic->GetLocalApicID());
  }
  return *it->second;
}

Processor& ProcessorManager::getCurrentProcessor() const {
  return getProcessor(_apic->GetLocalApicID());
}