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

#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unet.h>
#include <DNSClient.h>
#include <NetworkDevice.h>
#include <ProcessManager.h>

#define DNS_PORT 53
#define DNS_SERVER "8.8.8.8"
#define MAX_DNS_PACKET_SIZE 512

struct DNSHeader {
  uint16_t _id;
  uint16_t _flags;
  uint16_t _qdCount;
  uint16_t _anCount;
  uint16_t _nsCount;
  uint16_t _arCount;
} PACKED;

struct DNSQuestion {
  uint16_t _qtype;
  uint16_t _qclass;
} PACKED;

struct DNSAnswer {
  uint16_t _type;
  uint16_t _class;
  uint32_t _ttl;
  uint16_t _rdLen;
  // followed by rdata
} PACKED;

void DNSClient::encodeName(char *dns, upan::string name) {
  int pos = 0;
  name += ".";
  for (int i = 0; i < name.length(); i++) {
    if (name[i] == '.') {
      *dns++ = i - pos;
      for (; pos < i; pos++) {
        *dns++ = name[pos];
      }
      pos++;
    }
  }
  *dns++ = 0;
}

void DNSClient::decodeName(const uint8_t* payload, const uint8_t *qname, char *out, size_t maxlen) {
  size_t i = 0;
  while (*qname != 0 && i < maxlen - 1) {
    if ((*qname & 0xC0) == 0xC0) {
      // Compression: pointer to another location
      int offset = ((*qname & 0x3F) << 8) | *(qname + 1);
      decodeName(payload, payload + offset, out + i, maxlen - i);
      return;
    } else {
      const int len = *qname++;
      if (i != 0) out[i++] = '.';
      if (i + len >= maxlen - 1) break;
      memcpy(out + i, qname, len);
      qname += len;
      i += len;
    }
  }
  out[i] = '\0';
}

struct hostent* DNSClient::resolveHost(const upan::string& name) {
  Process& process = ProcessManager::Instance().GetCurrentPAS();

  struct in_addr addr;
  if (inet_aton(name.c_str(), &addr) == 1) {
    auto hostinfo = (struct hostent*)process.dmm().allocate(sizeof(struct hostent));
    hostinfo->h_addrtype = AF_INET;
    hostinfo->h_length = IPV4_ADDR_LEN;

    hostinfo->h_name = (char*)process.dmm().allocate(name.length() + 1);
    strcpy(hostinfo->h_name, name.c_str());

    hostinfo->h_aliases = (char**)process.dmm().allocate(sizeof(char *));  // only NULL
    hostinfo->h_aliases[0] = nullptr;

    hostinfo->h_addr_list = (char**)process.dmm().allocate(2 * sizeof(char *));
    char* address = (char*)process.dmm().allocate(IPV4_ADDR_LEN);
    memcpy(address, &addr.s_addr, IPV4_ADDR_LEN);
    hostinfo->h_addr_list[0] = address;
    hostinfo->h_addr_list[1] = nullptr;

    return hostinfo;
  }

  uint8_t payload[MAX_DNS_PACKET_SIZE];
  struct DNSHeader& dnsHeader = *(struct DNSHeader*)payload;

  int sd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sd < 0) {
    throw upan::exception(XLOC, "socket creation failed");
  }

  struct sockaddr_in dest;
  dest.sin_family = AF_INET;
  dest.sin_port = htons(DNS_PORT);
  dest.sin_addr.s_addr = upan::net::inet_strton(DNS_SERVER);

  memset(payload, 0, MAX_DNS_PACKET_SIZE);

  dnsHeader._id = htons(0x1234);
  dnsHeader._flags = htons(0x0100); // standard query
  dnsHeader._qdCount = htons(1);
  dnsHeader._anCount = 0;
  dnsHeader._nsCount = 0;
  dnsHeader._arCount = 0;

  // add the question section
  auto *qname = (uint8_t *)&payload[sizeof(struct DNSHeader)];
  encodeName((char *)qname, name);
  const auto qnameLen = strlen((const char *)qname) + 1;

  DNSQuestion& dnsQuestion = *(DNSQuestion*)(qname + qnameLen);
  dnsQuestion._qtype = htons(1); // A record
  dnsQuestion._qclass = htons(1); // IN class

  const int queryLen = sizeof(struct DNSHeader) + qnameLen + sizeof(DNSQuestion);

  struct timeval timeout {};
  timeout.tv_sec = 10;
  if (setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
    close(sd);
    throw upan::exception(XLOC, "failed to set socket option: SO_RCVTIMEO");
  }

  if (sendto(sd, payload, queryLen, 0, (struct sockaddr *)&dest, sizeof(dest)) < 0) {
    throw upan::exception(XLOC, "sendto failed");
  }

  socklen_t len = sizeof(dest);
  if (recvfrom(sd, payload, MAX_DNS_PACKET_SIZE, 0, (struct sockaddr *)&dest, &len) < 0) {
    throw upan::exception(XLOC, "recvfrom failed");
  }

  close(sd);

  if (ntohs(dnsHeader._flags) & 0xF) {
    throw upan::exception(XLOC, "DNS response has error flags - %d", ntohs(dnsHeader._flags) & 0xF);
  }

  if (ntohs(dnsHeader._id) != 0x1234) {
    throw upan::exception(XLOC, "DNS response has unexpected ID - 0x%x", ntohs(dnsHeader._id));
  }

  uint8_t* response = payload + sizeof(struct DNSHeader);
  // Skip encoded QNAME
  while (*response != 0) {
    response += *response + 1;
  }
  response++; // null terminator

  // Skip question section
  response += sizeof(struct DNSQuestion);

  auto anCount = ntohs(dnsHeader._anCount);

  upan::vector<char*> addresses;
  upan::vector<char*> aliases;

  for (int i = 0; i < anCount; i++) {
    // Skip the name (could be compressed)
    if ((*response & 0xC0) == 0xC0) {
      // Name is compressed: 2 bytes
      response += 2;
    } else {
      // Name is not compressed (rare in answers)
      while (*response != 0) {
        response += *response + 1;
      }
      response++;
    }

    auto& answer = *(struct DNSAnswer*) response;
    response += sizeof(struct DNSAnswer);

    // Extract RDATA (depends on ans->type)
    uint16_t ansType = ntohs(answer._type);
    uint16_t ansClass = ntohs(answer._class);
    uint16_t ansDataLen = ntohs(answer._rdLen);

    if (ansType == 1 && ansClass == 1 && ansDataLen == IPV4_ADDR_LEN) {  // A record
      char* ip = (char*)process.dmm().allocate(IPV4_ADDR_LEN);
      memcpy(ip, response, IPV4_ADDR_LEN);
      addresses.push_back(ip);
    } else if (ansType == 5 && ansClass == 1) { // CNAME
      char cname[256] = { '\0' };
      decodeName(payload, response, cname, sizeof(cname));
      char* cname_copy = (char*)process.dmm().allocate(strlen(cname) + 1);
      strcpy(cname_copy, cname);
      aliases.push_back(cname_copy);
    } else {
      KLog::info("DNS record type %d not handled", ansType);
    }
    response += ansDataLen;
  }

  auto hostinfo = (struct hostent*)process.dmm().allocate(sizeof(struct hostent));
  hostinfo->h_addrtype = AF_INET;
  hostinfo->h_length = IPV4_ADDR_LEN;

  if (aliases.empty()) {
    hostinfo->h_name = (char*)process.dmm().allocate(name.length() + 1);
    strcpy(hostinfo->h_name, name.c_str());
    hostinfo->h_aliases = (char**)process.dmm().allocate(sizeof(char *));  // only NULL
    hostinfo->h_aliases[0] = nullptr;
  } else {
    int alias_count = aliases.size();
    hostinfo->h_name = aliases[alias_count - 1];  // canonical name is last
    hostinfo->h_aliases = (char**)process.dmm().allocate(alias_count * sizeof(char *)); // one less than alias_count+1
    for (int i = 0; i < alias_count - 1; i++) {
      hostinfo->h_aliases[i] = aliases[i];  // earlier CNAMEs
    }
    hostinfo->h_aliases[alias_count - 1] = nullptr;
  }

  hostinfo->h_addr_list = (char**)process.dmm().allocate((addresses.size() + 1) * sizeof(char *));
  for (int i = 0; i < addresses.size(); i++) {
    hostinfo->h_addr_list[i] = addresses[i];
  }
  hostinfo->h_addr_list[addresses.size()] = nullptr;

  return hostinfo;
}

struct hostent* DNSClient::resolveReverseHost(const void *addr, socklen_t len, int type) {
  if (type != AF_INET || len != IPV4_ADDR_LEN) {
    return nullptr;
  }

  // Convert to reverse name
  auto ip = (const uint8_t*)addr;
  char reverseName[64];
  snprintf(reverseName, sizeof(reverseName), "%u.%u.%u.%u.in-addr.arpa", ip[3], ip[2], ip[1], ip[0]);

  uint8_t payload[MAX_DNS_PACKET_SIZE];
  memset(payload, 0, MAX_DNS_PACKET_SIZE);
  struct DNSHeader& dnsHeader = *(struct DNSHeader*)payload;

  dnsHeader._id = htons(0x1234);
  dnsHeader._flags = htons(0x0100); // standard query
  dnsHeader._qdCount = htons(1);
  dnsHeader._anCount = 0;
  dnsHeader._nsCount = 0;
  dnsHeader._arCount = 0;

  // add the question section
  auto *qname = (uint8_t *)&payload[sizeof(struct DNSHeader)];
  encodeName((char *)qname, reverseName);
  const auto qnameLen = strlen((const char *)qname) + 1;

  DNSQuestion& dnsQuestion = *(DNSQuestion*)(qname + qnameLen);
  dnsQuestion._qtype = htons(12); // PTR
  dnsQuestion._qclass = htons(1); // IN class

  const int queryLen = sizeof(struct DNSHeader) + qnameLen + sizeof(DNSQuestion);

  int sd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sd < 0) {
    throw upan::exception(XLOC, "socket creation failed");
  }

  struct sockaddr_in dest;
  dest.sin_family = AF_INET;
  dest.sin_port = htons(DNS_PORT);
  dest.sin_addr.s_addr = upan::net::inet_strton(DNS_SERVER);

  struct timeval timeout {};
  timeout.tv_sec = 10;
  if (setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
    close(sd);
    throw upan::exception(XLOC, "failed to set socket option: SO_RCVTIMEO");
  }

  if (sendto(sd, payload, queryLen, 0, (struct sockaddr *)&dest, sizeof(dest)) < 0) {
    throw upan::exception(XLOC, "sendto failed");
  }

  len = sizeof(dest);
  if (recvfrom(sd, payload, MAX_DNS_PACKET_SIZE, 0, (struct sockaddr *)&dest, &len) < 0) {
    throw upan::exception(XLOC, "recvfrom failed");
  }

  close(sd);

  if (ntohs(dnsHeader._flags) & 0xF) {
    throw upan::exception(XLOC, "DNS response has error flags - %d", ntohs(dnsHeader._flags) & 0xF);
  }

  if (ntohs(dnsHeader._id) != 0x1234) {
    throw upan::exception(XLOC, "DNS response has unexpected ID - 0x%x", ntohs(dnsHeader._id));
  }

  uint8_t* response = payload + sizeof(struct DNSHeader);
  // Skip encoded QNAME
  while (*response != 0) {
    response += *response + 1;
  }
  response++; // null terminator

  // Skip question section
  response += sizeof(struct DNSQuestion);

  auto anCount = ntohs(dnsHeader._anCount);

  char hostname[256] = { '\0' };

  for (int i = 0; i < anCount; i++) {
    // Skip the name (could be compressed)
    if ((*response & 0xC0) == 0xC0) {
      // Name is compressed: 2 bytes
      response += 2;
    } else {
      // Name is not compressed (rare in answers)
      while (*response != 0) {
        response += *response + 1;
      }
      response++;
    }

    auto& answer = *(struct DNSAnswer*) response;
    response += sizeof(struct DNSAnswer);

    // Extract RDATA (depends on ans->type)
    uint16_t ansType = ntohs(answer._type);
    uint16_t ansClass = ntohs(answer._class);
    uint16_t ansDataLen = ntohs(answer._rdLen);

    if (ansType == 12 && ansClass == 1) {  // PTR
      decodeName(payload, response, hostname, sizeof(hostname));
      break;
    }

    response += ansDataLen;
  }

  if (strlen(hostname) == 0) {
    return nullptr;
  }

  Process& process = ProcessManager::Instance().GetCurrentPAS();

  auto hostinfo = (struct hostent*)process.dmm().allocate(sizeof(struct hostent));
  hostinfo->h_addrtype = AF_INET;
  hostinfo->h_length = IPV4_ADDR_LEN;

  hostinfo->h_name = (char*)process.dmm().allocate(strlen(hostname) + 1);
  strcpy(hostinfo->h_name, hostname);
  hostinfo->h_aliases = (char**)process.dmm().allocate(sizeof(char *));  // only NULL
  hostinfo->h_aliases[0] = nullptr;

  hostinfo->h_addr_list = (char**)process.dmm().allocate(2 * sizeof(char *));
  char* retAddr = (char*)process.dmm().allocate(IPV4_ADDR_LEN);
  memcpy(retAddr, addr, IPV4_ADDR_LEN);
  hostinfo->h_addr_list[0] = retAddr;
  hostinfo->h_addr_list[1] = nullptr;

  return hostinfo;
}

void DNSClient::freeHostinfo(struct hostent* hostinfo) {
  if (!hostinfo) {
    return;
  }

  Process& process = ProcessManager::Instance().GetCurrentPAS();
  process.dmm().free((uintptr_t)hostinfo->h_name);

  if (hostinfo->h_aliases) {
    for (char **alias = hostinfo->h_aliases; *alias; alias++) {
      process.dmm().free((uintptr_t)*alias);
    }
    process.dmm().free((uintptr_t)hostinfo->h_aliases);
  }

  if (hostinfo->h_addr_list) {
    for (char **addr = hostinfo->h_addr_list; *addr; addr++) {
      process.dmm().free((uintptr_t)*addr);
    }
    process.dmm().free((uintptr_t)hostinfo->h_addr_list);
  }

  process.dmm().free((uintptr_t)hostinfo);
}