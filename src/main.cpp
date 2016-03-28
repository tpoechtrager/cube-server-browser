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
#include <cstdio>
#include <csignal>
#include <iostream>
#include <atomic>
#include "network.h"
#include "geoip.h"
#include "extinfo.h"
#include "plugin.h"
#include "main.h"
#include "tools.h"

#ifndef _WIN32
#include <unistd.h>
#endif

config::Config *cfg;
LogFile *logFile;

const char *getApplicationName() { return "Cube Server Browser"; }
const char *getApplicationNameLC() { return "cube server browser"; }
const char *getApplicationConfigFileName() { return "cube_server_browser.cfg"; }
const char *getApplicationLogFileName() { return "cube_server_browser.log"; }
const char *getApplicationLicense() { return "AGPLv3"; }

namespace {

std::atomic_bool shutdownRequest;
std::atomic_bool reloadRequest;
std::atomic_bool reloadGeoIP;

void signalHandler(int signal) {
  switch (signal) {
  case SIGUSR1:
    reloadRequest = true;
    break;
  case SIGUSR2:
    reloadGeoIP = true;
    break;
  default:
    shutdownRequest = true;
  }
}

} // anonymous namespace

void shouldShutdown() { shutdownRequest = true; }
void shouldReload() { reloadRequest = true; }

int main() {
  int retVal;
  std::string fileName = getApplicationNameLC();

#ifndef _WIN32
  bool child = false;
  bool once = false;
#endif

  do {
    reloadRequest = false;

    cfg = new config::Config(getApplicationConfigFileName());
    logFile = new LogFile(cfg->getString("logFile", getApplicationLogFileName()));

#ifndef _WIN32
    if (!child && cfg->getBool("runAsDaemon", false)) {
      info << "*** forking to background ***" << info.endl();

      switch (fork()) {
      case 0:
//#warning this is wrong
        child = true;
        break;
      case -1:
        err << "fork() failed" << err.endl();
        break;
      default:
        _exit(EXIT_SUCCESS);
      }
    }
#endif

    if (tools::init() && network::init() && geoip::init() && extinfo::init() && plugin::init()) {

#ifndef _WIN32
      if (child && !once) {
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
        once = true;
      }
#endif

      signal(SIGINT, signalHandler);
      signal(SIGTERM, signalHandler);
      signal(SIGUSR1, signalHandler);
      signal(SIGUSR2, signalHandler);
      signal(SIGPIPE, SIG_IGN);

      *logFile << "*** " << getApplicationNameLC() << " started ***" << logFile->endl();

      while (!shutdownRequest && !reloadRequest) {
        extinfo::process();
        plugin::process();

        if (reloadGeoIP) {
          *logFile << "*** reloading GeoIP database ***" << logFile->endl();

          reloadGeoIP = false;
          geoip::deinit();

          if (!geoip::init()) {
            retVal = 1;
            break;
          }
        }
      }

      *logFile << "*** received shutdown request ***" << logFile->endl();
      retVal = 0;
    } else {
      err << "init failed (please check your configuration!)" << err.endl();
      retVal = 1;
    }

    plugin::deinit();
    extinfo::deinit();
    geoip::deinit();
    network::deinit();
    tools::deinit();

    delete cfg;
    delete logFile;

#ifdef _WIN32
    if (retVal) getchar();
#endif
  } while (!retVal && reloadRequest);

  return retVal;
}
