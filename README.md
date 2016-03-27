# Cube Server Browser #

## Plugins ##

Web

## Compiling ##

`cd src; make plugins -j...`

**Supported Compilers:**

* clang 3.2+, g++ 4.7+ and Visual Studio 2015

A C++14 compiler with `shared_timed_mutex` support is recommended,
but not necessarily required.

MINGW32 is not supported, please use mingw-w64 instead.

**Supported Operating Systems:**

* Linux, *BSD, Darwin and Windows

## Required Libraries ##

`libgeoip-dev`, `libconfig-dev`, `zlib-dev` and `libenet-dev`

Web Plugin: `libmicrohttpd-dev`

## Setup ##

* Adjust `cube_server_browser.cfg` and `plugins/*.cfg` for your needs
* Run `./cube_server_browser`
* Open http://server-ip:server-port/ in your browser

## License ##

This product includes GeoLite data created by MaxMind,
available from http://www.maxmind.com.

Cube Server Browser is released under the terms of the
[AGPL v3](https://www.gnu.org/licenses/agpl-3.0.en.html).

