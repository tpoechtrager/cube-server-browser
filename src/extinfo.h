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

#ifndef __EXTINFO_H__
#define __EXTINFO_H__

#include <thread>
#include <mutex>
#include <vector>
#include <deque>
#include <list>
#include <string>
#include "network.h"
#include "tools.h"

#define EXT_ACK -1
#define EXT_VERSION 105
#define EXT_NO_ERROR 0
#define EXT_ERROR 1
#define EXT_PLAYERSTATS_RESP_IDS -10
#define EXT_PLAYERSTATS_RESP_STATS -11
#define EXT_UPTIME 0
#define EXT_PLAYERSTATS 1
#define EXT_EXTENDED_PLAYERSTATS 0xC8343F2
#define EXT_TEAMSCORE 2

#define EXT_VERSION_MIN 104

namespace extinfo {

//
// Structs & Constants
//

enum GameIdentifier { SAUERBRATEN, TESSERACT, REDECLIPSE, ASSAULTCUBE };

struct GameInfo {
  const GameIdentifier identifier;
  const char *const game;
  const char *const desc;
  const char *const masterHost;
  const uint16_t masterPort;
  const int16_t infoPortOffset;
  const bool extInfoSupported;
};

constexpr GameInfo GAME_INFO_SAUERBRATEN = {
  SAUERBRATEN, "sauerbraten", "Cube 2: Sauerbraten",
  "master.sauerbraten.org", 28787, 1, true
};

constexpr GameInfo GAME_INFO_TESSERACT = {
  TESSERACT, "tesseract", "Tesseract",
  "master.tesseract.gg", 41999, 0, true
};

constexpr GameInfo GAME_INFO_REDECLIPSE = {
  REDECLIPSE, "redeclipse", "Red Eclipse",
  "play.redeclipse.net", 28800, 1, false
};

#if 0
constexpr GameInfo GAME_INFO_INEXOR = {
  INEXOR, "inexor", "Inexor",
  "master.inexor.org", 28787, 1, false
};
#endif

constexpr GameInfo GAME_INFO_ASSAULTCUBE = {
  ASSAULTCUBE, "assaultcube", "AssaultCube",
  "ms.cubers.net", 28760, 1, true
};

constexpr size_t NUMGAMES = 4;

constexpr size_t MAX_NAME_LENGTH = 15;
constexpr size_t MAX_TEAM_LENGTH = 10;

constexpr size_t MAX_PLAYERS = 256;
constexpr size_t MAX_SERVERS = 512;

constexpr TimeType UNKNOWN_ONLINE_TIME = static_cast<TimeType>(-1);

enum ServerMod : int {
  SM_INVALID = 0,
  SM_HOPMOD = -2,
  SM_OOMOD = -3,
  SM_SPAGHETTIMOD = -4,
  SM_SUCKERSERV = -5,
  SM_REMOD = -6,
  SM_NOOBMOD = -7,
  SM_ZEROMOD = -8
};

enum Event {
  MASTER_UPDATE,
  SERVER_ADD,
  SERVER_DELETE,
  SERVER_RESTART,
  SERVER_UPDATE,
  PLAYER_CONNECT,
  PLAYER_DISCONNECT,
  PLAYER_RENAME
};

struct EventData {
  const struct Server *server;
  const struct Player *player[2];
  const int val[2];
  const void *data[2];

#ifndef PROPER_LIST_INITIALIZATION_SUPPORTED
  EventData(const struct Server *server, const std::initializer_list<const Player *> player = {},
            const std::initializer_list<int> val = {}, const std::initializer_list<const void *> data = {})
      : server(server), player{getListElement(player, 0), getListElement(player, 1)},
        val{getListElement(val, 0), getListElement(val, 1)}, data{getListElement(data, 0), getListElement(data, 1)} {}
#endif
};

struct EventCallback {
  typedef void (*Fun)(const struct ExtInfoHost *host, const Event event,
                      const EventData &eventData, void *callbackData);
  Fun fun;
  void *data;

  bool operator==(const EventCallback &in) const {
    return fun == in.fun && data == in.data;
  }
};

struct FindPlayer {
  StrCmp country;
  StrCmp name;
  IntCmp cn;
  IntCmp frags;
  IntCmp deaths;
  IntCmp accuracy;
};

typedef void (*FindPlayerCallback)(const struct Server *server, const struct Player &player, void *callbackData);

//
// Player
//

struct Player {
  int cn;
  int ping;
  char name[MAX_NAME_LENGTH + 1]; // 16
  char team[MAX_TEAM_LENGTH + 6]; // 16
  int frags;
  int flags;
  int deaths;
  int teamkills;
  int accuracy;
  int health;
  int armour;
  int gun;
  int priv;
  int state;
  union {
    uint8_t ia[sizeof(uint32_t)];
    uint32_t ui32;
  } ip;
  uint8_t dataflags;
  uint8_t data[3];

  struct Extended {
    bool infoOK;
    ExtVar<ServerMod> serverMod;
    ExtVar<int> sessionID;
    ExtVar<int> suicides;
    ExtVar<int> shotdamage;
    ExtVar<int> damage;
    ExtVar<int> explosivedamage;
    ExtVar<int> hits;
    ExtVar<int> misses;
    ExtVar<int> shots;
    ExtVar<int> captured;
    ExtVar<int> stolen;
    ExtVar<int> defended;
    char countryCode[4];
  } extended{};

  struct Info {
    TimeType connectTime;
    TimeType lastUpdate;
    uint64_t sessionID; // Session ID set by this application
    const char *country[2];

    TimeType getOnlineTime(TimeType now = TimeType()) const;
    const char *getOnlineTime(TimeType now, char *buf, size_t size) const;
    const char *getCountry(const bool code = false) const;
  } info{};

  bool shouldBeDeleted = false;

  bool isBot() const;

  const char *getName() const;
  const char *getTeam() const;

  void update(const Player &player);
};

//
// Server
//

struct Ping {
  TimeType lastPing;
  TimeType lastPong;
  uint64_t numPackets;
  uint64_t id;
  void setID(struct Server *server);
};

struct Server {
  bool persist;
  struct ExtInfoHost *host;
  std::string serverHost;
  network::Address address;
  const char *country[2];

  bool shouldBeDeleted;
  uint64_t randomNumbers[3];

  TimeType pingVal;

  Ping info;
  Ping player;
  Ping uptime;
  TimeType lastPong;

  bool havePlayerCNs;
  std::list<int> playerCNs;
  std::vector<Player> players;
  std::vector<Player> playerReceiveTmp;

  float highResPing;
  int ping;
  int numPlayers;
  int protocolVersion;
  int gameMode;
  int secondsLeft;
  int maxPlayers;
  int masterMode;
  bool gamePaused;
  int gameSpeed;
  ShortString mapName;
  ShortString description;
  int mutators;
  bool infoOK;

  struct Extended {
    bool infoOK;
    int uptime;
    ExtVar<ServerMod> serverMod;
  } extended;

  uint64_t getKey() const;

  const char *getGameModeName() const;
  bool isTeamMode() const;

  const char *getMasterModeName() const;
  const char *getGameReleaseName(char *buf, const size_t size) const;
  const char *getServerModName() const;
  const char *getCountry(const bool code = false) const;
  const char *getDescription() const;
  const std::string &getUniqueDescription(FString &description) const;
  const char *getMapName() const;

  int getUptime() const;
  int getCurrentUptime(TimeType now = TimeType()) const;

  bool addPlayer(const Player &player);
  bool addOrUpdatePlayer(const Player &player);
  void deletePlayer(decltype(players)::iterator player);
  void deleteDisconnectedPlayers();
  void deleteAllPlayers();
  void markAllPlayersForDeletion();

  bool addPlayerToReceiveTmp(const Player &player);
  void deleteInvalidPlayersFromReceiveTmp();
  void deleteAllPlayersFromReceiveTmp();
  bool allPlayersReceived() const;
  void updatePlayers();

  const Player *findPlayerByCNAndExtSessionID(const int cn, const int sessionID) const;
  const Player *findPlayerByCNAndIPAddress(const int cn, const uint32_t ipAddress) const;
  const Player *findPlayerByCNAndName(const int cn, const char *name) const;

  bool isValidServerMod(const int serverMod) const;

  int isValidExtInfoPacket(const int type, network::PacketBuf &pb,
                           const bool checkRequest = false,
                           const bool checkExtNoError = true) const;

  bool isValidExtInfoPlayerCN(const int cn, bool remove = false);

  bool shouldInfoPing() const;
  bool shouldExtPlayerPing() const;
  bool shouldExtUptimePing() const;

  bool sendPing(network::PacketBuf &pb, Ping &ping);
  void preparePing(network::PacketBuf &pb);
  bool infoPing();
  bool extPlayerPing();
  bool extUptimePing();
};

//
// ExtInfoHost
//

struct ParseServersStatus {
  size_t numServers;
  size_t newServers;
  size_t deletedServers;
};

struct MasterUpdateStatus : ParseServersStatus {
  bool done;
  int id;
  int success; // < 0: failed | > 0: success
  void reset() { std::memset(this, 0, sizeof(*this)); }
};

struct ExtInfoHost {
  const GameInfo info;
  bool enabled;
  std::string masterHost;
  uint16_t masterPort;
  std::deque<int> masterUpdateQueue;
  std::thread *masterUpdateThread;
  MasterUpdateStatus masterUpdateStatus;
  TimeType lastMasterUpdate;
  TimeType lastSuccessMasterUpdate;
  network::Socket socket;
  std::vector<Server *> servers;
  std::vector<EventCallback> eventCallbacks;
  SharedMutex mutex;
  size_t index;

  size_t getPlayerCount() const;

  void findPlayer(const FindPlayer &findPlayer, FindPlayerCallback callback, void *callbackData = nullptr) const;
  const Server *findServer(network::Address address, bool extInfoPort = true) const;

  int addServer(const char *serverHost, const network::Address &address, bool persist = false);
  void deleteServer(decltype(servers)::iterator);
  size_t deleteOrphanedServers();
  void markAllNonPersistServersForDeletion();

  void addEventCallback(const EventCallback &eventCallback);
  void deleteEventCallback(const EventCallback &eventCallback);
  void event(const Event event, const EventData &eventData) const;

  bool shouldUpdateFromMaster() const;

  void updateFromMaster(int id = -1, bool queue = false);
  void parseServers(const CString &servers, ParseServersStatus *parseServersStatus = nullptr, bool lock = true);
  void processUpdateFromMaster();

  // Do not call these before network::init()
  void init(const size_t index);
  void deinit();
};

//
// Misc
//

PLUGIN_IMPORT extern ExtInfoHost hosts[NUMGAMES];

ExtInfoHost *getExtInfoHost(const char *game);

void addEventCallback(const EventCallback &eventCallback);
void deleteEventCallback(const EventCallback &eventCallback);

void process();

bool init();
void deinit();

} // namespace extinfo

#endif //__EXTINFO_H__
