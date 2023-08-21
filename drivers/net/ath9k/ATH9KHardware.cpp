#include <PCIBusHandler.h>
#include <MemManager.h>
#include <ATH9KHardware.h>

ATH9KHardware::ATH9KHardware(const PCIEntry& pciEntry) : _pciEntry(pciEntry)
{
  unsigned uiIOAddr = pciEntry.BusEntity.NonBridge.uiBaseAddress0;
  printf("\n PCI BaseAddr: %x", uiIOAddr);

  uiIOAddr = uiIOAddr & PCI_ADDRESS_MEMORY_32_MASK;
  unsigned ioSize = pciEntry.GetPCIMemSize(0);
  printf(", Raw MMIO BaseAddr: %x, IOSize: %d", uiIOAddr, ioSize);

  const unsigned availableMemMapSize = NET_ATH9K_MMIO_BASE_ADDR_END - NET_ATH9K_MMIO_BASE_ADDR;
  if(ioSize > availableMemMapSize)
    throw upan::exception(XLOC, "ATH9KDevice IO Size is %x > greater than available size %x !", ioSize, availableMemMapSize);

  unsigned pagesToMap = ioSize / PAGE_SIZE;
  if(ioSize % PAGE_SIZE)
    ++pagesToMap;
  unsigned uiPDEAddress = MEM_PML4 ;
  uint32_t memMapAddress = NET_ATH9K_MMIO_BASE_ADDR;
  printf("\n Total pages to Map: %d", pagesToMap);
  ReturnCode markPageRetCode = Success;
  for(unsigned i = 0; i < pagesToMap; ++i)
  {
    unsigned uiPDEIndex = ((memMapAddress >> 22) & 0x3FF) ;
    unsigned uiPTEIndex = ((memMapAddress >> 12) & 0x3FF) ;
    unsigned uiPTEAddress = (((unsigned*)(KERNEL_VIRTUAL_ADDRESS(uiPDEAddress)))[uiPDEIndex]) & 0xFFFFF000 ;
    // This page is a Read Only area for user process. 0x5 => 101 => User Domain, Read Only, Present Bit
    ((unsigned*)(KERNEL_VIRTUAL_ADDRESS(uiPTEAddress)))[uiPTEIndex] = (uiIOAddr & 0xFFFFF000) | 0x5 ;
    markPageRetCode = MemManager::Instance().MarkPageAsAllocated(uiIOAddr / PAGE_SIZE, markPageRetCode);
    if(markPageRetCode != Success) {
    }

    memMapAddress += PAGE_SIZE;
    uiIOAddr += PAGE_SIZE;
  }

  Mem_FlushTLB();

  _regBase = KERNEL_VIRTUAL_ADDRESS(NET_ATH9K_MMIO_BASE_ADDR + (uiIOAddr % PAGE_SIZE));

  ReadCacheLineSize();
  ReadRevisions();
}

void ATH9KHardware::ReadCacheLineSize()
{
  //Read cache line size
  uint8_t tmp;
	_pciEntry.ReadPCIConfig(PCI_CACHE_LINE_SIZE, 1, &tmp);
	_cacheLineSize = (int)tmp;

  /*
   * This check was put in to avoid "unpleasant" consequences if
   * the bootrom has not fully initialized all PCI devices.
   * Sometimes the cache line size register is not set
   */

  if(_cacheLineSize == 0)
	{
		static const int DEFAULT_CACHELINE = 32;
		_cacheLineSize = DEFAULT_CACHELINE >> 2;   /* Use the default size */
	}
}

void ATH9KHardware::ReadRevisions()
{
  /*
  val = Read(REG_OFFSET::AR_SREV) & AR_SREV_ID;

  if (val == 0xFF) {
    val = Read(REG_OFFSET::SREV);
    ah->hw_version.macVersion =
      (val & AR_SREV_VERSION2) >> AR_SREV_TYPE2_S;
    ah->hw_version.macRev = MS(val, AR_SREV_REVISION2);

    if (AR_SREV_9462(ah) || AR_SREV_9565(ah))
      ah->is_pciexpress = true;
    else
      ah->is_pciexpress = (val &
               AR_SREV_TYPE2_HOST_MODE) ? 0 : 1;
  } else {
    if (!AR_SREV_9100(ah))
      ah->hw_version.macVersion = MS(val, AR_SREV_VERSION);

    ah->hw_version.macRev = val & AR_SREV_REVISION;

    if (ah->hw_version.macVersion == AR_SREV_VERSION_5416_PCIE)
      ah->is_pciexpress = true;
  }
  */
}

uint32_t ATH9KHardware::Read(const REG_OFFSET regOffset)
{
  return *((uint32_t*)(_regBase + regOffset));
}

void ATH9KHardware::MultiRead(REG_OFFSET* addr, uint32_t* val, uint16_t count)
{
	for (int i = 0; i < count; ++i)
		val[i] = Read(addr[i]);
}

void ATH9KHardware::Write(uint32_t value, const REG_OFFSET regOffset)
{
  *(uint32_t*)(_regBase + regOffset) = value;
}

uint32_t ATH9KHardware::ReadMofidyWrite(const REG_OFFSET regOffset, const uint32_t set, const uint32_t clr)
{
  uint32_t val;

  val = Read(regOffset);
  val &= ~clr;
  val |= set;
  Write(val, regOffset);

  return val;
}
