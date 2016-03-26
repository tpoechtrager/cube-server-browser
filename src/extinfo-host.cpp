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

#include "main.h"
#include "geoip.h"
#include "extinfo.h"
#include "extinfo-internal.h"
#include <cassert>
#include <sys/stat.h>

#ifndef _MSC_VER
#include <unistd.h>
#endif

namespace extinfo {

size_t ExtInfoHost::getPlayerCount() const {
  size_t numPlayers = 0;

  for (const Server *server : servers) {
    if (!server->infoOK) continue;
    numPlayers += server->numPlayers;
  }

  return numPlayers;
}

void ExtInfoHost::findPlayer(const FindPlayer &findPlayer, FindPlayerCallback callback, void *callbackData) const {
  for (const Server *server : servers) {
    if (!server->infoOK) continue;

    for (const Player &player : server->players) {
      // Always check integer variables before
      // string variables, StrCmp (class) may invoke
      // invoke function calls.

      if (!findPlayer.cn.match(player.cn) || !findPlayer.frags.match(player.frags) ||
          !findPlayer.deaths.match(player.deaths) || !findPlayer.accuracy.match(player.accuracy)) continue;

      if (findPlayer.country.isSet()) {
        const char *playerCountry = player.info.getCountry(findPlayer.country.valLength() <= 2);
        if (!findPlayer.country.match(playerCountry)) continue;
      }
  
      if (!findPlayer.name.match(player.getName())) continue;

      callback(server, player, callbackData);
    }
  }
}

const Server *ExtInfoHost::findServer(network::Address address, bool extInfoPort) const {
  if (!extInfoPort) address.port += info.infoPortOffset;
  for (const Server *server : servers) if (network::addressEqual(server->address, address)) return server;
  return nullptr;
}

int ExtInfoHost::addServer(const char *serverHost, const network::Address &address, bool persist) {
  Server *server = const_cast<Server *>(findServer(address, false));

  if (server) {
    server->shouldBeDeleted = false;
    server->serverHost = serverHost;
    if (persist) server->persist = true;
    return 2;
  }

  if (servers.size() >= MAX_SERVERS) return 0;

  server = new Server{persist, this, serverHost, address};
  geoip::country(address.host, server->country, sizeof(server->country));
  server->address.port += info.infoPortOffset;
  for (uint64_t &randomNumber : server->randomNumbers) randomNumber = getRandomNumber();
  event(SERVER_ADD, {server});
  servers.push_back(server);

  return 1;
}

void ExtInfoHost::deleteServer(decltype(servers)::iterator server) {
  (*server)->deleteAllPlayers();
  event(SERVER_DELETE, {*server});
  delete *server;
  servers.erase(server);
}

size_t ExtInfoHost::deleteOrphanedServers() {
  size_t numDeletedServers = 0;

  for (size_t i = servers.size(); i-- > 0;) {
    Server *server = servers[i];
    if (!server->shouldBeDeleted) continue;
    deleteServer(servers.begin() + i);
    ++numDeletedServers;
  }

  return numDeletedServers;
}

void ExtInfoHost::markAllNonPersistServersForDeletion() {
  for (Server *server : servers) if (!server->persist) server->shouldBeDeleted = true;
}

bool ExtInfoHost::shouldUpdateFromMaster() const {
  return !masterUpdateThread &&
          ( !masterUpdateQueue.empty() || ( !lastMasterUpdate ||
                                           (now - lastMasterUpdate >= masterUpdateInterval ||
                                           (lastMasterUpdate != lastSuccessMasterUpdate &&
                                           now - lastMasterUpdate >= masterUpdateRetryInterval)) )
          );
}


void ExtInfoHost::parseServers(const CString &servers, ParseServersStatus *parseServersStatus, bool lock) {
  LockGuard(lock ? &mutex : nullptr);

  markAllNonPersistServersForDeletion();

  if (!parseServersStatus) {
    ParseServersStatus dummy{};
    parseServersStatus = &dummy;
  }

  char command[120];
  char serverHost[120];
  int serverPort;
  network::Address serverAddress;
  const char *p = *servers;

  while (true) {
    int addServerStatus;
    if (std::sscanf(p, "%119s %119s %d", command, serverHost, &serverPort) < 3) goto next;
    if (std::strcmp(command, "addserver") || serverPort < 0 || serverPort > 0xFFFF) goto next;
    if (!network::setHostAddress(serverHost, serverAddress)) goto next;
    serverAddress.port = serverPort;
    addServerStatus = addServer(serverHost, serverAddress);
    if (addServerStatus) {
      ++parseServersStatus->numServers;
      if (addServerStatus == 1) ++parseServersStatus->newServers;
    }
    next:;
    p = std::strchr(p, '\n');
    if (!p || !*++p) break;
  }

  parseServersStatus->deletedServers = deleteOrphanedServers();
}

void ExtInfoHost::updateFromMaster(int id, bool queue) {
  if (queue) {
    for (const int updateID : masterUpdateQueue) if (updateID == id) return;
    masterUpdateQueue.push_back(id);
    return;
  }

  auto performUpdate = [](ExtInfoHost *host) {
    std::string masterHost;
    uint16_t masterPort;
    std::string servers;
    size_t numServers = 0;

    {
      LockGuard(&host->mutex);
      host->lastMasterUpdate = getMilliSeconds();

      if (!host->masterHost.empty()) {
        // Avoid CoW.
        masterHost = {host->masterHost.begin(), host->masterHost.end()};
        masterPort = host->masterPort;
      } else {
        masterHost = host->info.masterHost;
        masterPort = host->info.masterPort;
      }
    }

    if (!network::recvTCPData(masterHost.c_str(), masterPort, "list\n", servers, 100 * 1024, 20000)) {
      LockGuard(&host->mutex);
      host->masterUpdateStatus.done = true;
      host->masterUpdateStatus.success = -1;
      return;
    }

    if (!servers.compare("banned")) {
      LockGuard(&host->mutex);
      host->masterUpdateStatus.done = true;
      host->masterUpdateStatus.success = -2;
      return;
    }

    if (servers.empty()) {
      LockGuard(&host->mutex);
      host->masterUpdateStatus.done = true;
      host->masterUpdateStatus.success = -3;
      return;
    }

    {
      LockGuard(&host->mutex);
      host->parseServers(servers, &host->masterUpdateStatus, false);
      host->masterUpdateStatus.done = true;
      host->masterUpdateStatus.success = 1;
      host->lastSuccessMasterUpdate = host->lastMasterUpdate;
    }

    if (numServers) {
      FString file;
      file << TMP_DIR << PATH_DIV << host->info.game << ".servers";
      writeFile(file, servers);
    }
  };

  *logFile << info.game << ": updating from master" << logFile->endl();

  if (id == -1 && !masterUpdateQueue.empty()) {
    id = masterUpdateQueue.front();
    masterUpdateQueue.pop_front();
  }

  assert(!masterUpdateThread);

  masterUpdateStatus.id = id;
  masterUpdateThread = new std::thread(performUpdate, this);
}

void ExtInfoHost::processUpdateFromMaster() {
  if (!masterUpdateStatus.done) return;

  event(MASTER_UPDATE, {{}, {}, {}, {&masterUpdateStatus}});

  switch (masterUpdateStatus.success) {
  case -1:
    *logFile << info.game << ": master update failed" << logFile->endl();
    break;
  case -2:
    *logFile << info.game << ": master update failed: banned" << logFile->endl();
    break;
  case -3:
    *logFile << info.game << ": master update failed: empty reply" << logFile->endl();
    break;
  default:
    *logFile << info.game << ": received " << masterUpdateStatus.numServers << " servers from master server" << logFile->endl();
  }

  masterUpdateThread->join();
  delete masterUpdateThread;

  masterUpdateThread = nullptr;
  masterUpdateStatus.reset();
}

void ExtInfoHost::addEventCallback(const EventCallback &eventCallback) {
  eventCallbacks.push_back(eventCallback);
}

void ExtInfoHost::deleteEventCallback(const EventCallback &eventCallback) {
  for (size_t i = eventCallbacks.size(); i-- > 0;)
    if (eventCallbacks[i] == eventCallback)
      eventCallbacks.erase(eventCallbacks.begin() + i);
}

void ExtInfoHost::event(const Event event, const EventData &eventData) const {
  for (const EventCallback eventCallback : eventCallbacks)
    eventCallback.fun(this, event, eventData, eventCallback.data);
}

void ExtInfoHost::init(const size_t index_) {
  socket = network::newSocket();
  index = index_;

  FString file;
  struct stat st;

  file << TMP_DIR << PATH_DIV << info.game << ".servers";

  if (fileExists(file, &st)) {
    std::string servers;

    readFile(file, servers);
    parseServers(servers);

    // This is OK to wrap
    lastMasterUpdate = now - (time(nullptr) - st.st_mtime) * 1000;
    lastSuccessMasterUpdate = lastMasterUpdate;
  }
}

void ExtInfoHost::deinit() {
  {
    SharedLockGuard(&mutex);
    if (masterUpdateThread)
      ::info << info.desc << ": " << "waiting for master update thread..." << ::info.endl();
  }

  do {
    {
      LockGuard(&mutex);
      if (masterUpdateThread) processUpdateFromMaster();
      else break;
    }

    usleep(1000);
  } while (true);

  network::deleteSocket(socket);

  for (Server *server : servers) delete server;

  // Reset variables for reloading.

  enabled = false;
  masterHost.clear();
  lastMasterUpdate = 0;
  lastSuccessMasterUpdate = 0;
  servers.clear();

  assert(eventCallbacks.empty());
}

} // namespace extinfo
