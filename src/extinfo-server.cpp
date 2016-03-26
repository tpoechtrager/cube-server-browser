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

#include "geoip.h"
#include "extinfo.h"
#include "extinfo-internal.h"
#include "main.h"

namespace extinfo {

void Ping::setID(Server *server) {
  id = ++numPackets + server->randomNumbers[0];
}

uint64_t Server::getKey() const {
  const union {
    uint32_t ui32[2];
    uint64_t ui64;
  } key{{address.host, address.port}};

  return key.ui64;
}

const char *Server::getGameModeName() const {
  switch (host->info.identifier) {
  case SAUERBRATEN: {
    constexpr const char *gameModes[] = {
      "ffa", "coop edit", "teamplay", "instagib", "instagib team",
      "efficiency", "efficiency team", "tactics", "tactitcs team", "capture",
      "regen capture", "ctf", "insta ctf", "protect", "insta protect", "hold",
      "insta hold", "efficiency ctf", "efficiency protect", "efficiency hold",
      "collect", "insta collect", "efficiency collect"
    };
    returnArrayValueOrBreak(gameModes, gameMode);
  }
  case TESSERACT: {
    constexpr const char *gameModes[] = {
      "edit", "rdm",  "pdm", "rtdm", "ptdm", "rctf", "pctf"
    };
    returnArrayValueOrBreak(gameModes, gameMode);
  }
  case REDECLIPSE: {
    constexpr const char *gameModes[] = {
      "demo", "editing", "deathmatch", "capture-the-flag",
      "defend-and-control", "bomber-ball", "race"
    };
    constexpr const char *mutatorNames[] = {
      "multi",     "ffa",     "coop",     "instagib", "medieval",
      "kaboom",    "duel",    "survivor", "classic",  "onslaught",
      "freestyle", "vampire", "resize",   "hard",     "basic"
    };
    (void)mutatorNames;
    returnArrayValueOrBreak(gameModes, gameMode);
  }
  case ASSAULTCUBE: {
    constexpr const char *gameModes[] = {
      "team deathmatch", "coop edit", "deathmatch", "survivor",
      "team survivor", "ctf", "pistol frenzy", "bot team deathmatch",
      "bot deathmatch", "last swiss standing", "one shot, one kill",
      "team one shot, one kill", "bot one shot, one kill", "hunt the flag",
      "team keep the flag", "keep the flag"
    };
    returnArrayValueOrBreak(gameModes, gameMode);
  }
  }

  return "<unknown>";
}

bool Server::isTeamMode() const {
  const char *gameModeName = getGameModeName();
  const char * const *teamModeNamesPtr;

  switch (host->info.identifier) {
  case SAUERBRATEN: {
    constexpr const char *teamModeNames[] = {
      "team", "capture", "ctf", "protect", "hold", "collect", nullptr
    };
    teamModeNamesPtr = reinterpret_cast<const char *const *>(teamModeNames);
  }
  default:
    return false; // TODO: implement.
  }

  for (const char *const *teamModeName = teamModeNamesPtr; *teamModeName; ++teamModeName)
    if (std::strstr(gameModeName, *teamModeName)) return true;

  return false;
}

const char *Server::getMasterModeName() const {
  switch (host->info.identifier) {
  case SAUERBRATEN:
  case TESSERACT: {
    constexpr const char *const masterModeNames[] = {
      "auth", "open", "veto", "locked", "private", "password"
    };
    returnArrayValueOrBreak(masterModeNames, masterMode + 1);
  }
  case REDECLIPSE: {
    constexpr const char *const masterModeNames[] = {
      "open", "veto", "locked", "private", "password"
    };
    returnArrayValueOrBreak(masterModeNames, masterMode);
  }
  case ASSAULTCUBE: {
    constexpr const char *const masterModeNames[] = {
      "open", "private", "match"
    };
    returnArrayValueOrBreak(masterModeNames, masterMode);
  }
  }

  return "<unknown>";
}

const char *Server::getGameReleaseName(char *buf, const size_t size) const {
  switch (host->info.identifier) {
  case SAUERBRATEN: {
    constexpr const char *releaseNames[] = {
      "summer", "assassin", "ctf", "trooper", "justice"
    };
    returnArrayValueOrBreak(releaseNames, protocolVersion - 254 - 1);
  }
  case TESSERACT: {
    constexpr const char *releaseNames[] = {
      "first", "second"
    };
    returnArrayValueOrBreak(releaseNames, protocolVersion - 1);
  }
  case REDECLIPSE: {
    constexpr const char *releaseNames[] = {
      "ides", "supernova", "cosmic", "elara", "aurora"
    };
    returnArrayValueOrBreak(releaseNames, protocolVersion - 221 - 1);
  }
  case ASSAULTCUBE:
    break;
  }

  if (/*std::*/ snprintf(buf, size, "%d", protocolVersion) > 0)
    return buf;

  return "<unknown>";
}

const char *Server::getServerModName() const {
  switch (host->info.identifier) {
  case SAUERBRATEN: {
    constexpr const char *modNames[] = {
      "hopmod", "oo|mod", "spaghettimod", "suckerserv",
      "remod", "noobmod", "zeromod"
    };
    returnArrayValueOrBreak(modNames, (*extended.serverMod * -1) - 2);
  }
  default:;
  }

  return "<unknown>";
}

const char *Server::getCountry(const bool code) const {
  return country[code] ? country[code] : "<unknown>";
}

const char *Server::getDescription() const {
  if (*description) return description;
  return "<no server description>";
}


const std::string &Server::getUniqueDescription(FString &description) const {
  size_t dupNum = 0;
  size_t numDups = 0;

  description << getDescription();

  const char *descriptionStr = description.c_str();

  for (const extinfo::Server *server : host->servers) {
    if (!server->infoOK) continue;

    const char *a = descriptionStr;
    const char *b = server->getDescription();

    // Inline Byte-by-Byte comparison is five times
    // faster than std::string::compare().

    if (streq(a, b)) { ++numDups;
      if (server == this) dupNum = numDups;
      if (numDups >= 2 && dupNum) break;
    }
  }

  if (numDups >= 2) description << " [" << dupNum << "]";

  return description;
}

const char *Server::getMapName() const {
  if (*mapName) return mapName;
  return "<no map set>";
}

int Server::getUptime() const {
  if (!extended.infoOK || extended.uptime < 0) return -1;
  return extended.uptime;
}

int Server::getCurrentUptime(TimeType now) const {
  if (!extended.infoOK || extended.uptime < 0) return -1;

  if (extended.serverMod.isSet()) {
    switch (*extended.serverMod) {
      case SM_HOPMOD:
      case SM_SUCKERSERV:
        // https://github.com/SuckerServ/suckerserv/issues/22
        return extended.uptime;
        break;
      default:;
    }
  }

  if (!now) now = getMilliSeconds();

  return extended.uptime + (now - uptime.lastPong)/1000;
}

bool Server::addPlayer(const Player &player) {
  if (players.size() >= MAX_PLAYERS) return false;

  players.push_back(player);
  Player &newPlayer = players.back();

  newPlayer.info.sessionID = ++playerSessionID;

  if (newPlayer.extended.infoOK && newPlayer.extended.countryCode[0])
    geoip::country(newPlayer.extended.countryCode,newPlayer.info.country, sizeof(newPlayer.info.country));
  else
    geoip::country(newPlayer.ip.ui32, newPlayer.info.country, sizeof(newPlayer.info.country));

  host->event(PLAYER_CONNECT, {this, {&newPlayer}});

  return true;
}

bool Server::addOrUpdatePlayer(const Player &player) {
  Player *oldPlayer;

  if (player.extended.sessionID.isSet()) {
    // Session ID + CN is our best bet.
    oldPlayer = const_cast<Player *>(findPlayerByCNAndExtSessionID(player.cn, *player.extended.sessionID));
  } else if (player.ip.ui32) {
    // IP Address + CN is still unique enough.
    oldPlayer = const_cast<Player *>(findPlayerByCNAndIPAddress(player.cn, player.ip.ui32));
  } else {
    // We are doomed. Try by CN + Name.
    oldPlayer = const_cast<Player *>(findPlayerByCNAndName(player.cn, player.name));
  }

  if (oldPlayer) {
    if (std::strcmp(oldPlayer->name, player.name)) host->event(PLAYER_RENAME, {this, {oldPlayer, &player}});
    oldPlayer->update(player);
    return true;
  }

  return addPlayer(player);
}

void Server::deletePlayer(decltype(players)::iterator player) {
  host->event(PLAYER_DISCONNECT, {this, {&*player}});
  players.erase(player);
}

void Server::deleteDisconnectedPlayers() {
  for (size_t i = players.size(); i-- > 0;) if (players[i].shouldBeDeleted) deletePlayer(players.begin() + i);
}

void Server::deleteAllPlayers() {
  for (size_t i = players.size(); i-- > 0;) deletePlayer(players.begin() + i);
}

void Server::markAllPlayersForDeletion() {
  for (Player &player : players) player.shouldBeDeleted = true;
}

bool Server::addPlayerToReceiveTmp(const Player &player) {
  if (playerReceiveTmp.size() >= MAX_PLAYERS) return false;
  playerReceiveTmp.push_back(player);
  return true;
}

void Server::deleteInvalidPlayersFromReceiveTmp() {
  for (size_t i = playerReceiveTmp.size(); i-- > 0;)
    if (!isValidExtInfoPlayerCN(playerReceiveTmp[i].cn, true))
      playerReceiveTmp.erase(playerReceiveTmp.begin() + i);
}

void Server::deleteAllPlayersFromReceiveTmp() {
  playerReceiveTmp.clear();
}

bool Server::allPlayersReceived() const {
  return havePlayerCNs && playerCNs.empty();
}

void Server::updatePlayers() {
  markAllPlayersForDeletion();
  for (const Player &player : playerReceiveTmp) addOrUpdatePlayer(player);
  deleteDisconnectedPlayers();
  playerReceiveTmp.clear();
}

const Player *Server::findPlayerByCNAndExtSessionID(const int cn, const int sessionID) const {
  for (const Player &player : players) {
    if (player.cn != cn) continue;
    const ExtVar<int> &playerSessionID = player.extended.sessionID;
    if (playerSessionID.isSet() && *playerSessionID == sessionID) return &player;
  }
  return nullptr;
}

const Player *Server::findPlayerByCNAndIPAddress(const int cn, const uint32_t ipAddress) const {
  for (const Player &player : players)  if (player.cn == cn && player.ip.ui32 == ipAddress) return &player;
  return nullptr;
}

const Player *Server::findPlayerByCNAndName(const int cn, const char *name) const {
  for (const Player &player : players)
    if (player.cn == cn && !std::strcmp(player.name, name))
      return &player;
  return nullptr;
}

bool Server::isValidServerMod(const int serverMod) const {
  switch (host->info.identifier) {
  case SAUERBRATEN: return serverMod <= SM_HOPMOD && serverMod >= SM_ZEROMOD;
  default: return false;
  }
}

int Server::isValidExtInfoPacket(const int type, network::PacketBuf &pb,
                                 const bool checkRequest, const bool checkExtNoError) const {
  int version;
  int requestID;

  if (checkRequest) {
    pb.getInt(); // request
    if (type == EXT_PLAYERSTATS && pb.getInt() != EXT_EXTENDED_PLAYERSTATS) goto error;
  }

  requestID = pb.getInt();

  switch (type) {
  case EXT_PLAYERSTATS:
    if (requestID != shrinkTo32BitSignedInteger(player.id)) return -1;
    break;
  case EXT_UPTIME:
    if (requestID != shrinkTo32BitSignedInteger(uptime.id)) return -1;
    break;
  }

  if (pb.getInt() != EXT_ACK) goto error;

  version = pb.getInt();
  if (version < EXT_VERSION_MIN || version > EXT_VERSION) goto error;
  if (checkExtNoError && pb.getInt() != EXT_NO_ERROR) goto error;

  return 1;
error:;
  return 0;
}

bool Server::isValidExtInfoPlayerCN(const int cn, bool remove) {
  decltype(playerCNs)::iterator it = std::find(playerCNs.begin(), playerCNs.end(), cn);
  if (it == playerCNs.end()) return false;
  if (remove) playerCNs.erase(it);
  return true;
}

bool Server::shouldInfoPing() const {
  return !info.lastPing || now - info.lastPing >= pingInterval;
}

bool Server::shouldExtPlayerPing() const {
  return infoOK && numPlayers > 0 && (!player.lastPing || now - player.lastPing >= extPlayerPingInterval);
}

bool Server::shouldExtUptimePing() const {
  return infoOK && (!uptime.lastPing || now - uptime.lastPing >= extUptimePingInterval);
}

bool Server::sendPing(network::PacketBuf &pb, Ping &ping) {
  ping.setID(this);
  pb.addInt(shrinkTo32BitSignedInteger(ping.id));
  return pb.send(host->socket, address);
}

void Server::preparePing(network::PacketBuf &pb) {
  switch (host->info.identifier) {
  case TESSERACT:
    pb.addByte(0xFF);
    pb.addByte(0xFF);
    break;
  default:
    ;
  }
}

bool Server::infoPing() {
  network::PacketBuf16 pb;
  preparePing(pb);
  pb.addInt(1);
  pingVal = nowus;
  info.lastPing = now;
  return sendPing(pb, info);
}

bool Server::extPlayerPing() {
  network::PacketBuf32 pb;
  preparePing(pb);
  pb.addInt(0);
  pb.addInt(EXT_PLAYERSTATS);
  pb.addInt(-1);
  pb.addInt(EXT_EXTENDED_PLAYERSTATS);
  player.lastPing = now;
  havePlayerCNs = false;
  deleteAllPlayersFromReceiveTmp();
  return sendPing(pb, player);
}

bool Server::extUptimePing() {
  network::PacketBuf32 pb;
  preparePing(pb);
  pb.addInt(0);
  pb.addInt(EXT_UPTIME);
  pb.addByte(1); // request server mod
  uptime.lastPing = now;
  return sendPing(pb, uptime);
}

} // namespace extinfo
