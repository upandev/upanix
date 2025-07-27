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
#include <PCIBusHandler.h>
#include <MemManager.h>
#include <ATH9KHardware.h>
#include <ATH9KDevice.h>

struct SupportedDevices
{
  uint16_t vendorId;
  uint16_t deviceId;
};

static SupportedDevices SINGLE_ANTENNA_DEVICES[] = {
  { PCI_VENDOR_ID_FOXCONN, 0xE068 },
  { PCI_VENDOR_ID_WNC, 0xA119 },
  { PCI_VENDOR_ID_LITEON, 0x0632 },
  { PCI_VENDOR_ID_LITEON, 0x06B2 },
  { PCI_VENDOR_ID_LITEON, 0x0842 },
  { PCI_VENDOR_ID_LITEON, 0x1842 },
  { PCI_VENDOR_ID_LITEON, 0x6671 },
  { PCI_VENDOR_ID_XAVI, 0x2811 },
  { PCI_VENDOR_ID_XAVI, 0x2812 },
  { PCI_VENDOR_ID_XAVI, 0x28A1 },
  { PCI_VENDOR_ID_XAVI, 0x28A3 },
  { PCI_VENDOR_ID_AZWAVE, 0x218A },
  { PCI_VENDOR_ID_AZWAVE, 0x2F8A }
};

static SupportedDevices SINGLE_BT_DIV_ANTENNA_DEVICES[] = {
  { PCI_VENDOR_ID_ATHEROS, 0x3025 },
  { PCI_VENDOR_ID_ATHEROS, 0x3026 },
  { PCI_VENDOR_ID_ATHEROS, 0x302B },
  { PCI_VENDOR_ID_FOXCONN, 0xE069 },
  { PCI_VENDOR_ID_WNC, 0x3028 },
  { PCI_VENDOR_ID_LITEON, 0x0622 },
  { PCI_VENDOR_ID_LITEON, 0x0672 },
  { PCI_VENDOR_ID_LITEON, 0x0662 },
  { PCI_VENDOR_ID_LITEON, 0x06A2 },
  { PCI_VENDOR_ID_LITEON, 0x0682 },
  { PCI_VENDOR_ID_AZWAVE, 0x213A },
  { PCI_VENDOR_ID_AZWAVE, 0x213C },
  { PCI_VENDOR_ID_HP, 0x18E3 },
  { PCI_VENDOR_ID_HP, 0x217F },
  { PCI_VENDOR_ID_HP, 0x2005 },
  { PCI_VENDOR_ID_DELL, 0x020C }
};

ATH9KDevice::ATH9KDevice(PCIEntry& pciEntry) : RealNetworkDevice(pciEntry)
{
}

void ATH9KDevice::DetectDevice()
{
  for(uint32_t i = 0; i < sizeof(SINGLE_ANTENNA_DEVICES) / sizeof(SupportedDevices); ++i)
  {
    if(SINGLE_ANTENNA_DEVICES[i].vendorId == _pciEntry.BusEntity.NonBridge.usSubsystemVendorID
      && SINGLE_ANTENNA_DEVICES[i].deviceId == _pciEntry.BusEntity.NonBridge.usSubsystemDeviceID)
    {
      _antennaType = ANTENNA_TYPE::SINGLE;
      _btAntennaDiversity = false;
      return;
    }
  }

  for(uint32_t i = 0; i < sizeof(SINGLE_BT_DIV_ANTENNA_DEVICES) / sizeof(SupportedDevices); ++i)
  {
    if(SINGLE_BT_DIV_ANTENNA_DEVICES[i].vendorId == _pciEntry.BusEntity.NonBridge.usSubsystemVendorID
      && SINGLE_BT_DIV_ANTENNA_DEVICES[i].deviceId == _pciEntry.BusEntity.NonBridge.usSubsystemDeviceID)
    {
      _antennaType = ANTENNA_TYPE::SINGLE;
      _btAntennaDiversity = true;
      return;
    }
  }
  throw upan::exception(XLOC, "ATH9K device not supported!");
}

void ATH9KDevice::Initialize()//u16 devid, struct ath_softc *sc, const struct ath_bus_ops *bus_ops)
{
  //  struct ath_softc *sc;
//  struct ieee80211_hw *hw;
//  char hw_name[64];
  printf("\n Bus: %d, Dev: %d, Func: %d", _pciEntry.uiBusNumber, _pciEntry.uiDeviceNumber, _pciEntry.uiFunction);
  printf("\n Vendor: %x, Device: %x, Rev: %x", _pciEntry.usVendorID, _pciEntry.usDeviceID, _pciEntry.bRevisionID);
  printf("\n Sub-Vendor: %x, Sub-Device: %x", _pciEntry.BusEntity.NonBridge.usSubsystemVendorID, _pciEntry.BusEntity.NonBridge.usSubsystemDeviceID);

  DetectDevice();

  uint16_t usCommand;
  _pciEntry.ReadPCIConfig(PCI_COMMAND, 2, &usCommand);
  printf("\n CurVal of PCI_COMMAND: %x", usCommand);
  _pciEntry.WritePCIConfig(PCI_COMMAND, 2, usCommand | PCI_COMMAND_MMIO);

  /*
   * The default setting of latency timer yields poor results,
   * set it to the value used by other systems. It may be worth
   * tweaking this setting more.
   */
  _pciEntry.WritePCIConfig(PCI_LATENCY_TIMER, 1, 0xa8);

  _pciEntry.ReadPCIConfig(PCI_COMMAND, 2, &usCommand);
  printf("\n CurVal of PCI_COMMAND: %x", usCommand);
  _pciEntry.WritePCIConfig(PCI_COMMAND, 2, usCommand | PCI_COMMAND_MASTER);

  /*
   * Disable the RETRY_TIMEOUT register (0x41) to keep
   * PCI Tx retries from interfering with C3 CPU state.
   */
  uint32_t val;
  _pciEntry.ReadPCIConfig(0x40, 4, &val);
  if ((val & 0x0000ff00) != 0)
    _pciEntry.WritePCIConfig(0x40, 4, val & 0xffff00ff);

  _hw = new ATH9KHardware(_pciEntry);

//  ath9k_fill_chanctx_ops();
//  hw = ieee80211_alloc_hw(sizeof(struct ath_softc), &ath9k_ops);
//  if (!hw) {
//    dev_err(&pdev->dev, "No memory for ieee80211_hw\n");
//    return -ENOMEM;
//  }

//  SET_IEEE80211_DEV(hw, &pdev->dev);
//  pci_set_drvdata(pdev, hw);

//  sc = hw->priv;
//  sc->hw = hw;
//  sc->dev = &pdev->dev;
//  sc->mem = pcim_iomap_table(pdev)[0];
//  sc->driver_data = id->driver_data;

  //Can only support MSI capable ATH
  if(!_pciEntry.SetupMsiInterrupt(WIFINET_IRQ_NO))
    throw upan::exception(XLOC, "ATH WIFI Adaptor is not capable of MSI interrupts!");
  _pciEntry.SwitchToMsi();

//  sc->irq = pdev->irq;

//  ret = ath9k_init_device(id->device, sc, &ath_pci_bus_ops);
//  if (ret) {
//    dev_err(&pdev->dev, "Failed to initialize device\n");
//    goto err_init;
//  }

//  ath9k_hw_name(sc->sc_ah, hw_name, sizeof(hw_name));
//  wiphy_info(hw->wiphy, "%s mem=0x%lx, irq=%d\n",
//       hw_name, (unsigned long)sc->mem, pdev->irq);

//  return 0;

//err_init:
//  free_irq(sc->irq, sc);
//err_irq:
//  ieee80211_free_hw(hw);
//  return ret;

//	struct ieee80211_hw *hw = sc->hw;
//	struct ath_common *common;
//	struct ath_hw *ah;
//	int error = 0;
//	struct ath_regulatory *reg;
//
//	/* Bring up device */
//	error = ath9k_init_softc(devid, sc, bus_ops);
//	if (error)
//		return error;
//
//	ah = sc->sc_ah;
//	common = ath9k_hw_common(ah);
//	ath9k_set_hw_capab(sc, hw);
//
//	/* initialize regulatory */
//	error = ath_regd_init(&common->regulatory, sc->hw->wiphy,
//			      ath9k_reg_notifier);
//	if (error)
//		goto deinit;
//
//	reg = &common->regulatory;
//
//	/* Setup TX DMA */
//	error = ath_tx_init(sc, ATH_TXBUF);
//	if (error != 0)
//		goto deinit;
//
//	/* Setup RX DMA */
//	error = ath_rx_init(sc, ATH_RXBUF);
//	if (error != 0)
//		goto deinit;
//
//	ath9k_init_txpower_limits(sc);
//
//#ifdef CONFIG_MAC80211_LEDS
//	/* must be initialized before ieee80211_register_hw */
//	sc->led_cdev.default_trigger = ieee80211_create_tpt_led_trigger(sc->hw,
//		IEEE80211_TPT_LEDTRIG_FL_RADIO, ath9k_tpt_blink,
//		ARRAY_SIZE(ath9k_tpt_blink));
//#endif
//
//	/* Register with mac80211 */
//	error = ieee80211_register_hw(hw);
//	if (error)
//		goto rx_cleanup;
//
//	error = ath9k_init_debug(ah);
//	if (error) {
//		ath_err(common, "Unable to create debugfs files\n");
//		goto unregister;
//	}
//
//	/* Handle world regulatory */
//	if (!ath_is_world_regd(reg)) {
//		error = regulatory_hint(hw->wiphy, reg->alpha2);
//		if (error)
//			goto debug_cleanup;
//	}
//
//	ath_init_leds(sc);
//	ath_start_rfkill_poll(sc);
//
//	return 0;
//
//debug_cleanup:
//	ath9k_deinit_debug(sc);
//unregister:
//	ieee80211_unregister_hw(hw);
//rx_cleanup:
//	ath_rx_cleanup(sc);
//deinit:
//	ath9k_deinit_softc(sc);
//	return error;
}

void ATH9KDevice::NotifyEvent()
{
}

//int ath9k_config(struct ieee80211_hw *hw, u32 changed)
int ATH9KDevice::Configure()
{
//	struct ath_softc *sc = hw->priv;
//	struct ath_hw *ah = sc->sc_ah;
//	struct ath_common *common = ath9k_hw_common(ah);
//	struct ieee80211_conf *conf = &hw->conf;
//	struct ath_chanctx *ctx = sc->cur_chan;
//
//	ath9k_ps_wakeup(sc);
//	mutex_lock(&sc->mutex);
//
//	if (changed & IEEE80211_CONF_CHANGE_IDLE) {
//		sc->ps_idle = !!(conf->flags & IEEE80211_CONF_IDLE);
//		if (sc->ps_idle) {
//			ath_cancel_work(sc);
//			ath9k_stop_btcoex(sc);
//		} else {
//			ath9k_start_btcoex(sc);
//			/*
//			 * The chip needs a reset to properly wake up from
//			 * full sleep
//			 */
//			ath_chanctx_set_channel(sc, ctx, &ctx->chandef);
//		}
//	}
//
//	/*
//	 * We just prepare to enable PS. We have to wait until our AP has
//	 * ACK'd our null data frame to disable RX otherwise we'll ignore
//	 * those ACKs and end up retransmitting the same null data frames.
//	 * IEEE80211_CONF_CHANGE_PS is only passed by mac80211 for STA mode.
//	 */
//	if (changed & IEEE80211_CONF_CHANGE_PS) {
//		unsigned long flags;
//		spin_lock_irqsave(&sc->sc_pm_lock, flags);
//		if (conf->flags & IEEE80211_CONF_PS)
//			ath9k_enable_ps(sc);
//		else
//			ath9k_disable_ps(sc);
//		spin_unlock_irqrestore(&sc->sc_pm_lock, flags);
//	}
//
//	if (changed & IEEE80211_CONF_CHANGE_MONITOR) {
//		if (conf->flags & IEEE80211_CONF_MONITOR) {
//			ath_dbg(common, CONFIG, "Monitor mode is enabled\n");
//			sc->sc_ah->is_monitoring = true;
//		} else {
//			ath_dbg(common, CONFIG, "Monitor mode is disabled\n");
//			sc->sc_ah->is_monitoring = false;
//		}
//	}
//
//	if (!ath9k_is_chanctx_enabled() && (changed & IEEE80211_CONF_CHANGE_CHANNEL)) {
//		ctx->offchannel = !!(conf->flags & IEEE80211_CONF_OFFCHANNEL);
//		ath_chanctx_set_channel(sc, ctx, &hw->conf.chandef);
//	}
//
//	mutex_unlock(&sc->mutex);
//	ath9k_ps_restore(sc);

	return 0;
}

//void ath9k_tx(struct ieee80211_hw *hw, struct ieee80211_tx_control *control, struct sk_buff *skb)
void ATH9KDevice::Tx(SocketBuffer& socketBuffer)
{
  //IEEE80211Header* header = socketBuffer.Header();
//	struct ath_softc *sc = hw->priv;
//	struct ath_common *common = ath9k_hw_common(sc->sc_ah);
//	struct ath_tx_control txctl;
//	unsigned long flags;
//
//	if (sc->ps_enabled) {
//		/*
//		 * mac80211 does not set PM field for normal data frames, so we
//		 * need to update that based on the current PS mode.
//		 */
//		if (ieee80211_is_data(hdr->frame_control) &&
//		    !ieee80211_is_nullfunc(hdr->frame_control) &&
//		    !ieee80211_has_pm(hdr->frame_control)) {
//			ath_dbg(common, PS,
//				"Add PM=1 for a TX frame while in PS mode\n");
//			hdr->frame_control |= cpu_to_le16(IEEE80211_FCTL_PM);
//		}
//	}
//
//	if (unlikely(sc->sc_ah->power_mode == ATH9K_PM_NETWORK_SLEEP)) {
//		/*
//		 * We are using PS-Poll and mac80211 can request TX while in
//		 * power save mode. Need to wake up hardware for the TX to be
//		 * completed and if needed, also for RX of buffered frames.
//		 */
//		ath9k_ps_wakeup(sc);
//		spin_lock_irqsave(&sc->sc_pm_lock, flags);
//		if (!(sc->sc_ah->caps.hw_caps & ATH9K_HW_CAP_AUTOSLEEP))
//			ath9k_hw_setrxabort(sc->sc_ah, 0);
//		if (ieee80211_is_pspoll(hdr->frame_control)) {
//			ath_dbg(common, PS,
//				"Sending PS-Poll to pick a buffered frame\n");
//			sc->ps_flags |= PS_WAIT_FOR_PSPOLL_DATA;
//		} else {
//			ath_dbg(common, PS, "Wake up to complete TX\n");
//			sc->ps_flags |= PS_WAIT_FOR_TX_ACK;
//		}
//		/*
//		 * The actual restore operation will happen only after
//		 * the ps_flags bit is cleared. We are just dropping
//		 * the ps_usecount here.
//		 */
//		spin_unlock_irqrestore(&sc->sc_pm_lock, flags);
//		ath9k_ps_restore(sc);
//	}
//
//	/*
//	 * Cannot tx while the hardware is in full sleep, it first needs a full
//	 * chip reset to recover from that
//	 */
//	if (unlikely(sc->sc_ah->power_mode == ATH9K_PM_FULL_SLEEP)) {
//		ath_err(common, "TX while HW is in FULL_SLEEP mode\n");
//		goto exit;
//	}
//
//	memset(&txctl, 0, sizeof(struct ath_tx_control));
//	txctl.txq = sc->tx.txq_map[skb_get_queue_mapping(skb)];
//	txctl.sta = control->sta;
//
//	ath_dbg(common, XMIT, "transmitting packet, skb: %p\n", skb);
//
//	if (ath_tx_start(hw, skb, &txctl) != 0) {
//		ath_dbg(common, XMIT, "TX failed\n");
//		TX_STAT_INC(txctl.txq->axq_qnum, txfailed);
//		goto exit;
//	}
//
//	return;
//exit:
//	ieee80211_free_txskb(hw, skb);
}

int ATH9KDevice::Start() { return -1; }
void ATH9KDevice::Stop() { }
int ATH9KDevice::AddInterface() { return -1; }
int ATH9KDevice::ChangeInterface() { return -1; }
void ATH9KDevice::RemoveInterface() { }
void ATH9KDevice::ConfigureFilter() { }
int ATH9KDevice::STAState() { return -1; }
void ATH9KDevice::STANotify() { }
void ATH9KDevice::ConfigureTx() { }
void ATH9KDevice::BSSInfoChanged() { }
int ATH9KDevice::SetKey() { return -1; }
uint64_t ATH9KDevice::GetTSF() { return 0; }
void ATH9KDevice::SetTFS() { }
void ATH9KDevice::ResetTSF() { }
int ATH9KDevice::AMPDUAction() { return -1; }
int ATH9KDevice::GetSurvey() { return -1; }
void ATH9KDevice::SetCoverageClass() { }
void ATH9KDevice::Flush() { }
bool ATH9KDevice::TxFramesPending() { return true; }
int ATH9KDevice::TxLastBeacon() { return -1; }
int ATH9KDevice::GetStats() { return -1; }
int ATH9KDevice::GetAntenna() { return -1; }
void ATH9KDevice::ReleaseBufferedFrames() { }
void ATH9KDevice::SWScanStart() { }
void ATH9KDevice::SWScanComplete() { }
void ATH9KDevice::WakeTxQueue() { }
