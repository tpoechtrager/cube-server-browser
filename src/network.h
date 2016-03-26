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

#ifndef __NETWORK_H__
#define __NETWORK_H__

#include <cstdlib>
#include <cctype>
#include <cstdint>
#include <limits>
#include <string>

#ifdef _WIN32
#include "compat/win32/compat.h"
#else
#include <sys/types.h>
#endif

namespace network {

//
// Types
//

struct Socket {
#ifdef _WIN32
  uintptr_t socket;
#else
  int socket;
#endif
};

struct SelectSocket {
  Socket socket;
  void *data;
};

struct Address {
  uint32_t host;
  uint16_t port;
};

typedef void (*SocketSelectCallback)(const SelectSocket &socket);

//
// Misc
//

inline bool addressEqual(const Address &a, const Address &b) {
  return (a.host == b.host && a.port == b.port);
}

bool init();
void deinit();

#if defined(_MSC_VER) || __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
inline uint32_t hostToNet(uint32_t val) { return __builtin_bswap32(val); }
inline uint32_t netToHost(uint32_t val) { return __builtin_bswap32(val); }
#else
inline uint32_t hostToNet(uint32_t val) { return val; }
inline uint32_t netToHost(uint32_t val) { return val; }
#endif

Socket newSocket(bool TCP = false);

void deleteSocket(Socket socket);
bool socketBind(Socket socket, const Address &address);

bool setHostAddress(const char *hostName, Address &address);
bool getHostAddress(const Address &address, char *buf, size_t size);
bool getHostIPAddress(const Address &address, char *buf, size_t size);
bool getHostIPAddress(const void *sockAddr, char *buf, size_t size, network::Address *address = nullptr);

bool isIPv4Address(const char *ipAddress, uint32_t *ip = nullptr);
uint32_t getMask(const uint8_t bits);

ssize_t socketRecv(Socket socket, Address *address, unsigned char *buf, const size_t size);
ssize_t socketSend(Socket socket, const Address *address, const unsigned char *buf, const size_t size);

int socketSelect(const SelectSocket *sockets, const size_t numSockets,
                 SocketSelectCallback readCallback, SocketSelectCallback writeCallback,
                 const uint32_t wait = -1u);

bool recvTCPData(const char *hostName, const uint16_t hostPort, const char *request, std::string &content,
                 const size_t limit = std::numeric_limits<size_t>::max(), const uint32_t maxWait = -1u);

//
// PacketBuf
//

class PacketBuf {
private:
  size_t pos = 0;
  unsigned char *buf;
  const size_t size;
  size_t readLen;

public:
  void reset();
  size_t available();
  size_t length();
  bool overRead();
  size_t remaining();

  void addByte(const unsigned char byte);
  unsigned char getByte();
  signed char getByteSigned();

  void addInt(const int val);
  int getInt();

  void addInt64(const int64_t val);
  int64_t getInt64();

  void addString(const char *str);
  char *getString(char *str, const size_t size);

  bool send(Socket socket, const Address &address);
  bool receive(Socket socket, Address &address);

  PacketBuf() = delete;
  PacketBuf(unsigned char *buf, const size_t size, const size_t readLen = 0)
      : buf(buf), size(size), readLen(readLen) {}
};

template <size_t bufSize> class PacketBuf_ : public PacketBuf {
private:
  unsigned char buf[bufSize];

public:
  unsigned char *getBuf() { return buf; }

  PacketBuf_() : PacketBuf(buf, bufSize) {}
};

typedef PacketBuf_<8> PacketBuf8;
typedef PacketBuf_<16> PacketBuf16;
typedef PacketBuf_<32> PacketBuf32;
typedef PacketBuf_<1 * 1024> PacketBuf1K;
typedef PacketBuf_<5 * 1024> PacketBuf5K;

} // namespace network

#endif //__NETWORK_H__
