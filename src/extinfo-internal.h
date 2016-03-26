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

namespace extinfo {
PLUGIN_IMPORT extern size_t maxPingsPerSecond;
PLUGIN_IMPORT extern TimeType pingInterval;
PLUGIN_IMPORT extern TimeType extPlayerPingInterval;
PLUGIN_IMPORT extern TimeType extUptimePingInterval;
PLUGIN_IMPORT extern TimeType masterUpdateInterval;
PLUGIN_IMPORT extern TimeType masterUpdateRetryInterval;
PLUGIN_IMPORT extern uint64_t playerSessionID;
PLUGIN_IMPORT extern TimeType nowus;
PLUGIN_IMPORT extern TimeType now;
PLUGIN_IMPORT extern TimeType32 now32;
} // namespace extinfo
