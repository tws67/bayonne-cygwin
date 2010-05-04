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
#include <cmath>
#include <stdlib.h>

#ifndef M_PI
#define M_PI    3.14159265358979323846
#endif

namespace ccscript3Extension {

using namespace std;
using namespace ost;

class MathMethods : public ScriptMethods
{
public:
	bool scrRandom(void);
	bool scrSeed(void);
};

class MathChecks : public ScriptChecks
{
public:
	const char *chkRandom(Line *line, ScriptImage *img);
	const char *chkSeed(Line *line, ScriptImage *img);
};

class MathRuntime : public ScriptBinder
{
public:
	MathRuntime();
};

static Script::Define runtime[] = {
	{"seed", false, (Script::Method)&MathMethods::scrSeed,
		(Script::Check)&MathChecks::chkSeed},
	{"random", false, (Script::Method)&MathMethods::scrRandom,
		(Script::Check)&MathChecks::chkRandom},
	{NULL, false, NULL, NULL}};

static long fRandom(long *values, unsigned prec)
	{return (long)(((double)(*values)) * (rand()/(RAND_MAX + 0.)));};

static long fSine(long *values, unsigned prec)
	{return (long)(sin(values[0] * M_PI / values[1]) * values[1]);}

static long fCosine(long *values, unsigned prec)
	{return (long)(cos(values[0] * M_PI / values[1]) * values[1]);}

static long fTangent(long *values, unsigned prec)
	{return (long)(tan(values[0] * M_PI / values[1]) * values[1]);};

static long fArcTangent(long *values, unsigned prec)
	{return (long)(atan(values[0] * M_PI / values[1]) * values[1]);};

static long fPi(long *values, unsigned prec)
{
	char pi[10];
	strcpy(pi, "3141592653");
	pi[prec + 1] = 0;
	return atol(pi);
}

static long fAbs(long *values, unsigned prec)
{
	if(*values < 0)
		return -*values;
	else
		return *values;
}

static long fMin(long *values, unsigned prec)
{
	if(values[0] < values[1])
		return values[0];

	return values[1];
}

static long fMax(long *values, unsigned prec)
{
	if(values[0] > values[1])
		return values[0];

	return values[1];
}

static long fLimit(long *values, unsigned prec)
{
	if(values[0] < values[1])
		return values[1];

	if(values[0] > values[2])
		return values[2];

	return values[0];
}

static long fRound(long *values, unsigned prec)
{
	if(!values[1])
		return values[0];

	return (values[0] / values[1]) * values[1];
}

static long fSqrt(long *values, unsigned prec)
{
	double sq = sqrt(ScriptMethods::getDouble(*values, prec));
	return ScriptMethods::getRealValue(sq, prec);
}

static long fInt(long *values, unsigned prec)
{
	return ScriptMethods::getInteger(*values, prec) * ScriptMethods::getTens(prec);
}

static MathRuntime math;

MathRuntime::MathRuntime() : ScriptBinder()
{
	addFunction("abs", 1, &fAbs);
	addFunction("pi", 0, &fPi);
	addFunction("min", 2, &fMin);
	addFunction("max", 2, &fMax);
	addFunction("round", 2, &fRound);
	addFunction("limit", 3, &fLimit);
	addFunction("int", 1, &fInt);
	addFunction("sqrt", 1, &fSqrt);
	addFunction("sine", 2, &fSine);
	addFunction("cosine", 2, &fCosine);
	addFunction("tangent", 2, &fTangent);
	addFunction("arctangent", 2, &fArcTangent);
	addFunction("random", 1, &fRandom);
	bind(runtime);
}

const char *MathChecks::chkRandom(Line *line, ScriptImage *img)
{
	const char *cp = getMember(line);

	if(cp) {
		if(atoi(++cp) < 1)
			return "random member must be integer range";
	}
	else if(!findKeyword(line, "range"))
		return "random requires range keyword or member";

	if(!useKeywords(line, "=range=seed=offset=min=max=reroll=count"))
		return "unknown or invalid keyword used";

	return chkAllVars(line, img);
}

const char *MathChecks::chkSeed(Line *line, ScriptImage *img)
{
	if(getMember(line))
		return "seed does not use members";

	if(line->argc > 1)
		return "invalid number of arguments for seed";

	return NULL;
}

bool MathMethods::scrSeed(void)
{
	const char *cp = getValue(NULL);
	char buf[12];
	time_t now;

	if(!cp) {
		snprintf(buf, sizeof(buf), "%ld", (long)time(&now));
		cp = buf;
	}

	srand(atoi(cp));
	skip();
	return true;
}

bool MathMethods::scrRandom(void)
{
	const char *cp = getMember();
	unsigned range = 0, count = 1, roll;
	int val, offset = 0, min = 1, max = 0, reroll = 0;
	Symbol *sym;
	const char *errmsg = NULL;
	char buf[12];

	if(cp)
		range = atoi(cp);

	cp = getKeyword("range");
	if(cp)
		range = atoi(cp);

	if(!range) {
		error("random-range-invalid");
		return true;
	}

	cp = getKeyword("count");
	if(cp)
		count = atoi(cp);

	cp = getKeyword("seed");
	if(cp)
		srand(atoi(cp));

	cp = getKeyword("offset");
	if(cp)
		offset = atoi(cp);

	cp = getKeyword("min");
	if(cp)
		min = atoi(cp);

	cp = getKeyword("max");
	if(cp)
		max = atoi(cp);

	cp = getKeyword("reroll");
	if(cp)
		reroll = atoi(cp);

	while(NULL != (cp = getOption(NULL))) {
		sym = mapSymbol(cp, 11);
		if(!sym) {
			errmsg = "symbol-invalid";
			continue;
		}

		if(sym->type == symINITIAL)
			sym->type = symNUMBER;

retry:
		val = offset;
		roll = count;

		while(roll--)
			val += 1 + (int)(((double)(range)) * rand()/(RAND_MAX + 1.0));

		if(val < min)
			val = min;

		if(val <= reroll)
			goto retry;

		if(val > max && max)
			val = max;

		snprintf(buf, sizeof(buf), "%d", val);
		commit(sym, buf);
	}
	if(errmsg)
		error(errmsg);
	else
		skip();
	return true;
}

};

