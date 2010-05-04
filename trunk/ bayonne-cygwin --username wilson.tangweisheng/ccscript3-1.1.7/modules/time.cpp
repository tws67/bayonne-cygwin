// Copyright (C) 1999-2005 Open Source Telecom Corporation.
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
//
// As a special exception, you may use this file as part of a free software
// library without restriction.  Specifically, if other files instantiate
// templates or use macros or inline functions from this file, or you compile
// this file and link it with other files to produce an executable, this
// file does not by itself cause the resulting executable to be covered by
// the GNU General Public License.  This exception does not however
// invalidate any other reasons why the executable file might be covered by
// the GNU General Public License.
//
// This exception applies only to the code released under the name GNU
// ccScript.  If you copy code from other releases into a copy of GNU
// ccScript, as the General Public License permits, the exception does
// not apply to the code that you add in this way.  To avoid misleading
// anyone as to the status of such modified files, you must delete
// this exception notice from them.
//
// If you write modifications of your own for GNU ccScript, it is your choice
// whether to permit this exception to apply to your modifications.
// If you do not wish that, delete this exception notice.
//

#include "script3.h"
#include <stdlib.h>

namespace ccscript3Extension {

using namespace std;
using namespace ost;

class TimeProperty : public ScriptProperty
{
public:
	TimeProperty();

	void set(const char *data, char *temp, unsigned size);

	long getValue(const char *data);

	char token(void);

	void clear(char *data, unsigned size);

	static bool testLocaltime(ScriptInterp *interp, const char *v);
};

class DateProperty : public ScriptProperty
{
public:
	DateProperty();

	void set(const char *data, char *temp, unsigned size);

	long getValue(const char *data);

	char token(void);

	void clear(char *data, unsigned size);
};

class TimeMethods : public ScriptMethods
{
public:
	bool scrTime(void);
	bool scrDate(void);
};

class TimeChecks : public ScriptChecks
{
public:
};

static Script::Define runtime[] = {
	{"time", true, (Script::Method)&TimeMethods::scrTime,
		(Script::Check)&ScriptChecks::chkType},
	{"date", true, (Script::Method)&TimeMethods::scrDate,
		(Script::Check)&ScriptChecks::chkType},
	{NULL, false, NULL, NULL}};

static ScriptBinder bindTime(runtime);
static TimeProperty typeTime;
static DateProperty typeDate;

DateProperty::DateProperty() :
ScriptProperty("date")
{
}

char DateProperty::token(void)
{
	return '-';
}

long DateProperty::getValue(const char *date)
{
	long year, month, day;
	const char *fp, *lp;

	fp = strchr(date, '-');
	lp = strrchr(date, '-');

	if(fp && fp != lp) {
		year = atol(date);
		month = atol(++fp);
		day = atol(++lp);
		goto jul;
	}

	fp = strchr(date, '/');
	lp = strrchr(date, '/');

	if(fp && fp != lp) {
		year = atol(++lp);
		month = atol(date);
		day = atol(++fp);
		goto jul;
	}

	fp = strchr(date, '.');
	lp = strrchr(date, '.');
	if(fp && fp != lp) {
		year = atol(++lp);
		month = atol(++fp);
		day = atol(date);
		goto jul;
	}

	return atol(date);

jul:
	if(year < 10)
		year += 2000;
	else if(year < 100)
		year += 1900;

	return day - 32075l +
		1461l * (year + 4800l + ( month - 14l) / 12l) / 4l +
		367l * (month - 2l - (month - 14l) / 12l * 12l) / 12l -
		3l * ((year + 4900l + (month - 14l) / 12l) / 100l) / 4l;
}

void DateProperty::set(const char *data, char *save, unsigned size)
{
	long julian = getValue(data);
	long year, month, day;

	if(size < 11) {
		snprintf(save, size, "%ld", julian);
		return;
	}

	double i, j, k, l, n;

	l = julian + 68569.0;
	n = int( 4 * l / 146097.0);
	l = l - int( (146097.0 * n + 3)/ 4 );
	i = int( 4000.0 * (l+1)/1461001.0);
	l = l - int(1461.0*i/4.0) + 31.0;
	j = int( 80 * l/2447.0);
	k = l - int( 2447.0 * j / 80.0);
	l = int(j/11);
	j = j+2-12*l;
	i = 100*(n - 49) + i + l;
	year = int(i);
	month = int(j);
	day = int(k);

	snprintf(save, size, "%04ld-%02ld-%02ld", year, month, day);
}

TimeProperty::TimeProperty() :
ScriptProperty("time")
{
	addConditional("localtime", &testLocaltime);
}

char TimeProperty::token(void)
{
	return ':';
}

long TimeProperty::getValue(const char *data)
{
	long val;

	const char *mp = strchr(data, ':');
	const char *sp = strrchr(data, ':');

	if(mp == sp)
		sp = NULL;

	if(!mp)
		return atol(data);

	val = atol(data) * 3600l;
	if(mp)
		val += atol(++mp) * 60l;

	if(sp)
		val += atol(++sp);

	return val;
}

void DateProperty::clear(char *data, unsigned size)
{
	if(!size)
		size = 11;

	if(size < 11)
		data[0] = 0;
	else
		strcpy(data, "0000-00-00");
}

void TimeProperty::set(const char *data, char *save, unsigned size)
{
	long val = getValue(data);

	val %= 86400l;

	if(size < 6)
		snprintf(save, size, "%ld", val);
	else if(size < 9)
		snprintf(save, size, "%02ld:%02ld",
			val / 3600l, (val / 60l) % 60);
	else
		snprintf(save, size, "%02ld:%02ld:%02ld",
			val / 3600l, (val / 60l) % 60, val % 60);
}

void TimeProperty::clear(char *data, unsigned size)
{
	if(!size)
		size = 9;

	if(size < 6) {
		strcpy(data, "0");
		return;
	}
	else if(size < 9)
		snprintf(data, size, "00:00");
	else
		snprintf(data, size, "00:00:00");
}

bool TimeMethods::scrDate(void)
{
	const char *cp;
	ScriptProperty *p = &typeDate;
	Symbol *sym;
	time_t now;
	struct tm *dt, dtd;
	char dts[12];

	time(&now);
	dt = localtime_r(&now, &dtd);
	if(dt->tm_year < 500)
		dt->tm_year += 1900;

	snprintf(dts, sizeof(dts), "%04d-%02d-%02d",
		dt->tm_year, dt->tm_mon + 1, dt->tm_mday);


	while(NULL != (cp = getOption(NULL))) {
		sym = mapSymbol(cp, 11 + sizeof(p));
		if(!sym)
			continue;

		if(sym->type != symINITIAL) {
			commit(sym, dts);
			continue;
		}

		sym->type = symPROPERTY;
		memcpy(sym->data, &p, sizeof(p));
		strcpy(sym->data + sizeof(p), dts);
	}
	advance();
	return true;
}

bool TimeMethods::scrTime(void)
{
	const char *cp;
	ScriptProperty *p = &typeTime;
	Symbol *sym;
	time_t now;
	struct tm *dt, dtd;
	char dts[10];

	time(&now);
	dt = localtime_r(&now, &dtd);
	snprintf(dts, sizeof(dts), "%02d:%02d:%02d",
		dt->tm_hour, dt->tm_min, dt->tm_sec);

	while(NULL != (cp = getOption(NULL))) {
		sym = mapSymbol(cp, 9 + sizeof(p));
		if(!sym)
			continue;

		if(sym->type != symINITIAL) {
			commit(sym, dts);
			continue;
		}

		sym->type = symPROPERTY;
		memcpy(sym->data, &p, sizeof(p));
		strcpy(sym->data + sizeof(p), dts);
	}
	advance();
	return true;
}

bool TimeProperty::testLocaltime(ScriptInterp *interp, const char *v)
{
	char *tok;
	char *tp;
	struct tm *dt, dtp;
	time_t now;
	char buf[256];
	long ftime;
	long ltime;
	long ntime;
	bool day[7] = {false, false, false, false, false, false, false};
	static char *dayname[7] = {"sun", "mon", "tue", "wed", "thu", "fri", "sat"};
	unsigned idx;

	time(&now);
	dt = localtime_r(&now, &dtp);
	setString(buf, sizeof(buf), v);
	ntime = dt->tm_hour * 60 + dt->tm_min;
	bool weekday = false;

	tok = strtok_r(buf, ", ;\t", &tp);
	while(NULL != tok) {
		if(strchr(tok, ':')) {
			ftime = 0;
			ltime = 24 * 60;
			if(*tok != '-') {
				ftime = 60 * atoi(tok);
				tok = strchr(tok, ':');
				if(tok)
					ftime += atoi(++tok);
				else
					tok = "";
			}

			tok = strchr(tok, '-');
			if(tok) {
				ltime = 60 * atoi(++tok);
				tok = strchr(tok, ':');
				if(tok)
					ltime += atoi(++tok);
			}

			if(ntime < ftime || ntime > ltime)
				return false;

			goto loop;
		}


		for(idx = 0 ; idx < 7; ++idx)
		{
			if(!strnicmp(dayname[idx], tok, 3))
				break;
		}

		if(idx < 7) {
			weekday = true;
			day[idx] = true;
			goto loop;
		}

		if(!stricmp(tok, "weekday")) {
			weekday = true;
			day[1] = day[2] = day[3] = day[4] = day[5] = true;
			goto loop;
		}

		if(!stricmp(tok, "weekend")) {
			weekday = true;
			day[0] = day[6] = true;
			goto loop;
		}
loop:
		tok = strtok_r(NULL, ", ;\t", &tp);
	}
	if(weekday)
		return day[dt->tm_wday];
	return true;
}

}; // namespace

