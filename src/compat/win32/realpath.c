#include <stdlib.h>
#include "compat.h"

char *realpath(const char *path, char *resolved) {
  return _fullpath(resolved, path, PATH_MAX);
}
