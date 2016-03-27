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
#include <cctype>
#include <cstring>
#include <ctime>
#include <fstream>
#include <sys/stat.h>
#include <zlib.h>

#ifndef _MSC_VER
#include <unistd.h>
#endif

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif

#if defined(__APPLE__) && !defined(USE_GETTIMEOFDAY)
#include <CoreServices/CoreServices.h>
#include <mach/mach.h>
#include <mach/mach_time.h>
#endif

#include "tools.h"
#include "cube/tools.h"

namespace tools {

//
// Strings
//

//
// Message String Parser
//
// Example:
//    "Hello World" A "B\"""" "C" D
// ================================
//    0: Hello World
//    1: A
//    2: B"
//    3:
//    4: C
//    5: D
//

size_t parseMsg(const char *msg, std::vector<std::string> &args, const bool stripEmpty) {
  std::string arg;

  while (*msg) {
    if (arg.empty()) while (*msg == ' ') ++msg;
  
    if (*msg == '"') {
      ++msg;
      while (*msg) {
        if (*msg == '\\') {
          if (!*++msg) break;
          arg += *msg++;
          continue;
        }
        if (*msg == '"') break;
        arg += *msg++;
      }
      if (*msg) ++msg;
      if (!stripEmpty || !arg.empty()) args.push_back(std::move(arg));
      continue;
    }

    const char *pMsg = msg;
    while (*msg && *msg != ' ') ++msg;
    if (pMsg != msg) arg.assign(pMsg, msg - pMsg);
    if (!arg.empty()) args.push_back(std::move(arg));
  }

  if (!arg.empty()) args.push_back(std::move(arg));
  return args.size();
}

// IntCmp

IntCmpOp IntCmp::getOperator(CString &val, const IntCmpOp fallback) {
  IntCmpOp op = fallback;

  if (*val) {
    while (**val == ' ') ++val;

    if (!std::strncmp(*val, ">=", 2)) {
      val += 2;
      op = INTCMP_OP_GREATER_EQUAL;
    } else if (!std::strncmp(*val, "<=", 3)) {
      val += 2;
      op = INTCMP_OP_LOWER_EQUAL;
    } else if (!std::strncmp(*val, "!=", 2)) {
      val += 2;
      op = INTCMP_OP_NOT_EQUAL;
    } else if (!std::strncmp(*val, "==", 2)) {
      val += 2;
      op = INTCMP_OP_EQUAL;
    } else if (**val == '>') {
      val += 1;
      op = INTCMP_OP_GREATER_THAN;
    } else if (**val == '<') {
      val += 1;
      op = INTCMP_OP_LOWER_THAN;
    } else if (**val == '=') {
      val += 1;
      op = INTCMP_OP_EQUAL;
    }

    while (**val == ' ') ++val;
  }

  return op;
}

void IntCmp::set(CString &val_, const bool parseOperator, const int fallback) {
  if (parseOperator) op = getOperator(val_);
  else op = INTCMP_OP_DEFAULT;
  val = !val_.empty() ? std::atoi(*val_) : fallback;
}

// StrCmp

bool StrCmp::match(const CString &inVal) const {
  if (val.empty()) return true;

  switch (op) {
  case STRCMP_OP_CONTAINS_CASE_INSENSITIVE:
    return !!strcasestr(*inVal, *val);
  case STRCMP_OP_CONTAINS_NOT_CASE_INSENSITIVE:
    return !strcasestr(*inVal, *val);
  case STRCMP_OP_CONTAINS:
    return !!std::strstr(*inVal, *val);
  case STRCMP_OP_CONTAINS_NOT:
    return !std::strstr(*inVal, *val);
  case STRCMP_OP_EQUAL_CASE_INSENSITIVE:
    return val.length() == inVal.length() && !strcasecmp(*val, *inVal);
  case STRCMP_OP_NOT_EQUAL_CASE_INSENSITIVE:
    return val.length() != inVal.length() || strcasecmp(*val, *inVal);
  case STRCMP_OP_EQUAL:
    return inVal.length() == val.length() && !std::memcmp(*val, *inVal, val.length());
  case STRCMP_OP_NOT_EQUAL:
    return inVal.length() != val.length() || std::memcmp(*val, *inVal, val.length());
  default:
    __builtin_unreachable();
  }
}

StrCmpOp StrCmp::getOperator(CString2 &val, const StrCmpOp fallback) {
  StrCmpOp op = fallback;

  if (*val) {
    while (**val == ' ') ++val;

    if (!std::strncmp(*val, "==", 2)) {
      val += 2;
      op = STRCMP_OP_EQUAL;
    } else if (!std::strncmp(*val, "cnc", 3)) {
      val += 3;
      op = STRCMP_OP_CONTAINS_NOT_CASE_INSENSITIVE;
    } else if (!std::strncmp(*val, "cc", 2)) {
      val += 2;
      op = STRCMP_OP_CONTAINS_CASE_INSENSITIVE;
    } else if (!std::strncmp(*val, "!==", 3)) {
      val += 3;
      op = STRCMP_OP_NOT_EQUAL_CASE_INSENSITIVE;
    } else if (!std::strncmp(*val, "!=", 2)) {
      val += 2;
      op = STRCMP_OP_DEFAULT_NEQ;
    } else if (!strncasecmp(*val, "eqc", 3)) {
      val += 3;
      op = STRCMP_OP_EQUAL_CASE_INSENSITIVE;
    } else if (!strncasecmp(*val, "eqc", 3)) {
      val += 3;
      op = STRCMP_OP_EQUAL;
    } else if (!strncasecmp(*val, "eq", 2)) {
      val += 2;
      op = STRCMP_OP_EQUAL_CASE_INSENSITIVE;
    } else if (!strncasecmp(*val, "nec", 3)) {
      val += 3;
      op = STRCMP_OP_NOT_EQUAL_CASE_INSENSITIVE;
    } else if (!strncasecmp(*val, "ne", 2)) {
      val += 2;
      op = STRCMP_OP_NOT_EQUAL;
    } else if (!std::strncmp(*val, "cn", 2)) {
      val += 2;
      op = STRCMP_OP_CONTAINS_NOT;
    } else if (std::toupper(**val) == 'C') {
      val += 1;
      op = STRCMP_OP_CONTAINS;
    } else if (**val == '=') {
      val += 1;
      op = STRCMP_OP_DEFAULT_EQ;
    }

    while (**val == ' ') ++val;
  }

  return op;
}

void StrCmp::set(CString2 &val_, const bool parseOperator, const CString &fallback) {
  if (parseOperator) op = getOperator(val_);
  else op = STRCMP_OP_DEFAULT_EQ;
  val = CString(!val_.empty() ? *val_ : *fallback, !val_.empty() ? val_.length() : fallback.length());
  valAssigned = true;
}

bool readFile(const CString &file, std::string &buf) {
  std::ifstream stream(*file, std::ifstream::binary);
  if (!stream.is_open()) return false;
  stream.seekg(0, std::ios::end);
  std::streampos size = stream.tellg();
  if (size == std::numeric_limits<std::streamoff>::max()) return false;
  buf.reserve(static_cast<size_t>(size));
  stream.seekg(0, std::ios::beg);
  buf.assign(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
  return stream.good();
}

bool writeFile(const CString &file, const std::string &content) {
  std::ofstream stream(*file, std::ifstream::binary);
  if (!stream.is_open()) return false;
  stream << content;
  return stream.good();
}

const char *convertCubeToUTF8(const CString &str, char *buf, const size_t size) {
  size_t len = cubetools::encodeutf8(reinterpret_cast<unsigned char *>(buf), size,
                                     reinterpret_cast<const unsigned char *>(*str), str.length());
  buf[std::min(len, size - 1)] = '\0';
  return buf;
}

const char *convertUTF8ToCube(const CString &str, char *buf, const size_t size) {
  size_t len = cubetools::decodeutf8(
      reinterpret_cast<unsigned char *>(buf), size,
      reinterpret_cast<const unsigned char *>(*str), str.length());
  buf[std::min(len, size - 1)] = '\0';
  return buf;
}

//
// Files & Directories
//

const char *getFileExtension(const CString &fileName) {
  const char *ext = std::strrchr(*fileName, '.');
  if (ext) return ++ext;
  return nullptr;
}

bool fileExists(const CString &fileName, struct stat *st) {
  struct stat tmp;
  if (!st) st = &tmp;
  return !stat(*fileName, st);
}

//
// Terminal text colors
//

bool isTerminal() {
#ifndef _WIN32
  static bool first = false;
  static bool val;

  if (!first) {
    val = !!isatty(STDERR_FILENO);
    first = true;
  }

  return val;
#else
  return false;
#endif
}

//
// Log helper
//

void LogFile::printTimeStamp() {
  ShortString buf;
  struct tm tm;
  std::time_t now = std::time(nullptr);
  if (!localtime_r(&now, &tm)) return;
  if (std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S | ", &tm)) os << buf;
}

LogFile::LogFile(const char *file)
    : LogFileStream(file), MessageLocked(log.is_open() ? log : std::cerr) {
  if (!log.is_open()) {
    err << "cannot open log file '" << file << "', using stderr instead" << err.endl();
  }
}

MessageLocked warn("warning");
MessageLocked err("error");
MessageDebug dbg("debug", FG_LIGHT_MAGENTA);
MessageLocked info("info", FG_LIGHT_MAGENTA);
MessageLocked warninfo("   info", FG_LIGHT_MAGENTA);

//
// Time
//

namespace {
TimeType nanoSecondsBase /*= getNanoSeconds()*/;

#ifdef _WIN32
uint64_t clockSpeed;

struct initNanoTimer {
  initNanoTimer() {
    LARGE_INTEGER speed;
    if (!QueryPerformanceFrequency(&speed)) speed.QuadPart = 0;
    if (speed.QuadPart <= 0) std::abort();
    clockSpeed = speed.QuadPart;
  }
} initNanoTimer;
#endif
} // anonymous namespace

TimeType getNanoSeconds() {
#ifdef _WIN32
  TimeType ticks;
  if (!QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER *>(&ticks))) std::abort();
  double tmp = (ticks / static_cast<double>(clockSpeed)) * 1000000000.0;
  return static_cast<TimeType>(tmp) - nanoSecondsBase;
#elif defined(__APPLE__) && !defined(USE_GETTIMEOFDAY)
  union {
    AbsoluteTime at;
    TimeType tt;
  } tmp;
  tmp.tt = mach_absolute_time();
  Nanoseconds ns = AbsoluteToNanoseconds(tmp.at);
  tmp.tt = UnsignedWideToUInt64(ns) - nanoSecondsBase;
  return tmp.tt;
#elif !defined(USE_GETTIMEOFDAY)
  struct timespec tp;
  if (clock_gettime(CLOCK_MONOTONIC, &tp) == 0)
    return static_cast<TimeType>(((tp.tv_sec * 1000000000LL) + tp.tv_nsec)) - nanoSecondsBase;
#endif // WIN32
#ifndef _WIN32
  struct timeval tv;
  if (gettimeofday(&tv, nullptr) == 0)
    return static_cast<TimeType>(((tv.tv_sec * 1000000000LL) + (tv.tv_usec * 1000))) - nanoSecondsBase;
  std::abort();
#endif
}

const char *fmtMillis(TimeType millis, char *buf, size_t size,
                      const FmtMillisFlags flags) {
  constexpr TimeType oneSecond = 1000;
  constexpr TimeType oneMinute = oneSecond * 60;
  constexpr TimeType oneHour = oneMinute * 60;
  constexpr TimeType oneDay = oneHour * 24;
  constexpr TimeType oneYear = oneDay * 365;

  auto getVal = [&](const TimeType timePeriod) {
    unsigned int val = millis / timePeriod;
    millis %= timePeriod;
    return val;
  };

  const unsigned int years = getVal(oneYear);
  const unsigned int days = getVal(oneDay);
  const unsigned int hours = getVal(oneHour);
  const unsigned int minutes = getVal(oneMinute);
  unsigned int seconds = getVal(oneSecond);

  char *pBuf = buf;

  auto printVal = [&](const unsigned int val, const char spec, const FmtMillisFlags flag) {
    if ((flags & flag) != flag || !val) return;
    int retVal = /*std::*/ snprintf(pBuf, size, "%u%c ", val, spec);
    if (retVal <= 0) return;
    size -= retVal;
    pBuf += retVal;
  };

  printVal(years, 'y', FMT_YEARS);
  printVal(days, 'd', FMT_DAYS);
  printVal(hours, 'h', FMT_HOURS);
  printVal(minutes, 'm', FMT_MINUTES);
  printVal(seconds, 's', FMT_SECONDS);

  if (pBuf == buf) strxcpy(buf, (flags & FMT_SECONDS) == FMT_SECONDS ? "0s" : "-", size);
  else pBuf[-1] = '\0';

  return buf;
}

/* http://stackoverflow.com/a/19865296/1392778 */

time_t mkgmtime(struct tm *tm) {
  auto isLeapYear = [](int year) {
    year += 1900;
    return (year % 4) == 0 && ((year % 100) != 0 || (year % 400) == 0);
  };

  static const int numDays[2][12] = {
    {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
    {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
  };

  time_t result = 0;

  for (int i = 70; i < tm->tm_year; ++i) result += isLeapYear(i) ? 366 : 365;

  for (int i = 0; i < tm->tm_mon; ++i) result += numDays[isLeapYear(tm->tm_year)][i];

  result += tm->tm_mday - 1;
  result *= 24;
  result += tm->tm_hour;
  result *= 60;
  result += tm->tm_min;
  result *= 60;
  result += tm->tm_sec;

  return result;
}

//
// Checksum
//

uint32_t crc32b(const void *data, const size_t dataLength) {
  uint32_t crc = 0xFFFFFFFF;
  uint32_t mask;
  for (size_t i = 0; i < dataLength; ++i) {
    crc = crc ^ (reinterpret_cast<const unsigned char *>(data))[i];
    for (size_t j = 8; j > 0; --j) {
      mask = -(crc & 1);
      crc = (crc >> 1) ^ (0xEDB88320 & mask);
    }
  }
  return ~crc;
}

//
// Compression
//

size_t compressBufSize(const size_t dataLength) {
  return compressBound(dataLength);
}

void *compress(void *buf, size_t &length, const void *data,
               const size_t dataLength, const int compressionLevel,
               const bool GZip) {
  Bytef *bufPtr = reinterpret_cast<Bytef *>(buf);

  z_stream zs{};

  zs.next_in = const_cast<Bytef *>(reinterpret_cast<const Bytef *>(data));
  zs.avail_in = dataLength;
  zs.next_out = bufPtr;
  zs.avail_out = length;

  if (GZip) {
    constexpr int windowBits = 15;
    constexpr int encoding = 16;
    constexpr int memLevel = 8;

    static_assert(windowBits <= MAX_WBITS, "");
    static_assert(memLevel <= MAX_MEM_LEVEL, "");

    if (deflateInit2(&zs, compressionLevel, Z_DEFLATED, windowBits + encoding, memLevel, Z_DEFAULT_STRATEGY) != Z_OK)
      return nullptr;
  } else {
    if (deflateInit(&zs, compressionLevel) != Z_OK)
      return nullptr;
  }

  if (deflate(&zs, Z_FINISH) != Z_STREAM_END) {
    deflateEnd(&zs);
    return nullptr;
  }

  if (deflateEnd(&zs) != Z_OK) return nullptr;
  length = zs.total_out;

  return buf;
}

//
// Random
//

uint64_t getRandomNumber() {
  std::srand(getNanoSeconds());
  return static_cast<uint64_t>(std::rand()) << 32 | std::rand();
}

//
// OS, Arch and Compiler info
//

const char *getOSName() {
#if defined(_WIN32)
  return "Windows";
#elif defined(__linux__)
  return "Linux";
#elif defined(__IOS__)
  return "iOS";
#elif defined(__APPLE__)
  return "Mac OS X";
#elif defined(__FreeBSD__)
  return "FreeBSD";
#elif defined(__NetBSD__)
  return "NetBSD";
#else
  return "Unknown";
#endif
}

const char **getCompilerInfo(const char *compilerInfo[2]) {
#ifdef _MSC_VER
  compilerInfo[0] = "Visual C++";
  switch (_MSC_VER) {
  case 1900:
    compilerInfo[1] = "2015";
    break;
  default:
    compilerInfo[1] = TOSTRING(_MSC_VER);
  }
#elif defined(__clang__)
  compilerInfo[0] = "clang";
  compilerInfo[1] = __clang_version__;
#elif defined(__INTEL_COMPILER)
  compilerInfo[0] = "icc";
  compilerInfo[1] = TOSTRING(__INTEL_COMPILER);
#elif defined(__GNUC__)
  compilerInfo[0] = "gcc";
  compilerInfo[1] = __VERSION__;
#else
  compilerInfo[0] = "unknown";
  compilerInfo[1] = "unknown";
#endif
  return compilerInfo;
}

//
// Misc
//

void SharedMutex::lock() {
  switch (type) {
  case STD_MUTEX:
    getMutex<std::mutex>()->lock();
    break;
  case STD_SHARED_TIMED_MUTEX:
#ifdef SHARED_MUTEX_SUPPORTED
    getMutex<std::shared_timed_mutex>()->lock();
#else
    std::abort();
#endif
    break;
  }
}
void SharedMutex::unlock() {
  switch (type) {
  case STD_MUTEX:
    getMutex<std::mutex>()->unlock();
    break;
  case STD_SHARED_TIMED_MUTEX:
#ifdef SHARED_MUTEX_SUPPORTED
    getMutex<std::shared_timed_mutex>()->unlock();
#else
    std::abort();
#endif
    break;
  }
}

void SharedMutex::lock_shared() {
  switch (type) {
  case STD_MUTEX:
    getMutex<std::mutex>()->lock();
    break;
  case STD_SHARED_TIMED_MUTEX:
#ifdef SHARED_MUTEX_SUPPORTED
    getMutex<std::shared_timed_mutex>()->lock_shared();
#else
    std::abort();
#endif
    break;
  }
}

void SharedMutex::unlock_shared() {
  switch (type) {
  case STD_MUTEX:
    getMutex<std::mutex>()->unlock();
    break;
  case STD_SHARED_TIMED_MUTEX:
#ifdef SHARED_MUTEX_SUPPORTED
    getMutex<std::shared_timed_mutex>()->unlock_shared();
#else
    std::abort();
#endif
    break;
  }
}

SharedMutex::SharedMutex() {
#ifdef SHARED_MUTEX_SUPPORTED
  mutex = new std::shared_timed_mutex;
  type = STD_SHARED_TIMED_MUTEX;
#else
  mutex = new std::mutex;
  type = STD_MUTEX;
#endif
}

SharedMutex::~SharedMutex() {
  switch (type) {
  case STD_MUTEX:
    delete getMutex<std::mutex>();
    break;
  case STD_SHARED_TIMED_MUTEX:
#ifdef SHARED_MUTEX_SUPPORTED
    delete getMutex<std::shared_timed_mutex>();
#else
    std::abort();
#endif
    break;
  }
}

Version Version::parse(const char *versionStr) {
  Version version;
  version.major = atoi(versionStr);
  while (*versionStr && *versionStr++ != '.');
  if (!*versionStr) return version;
  version.minor = atoi(versionStr);
  while (*versionStr && *versionStr++ != '.');
  if (!*versionStr) return version;
  version.patch = atoi(versionStr);
  return version;
}

#ifndef _MSC_VER
//#warning MVSC mkdir issue
#endif

bool init() {
  if (dirExists(TMP_DIR, nullptr) || !mkdir(TMP_DIR, 0700)) return true;
  err << "failed to create temporary directory " << "'" << TMP_DIR << "'" << err.endl();
  return false;
}

void deinit() {}

} // namespace tools
