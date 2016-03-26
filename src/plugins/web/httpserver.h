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


#ifndef __HTTPSERVER_H__
#define __HTTPSERVER_H__

#include <ctime>
#include <string>
#include <sstream>
#include <vector>
#include <cstdint>
#include "main.h"
#include "network.h"
#include "tools.h"

struct MHD_Connection;

namespace httpserver {

//
// Structs & Constants
//

constexpr size_t MAX_URI_LENGTH = 512;

struct Request {
  //TimeType start = getNanoSeconds();
  MHD_Connection *connection;
  network::Address address;
  char IPAddress[64];
  std::string uri;
  const char *parsedURI;
};

struct Header {
  const char *name;
  const char *value;
};

struct Response {
  FString &content;
  std::vector<Header> headers;
  unsigned int code = 200;
  const char *serverDesc = getApplicationName();
  const char *mimeType = nullptr;
  time_t lastModified = 0;
  bool fileRequest = false;
  const char *wwwRoot = nullptr;
  const char *indexFile = nullptr;
  Response(FString &content) : content(content) {}
};

struct CallbackArgs {
  Request &request;
  Response &response;
};

struct Callback {
  typedef bool (*Fun)(const CallbackArgs &args);
  Fun fun;
  uint32_t inUse;
  bool uninstallRequest;
};

class CallbackLocker {
private:
  Callback *callback;

  void acquire();
  void release();

public:
  Callback *operator->() const { return callback; }
  bool isValid() const { return !!callback; }

  void operator=(CallbackLocker &&callbackLocker);

  CallbackLocker() : callback() {}
  CallbackLocker(const char *uri);
  CallbackLocker(Callback *callback);

  ~CallbackLocker();
};

//
// Request Callback
//

bool addCallback(const char *request, Callback::Fun callbackFun);
void deleteCallback(const char *request);
Callback *getCallback(const char *request, bool lock = true);

//
// Misc
//

const char *getHeaderParamater(const Request &request, const char *param, const char *fallback = nullptr);
const char *getURLParamater(const Request &request, const char *param, const char *fallack = nullptr);
int getURLParamaterInt(const Request &request, const char *param, const int fallback = 0);

const char *getUserAgent(const Request &request, const char *fallback);
bool encodingSupported(const Request &request, const char *encoding);

bool containsHTMLEntities(const char *str);
const char *htmlEncode(const char *str, char *buf, const size_t size);

template <typename T, size_t N>
const char *htmlEncode(const char *str, T (&buf)[N]) {
  return htmlEncode(str, buf, N);
}

const char *getMimeType(const char *file);
bool isHTMLMimeType(const char *mimeType);
bool isPictureMimeType(const char *mimeType);

int getThreadPoolSize();

bool init();
void deinit();
} // namespace httpserver

#endif //__HTTPSERVER_H__
