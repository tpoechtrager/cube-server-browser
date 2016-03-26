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

#ifndef __MAIN_H__
#define __MAIN_H__

#include "config.h"
#include "tools.h"

const char *getApplicationName();
const char *getApplicationNameLC();

const char *getApplicationConfigFileName();
const char *getApplicationLogFileName();

const char *getApplicationLicense();

constexpr const char *DATA_DIR = "data";

extern config::Config *cfg;
PLUGIN_IMPORT extern LogFile *logFile;

void shouldReload();
void shouldShutdown();

#endif // __MAIN_H__
