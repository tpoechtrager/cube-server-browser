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

#include "extinfo.h"

namespace extinfo {

TimeType Player::Info::getOnlineTime(TimeType now) const {
  if (!now) now = getMilliSeconds();
  if (connectTime == UNKNOWN_ONLINE_TIME) return UNKNOWN_ONLINE_TIME;
  return now - connectTime;
}

const char *Player::Info::getOnlineTime(TimeType now, char *buf, size_t size) const {
  if (!now) now = getMilliSeconds();
  if (connectTime == UNKNOWN_ONLINE_TIME) {
    strxcpy(buf, "<unknown>", size);
    return buf;
  }
  return fmtMillis(now - connectTime, buf, size);
}

const char *Player::Info::getCountry(const bool code) const {
  return country[code] ? country[code] : "<unknown>";
}

const char *Player::getName() const { return *name ? name : "<unknown>"; }
const char *Player::getTeam() const { return *team ? team : "<unknown>"; }
bool Player::isBot() const { return cn >= 128; }

void Player::update(const Player &player) {
  Info infoTmp = info;
  *this = player;
  info = infoTmp;
  info.lastUpdate = player.info.lastUpdate;
}

} // namespace extinfo
