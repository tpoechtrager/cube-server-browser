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

#include <vector>
#include <string>
#include <memory>
#include <cstring>
#include <climits>
#include <dlfcn.h>
#include "plugin.h"
#include "tools.h"
#include "main.h"

#ifndef _WIN32
#include <unistd.h>
#endif

namespace plugin {

bool PLUGIN_API_VERSION_VAR;

bool Plugin::abicheck(const size_t size) const {
  if (size != sizeof(*this)) {
    err << "plugin ABI check failed" << err.endl();
    return false;
  }
  return true;
}

void *Plugin::getSymbolAddress(const char *symbol) {
  return dlsym(handle, symbol);
}

Plugin::Plugin(Plugin &&plugin) {
  name = plugin.name;
  fileName = std::move(plugin.fileName);
  handle = plugin.handle;
  cfg = plugin.cfg;
  plugin.handle = nullptr;
}

Plugin::Plugin(const char *name, bool &ok) : name(name), handle(), cfg() {
  fileName << PLUGIN_DIR << PATH_DIV << name << "-plugin" << PLUGIN_EXT;
  handle = dlopen(fileName.c_str(), RTLD_NOW | RTLD_LOCAL);
  ok = !!handle;
  if (ok) {
    cfgFileName << PLUGIN_DIR << PATH_DIV << name << "-plugin.cfg";
    if (fileExists(cfgFileName)) cfg = new config::Config(cfgFileName.c_str());
    else cfg = new config::Config;
    dataDir << DATA_DIR << PATH_DIV << "plugin" << PATH_DIV << name;
  }
}

Plugin::~Plugin() {
  if (handle) {
    dlclose(handle);
    delete cfg;
#ifdef RTLD_NOLOAD
    if (dlopen(fileName.c_str(), RTLD_NOW | RTLD_GLOBAL | RTLD_NOLOAD))
      warn << "the loader did not unload '" << fileName << "'" << warn.endl();
#endif
  }
}

namespace {
std::vector<Plugin *> plugins;
std::mutex reloadMutex;
std::vector<std::string> pluginsToReload;

void unload(const char *name);

bool load(const char *name) {
  unload(name);
  bool ok;
  std::unique_ptr<Plugin> plugin(new Plugin(name, ok));
  if (!ok) {
    const char *errMsg = dlerror();
    if (errMsg) err << "failed to load plugin '" << name << "': " << errMsg << err.endl();
    return false;
  }
  PluginInitFun init = plugin->getFunction<PluginInitFun>("init");
  if (init && !init(plugin.get())) return false;
  plugin->setProcessFun(plugin->getFunction<PluginProcessFun>("process"));
  plugins.push_back(plugin.release());
  return true;
}

void unload(const char *name) {
  for (size_t i = plugins.size(); i-- > 0;) {
    Plugin *plugin = plugins[i];
    if (!std::strcmp(plugin->getName(), name)) {
      PluginUnloadFun unload = plugin->getFunction<PluginUnloadFun>("unload");
      if (unload) unload();
      delete plugin;
      plugins.erase(plugins.begin() + i);
      name = nullptr;
      break;
    }
  }
}
} // anonymous namespace

void reload(const char *name) {
  LockGuard(&reloadMutex);
  pluginsToReload.push_back(name);
}

void process() {
  for (const Plugin *plugin : plugins) {
    PluginProcessFun process = plugin->getProcessFun();
    if (process) process();
  }
  LockGuard(&reloadMutex);
  for (const CString name : pluginsToReload) {
    unload(*name);
    load(*name);
  }
  pluginsToReload.clear();
}

bool init() {
  const char **plugins = cfg->getStringArray("plugins");
  if (plugins) {
    for (const char **plugin = plugins; *plugin; ++plugin) load(*plugin);
    delete[] plugins;
  }
  return true;
}

void deinit() {
  for (size_t i = plugins.size(); i-- > 0;) unload(plugins[i]->getName());
}

} // namespace plugin
