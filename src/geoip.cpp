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

#include <cstring>
#include <mutex>
#include "network.h"
#include "main.h"
#include "geoip.h"

#include <GeoIP.h>

namespace geoip {

namespace {
constexpr int CHARSET = GEOIP_CHARSET_ISO_8859_1;

bool memoryCache;

GeoIP *db;
std::mutex dbMutex;
} // anonymous namespace

#define setCountryAndReturn(val)                                                                                       \
  do {                                                                                                                 \
    if (size >= sizeof(const char *)) country[0] = val ? GeoIP_country_name[id] : nullptr;                             \
    if (size >= sizeof(const char * [2])) country[1] = val ? GeoIP_country_code[id] : nullptr;                         \
    return val;                                                                                                        \
  } while (0)

bool country(uint32_t ip, const char **country, const size_t size) {
  LockGuard(&dbMutex);
  int id = GeoIP_id_by_ipnum(db, network::hostToNet(ip));
  if (id < 0 || GeoIP_country_code[id][0] == '-') setCountryAndReturn(false);
  setCountryAndReturn(true);
}

bool country(const char *countryCode, const char **country, const size_t size) {
  size_t id = 0;
  if (countryCode[0] == '-') setCountryAndReturn(false);
  for (; id < sizeofarray(GeoIP_country_code); ++id) {
    const char *countryCodePtr = GeoIP_country_code[id];
    if (!countryCodePtr) break;
    if (!std::memcmp(countryCodePtr, countryCode, 2)) setCountryAndReturn(true);
  }
  setCountryAndReturn(false);
}

bool init() {
  LockGuard(&dbMutex);
  FString dbFile;
  dbFile << DATA_DIR << PATH_DIV << "GeoIP.dat";
  memoryCache = cfg->getBool("geoip.enableMemoryCache", true);
  const int flags = memoryCache ? GEOIP_MEMORY_CACHE : GEOIP_STANDARD;
  if (!(db = GeoIP_open(dbFile.c_str(), flags)))
#ifndef _WIN32
    db = GeoIP_open("/usr/share/GeoIP/GeoIP.dat", flags);
#else
    do {} while (0);
#endif
  if (!db) {
    err << "cannot open " << dbFile << err.endl();
    return false;
  }
  db->charset = CHARSET;
  return true;
}

void deinit() {
  LockGuard(&dbMutex);
  if (db) {
    GeoIP_delete(db);
    db = nullptr;
  }
  // GeoIP_cleanup();
}

} // namespace geoip
