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
#include <limits>
#include <mutex>
#include <condition_variable>
#include <memory>
#include "httpserver.h"
#include "extinfo.h"
#include "tools.h"
#include "cube/tools.h"
#include "plugin.h"

namespace web {

namespace {

//
// Config
//

namespace {
TimeType updateInterval;
bool allowMasterUpdate;
TimeType masterUpdateLimit;
const char *defaultGame;
} // anonymous namespace

//
// Misc Tools
//

template <typename STR, typename BUF, size_t N>
const char *convertToUTF8AndEscape(const STR &str, BUF (&buf)[N]) {
  char tmp[N];
  convertCubeToUTF8(str, tmp);
  return httpserver::htmlEncode(tmp, buf);
}

const char *getIndentation(const size_t indent) {
  constexpr const char *str[] = {
    "", " ", "  ", "   ", "    "
  };
  return str[indent];
}

class XMLElementPrinter {
private:
  FString &fs;
  size_t numElement = 0;
  const char *elements[5];
  size_t indentation;

public:
  void element(const char *_element, const bool newLine = false, size_t _indentation = 0) {
    elements[numElement] = _element;
    if (numElement) ++indentation;
    else indentation = _indentation;
    fs << getIndentation(indentation) << '<' << elements[numElement] << '>';
    if (newLine) fs << "\n";
    ++numElement;
  }
  void elementEnd(const bool indent = false) {
    if (indent) fs << getIndentation(indentation);
    fs << "</" << elements[numElement - 1] << ">\n";
    if (numElement > 1 && indentation) --indentation;
    --numElement;
  }
  template <typename T> void printElement(const char *_element, T &&_val) {
    element(_element);
    fs << _val;
    elementEnd();
  }
  template <typename T> XMLElementPrinter &operator<<(const T &v) {
    fs << v;
    return *this;
  }
  XMLElementPrinter(httpserver::Response &response) : fs(response.content) {
    fs << "<?xml version=\"1.0\" ?>\n";
    response.mimeType = "text/xml; charset=utf-8";
  }
  ~XMLElementPrinter() {
    while (numElement) elementEnd(true);
  }
};

class XMLNodePrinter {
private:
  XMLElementPrinter &elementPrinter;

public:
  XMLNodePrinter(XMLElementPrinter &elementPrinter, const char *node) : elementPrinter(elementPrinter) {
     elementPrinter.element(node, true);
  }
  ~XMLNodePrinter() {
    elementPrinter.elementEnd(true);
  }
};

extinfo::ExtInfoHost *getExtInfoHost(const httpserver::Request &request, 
                                     httpserver::Response &response,
                                     const bool extInfoRequired = false) {
  extinfo::ExtInfoHost *host = extinfo::getExtInfoHost(
      httpserver::getURLParamater(request, "game", defaultGame));
  if (!host) {
    XMLElementPrinter elementPrinter(response);
    elementPrinter.printElement("error", "invalid game");
  } else if (extInfoRequired && !host->info.extInfoSupported) {
    FString error;
    error << host->info.game << " does not support extinfo";
    XMLElementPrinter elementPrinter(response);
    elementPrinter.printElement("error", error);
    return nullptr;
  }
  return host;
}

//
// HTTP Callbacks
//

void serverInfo(const extinfo::Server *server, const TimeType now,
                XMLElementPrinter &elementPrinter,
                const char *node = "server") {
  ShortString buf1;
  ShortString buf2;
  ShortString timeLeft;

  /*std::*/snprintf(timeLeft, sizeof(timeLeft), "%02d:%02d", server->secondsLeft / 60, server->secondsLeft % 60);

  XMLNodePrinter nodePrinter(elementPrinter, node);

  // TODO: protocolNum + Str
  // TODO: clients -> numclients
  // TODO: count down + raw, in JS?

  elementPrinter.printElement("protocol", server->protocolVersion);
  elementPrinter.printElement("release", convertToUTF8AndEscape(server->getGameReleaseName(buf1, sizeof(buf1)), buf2));
  elementPrinter.printElement("description", convertToUTF8AndEscape(server->getDescription(), buf1));

#ifndef _MSC_VER
//#warning change maxclients -> maxplayers
#endif
  elementPrinter.printElement("clients", server->numPlayers);
  elementPrinter.printElement("maxclients", server->maxPlayers);
  elementPrinter.printElement("gamemodeint", server->gameMode);
  elementPrinter.printElement("gamemode", convertToUTF8AndEscape(server->getGameModeName(), buf1));
  elementPrinter.printElement("mastermodeint", server->masterMode);
  elementPrinter.printElement("mastermode", server->getMasterModeName());
  elementPrinter.printElement("mapname", convertToUTF8AndEscape(server->getMapName(), buf1));
  elementPrinter.printElement("host", convertToUTF8AndEscape(server->serverHost, buf1));
  elementPrinter.printElement("hostlong", network::hostToNet(server->address.host));
  elementPrinter.printElement("port", server->address.port - server->host->info.infoPortOffset);
  elementPrinter.printElement("ping", static_cast<int>(server->ping));
  elementPrinter.printElement("gamespeed", server->gameSpeed);
  elementPrinter.printElement("gamepaused", server->gamePaused ? 1 : 0);
  elementPrinter.printElement("timeleftint", server->secondsLeft);
  elementPrinter.printElement("timeleft", timeLeft);
  elementPrinter.printElement("country", convertToUTF8AndEscape(server->getCountry(), buf1));
  elementPrinter.printElement("countrycode", convertToUTF8AndEscape(server->getCountry(true), buf1));

  const extinfo::Server::Extended &extended = server->extended;

  if (extended.infoOK) {
    XMLNodePrinter nodePrinter(elementPrinter, "extended");
    elementPrinter.printElement("uptime", server->getUptime());

    if (extended.serverMod.isSet()) {
      elementPrinter.printElement("servermod", convertToUTF8AndEscape(server->getServerModName(), buf1));
      elementPrinter.printElement("servermodid", *extended.serverMod);
    }

    elementPrinter.printElement("lastupdate", (now - server->uptime.lastPong));
  }

  elementPrinter.printElement("lastupdate", (now - server->info.lastPong));
}

void playerInfo(const extinfo::Player &player, const TimeType now,
                XMLElementPrinter &elementPrinter) {
  ShortString buf;

  XMLNodePrinter nodePrinter(elementPrinter, "player");

  elementPrinter.printElement("name", convertToUTF8AndEscape(player.getName(), buf));
  elementPrinter.printElement("team", convertToUTF8AndEscape(player.getTeam(), buf));

  // TODO: gun name, priv name, state name   [str]

  elementPrinter.printElement("frags", player.frags);
  elementPrinter.printElement("flags", player.flags);
  elementPrinter.printElement("deaths", player.deaths);
  elementPrinter.printElement("teamkills", player.teamkills);
  elementPrinter.printElement("accuracy", player.accuracy);
  elementPrinter.printElement("health", player.health);
  elementPrinter.printElement("armour", player.armour);
  elementPrinter.printElement("gunselect", player.gun);
  elementPrinter.printElement("priv", player.priv);
  elementPrinter.printElement("state", player.state);
  elementPrinter.printElement("ping", player.ping);
  elementPrinter.printElement("clientnum", player.cn);

  elementPrinter.printElement("country", convertToUTF8AndEscape(player.info.getCountry(), buf));
  elementPrinter.printElement("countrycode", convertToUTF8AndEscape(player.info.getCountry(true), buf));

  const extinfo::Player::Extended &extended = player.extended;

  if (extended.infoOK) {
    XMLNodePrinter nodePrinter(elementPrinter, "extended");

    if (extended.suicides.isSet()) elementPrinter.printElement("suicides", *extended.suicides);
    if (extended.shotdamage.isSet()) elementPrinter.printElement("shotdamage", *extended.shotdamage);
    if (extended.damage.isSet()) elementPrinter.printElement("damage", *extended.damage);
    if (extended.explosivedamage.isSet()) elementPrinter.printElement("explosivedamage", *extended.explosivedamage);
    if (extended.hits.isSet()) elementPrinter.printElement("hits", *extended.hits);
    if (extended.misses.isSet()) elementPrinter.printElement("misses", *extended.misses);
    if (extended.shots.isSet()) elementPrinter.printElement("shots", *extended.shots);
    if (extended.captured.isSet()) elementPrinter.printElement("captured", *extended.captured);
    if (extended.stolen.isSet()) elementPrinter.printElement("stolen", *extended.stolen);
    if (extended.defended.isSet()) elementPrinter.printElement("defended", *extended.defended);
  }

  const TimeType onlineTime = player.info.getOnlineTime(now);

  if (onlineTime == extinfo::UNKNOWN_ONLINE_TIME) elementPrinter.printElement("onlinetime", -1);
  else elementPrinter.printElement("onlinetime", onlineTime);

  elementPrinter.printElement("lastupdate", (now - player.info.lastUpdate));
  elementPrinter.printElement("sessionid", player.info.sessionID % 100000);
}

void listPlayers(const extinfo::Server *server, const TimeType now, XMLElementPrinter &elementPrinter) {
  XMLNodePrinter nodePrinter(elementPrinter, "server");
  serverInfo(server, now, elementPrinter, "info");
  for (const extinfo::Player &player : server->players) playerInfo(player, now, elementPrinter);
}

bool listPlayers(const httpserver::CallbackArgs &args) {
  extinfo::ExtInfoHost *host = getExtInfoHost(args.request, args.response, true);
  if (!host) return false;

  XMLElementPrinter elementPrinter(args.response);

  SharedLockGuard(&host->mutex);

  const TimeType now = getMilliSeconds();
  const char *serverHostStr = httpserver::getURLParamater(args.request, "server");
  const char *serverPortStr = httpserver::getURLParamater(args.request, "port");

  if (serverHostStr && serverPortStr) {
    uint32_t serverHost;
    uint16_t serverPort;

    if (!network::isIPv4Address(serverHostStr, &serverHost)) serverHost = std::strtoul(serverHostStr, nullptr, 10);

    serverPort = std::strtoul(serverPortStr, nullptr, 10);
    network::Address serverAddress{network::netToHost(serverHost), static_cast<uint16_t>(serverPort)};
    const extinfo::Server *server = host->findServer(serverAddress, false);

    if (server && server->infoOK) {
      listPlayers(server, now, elementPrinter);
    } else {
      XMLNodePrinter nodePrinter(elementPrinter, "server");
      elementPrinter.printElement("invalid", 1);
    }
  } else {
    XMLNodePrinter nodePrinter(elementPrinter, "players");

    for (const extinfo::Server *server : host->servers) {
      if (!server->infoOK || server->players.empty()) continue;
      listPlayers(server, now, elementPrinter);
    }
  }

  return true;
}

bool listServers(const httpserver::CallbackArgs &args) {
  extinfo::ExtInfoHost *host = getExtInfoHost(args.request, args.response);
  if (!host) return false;

  SharedLockGuard(&host->mutex);

  const TimeType now = getMilliSeconds();

  XMLElementPrinter elementPrinter(args.response);
  XMLNodePrinter nodePrinter(elementPrinter, "servers");

  for (const extinfo::Server *server : host->servers) {
    if (!server->infoOK) continue;
    serverInfo(server, now, elementPrinter);
  }

  return true;
}

bool findPlayer(const httpserver::CallbackArgs &args) {
  extinfo::ExtInfoHost *host = getExtInfoHost(args.request, args.response, true);
  if (!host) return false;

  ShortString name;

  constexpr int intMin = std::numeric_limits<int>::min();

  const extinfo::FindPlayer findPlayer{
    httpserver::getURLParamater(args.request, "country"),
    convertUTF8ToCube(httpserver::getURLParamater(args.request, "name"), name),
    {httpserver::getURLParamater(args.request, "clientnum"), true, intMin},
    {httpserver::getURLParamater(args.request, "frags"), true, intMin},
    {httpserver::getURLParamater(args.request, "deaths"), true, intMin},
    {httpserver::getURLParamater(args.request, "accuracy"), true, intMin}
  };

  struct CallbackInfo {
    const TimeType now;
    const extinfo::Server *lastServer;
    XMLElementPrinter elementPrinter;
  } callbackInfo{getMilliSeconds(), nullptr, args.response};

  auto callback = [](const extinfo::Server *server, const extinfo::Player &player, void *callbackData) {
    CallbackInfo &callbackInfo = *static_cast<CallbackInfo *>(callbackData);

    if (callbackInfo.lastServer != server) {
      if (callbackInfo.lastServer) callbackInfo.elementPrinter.elementEnd();
      callbackInfo.elementPrinter.element("server", true);
      serverInfo(server, callbackInfo.now, callbackInfo.elementPrinter, "info");
      callbackInfo.lastServer = server;
    }

    playerInfo(player, callbackInfo.now, callbackInfo.elementPrinter);
  };

  XMLNodePrinter nodePrinter(callbackInfo.elementPrinter, "players");

  SharedLockGuard(&host->mutex);
  host->findPlayer(findPlayer, callback, &callbackInfo);

  return true;
}

bool updateFromMaster(const httpserver::CallbackArgs &args) {
  extinfo::ExtInfoHost *host = getExtInfoHost(args.request, args.response);
  if (!host) return false;

  XMLElementPrinter elementPrinter(args.response);
  XMLNodePrinter nodePrinter(elementPrinter, "masterupdate");

  if (!allowMasterUpdate) {
    elementPrinter.printElement("disabled", 1);
    return true;
  }

  constexpr int masterUpdateID = 0x70FEFEFE;

  struct CallbackInfo {
    XMLElementPrinter &elementPrinter;
    bool timeout;
    std::mutex *infoMutex;
    std::mutex *conditionMutex;
    std::condition_variable *condition;

    void initLocking() {
      infoMutex = new std::mutex;
      conditionMutex = new std::mutex;
      condition = new std::condition_variable;
    }

    void deinitLocking() {
      delete infoMutex;
      delete conditionMutex;
      delete condition;
    }

    ~CallbackInfo() { deinitLocking(); }
  };

  auto eventCallback = [](const struct extinfo::ExtInfoHost *host, const extinfo::Event event,
                          const extinfo::EventData &eventData, void *callbackData) {
    if (event != extinfo::MASTER_UPDATE) return;

    const extinfo::MasterUpdateStatus *status = static_cast<const extinfo::MasterUpdateStatus *>(eventData.data[0]);

    CallbackInfo *callbackInfo = static_cast<CallbackInfo *>(callbackData);
    callbackInfo->infoMutex->lock();

    if (callbackInfo->timeout || status->id != masterUpdateID) {
      callbackInfo->infoMutex->unlock();
      return;
    }

    XMLElementPrinter &elementPrinter = callbackInfo->elementPrinter;

    elementPrinter.printElement<int>("success", status->success > 0);
    elementPrinter.printElement("numservers", status->numServers);

    callbackInfo->elementPrinter.printElement("elapsedtime", getMilliSeconds() - host->lastMasterUpdate);

    callbackInfo->condition->notify_one();
    callbackInfo->infoMutex->unlock();
  };

  std::unique_ptr<CallbackInfo> callbackInfo(new CallbackInfo({elementPrinter}));

  // Ensure addEventCallback() and deleteEventCallback()
  // are always called with the same pointers
  extinfo::EventCallback::Fun eventCallbackAddr = eventCallback;
  void *callbackInfoAddr = callbackInfo.get();

  auto printInProgress = [&]() {
    elementPrinter.printElement("inprogress", 1);
  };

  {
    LockGuard(&host->mutex);

    const TimeType timeDiff = getMilliSeconds() - host->lastMasterUpdate;

    if (timeDiff < masterUpdateLimit) {
      elementPrinter.printElement("wait", masterUpdateLimit - timeDiff);
      return true;
    }

    if (!host->masterUpdateThread) {
      // Queue to avoid creating the update thread from this plugin
      host->updateFromMaster(masterUpdateID, true);
    }

    const int threadPoolSize = httpserver::getThreadPoolSize();
    int threadsInUse = 1;

    for (extinfo::ExtInfoHost &extinfoHost : extinfo::hosts) {
      if (!extinfoHost.enabled || &extinfoHost == host) continue;
      SharedLockGuard(&extinfoHost.mutex);
      if (extinfoHost.masterUpdateThread && extinfoHost.masterUpdateStatus.id == masterUpdateID) ++threadsInUse;
    }

    if (threadPoolSize - threadsInUse < 1) {
      printInProgress();
      return true;
    }

    callbackInfo->initLocking();
    host->addEventCallback({eventCallbackAddr, callbackInfoAddr});
  }

  {
    std::unique_lock<std::mutex> lock(*callbackInfo->conditionMutex);

    bool timeout = callbackInfo->condition->wait_for(lock, std::chrono::milliseconds(500)) == std::cv_status::timeout;

    // Must be always locked to avoid data races
    LockGuard(callbackInfo->infoMutex);

    if (timeout) {
      callbackInfo->timeout = true;
      printInProgress();
    }
  }

  {
    LockGuard(&host->mutex);
    host->deleteEventCallback({eventCallbackAddr, callbackInfoAddr});
  }

  return true;
}

bool showInfo(const httpserver::CallbackArgs &args) {
  XMLElementPrinter elementPrinter(args.response);
  XMLNodePrinter nodePrinter(elementPrinter, "info");

  const char *compilerInfo[2];
  getCompilerInfo(compilerInfo);

  elementPrinter.printElement("revision", TOSTRING(REVISION));
  elementPrinter.printElement("os", getOSName());

  {
    XMLNodePrinter nodePrinter(elementPrinter, "compiler");
    elementPrinter.printElement("name", compilerInfo[0]);
    elementPrinter.printElement("version", compilerInfo[1]);
  }

  return true;
}

bool showConfiguration(const httpserver::CallbackArgs &args) {
  XMLElementPrinter elementPrinter(args.response);
  elementPrinter.element("config", true);

  elementPrinter.printElement("updateinterval", updateInterval);

  for (const extinfo::ExtInfoHost &host : extinfo::hosts) {
    if (!host.enabled) continue;

    XMLNodePrinter nodePrinter(elementPrinter, "game");

    elementPrinter.printElement("name", host.info.game);
    elementPrinter.printElement("desc", host.info.desc);
    elementPrinter.printElement("extinfo", host.info.extInfoSupported ? 1 : 0);

    if (!std::strcmp(host.info.game, defaultGame)) elementPrinter.printElement("default", 1);
  }

  return true;
}

} // anonymous namespace

bool init() {
  updateInterval = plugincfg->getInt("web.updateInterval", oneSecond, oneMinute, oneSecond * 5);

  allowMasterUpdate = plugincfg->getBool("web.allowMasterUpdate", true);
  masterUpdateLimit = plugincfg->getInt("web.masterUpdateLimit", oneMinute * 5, oneHour * 12, oneMinute * 15);
  defaultGame = plugincfg->getString("web.defaultGame", "sauerbraten");

  if (!extinfo::getExtInfoHost(defaultGame)) {
    err << "web plugin: invalid default game (check your configuration)!" << err.endl();
    return false;
  }

  httpserver::addCallback("/servers", listServers);
  httpserver::addCallback("/players", listPlayers);
  httpserver::addCallback("/findplayer", findPlayer);
  httpserver::addCallback("/updatefrommaster", updateFromMaster);
  httpserver::addCallback("/info", showInfo);
  httpserver::addCallback("/config", showConfiguration);
  return true;
}

void deinit() {
  httpserver::deleteCallback("/servers");
  httpserver::deleteCallback("/players");
  httpserver::deleteCallback("/findplayer");
  httpserver::deleteCallback("/updatefrommaster");
  httpserver::deleteCallback("/info");
  httpserver::deleteCallback("/config");
}

} // namespace web
