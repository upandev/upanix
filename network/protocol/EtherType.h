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

typedef enum {
  IPV4 = 0x0800,
  ARP = 0x0806,
  WAKE_ON_LAN = 0x0842,
  AVTP = 0x22F0,
  IETF_TRILL = 0x22F3,
  SRP = 0x22EA,
  DEC_MOP_RC = 0x6002,
  DECNET_PHASE_IV_DNA_ROUTING = 0x6003,
  DEC_LAT = 0x6004,
  RARP = 0x8035,
  APPLE_TALK = 0x809B,
  APPLE_TALK_ARP = 0x80F3,
  VLAN_SPB = 0x8100,
  SLPP = 0x8102,
  IPX = 0x8137,
  QNX = 0x8204,
  IPV6 = 0x86DD,
  EFC = 0x8808,
  ESP = 0x8809,
  COBRA_NET = 0x8819,
  MPLS_UNICASE = 0x8847,
  MPLS_MULTICAST = 0x8848,
  PPPoE_DISCOVERY_STAGE = 0x8863,
  PPPoE_SESSION_STAGE = 0x8864,
  HOMEPLUG_MME = 0x887B,
  EAP_OVER_LAN = 0x888E,
  PROFINET = 0x8892,
  HYPER_SCSI = 0x889A,
  ATA_OVER_ETHERNET = 0x88A2,
  ETHER_CAT = 0x88A4,
  SERVICE_VLAN_TAG = 0x88A8,
  ETHERNET_POWERLINK = 0x88AB,
  GOOSE = 0x88B8,
  GSE = 0x88B9,
  SV = 0x88BA,
  MIKROTIK_ROMON = 0x88BF,
  LLDP = 0x88CC,
  SERCOS_III = 0x88CD,
  WSMP = 0x88DC,
  MRP = 0x88E3,
  MAC_SECURITY = 0x88E5,
  PBB = 0x88E7,
  PTP_OVER_ETHERNET = 0x88F7,
  NC_SI = 0x88F8,
  PRP = 0x88FB,
  CFM_OAM = 0x8902,
  FCOE = 0x8906,
  FCOE_INIT = 0x8914,
  ROCE = 0x8915,
  TTE = 0x891D,
  HSR = 0x892F,
  ECTP = 0x9000,
  VLAN_DTAG = 0x9100,
  REDUNDANCY_TAG = 0xF1C1,
} EtherType;
