// Copyright (C) 2005 David Sugar, Tycho Softworks.
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
#include <cc++/slog.h>
#include <cstdio>

namespace ccscript3Extension {

using namespace std;
using namespace ost;

class LockerChecks : public ScriptChecks
{
public:
	const char *chkAquire(Line *line, ScriptImage *img);
	const char *chkRelease(Line *line, ScriptImage *img);
};

class LockerMethods : public ScriptMethods
{
public:
	bool scrAquire(void);
	bool scrRelease(void);
};

class LockerBinder : public ScriptBinder, public Assoc, public SharedMemPager
{
private:
	unsigned count;
	void detach(ScriptInterp *interp);
	void *getMemory(size_t size);

public:
	void release(const char *id, void *v);
	bool aquire(const char *id, void *v);

	LockerBinder();
	static LockerBinder locker;
};

static Script::Define runtime[] = {
	{"aquire", false, (Script::Method)&LockerMethods::scrAquire,
		(Script::Check)&LockerChecks::chkAquire},
	{"release", false, (Script::Method)&LockerMethods::scrRelease,
		(Script::Check)&LockerChecks::chkRelease},
	{NULL, false, NULL, NULL}};

LockerBinder LockerBinder::locker;

LockerBinder::LockerBinder() :
ScriptBinder("locker"), Assoc(), SharedMemPager(1024)
{
	count = 0;
	bind(runtime);
	slog.debug("locking service loaded");
}

void *LockerBinder::getMemory(size_t size)
{
	return alloc(size);
}

bool LockerBinder::aquire(const char *id, void *v)
{
	bool rtn = false;
	void *dp;

	enterMutex();
	dp = getPointer(id);
	if(!dp) {
		setPointer(id, v);
		++count;
		rtn = true;
	}
	else if(dp == v)
		rtn = true;
	leaveMutex();
	return rtn;
}

void LockerBinder::release(const char *id, void *v)
{
	void *dp;

	enterMutex();
	dp = getPointer(id);
	if(dp && dp == v) {
		setPointer(id, NULL);
		--count;
	}
	if(!count) {
		slog.debug("locker purging...");
		Assoc::clear();
		SharedMemPager::purge();
	}
	leaveMutex();
}

void LockerBinder::detach(ScriptInterp *interp)
{
	Symbol *list[65];
	unsigned count = interp->gathertype(list, 64, "aquired", symCONST);
	unsigned idx = 0;
	Symbol *sym;
	char name[65];

	while(idx < count) {
		sym = list[idx++];
		if(!sym->data[0])
			continue;
		snprintf(name, sizeof(name), "%s.%s",
			sym->id + 8, sym->data);
		LockerBinder::locker.release(name, interp);
		sym->data[0] = 0;
	}
}

bool LockerMethods::scrAquire(void)
{
	const char *tag = getMember();
	char name[65];
	char var[65];
	Symbol *sym;
	const char *id = getOption(NULL);
	Name *scr = getName();
	char *ep;

	if(!tag) {
		setString(var, sizeof(var), scr->name);
		ep = strrchr(var, ':');
		if(ep)
			*ep = 0;
		tag = var;
	}

	if(!id || !*id) {
		error("invalid-lock");
		return true;
	}

	snprintf(name, sizeof(name), "aquired.%s", tag);
	sym = mapSymbol(name, 32);
	if(sym->type == symINITIAL)
		sym->type = symCONST;
	if(!sym || sym->type != symCONST) {
		error("lock-failed");
		return true;
	}

	if(sym->data[0] && !stricmp(sym->data, id))
		goto locked;

	if(sym->data[0]) {
		snprintf(name, sizeof(name), "%s.%s", tag, sym->data);
		LockerBinder::locker.release(name, this);
		sym->data[0] = 0;
	}

	snprintf(name, sizeof(name), "%s.%s", tag, id);
	if(!LockerBinder::locker.aquire(name, this)) {
		error("lock-used");
		return false;
	}

locked:
	setString(sym->data, sym->size + 1, id);
	if(frame[stack].index < frame[stack].line->argc)
		intGoto();
	advance();
	return true;
}

bool LockerMethods::scrRelease(void)
{
	const char *tag = getMember();
	char name[65];
	char var[65];
	Symbol *sym;
	char *ep;
	Name *scr = getName();

	if(!tag) {
		setString(var, sizeof(var), scr->name);
		ep = strchr(var, ':');
		if(ep)
			*ep = 0;
		tag = var;
	}

	snprintf(name, sizeof(name), "aquired.%s", tag);
	sym = mapSymbol(name, 0);
	if(!sym || sym->type != symCONST || !sym->data[0]) {
		error("no-lock");
		return true;
	}

	snprintf(name, sizeof(name), "%s.%s", tag, sym->data);
	sym->data[0] = 0;
	LockerBinder::locker.release(name, this);
	advance();
	return true;
}

const char *LockerChecks::chkRelease(Line *line, ScriptImage *img)
{
	if(line->argc)
		return "release has no options or arguments";

	return NULL;
}

const char *LockerChecks::chkAquire(Line *line, ScriptImage *img)
{
	unsigned idx = 0;
	const char *cp;

	if(hasKeywords(line))
		return "aquire has no keywords";

	if(!getOption(line, &idx))
		return "aquire needs id argument";

	cp = getOption(line, &idx);
	if(!cp)
		return NULL;

	if(getOption(line, &idx))
		return "only one label for aquire";

	return NULL;
}

};
