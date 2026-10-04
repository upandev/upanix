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
}