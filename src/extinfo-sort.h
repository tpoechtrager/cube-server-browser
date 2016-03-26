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

#include <algorithm>

namespace extinfo {

// Player

typedef ObjPointer<Player> PlayerPtr;

enum PlayerSorter : int {
  PS_NAME_ASC,
  PS_NAME_DEC,
  PS_TEAM_ASC,
  PS_TEAM_DEC,
  PS_FRAGS_ASC,
  PS_FRAGS_DEC,
  PS_DEATHS_ASC,
  PS_DEATHS_DEC,
  PS_ACCURACY_ASC,
  PS_ACCURACY_DEC,
  PS_TEAMKILLS_ASC,
  PS_TEAMKILLS_DEC,
  PS_HEALTH_ASC,
  PS_HEALTH_DEC,
  PS_ARMOUR_ASC,
  PS_ARMOUR_DEC,
  PS_PING_ASC,
  PS_PING_DEC,
  PS_CLIENTNUM_ASC,
  PS_CLIENTNUM_DEC,
  PS_ONLINETIME_ASC,
  PS_ONLINETIME_DEC,
  PS_COUNTRY_ASC,
  PS_COUNTRY_DEC,
};

namespace {

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wunused-function"
#endif

enum PlayerSorter getPlayerSorter(const char *str) {
  constexpr PlayerSorter DEFAULT_PLAYER_SORTER = PS_FRAGS_DEC;

  if (!str) return DEFAULT_PLAYER_SORTER;

  constexpr struct {
    const char *name;
    PlayerSorter asc;
    PlayerSorter dec;
  } sorters[] = {
    {"name", PS_NAME_ASC, PS_NAME_DEC},
    {"team", PS_TEAM_ASC, PS_TEAM_DEC},
    {"frags", PS_FRAGS_ASC, PS_FRAGS_DEC},
    {"deaths", PS_DEATHS_ASC, PS_DEATHS_DEC},
    {"acc", PS_ACCURACY_ASC, PS_ACCURACY_DEC},
    {"teamkills", PS_TEAMKILLS_ASC, PS_TEAMKILLS_DEC},
    {"health", PS_HEALTH_ASC, PS_HEALTH_DEC},
    {"armour", PS_ARMOUR_ASC, PS_ARMOUR_DEC},
    {"ping", PS_PING_ASC, PS_PING_DEC},
    {"cn", PS_CLIENTNUM_ASC, PS_CLIENTNUM_DEC},
    {"country", PS_COUNTRY_ASC, PS_COUNTRY_DEC},
  };

  for (auto &sorter : sorters)
    if (strcasestr(str, sorter.name))
      return strcasestr(str, "asc") ? sorter.asc : sorter.dec;

  return DEFAULT_PLAYER_SORTER;
}

} // anonymous namespace

template <typename T>
void sortPlayers(std::vector<T> &players, const PlayerSorter sorter) {
  struct sortPlayersByNameAsc {
    bool operator()(const PlayerPtr &a, const PlayerPtr &b) const {
      return strcasecmp(a->getName(), b->getName()) < 0;
    }
  };

  struct sortPlayersByNameDec {
    bool operator()(const PlayerPtr &a, const PlayerPtr &b) const {
      return strcasecmp(a->getName(), b->getName()) > 0;
    }
  };

  struct sortPlayersByTeamAsc {
    bool operator()(const PlayerPtr &a, const PlayerPtr &b) const {
      return strcasecmp(a->getTeam(), b->getTeam()) < 0;
    }
  };

  struct sortPlayersByTeamDec {
    bool operator()(const PlayerPtr &a, const PlayerPtr &b) const {
      return strcasecmp(a->getTeam(), b->getTeam()) > 0;
    }
  };

  struct sortPlayersByFragsAsc {
    bool operator()(const PlayerPtr &a, const PlayerPtr &b) const {
      return a->frags < b->frags;
    }
  };

  struct sortPlayersByFragsDec {
    bool operator()(const PlayerPtr &a, const PlayerPtr &b) const {
      return a->frags > b->frags;
    }
  };

  struct sortPlayersByDeathsAsc {
    bool operator()(const PlayerPtr &a, const PlayerPtr &b) const {
      return a->deaths < b->deaths;
    }
  };

  struct sortPlayersByDeathsDec {
    bool operator()(const PlayerPtr &a, const PlayerPtr &b) const {
      return a->deaths > b->deaths;
    }
  };

  struct sortPlayersByAccuracyAsc {
    bool operator()(const PlayerPtr &a, const PlayerPtr &b) const {
      return a->accuracy < b->accuracy;
    }
  };

  struct sortPlayersByAccuracyDec {
    bool operator()(const PlayerPtr &a, const PlayerPtr &b) const {
      return a->accuracy > b->accuracy;
    }
  };

  struct sortPlayersByTeamkillsAsc {
    bool operator()(const PlayerPtr &a, const PlayerPtr &b) const {
      return a->teamkills < b->teamkills;
    }
  };

  struct sortPlayersByTeamkillsDec {
    bool operator()(const PlayerPtr &a, const PlayerPtr &b) const {
      return a->teamkills > b->teamkills;
    }
  };

  struct sortPlayersByHealthAsc {
    bool operator()(const PlayerPtr &a, const PlayerPtr &b) const {
      return a->health < b->health;
    }
  };

  struct sortPlayersByHealthDec {
    bool operator()(const PlayerPtr &a, const PlayerPtr &b) const {
      return a->health > b->health;
    }
  };

  struct sortPlayersByArmourAsc {
    bool operator()(const PlayerPtr &a, const PlayerPtr &b) const {
      return a->armour < b->armour;
    }
  };

  struct sortPlayersByArmourDec {
    bool operator()(const PlayerPtr &a, const PlayerPtr &b) const {
      return a->armour > b->armour;
    }
  };

  struct sortPlayersByPingAsc {
    bool operator()(const PlayerPtr &a, const PlayerPtr &b) const {
      return a->ping < b->ping;
    }
  };

  struct sortPlayersByPingDec {
    bool operator()(const PlayerPtr &a, const PlayerPtr &b) const {
      return a->ping > b->ping;
    }
  };

  struct sortPlayersByClientnumAsc {
    bool operator()(const PlayerPtr &a, const PlayerPtr &b) const {
      return a->cn < b->cn;
    }
  };

  struct sortPlayersByClientnumDec {
    bool operator()(const PlayerPtr &a, const PlayerPtr &b) const {
      return a->cn > b->cn;
    }
  };

  struct sortPlayersByOnlineTimeAsc {
    bool operator()(const PlayerPtr &a, const PlayerPtr &b) const {
      TimeType x = a->info.getOnlineTime();
      TimeType y = b->info.getOnlineTime();
      return (x != UNKNOWN_ONLINE_TIME ? x : 0) < (y != UNKNOWN_ONLINE_TIME ? y : 0);
    }
  };

  struct sortPlayersByOnlineTimeDec {
    bool operator()(const PlayerPtr &a, const PlayerPtr &b) const {
      TimeType x = a->info.getOnlineTime();
      TimeType y = b->info.getOnlineTime();
      return (x != UNKNOWN_ONLINE_TIME ? x : 0) > (y != UNKNOWN_ONLINE_TIME ? y : 0);
    }
  };

  struct sortPlayersByCountryAsc {
    bool operator()(const PlayerPtr &a, const PlayerPtr &b) const {
      return std::strcmp(a->info.getCountry(), b->info.getCountry()) > 0;
    }
  };

  struct sortPlayersByCountryDec {
    bool operator()(const PlayerPtr &a, const PlayerPtr &b) const {
      return std::strcmp(a->info.getCountry(), b->info.getCountry()) < 0;
    }
  };

  switch (sorter) {
  case PS_NAME_ASC:
    std::sort(players.begin(), players.end(), sortPlayersByNameAsc());
    break;
  case PS_NAME_DEC:
    std::sort(players.begin(), players.end(), sortPlayersByNameDec());
    break;
  case PS_TEAM_ASC:
    std::sort(players.begin(), players.end(), sortPlayersByTeamAsc());
    break;
  case PS_TEAM_DEC:
    std::sort(players.begin(), players.end(), sortPlayersByTeamDec());
    break;
  case PS_FRAGS_ASC:
    std::sort(players.begin(), players.end(), sortPlayersByFragsAsc());
    break;
  case PS_FRAGS_DEC:
    std::sort(players.begin(), players.end(), sortPlayersByFragsDec());
    break;
  case PS_DEATHS_ASC:
    std::sort(players.begin(), players.end(), sortPlayersByDeathsAsc());
    break;
  case PS_DEATHS_DEC:
    std::sort(players.begin(), players.end(), sortPlayersByDeathsDec());
    break;
  case PS_ACCURACY_ASC:
    std::sort(players.begin(), players.end(), sortPlayersByAccuracyAsc());
    break;
  case PS_ACCURACY_DEC:
    std::sort(players.begin(), players.end(), sortPlayersByAccuracyDec());
    break;
  case PS_TEAMKILLS_ASC:
    std::sort(players.begin(), players.end(), sortPlayersByTeamkillsAsc());
    break;
  case PS_TEAMKILLS_DEC:
    std::sort(players.begin(), players.end(), sortPlayersByTeamkillsDec());
    break;
  case PS_HEALTH_ASC:
    std::sort(players.begin(), players.end(), sortPlayersByHealthAsc());
    break;
  case PS_HEALTH_DEC:
    std::sort(players.begin(), players.end(), sortPlayersByHealthDec());
    break;
  case PS_ARMOUR_ASC:
    std::sort(players.begin(), players.end(), sortPlayersByArmourAsc());
    break;
  case PS_ARMOUR_DEC:
    std::sort(players.begin(), players.end(), sortPlayersByArmourDec());
    break;
  case PS_PING_ASC:
    std::sort(players.begin(), players.end(), sortPlayersByPingAsc());
    break;
  case PS_PING_DEC:
    std::sort(players.begin(), players.end(), sortPlayersByPingDec());
    break;
  case PS_CLIENTNUM_ASC:
    std::sort(players.begin(), players.end(), sortPlayersByClientnumAsc());
    break;
  case PS_CLIENTNUM_DEC:
    std::sort(players.begin(), players.end(), sortPlayersByClientnumDec());
    break;
  case PS_ONLINETIME_ASC:
    std::sort(players.begin(), players.end(), sortPlayersByOnlineTimeAsc());
    break;
  case PS_ONLINETIME_DEC:
    std::sort(players.begin(), players.end(), sortPlayersByOnlineTimeDec());
    break;
  case PS_COUNTRY_ASC:
    std::sort(players.begin(), players.end(), sortPlayersByCountryAsc());
    break;
  case PS_COUNTRY_DEC:
    std::sort(players.begin(), players.end(), sortPlayersByCountryDec());
    break;
  default:
    err << "unknown player sorter!" << err.endl();
  }
}

// Server

struct SortServer {
  const Server *server;
  FString host;
  FString players;
  ShortString descriptionUTF8;
  ShortString mapUTF8;
  ShortString uptimeStr;    // may not be used
  ShortString versionStr;   // may not be used
  const char *uptime;
  const char *version;
};

typedef ObjPointer<SortServer> SortServerPtr;

enum ServerSorter : int {
  HOST_ASC,
  HOST_DEC,
  DESCRIPTION_ASC,
  DESCRIPTION_DEC,
  PROTOCOL_ASC,
  PROTOCOL_DEC,
  PING_ASC,
  PING_DEC,
  GAMEMODENAME_ASC,
  GAMEMODENAME_DEC,
  MAPNAME_ASC,
  MAPNAME_DEC,
  PLAYERS_ASC,
  PLAYERS_DEC,
  MASTERMODE_ASC,
  MASTERMODE_DEC,
  MASTERMODENAME_ASC,
  MASTERMODENAME_DEC,
  SERVERMODNAME_ASC,
  SERVERMODNAME_DEC,
  UPTIME_ASC,
  UPTIME_DEC,
  CURRENTUPTIME_ASC,
  CURRENTUPTIME_DEC,
  COUNTRYNAME_ASC,
  COUNTRYNAME_DEC
};

template <typename T>
void sortServers(std::vector<T> &servers, const ServerSorter sorter) {
  struct sortServersByHostAsc {
    bool operator()(const SortServerPtr &a, const SortServerPtr &b) const {
      return std::strcmp(a->host.c_str(), b->host.c_str()) < 0;
    }
  };

  struct sortServersByHostDec {
    bool operator()(const SortServerPtr &a, const SortServerPtr &b) const {
      return std::strcmp(a->host.c_str(), b->host.c_str()) > 0;
    }
  };

  struct sortServersByDescriptionAsc {
    bool operator()(const SortServerPtr &a, const SortServerPtr &b) const {
      return strcasecmp(a->descriptionUTF8, b->descriptionUTF8) < 0;
    }
  };

  struct sortServersByDescriptionDec {
    bool operator()(const SortServerPtr &a, const SortServerPtr &b) const {
      return strcasecmp(a->descriptionUTF8, b->descriptionUTF8) > 0;
    }
  };

  struct sortServersByProtocolAsc {
    bool operator()(const SortServerPtr &a, const SortServerPtr &b) const {
      return a->server->protocolVersion < b->server->protocolVersion;
    }
  };

  struct sortServersByProtocolDec {
    bool operator()(const SortServerPtr &a, const SortServerPtr &b) const {
      return a->server->protocolVersion > b->server->protocolVersion;
    }
  };

  struct sortServersByPingAsc {
    bool operator()(const SortServerPtr &a, const SortServerPtr &b) const {
      return a->server->highResPing < b->server->highResPing;
    }
  };

  struct sortServersByPingDec {
    bool operator()(const SortServerPtr &a, const SortServerPtr &b) const {
      return a->server->highResPing > b->server->highResPing;
    }
  };

  struct sortServersByGameModeNameAsc {
    bool operator()(const SortServerPtr &a, const SortServerPtr &b) const {
      return std::strcmp(a->server->getGameModeName(), b->server->getGameModeName()) < 0;
    }
  };

  struct sortServersByGameModeNameDec {
    bool operator()(const SortServerPtr &a, const SortServerPtr &b) const {
      return std::strcmp(a->server->getGameModeName(), b->server->getGameModeName()) > 0;
    }
  };

  struct sortServersByMapNameAsc {
    bool operator()(const SortServerPtr &a, const SortServerPtr &b) const {
      return strcasecmp(a->server->getMapName(), b->server->getMapName()) < 0;
    }
  };

  struct sortServersByMapNameDec {
    bool operator()(const SortServerPtr &a, const SortServerPtr &b) const {
      return strcasecmp(a->server->getMapName(), b->server->getMapName()) > 0;
    }
  };

  struct sortServersByPlayersAsc {
    bool operator()(const SortServerPtr &a, const SortServerPtr &b) const {
      return a->server->numPlayers < b->server->numPlayers;
    }
  };

  struct sortServersByPlayersDec {
    bool operator()(const SortServerPtr &a, const SortServerPtr &b) const {
      return a->server->numPlayers > b->server->numPlayers;
    }
  };

  struct sortServersByMasterModeAsc {
    bool operator()(const SortServerPtr &a, const SortServerPtr &b) const {
      return a->server->masterMode > b->server->masterMode;
    }
  };

  struct sortServersByMasterModeDec {
    bool operator()(const SortServerPtr &a, const SortServerPtr &b) const {
      return a->server->masterMode < b->server->masterMode;
    }
  };

  struct sortServersByMasterModeNameAsc {
    bool operator()(const SortServerPtr &a, const SortServerPtr &b) const {
      return std::strcmp(a->server->getMasterModeName(),
                         b->server->getMasterModeName()) < 0;
    }
  };

  struct sortServersByMasterModeNameDec {
    bool operator()(const SortServerPtr &a, const SortServerPtr &b) const {
      return std::strcmp(a->server->getMasterModeName(), b->server->getMasterModeName()) > 0;
    }
  };

  struct sortServersByMasterServerModNameAsc {
    bool operator()(const SortServerPtr &a, const SortServerPtr &b) const {
      return std::strcmp(a->server->getServerModName(), b->server->getServerModName()) < 0;
    }
  };

  struct sortServersByMasterServerModNameDec {
    bool operator()(const SortServerPtr &a, const SortServerPtr &b) const {
      return std::strcmp(a->server->getServerModName(), b->server->getServerModName()) > 0;
    }
  };

  struct sortServersByUptimeAsc {
    bool operator()(const SortServerPtr &a, const SortServerPtr &b) const {
      return a->server->getUptime() < b->server->getUptime();
    }
  };

  struct sortServersByUptimeDec {
    bool operator()(const SortServerPtr &a, const SortServerPtr &b) const {
      return a->server->getUptime() > b->server->getUptime();
    }
  };

  struct sortServersByCurrentUptimeAsc {
    bool operator()(const SortServerPtr &a, const SortServerPtr &b) const {
      return a->server->getCurrentUptime() < b->server->getCurrentUptime();
    }
  };

  struct sortServersByCurrentUptimeDec {
    bool operator()(const SortServerPtr &a, const SortServerPtr &b) const {
      return a->server->getCurrentUptime() > b->server->getCurrentUptime();
    }
  };

  struct sortServersByCountryNameAsc {
    bool operator()(const SortServerPtr &a, const SortServerPtr &b) const {
      return std::strcmp(a->server->getCountry(), b->server->getCountry()) < 0;
    }
  };

  struct sortServersByCountryNameDec {
    bool operator()(const SortServerPtr &a, const SortServerPtr &b) const {
      return std::strcmp(a->server->getCountry(), b->server->getCountry()) > 0;
    }
  };

  switch (sorter) {
  case HOST_ASC:
    std::sort(servers.begin(), servers.end(), sortServersByHostAsc());
    break;
  case HOST_DEC:
    std::sort(servers.begin(), servers.end(), sortServersByHostDec());
    break;
  case DESCRIPTION_ASC:
    std::sort(servers.begin(), servers.end(), sortServersByDescriptionAsc());
    break;
  case DESCRIPTION_DEC:
    std::sort(servers.begin(), servers.end(), sortServersByDescriptionDec());
    break;
  case PROTOCOL_ASC:
    std::sort(servers.begin(), servers.end(), sortServersByProtocolAsc());
    break;
  case PROTOCOL_DEC:
    std::sort(servers.begin(), servers.end(), sortServersByProtocolDec());
    break;
  case PING_ASC:
    std::sort(servers.begin(), servers.end(), sortServersByPingAsc());
    break;
  case PING_DEC:
    std::sort(servers.begin(), servers.end(), sortServersByPingDec());
    break;
  case GAMEMODENAME_ASC:
    std::sort(servers.begin(), servers.end(), sortServersByGameModeNameAsc());
    break;
  case GAMEMODENAME_DEC:
    std::sort(servers.begin(), servers.end(), sortServersByGameModeNameDec());
    break;
  case MAPNAME_ASC:
    std::sort(servers.begin(), servers.end(), sortServersByMapNameAsc());
    break;
  case MAPNAME_DEC:
    std::sort(servers.begin(), servers.end(), sortServersByMapNameDec());
    break;
  case PLAYERS_ASC:
    std::sort(servers.begin(), servers.end(), sortServersByPlayersAsc());
    break;
  case PLAYERS_DEC:
    std::sort(servers.begin(), servers.end(), sortServersByPlayersDec());
    break;
  case MASTERMODE_ASC:
    std::sort(servers.begin(), servers.end(), sortServersByMasterModeAsc());
    break;
  case MASTERMODE_DEC:
    std::sort(servers.begin(), servers.end(), sortServersByMasterModeDec());
    break;
  case MASTERMODENAME_ASC:
    std::sort(servers.begin(), servers.end(), sortServersByMasterModeNameAsc());
    break;
  case MASTERMODENAME_DEC:
    std::sort(servers.begin(), servers.end(), sortServersByMasterModeNameDec());
    break;
  case SERVERMODNAME_ASC:
    std::sort(servers.begin(), servers.end(), sortServersByMasterServerModNameAsc());
    break;
  case SERVERMODNAME_DEC:
    std::sort(servers.begin(), servers.end(), sortServersByMasterServerModNameDec());
    break;
  case UPTIME_ASC:
    std::sort(servers.begin(), servers.end(), sortServersByUptimeAsc());
    break;
  case UPTIME_DEC:
    std::sort(servers.begin(), servers.end(), sortServersByUptimeDec());
    break;
  case CURRENTUPTIME_ASC:
    std::sort(servers.begin(), servers.end(), sortServersByCurrentUptimeAsc());
    break;
  case CURRENTUPTIME_DEC:
    std::sort(servers.begin(), servers.end(), sortServersByCurrentUptimeDec());
    break;
  case COUNTRYNAME_ASC:
    std::sort(servers.begin(), servers.end(), sortServersByCountryNameAsc());
    break;
  case COUNTRYNAME_DEC:
    std::sort(servers.begin(), servers.end(), sortServersByCountryNameDec());
    break;
  default:
    err << "unknown server sorter!" << err.endl();
  }
}

} // namespace extinfo
