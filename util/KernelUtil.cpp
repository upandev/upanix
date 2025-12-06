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
#include <IrqManager.h>
#include <ProcessManager.h>
#include <KernelUtil.h>
#include <NetworkManager.h>
#include <NetworkDevice.h>
#include <RealNetworkDevice.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <openssl/sha.h>
#include <TerminalDescriptor.h>
#include <TerminalMasterDescriptor.h>
#include <RedirectDescriptor.h>

void KernelUtil::Wait(__volatile__ unsigned uiTimeInMilliSec)
{
	uiTimeInMilliSec = PIT::Instance().RoundSleepTime(uiTimeInMilliSec) ;
	__volatile__ unsigned uiStartTime = PIT::Instance().GetClockCount() ;

  const bool isIntEnabled = IrqManager::IsInterruptEnabled();
  int count = 100000;
  while((PIT::Instance().GetClockCount() - uiStartTime) < uiTimeInMilliSec)	{
    if (!isIntEnabled) {
      if (--count <= 0) {
        break;
      }
    }
		__asm__ __volatile__("nop") ;
		__asm__ __volatile__("nop") ;
	}
}

void KernelUtil::WaitOnInterrupt(const IRQ& irq)
{
	while(!irq.Consume())
	{	
		__asm__ __volatile__("nop") ;
		__asm__ __volatile__("nop") ;
	}
}

void KernelUtil::TightLoopWait(unsigned loop)
{
	unsigned i ;
	for(i = 0; i < loop * loop * loop; i++)
	{
		__asm__ __volatile__("nop") ;
		__asm__ __volatile__("nop") ;
		__asm__ __volatile__("nop") ;
		__asm__ __volatile__("nop") ;
		__asm__ __volatile__("nop") ;
		__asm__ __volatile__("nop") ;
	}
}

void KernelUtil::ScheduleTimedTask(const char* szName, unsigned uiTimeInMilliSec, TimerTask& task, bool isCoreProcess) {
  upan::vector<uintptr_t> params;
  params.push_back(uiTimeInMilliSec);
  params.push_back((uintptr_t)&task);
  ProcessManager::Instance().CreateKernelProcess(szName, (uintptr_t) &SystemTimer,
                                                 ProcessManager::GetCurrentProcessID(), false, isCoreProcess, params);
}

void KernelUtil::SystemTimer(unsigned timeInMilliSec, TimerTask* task)
{
	do
	{
		ProcessManager::Instance().Sleep(timeInMilliSec) ;
  } while(task->TimerTrigger()) ;
  ProcessManager_Exit(0);
}

void KernelUtil::IOCtl(int fd, uint64_t cmd, uint64_t arg) {
  switch (cmd) {
    case SIOCGIFADDR:
    {
      auto req = (struct ifreq*)arg;
      NetworkDevice& defaultDevice = NetworkManager::Instance().getDefaultRealDevice().value();
      auto& device = NetworkManager::Instance().getDeviceByName(req->ifr_name).valueOrElse(defaultDevice);
      reinterpret_cast<struct sockaddr_in&>(req->ifr_addr).sin_addr.s_addr = device.getIPAddress();
    }
    break;

    case SIOCGIFHWADDR:
    {
      auto req = (struct ifreq*)arg;
      auto& defaultDevice = NetworkManager::Instance().getDefaultRealDevice().value();
      auto& device = NetworkManager::Instance().getDeviceByName(req->ifr_name).valueOrElse(defaultDevice);
      memcpy(req->ifr_hwaddr.sa_data, device.getMACAddress().get(), ETH_ALEN);
    }
    break;

    case SIOCGIFINDEX:
    {
      auto req = (struct ifreq*)arg;
      auto& defaultDevice = NetworkManager::Instance().getDefaultRealDevice().value();
      auto& device = NetworkManager::Instance().getDeviceByName(req->ifr_name).valueOrElse(defaultDevice);
      req->ifr_ifindex = device.id();
    }
    break;

    case TIOCSCTTY:
    {
      auto& process = ProcessManager::Instance().GetCurrentPAS();
      auto terminalDevice = process.iodTable().getRealNonDupped(fd).cast<TerminalDescriptor>()->terminalDevice();
      auto masterDesc = process.iodTable().allocate([&](int fd) {
        return new TerminalMasterDescriptor(process.processID(), fd, terminalDevice);
      });

      process.setControllingTerminal(terminalDevice);
      process.iodTable().get(IODescriptorTable::TERMINAL_MASTER).cast<RedirectDescriptor>()->changeRedirection(masterDesc);
    }
    break;
  }

  throw upan::exception(XLOC, "unsupported IOCTL cmd: %ul", cmd);
}

static inline uint64_t rdtsc() {
  uint32_t lo, hi;
  __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
  return ((uint64_t)hi << 32) | lo;
}

// Collect `count` samples into out[32], hashed
void entropy_jitter_collect(uint8_t out[32], size_t count) {
  uint64_t last = rdtsc();
  uint8_t pool[512];
  size_t pool_pos = 0;

  for (size_t i = 0; i < count; i++) {
    uint64_t t1 = rdtsc();
    uint64_t delta = t1 - last;
    last = t1;

    // Mix delta into pool
    memcpy(pool + pool_pos, &delta, sizeof(delta));
    pool_pos += sizeof(delta);

    // When pool is full, hash it
    if (pool_pos == sizeof(pool)) {
      SHA256(pool, sizeof(pool), out);
      pool_pos = 0;
      memcpy(pool, out, 32); // feedback
      pool_pos = 32;
    }
  }

  // Final hash
  SHA256(pool, pool_pos, out);
}

void KernelUtil::GetEntropy(void* buffer, size_t length) {
  uint8_t hash[32];
  size_t pos = 0;
  while (pos < length) {
    entropy_jitter_collect(hash, 1024); // 1024 samples
    size_t c = (length - pos < 32) ? (length - pos) : 32;
    memcpy((uint8_t*)buffer + pos, hash, c);
    pos += c;
  }
}