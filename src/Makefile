PLATFORM= $(shell uname -s)
ARCH= $(shell uname -m)
SYS= $(PLATFORM)

ifneq (, $(HOSTPREFIX))
  override PLATFORM:= $(HOSTPREFIX)
  override ARCH:= $(shell echo $(HOSTPREFIX) | tr '-' ' ' | awk '{print $$1}')
  ifneq (, $(findstring mingw, $(HOSTPREFIX)))
    override SYS:= mingw
  else
    override SYS:= $(shell echo $(HOSTPREFIX) | tr '-' ' ' | awk '{print $$3}')
  endif
endif

override PLATFORM:= $(shell echo $(PLATFORM) | tr '[:upper:]' '[:lower:]')

ifneq (, $(findstring darwin, $(PLATFORM)))
  USE_CLANG=1
endif

ifneq (, $(findstring freebsd, $(PLATFORM)))
  USE_CLANG=1
endif

ifeq (1, $(USE_CLANG))
  CC= clang
  CXX= clang++
endif

ifneq (, $(HOSTPREFIX))
  CC:= $(shell which $(HOSTPREFIX)-$(CC) 2>/dev/null || echo $(HOSTPREFIX)-gcc 2>/dev/null)
  CXX:= $(shell which $(HOSTPREFIX)-$(CXX) 2>/dev/null || echo $(HOSTPREFIX)-g++ 2>/dev/null)
endif

override CXXFLAGS+= -DREVISION=$(shell git rev-parse --short HEAD 2>/dev/null || echo "unknown")

ifneq (, $(findstring icpc, $(CXX)))
  override LDFLAGS+= -static-intel -wd10237
endif

ifeq (1, $(LTO))
  ifneq (, $(findstring icpc, $(CXX)))
    override LTO= -ipo -fno-fat-lto-objects -fpic
  else
    override LTO= -flto
    ifneq (,$(findstring linux, $(PLATFORM)))
      ifneq (, $(findstring clang, $(CXX)))
        override LTO+= -fuse-ld=gold
      endif
    endif
    ifneq (, $(findstring mingw, $(PLATFORM)))
      ifeq (, $(findstring clang, $(CXX)))
        override LTO+= -fno-use-linker-plugin
      endif
    endif
  endif
else
  override LTO=
endif

ifneq (, $(findstring darwin, $(PLATFORM)))
  ifeq (,$(findstring arm, $(ARCH)))
    ifeq (, $(findstring clang++, $(CXX)))
      LIBSTDCXX= 1
    endif
    ifneq (, $(findstring stdc++, $(CXX)))
      LIBSTDCXX= 1
    endif
    ifeq (1, $(LIBSTDCXX))
      override VERSION_MIN= -mmacosx-version-min=10.4
    else
      override VERSION_MIN= -mmacosx-version-min=10.7
    endif
    override CXXFLAGS+= $(VERSION_MIN)
    override LDFLAGS+= $(VERSION_MIN) -framework Cocoa
  else
    override VERSION_MIN= -miphoneos-version-min=5.0
    override CXXFLAGS+= $(VERSION_MIN) -DUSE_GETTIMEOFDAY
    override LDFLAGS+= $(VERSION_MIN)
  endif
  ifneq (1, $(LIBSTDCXX))
    override CXXFLAGS+= -stdlib=libc++
    override LDFLAGS+= -stdlib=libc++
  else
    ifneq (, $(findstring g++, $(CXX)))
      override CXXFLAGS+= -fvisibility-inlines-hidden
      # GCC doesn't like CoreServices/CoreServices.h
      override CXXFLAGS+= -DUSE_GETTIMEOFDAY
    endif
  endif
endif

ifneq (, $(findstring mingw, $(PLATFORM)))
  EXESUFFIX= .exe
  override CFLAGS+= -DWIN32_LEAN_AND_MEAN
  override CXXFLAGS+= -DWIN32_LEAN_AND_MEAN
  #STATIC_RUNTIME= 1
  PTHREADFLAG=
else
  EXESUFFIX=
  ifeq (,$(findstring darwin, $(PLATFORM)))
    PTHREADFLAG= -pthread
  else
    PTHREADFLAG=
  endif
endif

### library setup ###

ifeq (, $(HOSTPREFIX))
  PKG_CONFIG= pkg-config
else
  PKG_CONFIG= $(HOSTPREFIX)-pkg-config
endif

ifneq (, $(LIBDIR))
  override CXXFLAGS+= -isystem $(LIBDIR)/include
  override LDFLAGS+= -L$(LIBDIR)/lib
endif

ifeq (1, $(USE_PKG_CONFIG))
  override CXXFLAGS+= $(shell $(PKG_CONFIG) --cflags libconfig geoip libenet zlib libmicrohttpd)
  LIBCONFIG= $(shell $(PKG_CONFIG) --libs libconfig)
  LIBGEOIP= $(shell $(PKG_CONFIG) --libs geoip)
  LIBENET= $(shell $(PKG_CONFIG) --libs libenet)
  LIBZ= $(shell $(PKG_CONFIG) --libs zlib)
  LIBMICROHTTPD= $(shell $(PKG_CONFIG) --libs libmicrohttpd)
  LIBIRCCLIENT= -lircclient
  LIBGLFW= $(shell $(PKG_CONFIG) --libs glfw3)
else
  LIBCONFIG= -lconfig
  LIBGEOIP= -lGeoIP
  LIBENET= -lenet
  LIBZ= -lz
  LIBMICROHTTPD= -lmicrohttpd
  LIBIRCCLIENT= -lircclient

  ifeq (, $(findstring mingw, $(PLATFORM)))
    LIBGLFW= -lglfw
  else
    LIBGLFW= -lglfw3
  endif
endif

LIBMATH= -lm

ENGINE_LIBS= $(LIBCONFIG) $(LIBGEOIP) $(LIBENET) $(LIBZ) $(LIBMATH)

ifneq (, $(findstring mingw, $(PLATFORM)))
  override ENGINE_LIBS+= -lws2_32 -lwinmm -lshlwapi
endif

ifeq (, $(findstring bsd, $(PLATFORM)))
  override LDFLAGS+= -ldl
endif

ifneq (, $(PTHREADFLAG))
  override LDFLAGS+= $(PTHREADFLAG)
endif

ifneq (, $(findstring linux, $(PLATFORM)))
  override LDFLAGS+= -lrt
endif

ifeq (1, $(STATIC_RUNTIME))
  override LDFLAGS+= -static-libgcc -static-libstdc++
endif

### debugging & optimizations ###

ifeq (1, $(DEBUG))
  override CXXFLAGS+= -g
  override LDFLAGS+= -g
else
  #override OPTFLAG+= -DNDEBUG
  ifeq (, $(findstring darwin, $(PLATFORM)))
    override LDFLAGS+= -Wl,-s
  endif
endif

ifeq (, $(OPTLEVEL))
  OPTLEVEL= 3
endif

override OPTFLAG+= -O$(OPTLEVEL)

ifneq (, $(LTO))
  override OPTFLAG+= $(LTO)
endif

ifneq (, $(OPTFLAG))
  override CFLAGS+= $(OPTFLAG)
  override CXXFLAGS+= $(OPTFLAG)
  override LDFLAGS+= $(OPTFLAG)
endif

### sanitizers ###

ifeq (1, $(FORTIFY))
  override SANOPTS+= -D_FORTIFY_SOURCE=2
endif

ifeq (1, $(ASAN))
  override SANOPTS+= -fsanitize=address
endif

ifeq (1, $(LSAN))
  override SANOPTS+= -fsanitize=leak
endif

ifeq (1, $(TSAN))
  override SANOPTS+= -fsanitize=thread
endif

ifeq (1, $(UBSAN))
  override SANOPTS+= -fsanitize=undefined
endif

ifneq (, $(SANOPTS))
  override CFLAGS+= $(SANOPTS)
  override CXXFLAGS+= $(SANOPTS)
  override LDFLAGS+= $(SANOPTS)
endif

### sources ###

SRCS= tools.cpp main.cpp network.cpp extinfo.cpp
SRCS+= extinfo-host.cpp extinfo-server.cpp extinfo-player.cpp config.cpp
SRCS+= plugin.cpp geoip.cpp cube/tools.cpp 3rd/itostr.cpp

W32_COMPAT_SRCS= compat/win32/strptime.cpp compat/win32/realpath.c

OBJS= $(subst .cpp,.o,$(SRCS))
OBJS:= $(subst .c,.o,$(OBJS))

W32_COMPAT_OBJS=  $(subst .cpp,.o,$(W32_COMPAT_SRCS))
W32_COMPAT_OBJS:= $(subst .c,.o,$(W32_COMPAT_OBJS))

ifneq (, $(findstring mingw, $(PLATFORM)))
  OBJS+= $(W32_COMPAT_OBJS)
else
  ALL_OBJS= $(W32_COMPAT_OBJS)
endif

override APPNAME= cube_server_browser

BINDIR= ../
BIN= $(BINDIR)$(APPNAME)$(EXESUFFIX)

ifneq (, $(findstring mingw, $(PLATFORM)))
  BINIMPLIB= $(APPNAME)$(EXESUFFIX).a
endif

ifneq (, $(LTO))
  # Let the compiler inline extinfo functions into the plugins.
  # DO NOT ADD SOURCE FILES WITH GLOBAL/STATIC VARIABLES HERE.
  LTO_PLUGIN_SRCS= extinfo-host.cpp extinfo-server.cpp extinfo-player.cpp
else
  LTO_PLUGIN_SRCS= 
endif

IRCBOT_PLUGIN_SRCS= plugins/ircbot/main.cpp plugins/ircbot/client.cpp
IRCBOT_PLUGIN_SRCS+= plugins/ircbot/commands.cpp $(LTO_PLUGIN_SRCS)
IRCBOT_PLUGIN_OBJS= $(subst .cpp,.o,$(IRCBOT_PLUGIN_SRCS))
IRCBOT_PLUGIN_LIBS= $(LIBIRCCLIENT)
ifneq (, $(findstring mingw, $(PLATFORM)))
IRCBOT_PLUGIN_LIBS+= -lws2_32
endif
IRCBOT_PLUGIN_BIN= $(BINDIR)plugins/ircbot-plugin$(PLUGIN_EXT)

WEB_PLUGIN_SRCS= plugins/web/main.cpp plugins/web/httpserver.cpp
WEB_PLUGIN_SRCS+= plugins/web/web.cpp $(LTO_PLUGIN_SRCS)
WEB_PLUGIN_OBJS= $(subst .cpp,.o,$(WEB_PLUGIN_SRCS))
WEB_PLUGIN_LIBS= $(LIBMICROHTTPD)
WEB_PLUGIN_BIN= $(BINDIR)plugins/web-plugin$(PLUGIN_EXT)

GUI_PLUGIN_SRCS= plugins/gui/main.cpp plugins/gui/glfw.cpp
GUI_PLUGIN_SRCS+= plugins/gui/imgui/imgui.cpp plugins/gui/imgui/imgui_draw.cpp
GUI_PLUGIN_SRCS+= plugins/gui/imgui/imgui_impl_glfw.cpp $(LTO_PLUGIN_SRCS)
GUI_PLUGIN_CXXFLAGS= -Iplugins/gui/imgui
GUI_PLUGIN_IMGUI_CXXFLAGS= -w
GUI_PLUGIN_OBJS= $(subst .cpp,.o,$(GUI_PLUGIN_SRCS))
GUI_PLUGIN_LIBS= $(LIBGLFW) $(LIBMATH)
ifneq (, $(findstring mingw, $(PLATFORM)))
GUI_PLUGIN_LIBS+= -lopengl32
endif
GUI_PLUGIN_BIN= $(BINDIR)plugins/gui-plugin$(PLUGIN_EXT)

ALL_OBJS+= $(OBJS) $(IRCBOT_PLUGIN_OBJS) $(WEB_PLUGIN_OBJS) $(GUI_PLUGIN_OBJS)
ALL_BINS+= $(BIN) $(BINIMPLIB) $(WEB_PLUGIN_BIN) $(GUI_PLUGIN_BIN)

CLEAN_OBJS= $(ALL_OBJS) $(APPNAME).exe.a $(BINDIR)$(APPNAME){,.exe}
CLEAN_OBJS+= $(BINDIR)plugins/*-plugin{.dylib,.so,.dll}

### compiler flags ###

ifeq (, $(SANOPTS))
  override CXXFLAGS+= -fno-rtti
endif

override CXXFLAGS+= -fno-exceptions
override CXXSTD+= $(shell F=1 ./check_compiler_flags.sh $(CXX) -std=c++1z -std=c++14 -std=c++1y -std=c++11 -std=c++0x)
ifeq (,$(CXXSTD))
  $(error A C++11 compiler is required)
endif
override CXXFLAGS+= $(CXXSTD)
#override CXXFLAGS+= -fstack-protector-strong
ifneq (, $(PTHREADFLAG))
  override CXXFLAGS+= $(PTHREADFLAG)
endif
override CXXFLAGS+= -Wall -Wextra
override CXXFLAGS+= $(shell ./check_compiler_flags.sh $(CXX) -Wrange-loop-analysis -Wlogical-not-parentheses -Wbool-compare)
override CXXFLAGS+= -Wno-missing-field-initializers
override CXXFLAGS+= -pedantic

### ld flags ###

PIC=-fpic

ifneq (, $(findstring darwin, $(PLATFORM)))
  AS_NEEDED= -Wl,-dead_strip_dylibs
  UNDEFINED_DYNAMIC_LOOKUP= -Wl,-undefined,dynamic_lookup
  EXPORT_DYNAMIC= -Wl,-export_dynamic
  PLUGIN_EXT= .dylib
  EXPORT_MAP= -Wl,-exported_symbols_list,plugins/exports.ld64
else
  AS_NEEDED= -Wl,--as-needed
  NO_AS_NEEDED= -Wl,--no-as-needed
  EXPORT_ALL_SYMBOLS= -Wl,--export-all-symbols
  ifneq (, $(findstring mingw, $(PLATFORM)))
    PLUGIN_EXT= .dll
    EXPORT_MAP= plugins/exports.def
    PIC=
  else
    EXPORT_DYNAMIC= -Wl,--export-dynamic
    PLUGIN_EXT= .so
    EXPORT_MAP= -Wl,--version-script,plugins/exports.ld
  endif
endif

override LDFLAGS:= $(AS_NEEDED) $(LDFLAGS)
override PLUGIN_LDFLAGS+= -shared $(BINIMPLIB) $(UNDEFINED_DYNAMIC_LOOKUP)
override PLUGIN_LDFLAGS+= $(EXPORT_MAP)
override PLUGIN_LDFLAGS+= $(LDFLAGS)

### targets ###

plugins/%.o: override CXXFLAGS+= -I. $(PIC)
plugins/gui/%.o: override CXXFLAGS+= $(GUI_PLUGIN_CXXFLAGS)
plugins/gui/imgui/%.o: override CXXFLAGS+= $(GUI_PLUGIN_IMGUI_CXXFLAGS)

all: $(APPNAME)

depend:
	makedepend -Y $(subst .o,.cpp,$(ALL_OBJS))

ifneq (, $(findstring mingw, $(PLATFORM)))

# Create the import library with -shared because $(EXPORT_ALL_SYMBOLS) in
# the final link step exports a lot of unnecessary symbols that do not belong
# into the import library.

$(APPNAME): $(OBJS)
	$(CXX) $(OBJS) -o $(BIN) -Wl,--out-implib=$(BINIMPLIB) -shared $(LDFLAGS) $(ENGINE_LIBS)
	$(CXX) $(OBJS) -o $(BIN) $(EXPORT_ALL_SYMBOLS) $(LDFLAGS) $(ENGINE_LIBS)

else

$(APPNAME): $(OBJS)
	$(CXX) $(OBJS) -o $(BIN) $(EXPORT_DYNAMIC) $(LDFLAGS) $(ENGINE_LIBS)

endif

ircbot: $(APPNAME) $(IRCBOT_PLUGIN_OBJS)
	$(CXX) $(IRCBOT_PLUGIN_OBJS) $(PLUGIN_LDFLAGS) $(IRCBOT_PLUGIN_LIBS) -o $(IRCBOT_PLUGIN_BIN)

web: $(APPNAME) $(WEB_PLUGIN_OBJS)
	$(CXX) $(WEB_PLUGIN_OBJS) $(PLUGIN_LDFLAGS) $(WEB_PLUGIN_LIBS) -o $(WEB_PLUGIN_BIN)

gui: $(APPNAME) $(GUI_PLUGIN_OBJS)
	$(CXX) $(GUI_PLUGIN_OBJS) $(PLUGIN_LDFLAGS) $(GUI_PLUGIN_LIBS) -o $(GUI_PLUGIN_BIN)

plugins: web #ircbot gui

.PHONY: clean $(APPNAME)

clean:
	rm -f $(CLEAN_OBJS)

# DO NOT DELETE

compat/win32/strptime.o: compat/win32/compat.h
tools.o: tools.h 3rd/itostr.h cube/tools.h
main.o: network.h geoip.h extinfo.h tools.h 3rd/itostr.h plugin.h config.h
main.o: main.h
network.o: tools.h 3rd/itostr.h main.h config.h network.h
extinfo.o: extinfo.h network.h tools.h 3rd/itostr.h geoip.h main.h config.h
extinfo.o: cube/tools.h
extinfo-host.o: main.h config.h tools.h 3rd/itostr.h geoip.h extinfo.h
extinfo-host.o: network.h extinfo-internal.h
extinfo-server.o: geoip.h extinfo.h network.h tools.h 3rd/itostr.h
extinfo-server.o: extinfo-internal.h main.h config.h
extinfo-player.o: extinfo.h network.h tools.h 3rd/itostr.h
config.o: config.h tools.h 3rd/itostr.h
plugin.o: plugin.h tools.h 3rd/itostr.h config.h main.h
geoip.o: network.h main.h config.h tools.h 3rd/itostr.h geoip.h
cube/tools.o: tools.h 3rd/itostr.h
3rd/itostr.o: 3rd/itostr.h
plugins/ircbot/main.o: config.h main.h tools.h 3rd/itostr.h
plugins/ircbot/main.o: plugins/ircbot/client.h plugins/ircbot/commands.h
plugins/ircbot/main.o: plugin.h
plugins/ircbot/client.o: config.h extinfo.h network.h tools.h 3rd/itostr.h
plugins/ircbot/client.o: main.h plugins/ircbot/client.h
plugins/ircbot/client.o: plugins/ircbot/commands.h
plugins/ircbot/commands.o: extinfo.h network.h tools.h 3rd/itostr.h
plugins/ircbot/commands.o: extinfo-sort.h plugins/ircbot/commands.h
plugins/ircbot/commands.o: plugins/ircbot/client.h
plugins/web/main.o: plugin.h tools.h 3rd/itostr.h config.h
plugins/web/main.o: plugins/web/httpserver.h main.h network.h
plugins/web/main.o: plugins/web/web.h
plugins/web/httpserver.o: plugins/web/httpserver.h main.h config.h tools.h
plugins/web/httpserver.o: 3rd/itostr.h network.h plugin.h
plugins/web/web.o: plugins/web/httpserver.h main.h config.h tools.h
plugins/web/web.o: 3rd/itostr.h network.h extinfo.h cube/tools.h plugin.h
plugins/gui/main.o: extinfo.h network.h tools.h 3rd/itostr.h extinfo-sort.h
plugins/gui/main.o: config.h main.h plugin.h
plugins/gui/glfw.o: main.h config.h tools.h 3rd/itostr.h plugin.h
plugins/gui/imgui/imgui.o: plugins/gui/imgui/imgui.h
plugins/gui/imgui/imgui.o: plugins/gui/imgui/imconfig.h
plugins/gui/imgui/imgui.o: plugins/gui/imgui/imgui_internal.h
plugins/gui/imgui/imgui.o: plugins/gui/imgui/stb_textedit.h
plugins/gui/imgui/imgui_draw.o: plugins/gui/imgui/imgui.h
plugins/gui/imgui/imgui_draw.o: plugins/gui/imgui/imconfig.h
plugins/gui/imgui/imgui_draw.o: plugins/gui/imgui/imgui_internal.h
plugins/gui/imgui/imgui_draw.o: plugins/gui/imgui/stb_textedit.h
plugins/gui/imgui/imgui_draw.o: plugins/gui/imgui/stb_rect_pack.h
plugins/gui/imgui/imgui_draw.o: plugins/gui/imgui/stb_truetype.h
plugins/gui/imgui/imgui_impl_glfw.o: plugins/gui/imgui/imgui.h
plugins/gui/imgui/imgui_impl_glfw.o: plugins/gui/imgui/imconfig.h
plugins/gui/imgui/imgui_impl_glfw.o: plugins/gui/imgui/imgui_impl_glfw.h
