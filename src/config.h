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

#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <string>
#include <vector>

struct config_t;

namespace config {

class Config {
private:
  std::string fileName;
  config_t *cfg;
  bool ok;
  void printError();

public:
  bool exists(const char *path);

  bool getBool(const char *varName, const bool fallback);

  int getInt(const char *varName, const int fallback = 0);
  int getInt(const char *varName, const int varValMin, const int varValMax, const int fallback);

  float getFloat(const char *varName, const float fallback = 0);
  float getFloat(const char *varName, const float varValMin, const float varValMax, const float fallback);

  const char *getString(const char *varName, const char *fallback = nullptr);
  const char **getStringArray(const char *varName, const char **fallback = nullptr);

  Config(const char *file);
  Config() : fileName(), cfg(), ok() {}
  ~Config();
};

template <typename T>
struct DefaultVarVal { T val; };

template <typename T>
DefaultVarVal<T> *dupDefaultVarVal(const size_t count, const T &&val) {
  DefaultVarVal<T> *p = new DefaultVarVal<T>[count];
  for (size_t i = 0; i < count; ++i) p[i].val = val;
  return p;
}

template <typename T>
void deleteDefaultVal(void *defaultVal, const bool isArray) {
  DefaultVarVal<T> *p = static_cast<DefaultVarVal<T> *>(defaultVal);
  if (isArray) delete[] p;
  else delete p;
}

template <typename T>
struct VarRange {
  const T min;
  const T max;
};

template <typename T>
void deleteVarRange(void *range, const bool isArray) {
  VarRange<T> *p = static_cast<VarRange<T> *>(range);
  if (isArray) delete[] p;
  else delete p;
}

template <typename T> struct VarType;
template <> struct VarType<bool> { static constexpr int type = 1; };
template <> struct VarType<int> { static constexpr int type = 2; };
template <> struct VarType<float> { static constexpr int type = 3; };
template <> struct VarType<const char *> { static constexpr int type = 4; };

struct Var {
  const int type;
  const char *name;
  void *p;
  const size_t numVals;
  void *defaultVal;
  void *range;

  bool isArray() const { return numVals > 1; }

  template <typename T = int, typename X = DefaultVarVal<T> >
  Var(const char *name = nullptr, T *p = nullptr, const size_t numVals = 1,
      X *defaultVal = nullptr, void *range = nullptr)
      : type(VarType<T>::type), name(name), p(p), numVals(numVals),
        defaultVal(defaultVal), range(range) {
    typedef decltype(defaultVal->val) dt;
    static_assert(VarType<T>::type == VarType<dt>::type, "");
  }

  ~Var();
};

void loadSettings(Config *cfg, const Var *vars);
bool saveSettings(const char *fileName, const Var *vars);

} // namespace config

#endif // __CONFIG_H__
