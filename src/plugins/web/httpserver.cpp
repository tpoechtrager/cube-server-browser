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
#include <climits>
#include <cstddef>
#include <cstring>
#include <cstdint>
#include <cassert>
#include <mutex>
#include <string>
#include <vector>
#include <map>
#include <sys/stat.h>
#include "httpserver.h"
#include "network.h"
#include "tools.h"
#include "main.h"
#include "plugin.h"
#include <microhttpd.h>

#ifndef _MSC_VER
#include <unistd.h>
#endif

#ifdef _WIN32
#include "compat/win32/compat.h"
#endif

namespace httpserver {

namespace {
std::string httpsCert;
std::string httpsKey;
std::vector<MHD_Daemon *> daemons;
FString wwwRoot;
const char *wwwRootIndexFile;
unsigned mhdFlags;
int threadPoolSize;
bool compress;
int compressionLevel;
std::mutex callbackMutex;
std::map<const std::string, Callback *> callbacks;
} // anonymous namespace

//
// Struct Functions
//

void CallbackLocker::acquire() {
  if (isValid()) ++callback->inUse;
}

void CallbackLocker::release() {
  if (isValid()) --callback->inUse;
}

void CallbackLocker::operator=(CallbackLocker &&callbackLocker) {
  assert(!callback && "callback already assigned");
  callback = callbackLocker.callback;
  callbackLocker.callback = nullptr;
}

CallbackLocker::CallbackLocker(Callback *callback) : callback(callback) {
  LockGuard(&callbackMutex);
  acquire();
}

CallbackLocker::CallbackLocker(const char *uri) {
  LockGuard(&callbackMutex);
  callback = getCallback(uri, false);
  acquire();
}

CallbackLocker::~CallbackLocker() {
  if (!isValid()) return;
  LockGuard(&callbackMutex);
  release();
}

//
// Request Callback
//

bool addCallback(const char *request, Callback::Fun callbackFun) {
  LockGuard(&callbackMutex);
  if (callbacks.find(request) != callbacks.end()) return false;
  callbacks.insert({request, new Callback{callbackFun}});
  return true;
}

void deleteCallback(const char *request) {
  do {
    {
      LockGuard(&callbackMutex);
      decltype(callbacks)::iterator it = callbacks.find(request);
      if (it == callbacks.end()) return;
      Callback *callback = it->second;
      if (!callback->inUse) {
        delete callback;
        callbacks.erase(it);
        break;
      }
      callback->uninstallRequest = true;
    }
    usleep(1000);
  } while (true);
}

Callback *getCallback(const char *request, bool lock) {
  LockGuard(lock ? &callbackMutex : nullptr);
  decltype(callbacks)::iterator it = callbacks.find(request);
  if (it != callbacks.end()) {
    Callback *callback = it->second;
    if (!callback->uninstallRequest) return callback;
  }
  return nullptr;
}

//
// Request processing
//

namespace {

bool readRequestFileIntoResponseStream(Request &request, Response &response) {
  if (*request.parsedURI != '/') return false;
  const char *file = request.parsedURI + 1;
  if (std::strlen(file) >= 120) return false;
  if (!*file) file = wwwRootIndexFile;

  std::string root;
  std::string strFile;

  char workingDir[PATH_MAX];
  char resolvedRoot[PATH_MAX];
  char resolvedFile[PATH_MAX];

  if (!getcwd(workingDir, sizeof(workingDir))) return false;

  root = workingDir;
  root += PATH_DIV;
  root += wwwRoot;

  if (!realpath(root.c_str(), resolvedRoot)) return false;

  root = resolvedRoot;

  strFile = root;
  strFile += PATH_DIV;
  strFile += file;

  if (!realpath(strFile.c_str(), resolvedFile))
    return false;

  if (std::strncmp(root.c_str(), resolvedFile, root.length())) {
    *logFile << "http request: " << request.IPAddress << ":"
             << request.address.port
             << ": directory traversal attempt: "
             << root << " != " << resolvedFile << logFile->endl();
    return false;
  }

  struct stat fileAttr;

  if (stat(strFile.c_str(), &fileAttr) || !S_ISREG(fileAttr.st_mode))
    return false;

  response.lastModified = fileAttr.st_mtime;
  const char *ifModifiedSince = getHeaderParamater(request, MHD_HTTP_HEADER_IF_MODIFIED_SINCE);
  struct tm tm;

  if (ifModifiedSince && strptime(ifModifiedSince, "%a, %d %b %Y %H:%M:%S", &tm)) {
    const char *timeZone = std::strrchr(ifModifiedSince, ' ');

    if (timeZone && (!strncasecmp(timeZone + 1, "GMT", 3) || !strncasecmp(timeZone + 1, "UTC", 3)) &&
                     mkgmtime(&tm) == response.lastModified) {
      response.code = MHD_HTTP_NOT_MODIFIED;
      response.lastModified = 0;
      return true;
    }
  }

  if (!readFile(strFile, response.content)) return false;
  response.mimeType = httpserver::getMimeType(file);
  return true;
}

Request *getRequest(void **cls) {
  if (!*cls) return nullptr;
  return static_cast<Request *>(*cls);
}

void *requestLogger(void *, const char *uri, struct MHD_Connection *connection) {
  Request *request = new Request;

  const union MHD_ConnectionInfo *connectionInfo =
      MHD_get_connection_info(connection, MHD_CONNECTION_INFO_CLIENT_ADDRESS);

  if (!connectionInfo ||
      !network::getHostIPAddress(connectionInfo->client_addr, request->IPAddress,
                                 sizeof(request->IPAddress), &request->address)) {
    delete request;
    return nullptr;
  }

  request->connection = connection;
  request->uri = uri;

  return request;
}

void requestCompleted(void *, struct MHD_Connection *, void **reqCls, enum MHD_RequestTerminationCode) {
  Request *request = getRequest(reqCls);

  if (!request)
    return;

  delete request;
}

int request(void *, MHD_Connection *connection, const char *uri, const char *method,
            const char *, const char *, size_t *, void **reqCls) {
  int retVal;

  if (std::strcmp(method, "GET")) return MHD_NO;

  Request *requestPtr = getRequest(reqCls);

  if (!requestPtr) return MHD_NO;

  Request &request = *requestPtr;
  request.parsedURI = uri;

  // Must assign a value here to work around a bug in libstdc++
  // https://gcc.gnu.org/ml/gcc/2013-02/msg00227.html

  thread_local FString content = " ";
  content.clear();

  Response response{content};
  CallbackLocker callback;

  if (request.uri.length() <= MAX_URI_LENGTH) {
    bool ok;

    callback = CallbackLocker(uri);

    if (callback.isValid())
      ok = callback->fun({request, response});
    else
      ok = readRequestFileIntoResponseStream(request, response);

    if (!ok) {
      response.code = MHD_HTTP_BAD_REQUEST;
      if (response.content.empty())
        response.content << "Bad Request";
    }
  } else {
    response.code = MHD_HTTP_REQUEST_URI_TOO_LONG;
    response.content << "Request-URI Too Long";
  }

  if (!response.content.empty() && !response.mimeType) response.mimeType = "text/html; charset=utf-8";

  //
  // Internet Explorer and friends are sending 'Accept-Encoding: deflate, gzip'
  // while they cannot cope with deflate, so use gzip everywhere to avoid
  // content decoding errors. http://stackoverflow.com/a/5186177
  //

  bool compressResponse = compress && !response.content.empty() && encodingSupported(request, "gzip");
  if (compressResponse && response.mimeType && isPictureMimeType(response.mimeType)) compressResponse = false;

  MHD_Response *resp;

  if (compressResponse) {
    size_t length = compressBufSize(response.content.length());
    void *buf;

#ifdef _MSC_VER
    const auto &malloc = mingw_malloc;
    const auto &free = mingw_free;
#endif

    buf = malloc(length);
    if (!buf) return MHD_NO;

    if (!::compress(buf, length, response.content.data(), response.content.length(), compressionLevel, true)) {
      free(buf);
      compressResponse = false;
      goto noCompression;
    }

    resp = MHD_create_response_from_buffer(length, buf, MHD_RESPMEM_MUST_FREE);
  } else {
    noCompression:;

    MHD_ResponseMemoryMode memoryModel;

#ifdef TLS_SUPPORTED
    memoryModel = MHD_RESPMEM_PERSISTENT;
#else
    memoryModel = MHD_RESPMEM_MUST_COPY;
#endif

    void *data = const_cast<void *>(reinterpret_cast<const void *>(response.content.data()));
    resp = MHD_create_response_from_buffer(response.content.length(), data, memoryModel);
  }

  if (!resp) return MHD_NO;

#define ADD_RESPONSE_HEADER(header, content)                                                                           \
  do {                                                                                                                 \
    if (!MHD_add_response_header(resp, header, content)) return MHD_NO;                                                \
  } while (0)

  if (response.serverDesc) ADD_RESPONSE_HEADER(MHD_HTTP_HEADER_SERVER, response.serverDesc);

  if (response.lastModified) {
    char buf[64];
    struct tm tm;
    if (gmtime_r(&response.lastModified, &tm) && std::strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S GMT", &tm))
      ADD_RESPONSE_HEADER(MHD_HTTP_HEADER_LAST_MODIFIED, buf);
  }

  if (response.mimeType) ADD_RESPONSE_HEADER(MHD_HTTP_HEADER_CONTENT_TYPE, response.mimeType);
  if (compressResponse) ADD_RESPONSE_HEADER(MHD_HTTP_HEADER_CONTENT_ENCODING, "gzip");

// TODO conf + check
#if 0
  if (!data.empty()) {
    if (response.mimeType && isHTMLMimeType(response.mimeType)) ADD_RESPONSE_HEADER("x-frame-options", "SAMEORIGIN");
    ADD_RESPONSE_HEADER("x-xss-protection", "1; mode=block");
  }
#endif

  for (const Header &header : response.headers) ADD_RESPONSE_HEADER(header.name, header.value);

#undef ADD_RESPONSE_HEADER

  retVal = MHD_queue_response(connection, response.code, resp);
  MHD_destroy_response(resp);

  // Log the HTTP-Request

  thread_local FString logEntry;
  logEntry.clear();

  logEntry << "http request: " << request.IPAddress << ":" << request.address.port << ": ";

  if (request.uri.length() <= MAX_URI_LENGTH) {
    logEntry << request.uri;
  } else {
    char shrinkedURI[516];
    std::memcpy(shrinkedURI, request.uri.data(), 512);
    shrinkedURI[512] = '\0';
    logEntry << shrinkedURI << "[...]";
  }

  logEntry << " [" << response.code << "]";
  if (compressResponse) logEntry << " [gzip]";

  *logFile << logEntry << logFile->endl();

  return retVal;
}

//
// Misc
//

const char *getConnectionParameter(const MHD_ValueKind kind, const Request &request,
                                   const char *param, const char *fallback) {
  const char *val = MHD_lookup_connection_value(request.connection, kind, param);
  return val ? val : fallback;
}

} // anonymous namespace

const char *getHeaderParamater(const Request &request, const char *param, const char *fallback) {
  return getConnectionParameter(MHD_HEADER_KIND, request, param, fallback);
}

const char *getURLParamater(const Request &request, const char *param, const char *fallback) {
  return getConnectionParameter(MHD_GET_ARGUMENT_KIND, request, param, fallback);
}

int getURLParamaterInt(const Request &request, const char *param, const int fallback) {
  const char *val = getURLParamater(request, param);
  if (!val) return fallback;
  return std::atoi(val);
}

const char *getUserAgent(const Request &request, const char *fallback) {
  return getHeaderParamater(request, MHD_HTTP_HEADER_USER_AGENT, fallback);
}

bool encodingSupported(const Request &request, const char *encoding) {
  const char *param = getHeaderParamater(request, "Accept-Encoding");
  const ptrdiff_t encodingStrLen = std::strlen(encoding);
  if (!param) return false;
  const char *start = param;
  while (*param) {
    if (*param == ',') {
      if (param > start && param - start == encodingStrLen)
        if (!std::memcmp(start, encoding, encodingStrLen))
          return true;
      ++param; // ','
      while (*param && *param == ' ') ++param;
      start = param;
    }
    if (*param) ++param;
  }
  return param - start == encodingStrLen && !std::memcmp(start, encoding, encodingStrLen);
}

bool containsHTMLEntities(const char *str) {
  while (*str) {
    switch (*str) {
    case '"':
    case '&':
    case '<':
    case '>':
      return true;
    }
  }
  return false;
}

const char *htmlEncode(const char *str, char *buf, const size_t size) {
  char *p = buf;
  size_t left = size;
  auto fits = [&](const size_t length) {
    if (left >= length) {
      left -= length;
      return true;
    }
    return false;
  };
  while (*str) {
    switch (*str) {
    case '"':
      if (!fits(5)) goto end;
      std::memcpy(p, "&#34;", 5);
      p += 5;
      break;
    case '&':
      if (!fits(5)) goto end;
      std::memcpy(p, "&#38;", 5);
      p += 5;
      break;
    case '<':
      if (!fits(5)) goto end;
      std::memcpy(p, "&#60;", 5);
      p += 5;
      break;
    case '>':
      if (!fits(5)) goto end;
      std::memcpy(p, "&#62;", 5);
      p += 5;
      break;
    default:
      if (!fits(1)) goto end;
      *p++ = *str;
    }
    str++;
  }
  end:;
  if (!fits(1)) --p;
  *p = '\0';
  return buf;
}

const char *getMimeType(const char *file) {
  const char *fileExt = getFileExtension(file);

  if (fileExt) {
    constexpr struct MimeType {
      const char *exts[4];
      const char *type;
    } mimeTypes[] = {
      {{"html", "htm"}, "text/html; charset=utf-8"},
      {{"xml"}, "text/xml; charset=utf-8"},
      {{"css"}, "text/css; charset=utf-8"},
      {{"js"}, "text/x-javascript; charset=utf-8"},
      {{"jpg", "jpeg"}, "image/jpeg"},
      {{"gif"}, "image/gif"},
      {{"png"}, "image/png"},
      {{"ico"}, "image/x-icon"},
      {{"zip"}, "application/zip"},
      {{"gz"}, "application/gzip"}
    };

    for (const MimeType &mimeType : mimeTypes)
      for (const char *const *ext = mimeType.exts; *ext; ++ext)
        if (!std::strcmp(fileExt, *ext))
          return mimeType.type;
  }

  return "application/octet-stream";
}

bool isHTMLMimeType(const char *mimeType) {
  return !std::strcmp(mimeType, "text/html");
}

bool isPictureMimeType(const char *mimeType) {
  return (!std::strcmp(mimeType, "image/jpeg") || !strcmp(mimeType, "image/gif") ||
          !std::strcmp(mimeType, "image/png") || !std::strcmp(mimeType, "image/x-icon"));
}

int getThreadPoolSize() {
  return threadPoolSize;
}

bool init() {
  const Version mhdVersion = Version::parse(MHD_get_version());

  const char *address = plugincfg->getString("httpd.listenAddress", "0.0.0.0"); // TODO: implement.
  int port = plugincfg->getInt("httpd.listenPort", -1, 65535, 8080);

  wwwRoot << plugincfg->getString("httpd.wwwRoot", "");

  if (wwwRoot.empty()) wwwRoot << pluginDataDir << PATH_DIV << "www";

  wwwRootIndexFile = plugincfg->getString("httpd.wwwRootIndexFile", "index.html");
  compress = plugincfg->getBool("httpd.enableCompression", true);
  compressionLevel = plugincfg->getInt("httpd.compressionLevel", 1, 9, 5);

  mhdFlags = 0;

  if (plugincfg->getBool("httpd.enableDebugMessages", false)) mhdFlags |= MHD_USE_DEBUG;

  if (plugincfg->getBool("httpd.enableFastOpen", false)) {
    if (mhdVersion >= Version(0, 9, 34)) mhdFlags |= static_cast<MHD_FLAG>(16384) /* MHD_USE_TCP_FASTOPEN */;
    else warn << "MHD_USE_TCP_FASTOPEN not supported" << warn.endl();
  }

  if (plugincfg->getBool("httpd.usePoll", false)) mhdFlags |= MHD_USE_POLL_INTERNALLY;
  else mhdFlags |= MHD_USE_SELECT_INTERNALLY;

  if (plugincfg->getBool("httpd.useThreadPerConnection", true)) mhdFlags |= MHD_USE_THREAD_PER_CONNECTION;

  threadPoolSize = plugincfg->getInt("httpd.threadPoolSize", 0, 1000, 2);

  int threadStackSize = plugincfg->getInt("httpd.threadStackSize", 49152, 1024 * 1024, 65536);
  int connectionTimeOut = plugincfg->getInt("httpd.connectionTimeOut", 0, 30, 60 * 60 * 1000);
  int connectionLimit = plugincfg->getInt("httpd.connectionLimit", 0, 65536, 64);
  int connectionPerIPLimit = plugincfg->getInt("httpd.connectionPerIPLimit", 0, 65536, 64);
  int connectionMemoryLimit = plugincfg->getInt("httpd.connectionMemoryLimit", threadStackSize, 1024 * 1024, threadStackSize);

  MHD_OptionItem optAddressReUse[2];

  if (mhdVersion >= Version(0, 9, 39)) {
    // MHD_OPTION_LISTENING_ADDRESS_REUSE
    optAddressReUse[0] = {static_cast<MHD_OPTION>(25), 1u};
    optAddressReUse[1] = {MHD_OPTION_END};
  } else {
    if ((mhdFlags & MHD_USE_DEBUG) == MHD_USE_DEBUG) warn << "MHD_OPTION_LISTENING_ADDRESS_REUSE not supported" << warn.endl();
    optAddressReUse[0] = {MHD_OPTION_END};
  }

  const MHD_OptionItem opts[] = {
      {MHD_OPTION_THREAD_POOL_SIZE, threadPoolSize},
      {MHD_OPTION_THREAD_STACK_SIZE, threadStackSize},
      {MHD_OPTION_CONNECTION_TIMEOUT, connectionTimeOut},
      {MHD_OPTION_CONNECTION_LIMIT, connectionLimit},
      {MHD_OPTION_PER_IP_CONNECTION_LIMIT, connectionPerIPLimit},
      {MHD_OPTION_CONNECTION_MEMORY_LIMIT, connectionMemoryLimit},
      {MHD_OPTION_END}};

  MHD_OptionItem httpsOpts[4];

  auto startDaemon = [&]() {
    MHD_Daemon *daemon = MHD_start_daemon(
        mhdFlags, port, nullptr, nullptr, &request, nullptr,
        MHD_OPTION_URI_LOG_CALLBACK, &requestLogger, nullptr,
        MHD_OPTION_NOTIFY_COMPLETED, &requestCompleted, nullptr,
        MHD_OPTION_ARRAY, &optAddressReUse, MHD_OPTION_ARRAY, &opts,
        MHD_OPTION_ARRAY, &httpsOpts, MHD_OPTION_END);

    if (daemon) {
      daemons.push_back(daemon);
      return true;
    }

    err << "failed to start http daemon on '" << address << ":" << port << "'" << err.endl();
    return false;
  };

  if (port != -1) {
    httpsOpts[0] = {MHD_OPTION_END};
    if (!startDaemon()) return false;
  }

  if (plugincfg->getBool("httpd.https.enabled", false)) mhdFlags |= MHD_USE_SSL;

  if ((mhdFlags & MHD_USE_SSL) != 0) {
    FString defaultFileName;
    defaultFileName << getApplicationNameLC();

    port = plugincfg->getInt("httpd.https.listenPort", 0, 65535, 8080);
    const char *cert = plugincfg->getString("httpd.https.defaultFileName", *tmpAppend<>(defaultFileName, ".crt"));
    const char *key = plugincfg->getString("httpd.https.key", *tmpAppend<>(defaultFileName, ".key"));

    if (!readFile(cert, httpsCert)) {
      err << "cannot read https certificate '" << cert << "'" << err.endl();
      return false;
    }

    if (!readFile(key, httpsKey)) {
      err << "cannot read https key '" << key << "'" << err.endl();
      return false;
    }

    httpsOpts[0] = {MHD_OPTION_HTTPS_MEM_CERT, 0, const_cast<void *>(static_cast<const void *>(httpsCert.c_str()))};
    httpsOpts[1] = {MHD_OPTION_HTTPS_MEM_KEY, 0, const_cast<void *>(static_cast<const void *>(httpsKey.c_str()))};
    httpsOpts[2] = {MHD_OPTION_HTTPS_PRIORITIES, 0, const_cast<void *>(static_cast<const void *>(plugincfg->getString("httpd.https.priorities", "NORMAL")))};
    httpsOpts[3] = {MHD_OPTION_END};

    if (!startDaemon()) return false;
  }

  return daemons.size() > 0;
}

void deinit() {
  for (MHD_Daemon *daemon : daemons) MHD_stop_daemon(daemon);
  daemons.clear();
}

} // namespace httpserver
