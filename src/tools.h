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

#ifndef __TOOLS_H__
#define __TOOLS_H__

#include <cstdlib>
#include <cstring>
#include <cstddef>
#include <cstdint>
#include <cinttypes>
#include <limits>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <mutex>

#include "3rd/itostr.h"

#ifdef __APPLE__
#include <Availability.h>
#endif

#ifdef _WIN32
#include "compat/win32/compat.h"
#include <windows.h>
#endif

#if defined(PLUGIN) && defined(_MSC_VER)
#define PLUGIN_IMPORT __declspec(dllimport)
#else
#define PLUGIN_IMPORT
#endif

struct stat;

//
// Compiler feature detection
//

#define GCC_VERSION_AT_LEAST(major, minor, patch)                              \
  (defined(__GNUC__) &&                                                        \
   (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__) >=          \
       (major * 10000 + minor * 100 + patch))

#define CLANG_VERSION_AT_LEAST(major, minor, patch)                            \
  (defined(__clang__) && !defined(__apple_build_version__) &&                  \
   (__clang_major__ * 10000 + __clang_minor__ * 100 + __clang_patchlevel__) >= \
       (major * 10000 + minor * 100 + patch))

#define APPLE_CLANG_VERSION_AT_LEAST(major, minor, patch)                      \
  (defined(__clang__) && defined(__apple_build_version__) &&                   \
   (__clang_major__ * 10000 + __clang_minor__ * 100 + __clang_patchlevel__) >= \
       (major * 10000 + minor * 100 + patch))

#define ICC_VERSION_AT_LEAST(major, minor, patch)                              \
  (defined(__INTEL_COMPILER) &&                                                \
   ((__INTEL_COMPILER * 10000 + __INTEL_COMPILER_UPDATE) >=                    \
    ((major * 100 + minor) * 10000) + patch))

#ifndef __has_include
#define __has_include(x) 0
#endif

#if __cplusplus > 201103
#define CXX14
#endif

// Workaround for MinGW GCC '-fno-use-linker-plugin' issues

#if defined(__MINGW32__) && defined(__OPTIMIZE__) && !defined(__clang__)
#define MINGW_LTO_BUG __attribute__((noinline))
#else
#define MINGW_LTO_BUG
#endif

// Block broken compilers

#if defined(_MSC_FULL_VER) && _MSC_FULL_VER < 190023026
#error The RC versions of Visual Studio 2015 are known to miscompile this\
 project; please upgrade to the RTM version!
#endif

// Implicit list-initialization constructor used in function calls

#if defined(__clang__) || GCC_VERSION_AT_LEAST(5, 0, 0)
#define PROPER_LIST_INITIALIZATION_SUPPORTED
#else

#include <initializer_list>

template <typename T>
constexpr T getListElement(const std::initializer_list<T> list, const size_t index) {
  return list.size() > index ? list.begin()[index] : T();
}

#endif

// Thread-local storage

#ifndef NO_TLS_SUPPORT

#if GCC_VERSION_AT_LEAST(4, 8, 0) && (defined(__i386__) || defined(__x86_64__))
#define TLS_SUPPORTED
#endif

#ifdef _MSC_VER
#define TLS_SUPPORTED
#endif

#if ICC_VERSION_AT_LEAST(15, 0, 0) && (defined(__APPLE__) || defined(__linux__))
#define TLS_SUPPORTED
#endif

#if CLANG_VERSION_AT_LEAST(3, 3, 0) &&                                         \
    (defined(__linux__) &&                                                     \
     (defined(__i386__) || defined(__x86_64__) ||                              \
      (defined(__APPLE__) && MAC_OS_X_VERSION_MIN_REQUIRED >= 1070)))
// TODO: __linux__ && (__i386__ || __x86_64__) is too inaccurate
#define TLS_SUPPORTED
#endif

#endif

#ifndef TLS_SUPPORTED

#if defined(__clang__) && CLANG_VERSION_AT_LEAST(3, 6, 0)
#pragma clang diagnostic ignored "-Wkeyword-macro"
#endif

#define thread_local

#endif

// Shared mutex

#if defined(CXX14) && (GCC_VERSION_AT_LEAST(4, 9, 0) || defined(_MSC_VER) ||   \
                       __has_include(<shared_mutex>)) &&                       \
    (!defined(__APPLE__) || defined(__GLIBCXX__))
#include <shared_mutex>
#define SHARED_MUTEX_SUPPORTED
#endif

// Inheriting constructors

#if CLANG_VERSION_AT_LEAST(3, 3, 0) || GCC_VERSION_AT_LEAST(4, 8, 0) ||        \
    defined(_MSC_VER) || ICC_VERSION_AT_LEAST(15, 0, 0)
#define INHERITING_CONSTRUCTORS_SUPPORTED
#endif

namespace tools {

//
// Misc
//

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#define CONC_(x, y) x##_##y
#define CONC(x, y) CONC_(x, y)

#define sizeofarray(arr) (sizeof(arr) / sizeof(*arr))

#define returnArrayValueOrBreak(array, _index)                                                                         \
  int index = _index;                                                                                                  \
  if (index >= 0 && index < static_cast<int>(sizeofarray(array)))   return array[index];                               \
  break

// Shared Mutex C++11 <-> C++14 Compatibility class

class SharedMutex {
private:
  enum Type {
    STD_MUTEX,
    STD_SHARED_TIMED_MUTEX
  };

  void *mutex;
  Type type;

  template <typename T> T *getMutex() { return static_cast<T*>(mutex); }

public:
  void lock();
  void unlock();
  void lock_shared();
  void unlock_shared();

  SharedMutex();
  ~SharedMutex();
};

#ifdef SHARED_MUTEX_SUPPORTED
template <typename T> void lockShared(T &m) { return m.lock_shared(); }
template <typename T> void unlockShared(T &m) { return m.unlock_shared(); }
#else
template <typename T> void lockShared(T &m) { return m.lock(); }
template <typename T> void unlockShared(T &m) { return m.unlock(); }
#endif

template <typename T, bool inverse = false>
class LockGuard {
private:
  T data;

public:
  LockGuard(T data) : data(data) {
    if (!data) return;
    if (!inverse) data->lock();
    else data->unlock();
  }

  ~LockGuard() {
    if (!data) return;
    if (!inverse) data->unlock();
    else data->lock();
  }
};

#define LockGuard(data) \
  LockGuard<decltype(data)> CONC(__lg, __COUNTER__)(data)

#define UnlockGuard(data) \
  LockGuard<decltype(data), true> CONC(__ulg, __COUNTER__)(data)

#ifdef SHARED_MUTEX_SUPPORTED
template <typename T>
struct SharedLockGuardImpl {
private:
  T data;
public:
  void lock() {
    if (data) data->lock_shared();
  }

  void unlock() {
    if (data) data->unlock_shared();
  }

  SharedLockGuardImpl(T data) : data(data) { }
};

#define SharedLockGuard(data)                                                  \
  SharedLockGuardImpl<decltype(data)> CONC(__slg_tmp, __LINE__)(data);         \
  LockGuard<SharedLockGuardImpl<decltype(data)> *>                             \
      CONC(__slg, __COUNTER__)(&CONC(__slg_tmp, __LINE__))

#define SharedUnlockGuard(data)                                                \
  SharedLockGuardImpl<decltype(data)> CONC(__sulg_tmp, __LINE__)(data);        \
  LockGuard<SharedLockGuardImpl<decltype(data, true)> *>                       \
      CONC(__sulg, __COUNTER__)(&CONC(__sulg_tmp, __LINE__))
#else
#define SharedLockGuard LockGuard
#define SharedUnlockGuard UnlockGuard
#endif

#ifdef _WIN32
constexpr char PATH_DIV = '\\';
#else
constexpr char PATH_DIV = '/';
#endif

static_assert(sizeof(int) == 4, "");

template <typename T> int32_t shrinkTo32BitSignedInteger(const T &val) {
  return val & 0xFFFFFFFF;
}

template <typename T> uint32_t shrinkTo32BitUnsignedInteger(const T &val) {
  return val & 0xFFFFFFFFu;
}

template <typename T> T clamp(const T val, const T valMin, const T valMax) {
  return std::max(valMin, std::min(val, valMax));
}

template <typename T> class ExtVar {
private:
  bool valAssigned = false;
  T val;

public:
  void reset() { valAssigned = false; }
  void operator=(const T &newVal) {
    val = newVal;
    valAssigned = true;
  }
  bool isSet() const { return valAssigned; }
  T getVal() const { return val; }
  T operator*() const { return getVal(); }
};

template<typename T>
class ObjPointer {
private:
  const T *ptr;

public:
  const T *operator->() const { return ptr; }

  ObjPointer(T &obj) : ptr(&obj) {}
  ObjPointer(T *obj) : ptr(obj) {}

  ObjPointer(const T &obj) : ptr(&obj) {}
  ObjPointer(const T *obj) : ptr(obj) {}

  template <typename X>
  ObjPointer(const X &data) : ptr(data.obj) {}
};

#undef major
#undef minor
#undef patch

struct Version {
  constexpr Version(int major, int minor, int patch = 0) : major(major), minor(minor), patch(patch) {}
  constexpr Version() : major(), minor(), patch() {}

  constexpr int num() const {
    return major * 10000 + minor * 100 + patch;
  };

  constexpr bool operator>(const Version &version) const {
    return num() > version.num();
  }

  constexpr bool operator>=(const Version &version) const {
    return num() >= version.num();
  }

  constexpr bool operator<(const Version &version) const {
    return num() < version.num();
  }

  constexpr bool operator<=(const Version &version) const {
    return num() <= version.num();
  }

  constexpr bool operator==(const Version &version) const {
    return num() == version.num();
  }

  constexpr bool operator!=(const Version &version) const {
    return num() != version.num();
  }

  static Version parse(const char *versionStr);

  int major;
  int minor;
  int patch;
};

bool init();
void deinit();

//
// Strings
//

typedef char ShortString[60];

class CString {
protected:
  const char *str;
  size_t strLength;
 
  static constexpr size_t UNSET = std::numeric_limits<size_t>::max();

public:
  const char *operator*() const { return str; }

  const char *operator++() {
    if (strLength != UNSET) --strLength;
    return ++str;
  }

  const char *operator++(int) {
    const char *tmp = str;
    ++*this;
    return tmp;
  }

  void operator+=(size_t val) {
    if (strLength != UNSET) strLength -= val;
    str += val;
  }

  size_t length() const {
    if (strLength != UNSET) return strLength;
    return str ? std::strlen(str) : 0;
  }

  bool empty() const { return !str || !length(); }

  CString() : str(), strLength(UNSET) {}
  CString(const char *str, const size_t length = UNSET) : str(str), strLength(length) {}
  template <typename T>
  CString(const T &str) : str(str.c_str()), strLength(str.length()) {}
};

struct CString2 : CString {
  CString2(const CString &str) : CString(*str, str.length()) {}
  CString2(const char *str) : CString(str, str ? std::strlen(str) : 0) {}
  CString2(const char *str, const size_t length) : CString(str, length) {}

  template <typename T>
  CString2(const T &str) : CString(str.c_str(), str.length()) {}
};

constexpr size_t xstrlen(const char *str) {
  return *str ? 1 + xstrlen(str + 1) : 0;
}

// Use this function only when a function call to std::strcmp()
// would be too expensive.

inline bool streq(const char *str1, const char *str2) {
  while (*str1 == *str2) {
    if (!*str1) break;
    ++str1;
    ++str2;
  }
  return *str1 == *str2;
}

inline char *strxcpy(char *dst, const CString &src, size_t size) {
  const size_t length = src.length();
  std::memcpy(dst, *src, std::min(size, length + 1));
  dst[size - 1] = '\0';
  return dst;
}

size_t parseMsg(const char *msg, std::vector<std::string> &args, const bool stripEmpty = false);
static const auto &parseString = parseMsg;

// This is a lot faster than std::stringstream.

struct FString : public std::string {
  FString &operator<<(char *val) {
    append(val);
    return *this;
  }
  FString &operator<<(const char *val) {
    append(val);
    return *this;
  }
  FString &operator<<(const char val) {
    *this += val;
    return *this;
  }
  FString &operator<<(const CString &val) {
    append(*val, val.length());
    return *this;
  }
  FString &operator<<(const std::string &val) {
    append(val);
    return *this;
  }
  FString &operator<<(const FString &val) {
    append(val);
    return *this;
  }
  template <typename T> FString &operator<<(const T &val) {
    itostr::itostr(*this, val);
    return *this;
  }
  template <typename T>
  FString(const T &str) : std::string(str) {}
  FString() {}
};

inline const char *toString(const char *val, std::string &) {
  return val;
}

inline const char *toString(const std::string &val, std::string &) {
  return val.c_str();
}

inline const char *toString(const float &val, std::string &buf) {
  char str[32];
  if (/*std::*/ snprintf(str, sizeof(str), "%.2f", val) > 0) buf = str;
  else buf.clear();
  return buf.c_str();
}

template <typename T>
const char *toString(const T val, std::string &buf) {
  buf.clear();
  itostr::itostr(buf, val);
  return buf.c_str();
}

constexpr const char *INVALID_FORMAT_STRING = " [invalid format string]";

template <typename STR>
bool fmtString(STR &buf, const char *fmt) {
  while (*fmt) {
    if (*fmt == '%') {
      if (fmt[1] == '%') {
        ++fmt;
      } else {
        buf << INVALID_FORMAT_STRING;
        return false;
      }
    }
    buf << *fmt++;
  }
  return true;
}

template <typename STR, typename VAL, typename... ARGS>
bool fmtString(STR &buf, const char *fmt, const VAL &val,
               ARGS&&... args) {
  while (*fmt) {
    if (*fmt == '%') {
      if (fmt[1] != '%') {
        if (fmt[1] == '\\' && fmt[2] == '%') ++fmt;
        buf << val;
        return fmtString(buf, fmt + 1, args...);
      } else {
        ++fmt;
      }
    }
    buf << *fmt++;
  }
  buf << INVALID_FORMAT_STRING;
  return false;
}

template <typename T = FString> class tmpAppend {
private:
  T &obj;
  const size_t prevLength;

public:
  const char *operator*() { return obj.c_str(); }
  T *operator->() const { return &obj; }

  template <typename S>
  tmpAppend(T &obj, const S &str) : obj(obj), prevLength(obj.length()) {
    obj += str;
  }

  ~tmpAppend() { obj.resize(prevLength); }
};

#define TmpAppend(obj, str)                                                    \
  tmpAppend<decltype(obj)> CONC(__tmpAppend, __COUNTER__)(obj, str)

template <typename T>
constexpr const char *plural(const T val) {
  return val == 1 ? "" : "s";
}

enum IntCmpOp : int {
  INTCMP_OP_GREATER_THAN,
  INTCMP_OP_GREATER_EQUAL,
  INTCMP_OP_LOWER_THAN,
  INTCMP_OP_LOWER_EQUAL,
  INTCMP_OP_EQUAL,
  INTCMP_OP_NOT_EQUAL,
  INTCMP_OP_DEFAULT = INTCMP_OP_GREATER_EQUAL
};

class IntCmp {
private:
  IntCmpOp op;
  int val;

public:
  bool match(const int &inVal) const {
    switch (op) {
    case INTCMP_OP_GREATER_THAN:
      return inVal > val;
    case INTCMP_OP_GREATER_EQUAL:
      return inVal >= val;
    case INTCMP_OP_LOWER_THAN:
      return inVal < val;
    case INTCMP_OP_LOWER_EQUAL:
      return inVal <= val;
    case INTCMP_OP_EQUAL:
      return inVal == val;
    case INTCMP_OP_NOT_EQUAL:
      return inVal != val;
    default:
      __builtin_unreachable();
    }
  }

  static IntCmpOp getOperator(CString &val_, const IntCmpOp fallback = INTCMP_OP_DEFAULT);

  void set(CString &val_, const bool parseOperator = true, const int fallback = 0);

  template <typename T>
  void set(const T &val, const bool parseOperator = true, const int fallback = 0) {
    CString tmp(val);
    set(tmp, parseOperator, fallback);
  }

  IntCmp() : op(INTCMP_OP_DEFAULT), val(std::numeric_limits<int>::min()) {}
  IntCmp(int val) : op(INTCMP_OP_DEFAULT), val(val) {}

  template <typename T>
  IntCmp(const T &val, const bool parseOperator = true, const int fallback = 0) {
    set(val, parseOperator, fallback);
  }
};

enum StrCmpOp : int {
  STRCMP_OP_CONTAINS_CASE_INSENSITIVE,
  STRCMP_OP_CONTAINS_NOT_CASE_INSENSITIVE,
  STRCMP_OP_CONTAINS,
  STRCMP_OP_CONTAINS_NOT,
  STRCMP_OP_EQUAL_CASE_INSENSITIVE,
  STRCMP_OP_NOT_EQUAL_CASE_INSENSITIVE,
  STRCMP_OP_EQUAL,
  STRCMP_OP_NOT_EQUAL,
  STRCMP_OP_DEFAULT_EQ = STRCMP_OP_CONTAINS_CASE_INSENSITIVE,
  STRCMP_OP_DEFAULT_NEQ = STRCMP_OP_CONTAINS_NOT_CASE_INSENSITIVE
};

class StrCmp {
private:
  StrCmpOp op;
  CString val;
  bool valAssigned;

public:
  bool match(const CString &inVal) const;
  bool isSet() const { return valAssigned; }
  size_t valLength() const { return val.length(); }

  static StrCmpOp getOperator(CString2 &val, const StrCmpOp fallback = STRCMP_OP_DEFAULT_EQ);

  void setOperator(const StrCmpOp op_) { op = op_; }

  void set(CString2 &val_, const bool parseOperator = true, const CString &fallback = "");

  template <typename T>
  void set(const T &val, const bool parseOperator = true, const CString &fallback = "") {
    CString2 tmp(val);
    set(tmp, parseOperator, fallback);
  }

  StrCmp() : op(STRCMP_OP_DEFAULT_EQ), valAssigned() {}

  template <typename T>
  StrCmp(const T &val, const bool parseOperator = true, const CString &fallback = "") {
    set(val, parseOperator, fallback);
  }

  template <typename T>
  StrCmp(const T &val, const StrCmpOp op, const CString &fallback = "") {
    set(val, false, fallback);
    setOperator(op);
  }
};

enum BindType {
  CONSTCHAR,
  INTCMP,
  STRCMP
};

template <size_t size = 0> struct ArgParser {
  const struct Bind {
    const CString2 name;
    const BindType type;
    void *data;
  } binds[size];

  bool parseArg(const CString &arg) {
    for (size_t i = 0; i < size; ++i) {
      const Bind &bind = binds[i];

      if (!strncasecmp(*arg, *bind.name, bind.name.length())) {
        const char *argVal = *arg + bind.name.length();

        switch (bind.type) {
        case BindType::CONSTCHAR:
          *static_cast<const char **>(bind.data) = argVal;
          break;
        case BindType::INTCMP:
          static_cast<IntCmp*>(bind.data)->set(argVal);
          break;
        case BindType::STRCMP:
          static_cast<StrCmp*>(bind.data)->set(argVal);
          break;
        }

        return true;
      }
    }
    return false;
  }
};

bool readFile(const CString &file, std::string &buf);
bool writeFile(const CString &file, const std::string &content);

const char *convertCubeToUTF8(const CString &str, char *buf, const size_t size);
const char *convertUTF8ToCube(const CString &str, char *buf, const size_t size);

template <typename T, size_t N>
const char *convertCubeToUTF8(const CString &str, T (&buf)[N]) {
  return convertCubeToUTF8(str, buf, N);
}

template <typename T, size_t N>
const char *convertUTF8ToCube(const CString &str, T (&buf)[N]) {
  return convertUTF8ToCube(str, buf, N);
}

template <size_t maxLength = 1024, typename T>
void convertUTF8ToCube(const T &in, std::vector<std::string> &out) {
  for (const auto &str : in) {
    char buf[maxLength];
    out.push_back(convertUTF8ToCube(str, buf));
  }
}

template <size_t maxLength = 1024, typename T>
void convertCubeToUTF8(const T &in, std::vector<std::string> &out) {
  for (const auto &str : in) {
    char buf[maxLength];
    out.push_back(convertCubeToUTF8(str, buf));
  }
}

//
// Files & Directories
//

constexpr const char *TMP_DIR = ".tmp";

const char *getFileExtension(const CString &fileName);
bool fileExists(const CString &fileName, struct stat *st = nullptr);
static const auto &dirExists = &fileExists;

//
// Terminal text colors
//

bool isTerminal();

// http://stackoverflow.com/a/17469726

enum ColorCode {
  COLOR_NONE = 0,
  FG_DEFAULT = 39,
  FG_BLACK = 30,
  FG_RED = 31,
  FG_GREEN = 32,
  FG_YELLOW = 33,
  FG_BLUE = 34,
  FG_MAGENTA = 35,
  FG_CYAN = 36,
  FG_LIGHT_GRAY = 37,
  FG_DARK_GRAY = 90,
  FG_LIGHT_RED = 91,
  FG_LIGHT_GREEN = 92,
  FG_LIGHT_YELLOW = 93,
  FG_LIGHT_BLUE = 94,
  FG_LIGHT_MAGENTA = 95,
  FG_LIGHT_CYAN = 96,
  FG_WHITE = 97,
  BG_RED = 41,
  BG_GREEN = 42,
  BG_BLUE = 44,
  BG_DEFAULT = 49
};

class Color {
  ColorCode cc;

public:
  Color(ColorCode cc) : cc(cc) {}
  friend std::ostream &operator<<(std::ostream &os, const Color &color) {
    if (color.cc != COLOR_NONE && isTerminal()) return os << "\033[" << color.cc << "m";
    return os;
  }
};

//
// Log helper
//

//
// This class locks until EOL.
// err << "xyz"; // Will cause a deadlock
// err << "xyz" << err.endl(); // OK
//

template <bool flushOnLineEnd = false, bool noop = false> class Message {
protected:
  std::ostream &os;

private:
  const char *msg;
  Color color;
  bool printPrefix;

  virtual void lock() {}
  virtual void unlock() {}

  virtual void printTimeStamp() {}

public:
  friend class LockGuard<Message *>;

  static constexpr char endl() { return '\n'; }
  bool isEndl(char c) { return c == '\n'; }
  template <typename T> bool isEndl(T &&) { return false; }

  void flush() {
    LockGuard(this);
    os.flush();
  }

  template <typename T> Message &operator<<(T &&v) {
    if (noop) return *this;
    LockGuard(this);
    if (printPrefix) {
      lock();
      printTimeStamp();
      if (msg) os << color << msg << ": " << Color(FG_DEFAULT);
      printPrefix = false;
    }
    if (isEndl(v)) {
      printPrefix = true;
      os << std::endl;
      if (flushOnLineEnd) os.flush();
      unlock();
    } else {
      os << v;
    }
    return *this;
  }

  Message(std::ostream &os) : os(os), msg(), color(COLOR_NONE), printPrefix(true) {}
 
  Message(const char *msg, Color color = FG_RED, std::ostream &os = std::cerr)
    : os(os), msg(msg), color(color), printPrefix(true) {}

  virtual ~Message() {}
};

class MessageLocked : public Message<> {
private:
  std::recursive_mutex mutex;

  MINGW_LTO_BUG void lock() { mutex.lock(); }
  MINGW_LTO_BUG void unlock() { mutex.unlock(); }

public:
#ifdef INHERITING_CONSTRUCTORS_SUPPORTED
  using Message::Message;
#else
  MessageLocked(std::ostream &os) : Message(nullptr, COLOR_NONE, os) {}
  MessageLocked(const char *msg, Color color = FG_RED, std::ostream &os = std::cerr) : Message(msg, color, os) {}
#endif
};

class LogFileStream {
private:
  std::ofstream log;
  friend class LogFile;

public:
  LogFileStream(const char *file) : log(file, std::ofstream::app) {}
};

class LogFile : private LogFileStream, public MessageLocked {
private:
  void printTimeStamp();

public:
  LogFile(const char *file);
};

#ifdef NDEBUG
typedef Message<false, true> MessageDebug;
#else
typedef MessageLocked MessageDebug;
#endif

PLUGIN_IMPORT extern MessageLocked warn;
PLUGIN_IMPORT extern MessageLocked err;
PLUGIN_IMPORT extern MessageDebug dbg;
PLUGIN_IMPORT extern MessageLocked info;
PLUGIN_IMPORT extern MessageLocked warninfo;

//
// Time
//

typedef uint64_t TimeType;
typedef uint32_t TimeType32;

constexpr TimeType oneSecond = 1 * 1000;
constexpr TimeType oneMinute = 60 * 1000;
constexpr TimeType oneHour = 60 * 60 * 1000;
constexpr TimeType oneDay = 24 * 60 * 60 * 1000;

TimeType getNanoSeconds();

inline TimeType getMicroSeconds() { return getNanoSeconds() / 1000; }
inline TimeType getMilliSeconds() { return getMicroSeconds() / 1000; }

enum FmtMillisFlags : int {
  FMT_YEARS = 0x01,
  FMT_DAYS = 0x02,
  FMT_HOURS = 0x04,
  FMT_MINUTES = 0x08,
  FMT_SECONDS = 0x10,
  FMT_ALL = FMT_YEARS | FMT_DAYS | FMT_HOURS | FMT_MINUTES | FMT_SECONDS
};

const char *fmtMillis(TimeType millis, char *buf, size_t size,
                      const FmtMillisFlags flags = FMT_ALL);

template <typename T, size_t N>
const char *fmtMillis(const TimeType millis, T (&buf)[N], const FmtMillisFlags flags = FMT_ALL) {
  return fmtMillis(millis, buf, N, flags);
}

inline
const char *fmtSeconds(const TimeType seconds, char *buf, size_t size, const FmtMillisFlags flags = FMT_ALL) {
  return fmtMillis(seconds * 1000, buf, size, flags);
}

template <typename T, size_t N>
const char *fmtSeconds(const TimeType seconds, T (&buf)[N], const FmtMillisFlags flags = FMT_ALL) {
  return fmtSeconds(seconds, buf, N, flags);
}

time_t mkgmtime(struct tm *tm);

class benchmark {
public:
  TimeType getDiff() { return getTime() - baseTime; }

  void halt() { haltTime = getTime(); }
  void resume() { baseTime += getTime() - haltTime; }

  benchmark(const char *description = "") : description(description) {
    baseTime = getTime();
  }

  ~benchmark() {
    TimeType diff = getTime() - baseTime;
    info << description << " took: " << diff / 1000000.0 << " ms" << info.endl();
  }

private:
  TimeType getTime() { return getNanoSeconds(); }

  const char *description;
  TimeType haltTime;
  TimeType baseTime;
};

#define benchmark(...)                                                         \
  benchmark CONC(__benchmark, __COUNTER__) { __VA_ARGS__ }

//
// Checksum
//

uint32_t crc32b(const void *data, const size_t dataLength);

//
// Compression
//

size_t compressBufSize(const size_t dataLength);

void *compress(void *buf, size_t &length, const void *data,
               const size_t dataLength, const int compressionLevel,
               const bool GZip = false);

//
// Random
//

uint64_t getRandomNumber();

//
// OS and Compiler version
//

#ifdef __ENVIRONMENT_IPHONE_OS_VERSION_MIN_REQUIRED__
#define __IOS__
#endif

const char *getOSName();
const char **getCompilerInfo(const char *compilerInfo[2]);

} // namespace tools

using namespace tools;

#endif //__TOOLS_H__
