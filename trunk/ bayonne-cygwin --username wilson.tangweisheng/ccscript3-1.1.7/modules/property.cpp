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
#include <cstdio>
#include <stdlib.h>

#ifdef	WIN32
#define	EXT_PROP	".pro"
#define	EXT_TEMP	".tmp"
#define	PRE_PROP	"\\Script Properties\\"
#define	PRE_MAKE	"\\Script Properties"
#else
#define	EXT_PROP	".init"
#define	EXT_TEMP	".temp"
#define PRE_PROP	"/.properties/"
#define	PRE_MAKE	"/.properties"
#endif

#define	MAX_LIST	256

namespace ccscript3Extension {

using namespace std;
using namespace ost;

class LoadThread : public ScriptThread
{
private:
	FILE *fp;
	Symbol *list[MAX_LIST];
	char path[128];
	char var[128];
	void run(void);

public:
	LoadThread(ScriptInterp *interp);
	~LoadThread();
};

class SaveThread : public ScriptThread
{
private:
	FILE *fp, *fr;
	Symbol *list[MAX_LIST];
	char path[128];
	char temp[128];
	char var[128];
	const char *id;

	void run(void);

public:
	SaveThread(ScriptInterp *interp, const char *uid);
	~SaveThread();
};

class PropertyChecks : public ScriptChecks
{
public:
	const char *chkIO(Line *line, ScriptImage *img);
	const char *chkVar(Line *line, ScriptImage *img);
};

class PropertyMethods : public ScriptMethods
{
public:
	bool scrLoad(void);
	bool scrSave(void);
	bool scrVar(void);
};

class PropertyBinder : public ScriptBinder
{
public:
	PropertyBinder(Script::Define *run);
};

static Script::Define runtime[] = {
	{"load", false, (Script::Method)&PropertyMethods::scrLoad,
		(Script::Check)&PropertyChecks::chkIO},
	{"save", false, (Script::Method)&PropertyMethods::scrSave,
		(Script::Check)&PropertyChecks::chkIO},
	{"prop", true, (Script::Method)&PropertyMethods::scrVar,
		(Script::Check)&PropertyChecks::chkVar},
	{"property", true, (Script::Method)&PropertyMethods::scrVar,
		(Script::Check)&PropertyChecks::chkVar},
	{NULL, false, NULL, NULL}};

PropertyBinder::PropertyBinder(Script::Define *run) :
ScriptBinder(run)
{
	char path[128];

	if(*Script::var_prefix != '.') {
		snprintf(path, sizeof(path), "%s" PRE_MAKE,
			Script::var_prefix);
		Dir::create(path);
	}
}

static PropertyBinder bindProperty(runtime);

LoadThread::LoadThread(ScriptInterp *interp) :
ScriptThread(interp, 0)
{
	fp = NULL;
}

SaveThread::SaveThread(ScriptInterp *interp, const char *uid) :
ScriptThread(interp, 0)
{
	fp = NULL;
	fr = NULL;
	id = uid;
}

LoadThread::~LoadThread()
{
	terminate();

	if(fp)
		::fclose(fp);
}

SaveThread::~SaveThread()
{
	terminate();

	if(fp)
		::fclose(fp);

	if(fr)
		::fclose(fr);
}

void SaveThread::run(void)
{
	char prof[128];
	char buf[128];
	const char *grp = interp->getMember();
	const char *cp;
	char *ep;
	Symbol *sym;
	unsigned idx = 0, count;
	char base[64];
	Name *scr = interp->getName();

	if(!grp) {
		setString(base, sizeof(base), scr->name);
		ep = strchr(base, ':');
		if(ep)
			*ep = 0;
		grp = base;
	}

#ifdef	WIN32
	if(*Script::var_prefix == '.') {
		snprintf(temp, sizeof(temp), "%s%s" EXT_TEMP, grp, id);
		snprintf(path, sizeof(path), "%s%s" EXT_PROP, grp, id);
	}
	else {
		snprintf(temp, sizeof(temp), "%s" PRE_PROP "%s%s" EXT_TEMP,
			Script::var_prefix, grp, id);
		snprintf(path, sizeof(path), "%s" PRE_PROP "%s%s" EXT_PROP,
			Script::var_prefix, grp, id);
	}
#else
	if(*Script::var_prefix == '.') {
			snprintf(path, sizeof(path), "%s.%s", grp, id);
			snprintf(temp, sizeof(temp), ".%s.%s", grp, id);
	}
	else {
		snprintf(temp, sizeof(temp), "%s" PRE_PROP ".%s.%s",
			Script::var_prefix, grp, id);
		snprintf(path, sizeof(path), "%s" PRE_PROP "%s.%s",
			Script::var_prefix, grp, id);
	}
#endif
	snprintf(prof, sizeof(prof), "%s/%s" EXT_PROP, Script::etc_prefix, grp);

	remove(temp);
	fp = ::fopen(temp, "w");
	if(!fp)
		exit("save-failed");

	fr = ::fopen(path, "r");
	if(!fr)
		fr = ::fopen(prof, "r");

	while(fr) {
		if(!fgets(buf, sizeof(buf), fr) || feof(fr))
			break;

		cp = buf;
		while(isspace(*cp))
			++cp;

		if(!isalpha(*cp))
			continue;

		ep = strrchr(buf, '\r');
		if(!ep)
			ep = strrchr(buf, '\n');

		if(ep)
			*ep = 0;

		ep = (char *)cp;
		while(isalnum(*ep))
			++ep;

		*(ep++) = 0;
		while(isspace(*ep) || *ep == '=')
			++ep;

		snprintf(var, sizeof(var), "%s.%s", grp, cp);

		lock();
		sym = interp->mapSymbol(var, 0);

		if(sym && sym->type == symMODIFIED) {
			fprintf(fp, "%s = %s\n", cp, sym->data);
			sym->type = symORIGINAL;
		}
		else
			fprintf(fp, "%s = %s\n", cp, ep);

		release();
		Thread::yield();
	}
	lock();
	count = interp->gathertype(list, MAX_LIST - 1, grp, symMODIFIED);
	release();

	while(idx < count) {
		sym = list[idx++];
		sym->type = symORIGINAL;
		cp = strrchr(sym->id, '.');
		if(!cp)
			continue;
		fprintf(fp, "%s = %s\n", ++cp, sym->data);
		Thread::yield();
	}

	::fflush(fp);
	rename(temp, path);
	exit(NULL);
}

void LoadThread::run(void)
{
	const char *id;
	const char *grp = interp->getMember();
	char *cp;
	char *ep;
	Symbol *sym;
	char base[64];
	Name *scr = interp->getName();
	unsigned count, idx = 0;

	if(!grp) {
		setString(base, sizeof(base), scr->name);
		ep = strchr(base, ':');
		if(ep)
			*ep = 0;
		grp = base;
	}

	lock();
	id = interp->getKeyword("id");
	idx = 0;
	count = interp->gathertype(list, MAX_LIST - 1, grp, symMODIFIED);
	while(idx < count) {
		sym = list[idx++];
		sym->data[0] = 0;
		sym->type = symORIGINAL;
	}
	idx = 0;
	count = interp->gathertype(list, MAX_LIST - 1, grp, symORIGINAL);
	while(idx < count) {
		sym = list[idx++];
		sym->data[0] = 0;
	}
	release();

	if(id && *id) {
#ifdef	WIN32
		if(*Script::var_prefix == '.')
			snprintf(path, sizeof(path), "%s%s" EXT_PROP, grp, id);
		else
			snprintf(path, sizeof(path), "%s" PRE_PROP "%s%s" EXT_PROP,
				Script::var_prefix, grp, id);
#else
		if(*Script::var_prefix == '.')
			snprintf(path, sizeof(path), "%s.%s", grp, id);
		else
			snprintf(path, sizeof(path), "%s" PRE_PROP "%s.%s",
				Script::var_prefix, grp, id);
#endif
		fp = ::fopen(path, "r");
		if(fp)
			goto load;
	}

	snprintf(path, sizeof(path), "%s/%s" EXT_PROP, Script::etc_prefix, grp);
	fp = ::fopen(path, "r");
	if(!fp)
		exit("load-failed");

load:
	for(;;)
	{
		if(!fgets(path, sizeof(path), fp) || feof(fp))
			break;

		cp = path;
		while(isspace(*cp))
			++cp;

		if(!isalpha(*cp))
			continue;

		ep = strrchr(path, '\r');
		if(!ep)
			ep = strrchr(path, '\n');

		if(ep)
			*ep = 0;

		ep = cp;
		while(isalnum(*ep))
			++ep;

		*(ep++) = 0;
		while(isspace(*ep) || *ep == '=')
			++ep;

		snprintf(var, sizeof(var), "%s.%s", grp, cp);

		lock();
		sym = interp->mapSymbol(var, 0);
		if(!sym)
			goto unlock;

		if(sym->type != symORIGINAL && sym->type != symMODIFIED)
			goto unlock;

		sym->type = symORIGINAL;
		snprintf(sym->data, sym->size + 1, ep);
unlock:
		release();
		Thread::yield();
	}
	exit(NULL);
}

bool PropertyMethods::scrVar(void)
{
	char base[64];
	char var[128];
	Symbol *sym;
	const char *cp;
	const char *prefix = getMember();
	char *ep;
	unsigned len;
	Name *scr = getName();

	if(!prefix) {
		setString(base, sizeof(base), scr->name);
		ep = strchr(base, ':');
		if(ep)
			*ep = 0;
		prefix = base;
	}

	while(NULL != (cp = getOption())) {
		if(strchr(cp, '.') || *cp == '%' || *cp == '&')
			setString(var, sizeof(var), cp);
		else
			snprintf(var, sizeof(var), "%s.%s", prefix, cp);

		ep = strrchr(var, ':');
		if(ep) {
			*(ep++) = 0;
			len = atoi(ep);
		}
		else
			len = symsize;

		sym = mapSymbol(var, len);
		if(!sym)
			continue;

		if(sym->type != symINITIAL)
			continue;

		sym->type = symORIGINAL;
	}
	advance();
	return true;
}

bool PropertyMethods::scrLoad(void)
{
	release();
	new LoadThread(dynamic_cast<ScriptInterp*>(this));
	return false;
}

bool PropertyMethods::scrSave(void)
{
	const char *id = getKeyword("id");
	if(!id || !*id) {
		error("save-missing-id");
		return true;
	}
	release();

	new SaveThread(dynamic_cast<ScriptInterp*>(this), id);
	return false;
}

const char *PropertyChecks::chkVar(Line *line, ScriptImage *img)
{
	unsigned idx = 0;
	const char *cp;

	if(hasKeywords(line))
		return "property uses no keywords";

	if(!line->argc)
		return "property requires arguments";

	while(NULL != (cp = getOption(line, &idx))) {
		if(*cp == '&' || *cp == '%')
			goto chksize;

		if(!isalpha(*cp))
			return "property must be field name";

		if(strchr(cp, '.'))
			return "field must not be compound";

chksize:
		cp = strrchr(cp, ':');
		if(!cp)
			continue;

		++cp;
		if(!isdigit(*cp))
			return "field size must be number";
	}
	return NULL;
}

const char *PropertyChecks::chkIO(Line *line, ScriptImage *img)
{
	unsigned idx = 0;

//	if(!getMember(line))
//		return "property requires .group";

	if(!useKeywords(line, "=id"))
		return "invalid keyword used";

	if(getOption(line, &idx))
		return "property io uses no arguments";

	return NULL;
}

};

