#ifdef _MSC_VER

#include <windows.h>
#include <stdint.h>
#include <errno.h>
#include "compat.h"

int usleep(uint32_t usec) {
  if (usec >= 1000000) {
    errno = EINVAL;
    return -1;
  }

  Sleep(usec / 1000);

  return 0;
}

#endif
