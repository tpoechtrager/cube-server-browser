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

#ifndef __WINDOWS_COMPAT_H__
#define __WINDOWS_COMPAT_H__

#ifdef __cplusplus
#include <cstdlib>
#include <ctime>
#include <cstdint>
#else
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#endif

#include <windows.h>
#include <direct.h>
#include <shlwapi.h>

#undef StrCmp

#ifndef PATH_MAX
#define PATH_MAX MAX_PATH
#endif

#ifndef SIGUSR1
#define SIGUSR1 10
#endif

#ifndef SIGUSR2
#define SIGUSR2 12
#endif

#ifndef SIGPIPE
#define SIGPIPE 13
#endif

#ifndef S_ISREG
#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#endif

#ifdef _MSC_VER

#ifdef __cplusplus
#include <cstdlib>
#else
#include <stdlib.h>
#endif

typedef intptr_t ssize_t;
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#define __builtin_bswap32 _byteswap_ulong
#define __builtin_unreachable() __assume(0)
#define __builtin_trap() abort()

#endif

#define getcwd _getcwd
#define strcasestr StrStrI

#ifdef __cplusplus
extern "C" {
#endif

char *strptime(const char *s, const char *format, struct tm *tm);
char *realpath(const char *path, char *resolved);

#ifdef _MSC_VER
int usleep(uint32_t usec);
void *mingw_malloc(size_t size);
void mingw_free(void *ptr);
#else
#define mingw_malloc malloc
#define mingw_free free
#endif

inline struct tm *gmtime_r_(const time_t *time, struct tm *tm) {
  if (gmtime_s(tm, time)) return NULL;
  return tm;
}

inline struct tm *localtime_r_(const time_t *time, struct tm *tm) {
  if (localtime_s(tm, time))return NULL;
  return tm;
}

inline int mkdir_(const char *path, int mode) {
  (void) mode;
  return mkdir(path);
}

#define gmtime_r gmtime_r_
#define localtime_r localtime_r_
#define mkdir mkdir_

#ifdef __cplusplus
}
#endif

#endif /* __WINDOWS_COMPAT_H__ */
