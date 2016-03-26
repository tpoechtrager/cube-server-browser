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

// Use the C variant of libconfig to avoid C++ exceptions
#include <libconfig.h>
#include <cstdio>
#include <cstring>
#include <limits>
#include "config.h"
#include "tools.h"

namespace config {

#define CHECK(fallback)                                                        \
  do {                                                                         \
    if (!ok) return fallback;                                                  \
  } while (0)

void Config::printError() {
  err << fileName << ": " << config_error_text(cfg) << err.endl();
}

bool Config::exists(const char *path) {
  CHECK(false);
  return !!config_lookup(cfg, path);
}

bool Config::getBool(const char *varName, const bool fallback) {
  CHECK(fallback);
  int varVal;
  if (!config_lookup_bool(cfg, varName, &varVal)) return fallback;
  return varVal != 0;
}

int Config::getInt(const char *varName, const int fallback) {
  CHECK(fallback);
  int varVal;
  if (!config_lookup_int(cfg, varName, &varVal)) return fallback;
  return varVal;
}

int Config::getInt(const char *varName, const int varValMin,
                   const int varValMax, const int fallback) {
  CHECK(fallback);
  int varVal = getInt(varName, fallback);
  return clamp(varVal, varValMin, varValMax);
}

float Config::getFloat(const char *varName, const float fallback) {
  CHECK(fallback);
  double varVal;
  if (!config_lookup_float(cfg, varName, &varVal)) return fallback;
  return varVal;
}

float Config::getFloat(const char *varName, const float varValMin,
                       const float varValMax, const float fallback) {
  CHECK(fallback);
  float varVal = getFloat(varName, fallback);
  return clamp(varVal, varValMin, varValMax);
}

const char *Config::getString(const char *varName, const char *fallback) {
  CHECK(fallback);
  const char *varVal;
  if (!config_lookup_string(cfg, varName, &varVal)) return fallback;
  return varVal;
}

const char **Config::getStringArray(const char *varName,
                                    const char **fallback) {
  CHECK(fallback);

  size_t i = 0;
  const char *arr[1024];

  for (; i < sizeofarray(arr); ++i) {
    char arrVarName[1024];
    /*std::*/ snprintf(arrVarName, sizeof(arrVarName), "%s.[%u]", varName, static_cast<unsigned int>(i));
    arr[i] = getString(arrVarName);
    if (!arr[i]) break;
  }

  if (i) {
    const char **arrBuf = new const char *[i + 1];
    std::memcpy(arrBuf, arr, sizeof(arr[0]) * (i + 1));
    return arrBuf;
  }

  return fallback;
}

Config::Config(const char *fileName) : fileName(fileName) {
  cfg = new config_t;
  config_init(cfg);
  ok = config_read_file(cfg, fileName) != 0;
  if (!ok) printError();
}

Config::~Config() {
  if (cfg) {
    config_destroy(cfg);
    delete cfg;
  }
}

Var::~Var() {
  if (defaultVal) {
    switch (type) {
    case VarType<bool>::type: {
      deleteDefaultVal<bool>(defaultVal, isArray());
      break;
    }
    case VarType<int>::type: {
      deleteDefaultVal<int>(defaultVal, isArray());
      break;
    }
    case VarType<float>::type: {
      deleteDefaultVal<float>(defaultVal, isArray());
      break;
    }
    case VarType<const char *>::type: {
      deleteDefaultVal<const char *>(defaultVal, isArray());
      break;
    }
    default:
      std::abort();
    }
  }

  if (range) {
    switch (type) {
    case VarType<int>::type: {
      deleteVarRange<int>(range, isArray());
      break;
    }
    case VarType<float>::type: {
      deleteVarRange<float>(range, isArray());
      break;
    }
    default:
      std::abort();
    }
  }
}

template <typename T>
T &getVarValRef(char *p, const size_t index) {
  return *reinterpret_cast<T *>(p + sizeof(T) * index);
}

template <typename T>
T getDefaultVarVal(const Var &var, const size_t index) {
  if (!var.defaultVal) return T();
  const DefaultVarVal<T> *p = static_cast<const DefaultVarVal<T> *>(var.defaultVal) + index;
  return p->val;
}

template <typename T>
void getVarRange(const Var &var, const size_t index, T &min, T &max) {
  if (!var.range) {
    min = std::numeric_limits<T>::min();
    max = std::numeric_limits<T>::max();
    return;
  }

  const VarRange<T> *p = static_cast<const VarRange<T> *>(var.range) + index;

  min = p->min;
  max = p->max;
}

void loadSettings(Config *cfg, const Var *vars) {
  FString varName;
  FString index;

  for (size_t i = 0; vars[i].p; ++i) {
    const Var &var = vars[i];
    char *p = static_cast<char *>(var.p);

    varName.assign(var.name);
    index.clear();

    for (size_t j = 0; j < var.numVals; ++j) {
      if (var.isArray()) {
        index.clear();
        index << ".[" << j << "]";
      }

      switch (var.type) {
      case VarType<bool>::type: {
        bool &val = getVarValRef<bool>(p, j);
        val = cfg->getBool(tmpAppend<>(varName, index)->c_str(), getDefaultVarVal<bool>(var, j));
        break;
      }
      case VarType<int>::type: {
        int min, max;
        getVarRange(var, j, min, max);
        int &val = getVarValRef<int>(p, j);
        val = cfg->getInt(tmpAppend<>(varName, index)->c_str(), min, max, getDefaultVarVal<int>(var, j));
        break;
      }
      case VarType<float>::type: {
        float min, max;
        getVarRange(var, j, min, max);
        float &val = getVarValRef<float>(p, j);
        val = cfg->getFloat(tmpAppend<>(varName, index)->c_str(), min, max, getDefaultVarVal<float>(var, j));
        break;
      }
      case VarType<const char *>::type: {
        const char *&val = getVarValRef<const char *>(p, j);
        val = cfg->getString(tmpAppend<>(varName, index)->c_str(), getDefaultVarVal<const char *>(var, j));
        break;
      }
      }
    }
  }
}

bool saveSettings(const char *fileName, const Var *vars) {
  std::FILE *f = std::fopen(fileName, "w");

  if (!f)
    return false;

  for (size_t i = 0; vars[i].p; ++i) {
    const Var &var = vars[i];
    char *p = static_cast<char *>(var.p);

    std::fprintf(f, "%s = ", var.name);

    if (var.isArray()) std::fprintf(f, "[");

    for (size_t j = 0; j < var.numVals; ++j) {
      switch (var.type) {
      case VarType<bool>::type: {
        const bool &val = getVarValRef<const bool>(p, j);
        std::fprintf(f, "%s", val ? "true" : "false");
        break;
      }
      case VarType<int>::type: {
        const int &val = getVarValRef<const int>(p, j);
        std::fprintf(f, "%d", val);
        break;
      }
      case VarType<float>::type: {
        const float &val = getVarValRef<const float>(p, j);
        std::fprintf(f, "%f", val);
        break;
      }
      case VarType<const char *>::type: {
        const char *const &val = getVarValRef<const char *>(p, j);
        std::fprintf(f, "\"%s\"", val); // TODO: escape
        break;
      }
      }

      if (var.isArray()) {
        if (j == var.numVals - 1) std::fprintf(f, "]");
        else std::fprintf(f, ", ");
      }
    }

    std::fprintf(f, ";\n");
  }

  std::fclose(f);
  return true;
}

} // namespace config
