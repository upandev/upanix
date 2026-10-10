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

#include <stdint.h>
#include <Global.h>
#include <Apic.h>

class Processor {
  public:
    explicit Processor(int seqId, uint32_t id);
    virtual void init() = 0;
    virtual void main() = 0;

    typedef struct {
      uint16_t _limit;
      uint64_t _base;
    } PACKED DTRegister;

  protected:
    typedef struct {
      uint32_t _reserved1;

      uint64_t _rsp0;
      uint64_t _rsp1;
      uint64_t _rsp2;

      uint64_t _reserved2;

      uint64_t _ist1;
      uint64_t _ist2;
      uint64_t _ist3;
      uint64_t _ist4;
      uint64_t _ist5;
      uint64_t _ist6;
      uint64_t _ist7;

      uint64_t _reserved3;
      uint16_t _reserved4;
      uint16_t _ioMapBase;
    } PACKED TaskState64;

    void LIDT();
    void LTR();

  protected:
    const int _seqId;
    const uint32_t _id;
    TaskState64* _tss;
};

class BootstrapProcessor : public Processor {
  public:
    explicit BootstrapProcessor(int seqId, uint32_t id);
    void init() override {}
    void main() override {}
};

class ApplicationProcessor : public Processor {
  public:
    explicit ApplicationProcessor(int seqId, uint32_t id, Apic& _apic);
    void init() override;
    void main() override;

  private:
    Apic& _apic;
    uintptr_t _gdtBase;
};