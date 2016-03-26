/************************************************************************
 *  Cube Server Browser                                                 *
 *  Copyright (C) 2015 by Thomas Poechtrager                            *
 *  t.poechtrager@gmail.com                                             *
 *                                                                      *
 *  This program is free software: you can redistribute it and/or       *
 *  modify it under the terms of the GNU Affero General Public License  *
 *  as published by the Free Software Foundation, either version 3      *
 *  of the License, or (at your option) any later version.              *
 *                                                                      *
 *  This program is distributed in the hope that it will be useful,     *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of      *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the       *
 *  GNU Affero General Public License for more details.                 *
 *                                                                      *
 *  You should have received a copy of the GNU Affero General Public    *
 *  License along with this program.                                    *
 *  If not, see <http://www.gnu.org/licenses/>.                         *
 ************************************************************************/

#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <algorithm>
#include <enet/enet.h>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <netinet/in.h>
#endif

#include "tools.h"
#include "main.h"
#include "network.h"

#include <enet/enet.h>

namespace network {

//
// Misc
//

namespace {
bool initialized;
} // anonymous namespace

bool init() {
  initialized = !enet_initialize();
  if (!initialized) *logFile << "enet_initialize() failed" << logFile->endl();
  return initialized;
}

void deinit() {
  if (initialized) {
    enet_deinitialize();
    initialized = false;
  }
}

namespace {

ENetSocket unwrap(const Socket socket) {
  return *reinterpret_cast<const ENetSocket *>(&socket);
}

ENetAddress *unwrap(Address &address) {
  return reinterpret_cast<ENetAddress *>(&address);
}

const ENetAddress *unwrap(const Address &address) {
  return reinterpret_cast<const ENetAddress *>(&address);
}

ENetAddress *unwrap(Address *address) {
  return reinterpret_cast<ENetAddress *>(address);
}

const ENetAddress *unwrap(const Address *address) {
  return reinterpret_cast<const ENetAddress *>(address);
}

} // anonymous namespace

Socket newSocket(bool TCP) {
  ENetSocket socket = enet_socket_create(TCP ? ENET_SOCKET_TYPE_STREAM
                                             : ENET_SOCKET_TYPE_DATAGRAM);
  if (socket == ENET_SOCKET_NULL) std::abort();
  return {socket};
}

void deleteSocket(Socket socket) { enet_socket_destroy(unwrap(socket)); }

bool socketBind(Socket socket, const Address &address) {
  return enet_socket_bind(unwrap(socket), unwrap(address)) >= 0;
}

bool setHostAddress(const char *hostName, Address &address) {
  return enet_address_set_host(unwrap(address), hostName) >= 0;
}

bool getHostAddress(const Address &address, char *buf, size_t size) {
  return enet_address_get_host(unwrap(address), buf, size) >= 0;
}

bool getHostIPAddress(const Address &address, char *buf, size_t size) {
  return enet_address_get_host_ip(unwrap(address), buf, size) >= 0;
}

bool getHostIPAddress(const void *sockAddr, char *buf, size_t size,
                      network::Address *address) {
  const sockaddr *addr = static_cast<const sockaddr *>(sockAddr);
  if (addr->sa_family == AF_INET) {
    const sockaddr_in *inAddr = reinterpret_cast<const sockaddr_in *>(addr);
    if (!network::getHostIPAddress({inAddr->sin_addr.s_addr}, buf, size)) return false;
    if (address) *address = {inAddr->sin_addr.s_addr, inAddr->sin_port};
  } else {
    // TODO
    std::abort();
    //(((struct sockaddr_in6 *)addr)->sin6_addr);
  }
  return true;
}

bool isIPv4Address(const char *ipAddress, uint32_t *ip) {
  int ipBuf[4];
  if (sscanf(ipAddress, "%d.%d.%d.%d", &ipBuf[0], &ipBuf[1], &ipBuf[2], &ipBuf[3]) != 4) return false;
  for (size_t i = 0; i < sizeofarray(ipBuf); ++i) if (ipBuf[i] > 0xFF || ipBuf[i] < 0x00) return false;
  if (ip) *ip = ipBuf[0] << 24 | ipBuf[1] << 16 | ipBuf[2] << 8 | ipBuf[3];
  return true;
}

uint32_t getMask(const uint8_t bits) {
  return hostToNet(((1 << bits) - 1) << (32 - bits));
}

ssize_t socketRecv(Socket socket, Address *address, unsigned char *buf, const size_t size) {
  ENetBuffer recvBuf;
  recvBuf.data = buf;
  recvBuf.dataLength = size;
  return enet_socket_receive(unwrap(socket), unwrap(address), &recvBuf, 1);
}

ssize_t socketSend(Socket socket, const Address *address, const unsigned char *buf, const size_t size) {
  ENetBuffer sendBuf;
  sendBuf.data = const_cast<void *>(static_cast<const void *>(buf));
  sendBuf.dataLength = size;
  return enet_socket_send(unwrap(socket), unwrap(address), &sendBuf, 1) == static_cast<int>(sendBuf.dataLength);
}

int socketSelect(const SelectSocket *sockets, const size_t numSockets,
                 SocketSelectCallback readCallback, SocketSelectCallback writeCallback,
                 const uint32_t maxWait) {
  TimeType start = getMicroSeconds();
  ENetSocketSet readSocketSet;
  ENetSocketSet writeSocketSet;
  ENetSocket highSocket = ENET_SOCKET_NULL;

  if (readCallback) ENET_SOCKETSET_EMPTY(readSocketSet);
  if (writeCallback) ENET_SOCKETSET_EMPTY(writeSocketSet);

  for (size_t i = 0; i < numSockets; ++i) {
    ENetSocket enetSocket = unwrap(sockets[i].socket);

    if (readCallback) ENET_SOCKETSET_ADD(readSocketSet, enetSocket);
    if (writeCallback) ENET_SOCKETSET_ADD(writeSocketSet, enetSocket);
    if ((readCallback || writeCallback) && (enetSocket > highSocket || highSocket == ENET_SOCKET_NULL)) highSocket = enetSocket;
  }

  if (highSocket == ENET_SOCKET_NULL) return EINVAL;

  const TimeType timeout = maxWait * 1000;
  TimeType elapsed = TimeType();

  do {
    TimeType us = timeout - elapsed;
    struct timeval timeVal;

    timeVal.tv_sec = us / 1000000;
    timeVal.tv_usec = us % 1000000;

    // Using select() here instead of enet_socketset_select()
    // because it allows more fine-grained timeouts.

    int retVal = select(highSocket + 1,
                        readCallback ? &readSocketSet : nullptr,
                        writeCallback ? &writeSocketSet : nullptr,
                        nullptr, &timeVal);

    if (retVal < 0) return errno == EINTR ? 0 : errno;
    if (retVal == 0) return 0;

    for (size_t i = 0; i < numSockets; ++i) {
      auto checkSocket = [&](ENetSocketSet &socketSet, const SelectSocket &socket,  SocketSelectCallback callback) {
        if (!callback) return;
        if (ENET_SOCKETSET_CHECK(socketSet, unwrap(socket.socket))) callback(socket);
      };

      checkSocket(readSocketSet, sockets[i], readCallback);
      checkSocket(writeSocketSet, sockets[i], writeCallback);
    }

    elapsed = getMicroSeconds() - start;
  } while (elapsed < timeout);

  return 0;
}

bool recvTCPData(const char *hostName, const uint16_t hostPort, const char *request,
                 std::string &content, const size_t limit, const uint32_t timeout) {
  Address address;
  uint32_t start = getMilliSeconds();

  auto timeLeft = [&]() {
    uint32_t diff = getMilliSeconds() - start;
    if (diff >= timeout) return 0u;
    return timeout - diff;
  };

  if (!setHostAddress(hostName, address) || !timeLeft()) return false;

  address.port = hostPort;
  Socket socket = newSocket(true);

  if (enet_socket_set_option(unwrap(socket), ENET_SOCKOPT_NONBLOCK, 1) < 0) goto failRet;
  if (enet_socket_connect(unwrap(socket), unwrap(address)) != 0) goto failRet;

  {
    auto connectSelect = [&]() {
      ENetSocketSet socketSet;

      ENET_SOCKETSET_EMPTY(socketSet);
      ENET_SOCKETSET_ADD(socketSet, unwrap(socket));

      if (enet_socketset_select(unwrap(socket), &socketSet, &socketSet, timeLeft()) <= 0) return false;
      if (!ENET_SOCKETSET_CHECK(socketSet, unwrap(socket))) return false;

      return true;
    };

    if (!connectSelect() || !timeLeft()) goto failRet;
  }

  ENetBuffer sendBuf;
  sendBuf.data = const_cast<void *>(static_cast<const void *>(request));
  sendBuf.dataLength = std::strlen(request);

  if (enet_socket_send(unwrap(socket), unwrap(address), &sendBuf, 1) != static_cast<int>(sendBuf.dataLength)) goto failRet;

  ENetSocketSet socketSet;
  content.clear();

  do {
    ENET_SOCKETSET_EMPTY(socketSet);
    ENET_SOCKETSET_ADD(socketSet, unwrap(socket));

    if (enet_socketset_select(unwrap(socket), &socketSet, 0, timeLeft()) <= 0)  break;
    if (!ENET_SOCKETSET_CHECK(socketSet, unwrap(socket))) break;

    unsigned char buf[4096];
    ssize_t len = socketRecv(socket, nullptr, buf, sizeof(buf));

    if (len < 0) goto failRet;
    else if (len == 0) break;

    if (content.length() + len > limit) goto failRet;

    content.append(reinterpret_cast<const char *>(buf), len);
  } while (timeLeft());

  deleteSocket(socket);
  return true;

  failRet:;

  deleteSocket(socket);
  return false;
}

//
// PacketBuf
//

void PacketBuf::reset() {
  pos = 0;
  readLen = 0;
}

size_t PacketBuf::available() { return size - pos; }
size_t PacketBuf::length() { return pos; }
size_t PacketBuf::remaining() { return overRead() ? 0 : readLen - pos; }
bool PacketBuf::overRead() { return pos > readLen; }

void PacketBuf::addByte(const unsigned char byte) {
  if (pos >= size) return;
  buf[pos++] = byte;
}

unsigned char PacketBuf::getByte() {
  if (pos >= readLen) return 0;
  return buf[pos++];
}

signed char PacketBuf::getByteSigned() {
  return static_cast<signed char>(getByte());
}

void PacketBuf::addInt(const int val) {
  if (val < 128 && val > -127)
    addByte(val);
  else if (val < 0x8000 && val >= -0x8000) {
    addByte(0x80);
    addByte(val);
    addByte(val >> 8);
  } else {
    addByte(0x81);
    addByte(val);
    addByte(val >> 8);
    addByte(val >> 16);
    addByte(val >> 24);
  }
}

int PacketBuf::getInt() {
  int val = getByteSigned();
  if (val == -128) {
    int n = getByte();
    // Avoid negative left shift
    n |= static_cast<uint32_t>(getByteSigned()) << 8;
    return n;
  } else if (val == -127) {
    int n = getByte();
    n |= getByte() << 8;
    n |= getByte() << 16;
    return n | (getByte() << 24);
  }
  return val;
}

void PacketBuf::addInt64(const int64_t val) {
  const union {
    int64_t i64;
    int i[sizeof(int64_t)/sizeof(int)];
  } v{val};
  for (const int &i : v.i) addInt(i);
}

int64_t PacketBuf::getInt64() {
  union {
    int64_t i64;
    int i[sizeof(int64_t)/sizeof(int)];
  } v;
  for (int &i : v.i) i = getInt();
  return v.i64;
}

void PacketBuf::addString(const char *str) {
  while (*str) addInt(*str++);
  addInt(0);
}

char *PacketBuf::getString(char *str, const size_t size) {
  size_t len = 0;
  int val;
  while ((val = getInt()) && len < size - 1) str[len++] = val;
  if (val) while (getInt());
  str[len] = '\0';
  return str;
}

bool PacketBuf::send(Socket socket, const Address &address) {
  return socketSend(socket, &address, buf, length()) > 0;
}

bool PacketBuf::receive(Socket socket, Address &address) {
  ssize_t len = socketRecv(socket, &address, buf, size);
  if (len <= 0) return false;
  pos = 0;
  readLen = len;
  return true;
}

#if 0

namespace {

struct Test {
  char dummy;

  Test() {
    PacketBuf16 send;

    union {
      int i[2];
      uint64_t ui64;
    };

    for (i[0] = std::numeric_limits<int>::min(),
         i[1] = std::numeric_limits<int>::max();
         i[0] < std::numeric_limits<int>::max() &&
         i[1] > std::numeric_limits<int>::min(); ++i[0], --i[1]) {
      if (i[0] % 10000000 == 0) info << i[0] << " | " << i[1] << info.endl();

      send.addInt(i[0]), send.addInt(i[1]);
      PacketBuf recv(send.getBuf(), 16, send.length());

      union {
        int v[2];
        uint64_t ui64;
      } v{{recv.getInt(), recv.getInt()}};

      if (v.ui64 != ui64 || recv.overRead() || recv.remaining()) std::abort();

      send.reset();
    }

    std::exit(EXIT_SUCCESS);
  }
} test;

} // anonymous namespace

#endif

} // namespace network
