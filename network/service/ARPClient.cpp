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

#include <net/if.h>
#include <ARPClient.h>
#include <NetworkManager.h>
#include <NetworkDevice.h>

ARPClient::ARPClient(NetworkDevice& networkDevice) : _networkDevice(networkDevice) {
}

MACAddress ARPClient::resolveMacAddress(in_addr_t dest_ip) {
  int sd = socket(AF_PACKET, SOCK_RAW, ETH_P_ARP);
  if (sd < 0) {
    throw upan::exception(XLOC, "socket creation failed");
  }

  struct ether_arp arp {};
  arp.ea_hdr.ar_op = htons(ARPOP_REQUEST);

  memcpy(arp.arp_sha, _networkDevice.GetMACAddress().get(), ETH_ALEN);
  arp.arp_spa = _networkDevice.GetIPAddress();

  memset(arp.arp_tha, 0, ETH_ALEN);
  arp.arp_tpa = dest_ip;

  struct sockaddr_ll addr = {
    .sll_family   = AF_PACKET,
    .sll_ifindex  = _networkDevice.id(),
    .sll_halen    = ETH_ALEN,
  };
  memcpy(addr.sll_addr, INADDR_MAC_BROADCAST, 6);

  struct timeval timeout {};
  timeout.tv_sec = 10;
  if (setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
    close(sd);
    throw upan::exception(XLOC, "failed to set socket option: SO_RCVTIMEO");
  }

  if (sendto(sd, &arp, sizeof(arp), 0, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
    throw upan::exception(XLOC, "sendto failed");
  }

  if(recvfrom(sd, &arp, sizeof(arp), 0, nullptr, nullptr) < 0) {
    throw upan::exception(XLOC, "recvfrom failed");
  }

  close(sd);
  NetworkManager::Instance().updateIPMACTable(arp.arp_spa, arp.arp_sha);

  return arp.arp_sha;
}