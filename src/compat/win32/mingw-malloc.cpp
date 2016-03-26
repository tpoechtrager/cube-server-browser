#ifdef _MSC_VER

#include <cstdlib>
#include <windows.h>

namespace {
  void *mallocAddr;
  void *freeAddr;

  struct Init {
    HMODULE msvcrt;
    Init() : msvcrt(LoadLibrary("MSVCRT.dll")) {
      if (!msvcrt) std::abort();
      mallocAddr = GetProcAddress(msvcrt, "malloc");
      freeAddr = GetProcAddress(msvcrt, "free");
      if (!mallocAddr || !freeAddr) std::abort();
    }
    ~Init() {
      FreeLibrary(msvcrt);
    }
  } init;
} // anonymous namespace

extern "C" void *mingw_malloc(size_t size) {
  return (decltype(&malloc)(mallocAddr))(size);
}

extern "C" void mingw_free(void *ptr) {
  (decltype(&free)(freeAddr))(ptr);
}

#endif
