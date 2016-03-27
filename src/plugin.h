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

#ifndef __PLUGIN_H__
#define __PLUGIN_H__

#include <cstring>
#include "tools.h"
#include "config.h"

namespace plugin {

#define PLUGIN_API_VERSION 1

#define PLUGIN_API_VERSION_VAR CONC(__PLUGIN_API_VERSION, PLUGIN_API_VERSION)
extern "C" PLUGIN_IMPORT bool PLUGIN_API_VERSION_VAR;

#ifdef __APPLE__
constexpr const char *PLUGIN_EXT = ".dylib";
#elif defined(_WIN32)
constexpr const char *PLUGIN_EXT = ".dll";
#else
constexpr const char *PLUGIN_EXT = ".so";
#endif

constexpr const char *PLUGIN_DIR = "plugins";

typedef bool (*PluginInitFun)(const class Plugin *pplugin);
typedef void (*PluginUnloadFun)();
typedef void (*PluginProcessFun)();

class Plugin {
private:
  std::string name;
  FString fileName;
  void *handle;
  PluginProcessFun process;
  config::Config *cfg;
  FString cfgFileName;
  FString dataDir;

public:
  bool abicheck(const size_t size) const;

  const char *getName() const { return name.c_str(); }
  void *getSymbolAddress(const char *symbol);

  template <typename T> T getFunction(const char *name) {
    static_assert(sizeof(T) == sizeof(void *), "");
    void *symbolAddr = getSymbolAddress(name);
    T functionAddr;
    std::memcpy(&functionAddr, &symbolAddr, sizeof(T));
    return functionAddr;
  }

  void setProcessFun(PluginProcessFun process_) { process = process_; }
  PluginProcessFun getProcessFun() const { return process; }

  config::Config *getcfg() const { return cfg; }
  const char *getcfgFileName() const { return cfgFileName.c_str(); }
  const char *getDataDir() const { return dataDir.c_str(); }

  Plugin(Plugin &&plugin);

  Plugin(const char *name, bool &ok);
  ~Plugin();
};

void reload(const char *name);

void process();
bool init();
void deinit();

} // namespace plugin

#define PLUGIN_INIT()                                                                                                  \
  const plugin::Plugin *pplugin;                                                                                       \
  static bool isInited = false;                                                                                        \
  static inline bool _init();                                                                                          \
  extern "C" bool init(const plugin::Plugin *_plugin) {                                                                \
    plugin::PLUGIN_API_VERSION_VAR = true; /* force reference */                                                       \
    if (isInited) return false;                                                                                        \
    pplugin = _plugin;                                                                                                 \
    if (!pplugin->abicheck(sizeof(plugin::Plugin))) return false;                                                      \
    if (_init()) {                                                                                                     \
      isInited = true;                                                                                                 \
      return true;                                                                                                     \
    }                                                                                                                  \
    return false;                                                                                                      \
  }                                                                                                                    \
  static inline bool _init()

#define PLUGIN_UNLOAD()                                                                                                \
  static inline void _unload();                                                                                        \
  extern "C" void unload() {                                                                                           \
    _unload();                                                                                                         \
    isInited = false;                                                                                                  \
  }                                                                                                                    \
  static inline void _unload()

#define PLUGIN_PROCESS()                                                                                               \
  extern "C" void process()

extern const plugin::Plugin *pplugin;
#define pluginName (pplugin->getName())
#define plugincfg (pplugin->getcfg())
#define plugincfgFileName (pplugin->getcfgFileName())
#define pluginDataDir (pplugin->getDataDir())

#endif // __PLUGIN_H__
