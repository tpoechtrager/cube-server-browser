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
#include <cstring>
#include <cstdio>
#include <cctype>
#include <cmath>
#include "extinfo.h"
#include "geoip.h"
#include "main.h"
#include "tools.h"
#include "cube/tools.h"

namespace extinfo {

int maxPingsPerSecond;
int maxPingsPer50MS;
TimeType pingInterval;
TimeType extPlayerPingInterval;
TimeType extUptimePingInterval;
TimeType masterUpdateInterval;
TimeType masterUpdateRetryInterval;
uint64_t playerSessionID;
TimeType nowus;
TimeType now;
TimeType32 now32;

namespace {

void updateTime() {
  TimeType prevNow = now;

  nowus = getMicroSeconds();
  now = nowus / 1000;
  now32 = static_cast<TimeType32>(now);

  if (prevNow && now < prevNow) {
    // This is very unlikely to ever happen.
    *logFile << "time wraparound occurred" << logFile->endl();
    shouldReload();
  }
}

} // anonymous namespace

ExtInfoHost hosts[NUMGAMES] = {
  {GAME_INFO_SAUERBRATEN}, {GAME_INFO_TESSERACT},
  {GAME_INFO_REDECLIPSE}, {GAME_INFO_ASSAULTCUBE}
};

ExtInfoHost *getExtInfoHost(const char *game) {
  for (ExtInfoHost &host : hosts)
    if (host.enabled && !std::strcmp(host.info.game, game))
      return &host;
  return nullptr;
}

void addEventCallback(const EventCallback &eventCallback) {
  for (ExtInfoHost &host : hosts)
    if (host.enabled)
      host.addEventCallback(eventCallback);
}

void deleteEventCallback(const EventCallback &eventCallback) {
  for (ExtInfoHost &host : hosts)
    if (host.enabled)
      host.deleteEventCallback(eventCallback);
}

namespace {

void readExtInfoReply(Server *server, network::PacketBuf &pb) {
  char text[260];
  int type = pb.getInt();

  switch (type) {
  case EXT_PLAYERSTATS: {
    if (server->isValidExtInfoPacket(type, pb, true) <= 0) break;
    if (!pb.remaining()) break;

    switch (pb.getInt()) {
    case EXT_PLAYERSTATS_RESP_STATS: {
      server->player.lastPong = now;
      int cn = pb.getInt();
  
      if (server->havePlayerCNs && !server->isValidExtInfoPlayerCN(cn, true)) break;

      Player player;

      player.cn = cn;
      player.ping = pb.getInt();
      pb.getString(text, sizeof(text));
      cubetools::filtertext(player.name, sizeof(player.name), text, false, false, MAX_NAME_LENGTH);
      pb.getString(text, sizeof(text));
      cubetools::filtertext(player.team, sizeof(player.team), text, false, false, MAX_TEAM_LENGTH);
      player.frags = pb.getInt();
      player.flags = pb.getInt();
      player.deaths = pb.getInt();
      player.teamkills = pb.getInt();
      player.accuracy = pb.getInt();
      player.health = pb.getInt();
      player.armour = pb.getInt();
      player.gun = pb.getInt();
      player.priv = pb.getInt();
      player.state = pb.getInt();

      if (pb.remaining()) {
        player.ip.ia[0] = pb.getByte();
        player.ip.ia[1] = pb.getByte();
        player.ip.ia[2] = pb.getByte();
        player.ip.ia[3] = 0x0;
      }

      if (pb.remaining()) {
        // Extended Stats

        Player::Extended &extended = player.extended;

        pb.getInt();
        int serverMod = pb.getInt();

        if (server->isValidServerMod(serverMod)) {
          extended.serverMod = static_cast<ServerMod>(serverMod);

          switch (*extended.serverMod) {
          case SM_HOPMOD:
          case SM_SUCKERSERV:
          case SM_ZEROMOD: {
            extended.suicides = pb.getInt();
            extended.shotdamage = pb.getInt();
            extended.damage = pb.getInt();
            extended.explosivedamage = pb.getInt();
            extended.hits = pb.getInt();
            extended.misses = pb.getInt();
            extended.shots = pb.getInt();

            if (*extended.serverMod == SM_ZEROMOD && pb.remaining() >= 2) {
              signed char info[2] = {pb.getByteSigned(), pb.getByteSigned()};
              bool isContinent = std::islower(info[0]) != 0;

              if (!isContinent) {
                static_assert(sizeof(info) < sizeof(extended.countryCode), "");
                std::memcpy(extended.countryCode, info, 2);
              }

              if (pb.remaining()) extended.sessionID = pb.getInt();
            }

            extended.infoOK = true;

            break;
          }

          case SM_OOMOD: {
            if (pb.getInt() != 1) break; // version
            extended.suicides = pb.getInt();
            extended.shotdamage = pb.getInt();
            extended.damage = pb.getInt();
            extended.captured = pb.getInt();
            extended.stolen = pb.getInt();
            extended.defended = pb.getInt();
            extended.infoOK = true;
            break;
          }

          default:;
          }
        }
      }

      if (!pb.overRead()) {
        // ignored by Player::update()
        if (server->info.numPackets > 2) player.info.connectTime = now;
        else player.info.connectTime = UNKNOWN_ONLINE_TIME;
        player.info.lastUpdate = now;
        server->addPlayerToReceiveTmp(player);
      }

      if (server->allPlayersReceived()) server->updatePlayers();

      break;
    } // EXT_PLAYERSTATS_RESP_STATS
    case EXT_PLAYERSTATS_RESP_IDS: {
      // Whoever designed the extinfo protocol has no clue about UDP.
      // There is absolutely *no* guarantee that this packet will
      // arrive before the players.

      server->player.lastPong = now;
      server->playerCNs.clear();

      while (pb.remaining() && server->playerCNs.size() < MAX_PLAYERS) server->playerCNs.push_back(pb.getInt());

      server->playerCNs.unique();
      server->havePlayerCNs = true;

      server->deleteInvalidPlayersFromReceiveTmp();

      if (server->allPlayersReceived()) server->updatePlayers();
      break;
    } // EXT_PLAYERSTATS_RESP_IDS
    }
    break;
  } // EXT_PLAYERSTATS
  case EXT_UPTIME: {
    server->uptime.lastPong = now;
    server->extended.infoOK = false;
    server->extended.serverMod.reset();

    // do not check for EXT_NO_ERROR due to noobmod brokenness
    // TODO: detect noobmod

    if (pb.getByte() != 1 || server->isValidExtInfoPacket(type, pb, false, false) <= 0) break;
    if (!pb.remaining()) break;

    int prevUptime = server->extended.uptime;
    server->extended.uptime = pb.getInt();
    server->extended.infoOK = true;

    if (pb.remaining()) {
      int serverMod = pb.getInt();
      if (server->isValidServerMod(serverMod)) server->extended.serverMod = static_cast<ServerMod>(serverMod);
    }

    if (prevUptime && server->extended.uptime < prevUptime)
      server->host->event(SERVER_RESTART, {server, {}, {prevUptime, server->extended.uptime}});
    break;
  } // EXT_UPTIME
  }
}

void readInfoReply(ExtInfoHost *host, Server *server, network::PacketBuf &pb) {
  server->lastPong = now;
  char text[260];
  int val = pb.getInt();
  if (!val && server->infoOK) return readExtInfoReply(server, pb);
  int requestID = pb.getInt();     
  server->infoOK = false;
  server->info.lastPong = now;
  if (requestID != shrinkTo32BitSignedInteger(server->info.id)) return;
  server->highResPing = (nowus - server->pingVal) / 1000.0f;
  server->ping = std::floor(server->highResPing + 0.5f);

  auto getMapString = [&]() {
    pb.getString(text, sizeof(text));
    cubetools::filtertext(server->mapName, sizeof(server->mapName), text, false, false, sizeof(server->mapName) - 1);
  };

  auto getServerDescriptionString = [&]() {
    pb.getString(text, sizeof(text));
    cubetools::filtertext(server->description, sizeof(server->description), text, true, false, sizeof(server->description) - 1);
  };

  switch (host->info.identifier) {
  case SAUERBRATEN: {
    server->numPlayers = pb.getInt();
    int numAttrs = pb.getInt();
    if (numAttrs != 5 && numAttrs != 7) break;
    server->protocolVersion = pb.getInt();
    server->gameMode = pb.getInt();
    server->secondsLeft = pb.getInt();
    server->maxPlayers = pb.getInt();
    server->masterMode = pb.getInt();
    if (numAttrs == 7) {
      server->gamePaused = pb.getInt() == 1;
      server->gameSpeed = pb.getInt();
    } else {
      server->gamePaused = false;
      server->gameSpeed = 100;
    }
    getMapString();
    getServerDescriptionString();
    server->infoOK = !pb.overRead();
    break;
  } // SAUERBRATEN
  case TESSERACT: {
    server->protocolVersion = pb.getInt();
    server->numPlayers = pb.getInt();
    server->maxPlayers = pb.getInt();
    int numAttrs = pb.getInt();
    if (numAttrs != 5 && numAttrs != 3) break;
    server->gameMode = pb.getInt();
    server->secondsLeft = pb.getInt();
    server->masterMode = pb.getInt();
    if (numAttrs == 5) {
      server->gamePaused = pb.getInt() == 1;
      server->gameSpeed = pb.getInt();
    } else {
      server->gamePaused = false;
      server->gameSpeed = 100;
    }
    getMapString();
    getServerDescriptionString();
    server->infoOK = !pb.overRead();
    break;
  } // TESSERACT
  case REDECLIPSE: {
    server->numPlayers = pb.getInt();
    if (pb.getInt() != 15) break;
    server->protocolVersion = pb.getInt();
    server->gameMode = pb.getInt();
    server->mutators = pb.getInt();
    server->secondsLeft = pb.getInt();
    server->maxPlayers = pb.getInt();
    server->masterMode = pb.getInt();
    pb.getInt(); // numgamevars
    pb.getInt(); // numgamemods
    pb.getInt(); // major
    pb.getInt(); // minor
    pb.getInt(); // patch
    pb.getInt(); // versionplatform
    pb.getInt(); // versionarch
    pb.getInt(); // gamestate
    pb.getInt(); // timeleft()
    getMapString();
    getServerDescriptionString();
    server->infoOK = !pb.overRead();
    break;
  } // REDECLIPSE
  case ASSAULTCUBE: {
    server->protocolVersion = pb.getInt();
    if (server->protocolVersion < 1128) break;
    server->gameMode = pb.getInt();
    server->numPlayers = pb.getInt();
    server->secondsLeft = pb.getInt();
    getMapString();
    getServerDescriptionString();
    server->maxPlayers = pb.getInt();
    server->infoOK = !pb.overRead();
    break;
  } // ASSAULTCUBE
  }

  if (!server->infoOK || server->numPlayers <= 0) {
    if (!server->infoOK || server->numPlayers < 0) {
      dbg << host->info.game << ": " << server->serverHost << ":"
          << server->address.port - server->host->info.infoPortOffset
          << ": broken info update" << dbg.endl();
    }
    server->deleteAllPlayers();
  }

  host->event(SERVER_UPDATE, {server});
}

void read(const network::SelectSocket &socket) {
  network::Address address;
  network::PacketBuf5K pb;
  if (!pb.receive(socket.socket, address)) return;
  ExtInfoHost *host = static_cast<ExtInfoHost *>(socket.data);
  LockGuard(&host->mutex);
  Server *server = const_cast<Server *>(host->findServer(address));
  if (!server) return;
  updateTime();
  readInfoReply(host, server, pb);
}

bool limitPings() {
  if (!maxPingsPer50MS) return false;

  static int numPings = 0;
  static TimeType lastReset = TimeType();

  if (!lastReset || now - lastReset >= 50) {
    lastReset = now;
    numPings = 0;
  }

  if (numPings < maxPingsPer50MS) {
    ++numPings;
    return false;
  }

  return true;
}

} // anonymous namespace

void process() {
  updateTime();

  for (ExtInfoHost &host : hosts) {
    if (!host.enabled) continue;

    LockGuard(&host.mutex);

    if (host.masterUpdateThread) host.processUpdateFromMaster();
    if (host.shouldUpdateFromMaster()) host.updateFromMaster();

    for (Server *server : host.servers) {
      if (server->infoOK && !server->players.empty() && now - server->lastPong >= oneMinute) {
        server->numPlayers = 0;
        server->deleteAllPlayers();
      }

      if (server->shouldInfoPing()) {
        if (limitPings()) break;
        server->infoPing();
      }

      if (!host.info.extInfoSupported) continue;

      if (server->shouldExtPlayerPing()) {
        if (limitPings()) break;
        server->extPlayerPing();
      }

      if (server->shouldExtUptimePing()) {
        if (limitPings()) break;
        server->extUptimePing();
      }
    }
  }

  size_t numSockets = 0;
  network::SelectSocket sockets[sizeofarray(hosts)];

  // MSVC needs curly braces for this loop
  for (ExtInfoHost &host : hosts) { if (host.enabled) sockets[numSockets++] = {host.socket, &host}; }

  int error;

  if ((error = network::socketSelect(sockets, numSockets, read, nullptr, 5))) {
    err << "socketSelect() failed with error: " << std::strerror(error) << err.endl();
    std::abort();
  }
}

bool init() {
  updateTime();

  maxPingsPerSecond = cfg->getInt("extinfo.maxPingsPerSecond", 0, 100000, 80);
  maxPingsPer50MS = maxPingsPerSecond ? maxPingsPerSecond / 20 : 0;
  pingInterval = cfg->getInt("extinfo.serverPingInterval", oneSecond, oneSecond * 30, oneSecond * 5);
  extPlayerPingInterval = cfg->getInt("extinfo.serverExtPlayerPingInterval", oneSecond, oneSecond * 30, oneSecond * 5);
  extUptimePingInterval = cfg->getInt("extinfo.serverExtUptimePingInterval", oneSecond * 5, oneHour, oneMinute * 2);
  masterUpdateInterval = cfg->getInt("extinfo.masterUpdateInterval", oneMinute * 5, oneDay, oneHour);
  masterUpdateRetryInterval = cfg->getInt("extinfo.masterUpdateRetryInterval", oneMinute * 5, oneDay, oneHour);

  playerSessionID = getRandomNumber();

  FString configEntry;
  size_t numEnabledGames = 0;
  size_t gameIndex = 0;

  for (ExtInfoHost &host : hosts) {
    ++gameIndex;

    configEntry.clear();
    configEntry << "extinfo.games." << host.info.game;

    if (cfg->getBool(*tmpAppend<>(configEntry, ".enabled"), false)) {
      host.enabled = true;
      host.init(gameIndex - 1);
      ++numEnabledGames;

      const char *masterServer = cfg->getString(*tmpAppend<>(configEntry, ".masterServer"));

      if (masterServer) {
        char masterHost[256];
        if (std::sscanf(masterServer, "%255s %hu", masterHost, &host.masterPort) == 2) host.masterHost = masterHost;
      }

      const char **additionalServers = cfg->getStringArray(*tmpAppend<>(configEntry, ".additionalServers"));
      if (!additionalServers) continue;

      for (const char **server = additionalServers; *server; ++server) {
        network::Address serverAddress;
        char serverHost[256];
        if (std::sscanf(*server, "%255s %hu", serverHost, &serverAddress.port) != 2) continue;
        if (!network::setHostAddress(serverHost, serverAddress)) continue;
        host.addServer(serverHost, serverAddress, true);
      }

      delete[] additionalServers;
    }
  }

  if (!numEnabledGames) {
    err << "no games enabled" << err.endl();
    return false;
  }

  return true;
}

void deinit() {
  for (ExtInfoHost &host : hosts) if (host.enabled) host.deinit();
}

} // namespace extinfo
