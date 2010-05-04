// Copyright (C) 2005 Open Source Telecom Corp.
//  
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software 
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

#include "server.h"

namespace server {
using namespace ost;
using namespace std;

TimeslotProperty typeTimeslot;
PositionProperty typePosition;

PositionProperty::PositionProperty() :
ScriptProperty("position")
{
}

unsigned PositionProperty::prec(void)
{
	return 3;
}

char PositionProperty::token(void)
{
	return ':';
}

void PositionProperty::clear(char *data, unsigned size)
{
	if(!size)
		size = 13;

	if(size >= 13)
		strcpy(data, "00:00:00.000");
	else
		*data = 0;
}

void PositionProperty::set(const char *data, char *save, unsigned size)
{
	char msec[4] = "000";
	unsigned pos = 0;
	long secs = getValue(data);

	data = strchr(data, '.');
	if(data)
	{
		while(*(++data) && pos < 3)
			msec[pos++] = *data;
	}
	
	if(size < 13)
	{
		snprintf(save, size, "%ld", secs);
		return;
	}

	snprintf(save, size, "%02ld:%02ld:%02ld.%s",
		secs / 3600l, (secs / 60l) % 60l, secs % 60, msec);
}

long PositionProperty::getValue(const char *data)
{
	long offset = 0;

	const char *hp = data;
	const char *sp = strrchr(data, ':');
	const char *mp = strchr(data, ':');
	
	if(mp == sp)
		mp = NULL;

	if(!mp)
		hp = NULL;

	if(!sp)
		sp = data;
	else
		++sp;

	offset = atol(sp);
	if(mp)
		offset += 60l * atol(++mp);
	if(hp)
		offset += 3600l * atol(hp);

	return offset;
}
	
TimeslotProperty::TimeslotProperty() :
ScriptProperty("timeslot")
{
}

void TimeslotProperty::set(const char *data, char *save, unsigned size)
{
	timeslot_t ts = toTimeslot(data);
	
	if(ts == NO_TIMESLOT)
	{
		*save = 0;
		return;
	}

	if(size > 14)
		setString(save, size, data);
	else
		snprintf(save, size, "%d", ts);
}

long TimeslotProperty::getValue(const char *data)
{
	timeslot_t ts = toTimeslot(data);

	if(ts == NO_TIMESLOT)
		return -1l;

	return ts;
}

long TimeslotProperty::adjustValue(long val)
{
	if(val < 0)
		return -1;

	if(val > ts_used)
		return -1;

	return val;
}

} // end namespace
