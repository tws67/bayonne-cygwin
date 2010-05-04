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

#include "engine.h"

#if defined(_MSC_VER) && _MSC_VER >= 1300
#if defined(_WIN64_) || defined(__WIN64__)
#define RLL_SUFFIX ".x64"
#elif defined(_M_IX86)
#define RLL_SUFFIX ".x86"
#else
#define RLL_SUFFIX ".xlo"
#endif
#endif

#if defined(__MINGW32__) | defined(__CYGWIN32__)
#define RLL_SUFFIX ".dso"
#endif

#ifdef  W32
#ifndef RLL_SUFFIX
#define RLL_SUFFIX ".rll"
#endif
#endif

#ifndef RLL_SUFFIX
#define RLL_SUFFIX ".dso"
#endif

using namespace std;
using namespace ost;

bool Script::fastStart = true;
unsigned Script::fastStepping = 32;
unsigned Script::autoStepping = 1;
bool Script::useBigmem = true;
size_t Script::pagesize = 1024;
unsigned Script::symsize = 64;
unsigned Script::symlimit = 960;
char Script::decimal = '.';
bool Script::use_definitions = false;
bool Script::use_macros = false;
bool Script::use_prefix = false;
bool Script::use_merge = false;
bool Script::use_funcs = false;
const char *Script::plugins = NULL;
const char *Script::altplugins = NULL;

const char *Script::etc_prefix = ".";
const char *Script::var_prefix = ".";
const char *Script::log_prefix = ".";

const char *Script::access_user = NULL;
const char *Script::access_pass = NULL;
const char *Script::access_host = "localhost";

#ifdef	WIN32
const char *Script::apps_extensions = ".app.apps";
const char *Script::exec_extensions = ".bat";
#else
const char *Script::apps_extensions = ".app.exec.apps";
const char *Script::exec_extensions = ".sh";
#endif
const char *Script::exec_token = "script:";
const char *Script::exec_prefix = "exec:";
const char *Script::exit_token = "exit";
const char *Script::apps_prefix = "libexec";

bool Script::exec_funcs = false;

Script::Test *Script::test = NULL;
Script::Fun *Script::ifun = NULL;
Script::Package *Script::Package::first = NULL;

class ScriptSysRegistry : public Script
{
public:
	ScriptSysRegistry();
};

static class ScriptSysRegistry registry;

#if     defined(CAPE_REGISTRY_PREFIX)
#define REGISTRY_SCRIPT_SETTINGS CAPE_REGISTRY_PREFIX "\\Script Settings"
#else
#define REGISTRY_SCRIPT_SETTINGS "SOFTWARE\\CAPE Framework\\Script Settings"
#endif

ScriptSysRegistry::ScriptSysRegistry()
{
#ifdef	WIN32
	static TCHAR regpath[256];
	LONG value;
	HKEY key;
	char *env;

	Script::plugins =
		"C:/Program Files/Common Files/GNU Telephony/Script Plugins";

#ifdef	DEBUG
#define	PLUGINS "Debug"
#else
#define	PLUGINS "Plugins"
#endif

	if(RegOpenKey(HKEY_LOCAL_MACHINE, REGISTRY_SCRIPT_SETTINGS, &key) == ERROR_SUCCESS) {
		if(RegQueryValue(key, PLUGINS, regpath, &value) == ERROR_SUCCESS) {
			Script::plugins = regpath;
			env = regpath;
			while(NULL != (env = strchr(env, '\\')))
				*env = '/';
		}

		if(RegQueryValue(key, "paging", NULL, &value) == ERROR_SUCCESS) {
			pagesize = value;
			symlimit = value - sizeof(Symbol) - 32;
		}

		if(RegQueryValue(key, "symsize", NULL, &value) == ERROR_SUCCESS)
			symsize = value;

		if(RegQueryValue(key, "autostep", NULL, &value) == ERROR_SUCCESS)
			autoStepping = value;

			if(RegQueryValue(key, "faststep", NULL, &value) == ERROR_SUCCESS)
			 fastStepping = value;

		RegCloseKey(key);
	}
#else
	plugins = SCRIPT_LIBPATH;
#endif
}

bool Script::isScript(Name *scr)
{
	const char *ext = strrchr(scr->filename, '.');
	if(!ext)
		return false;

	if(!stricmp(ext, ".scr"))
		return true;

	if(!stricmp(ext, ".mac"))
		return true;

	return false;
}

bool Script::isSymbol(const char *id)
{
	if(*id == '%' || *id == '&' || *id == '@')
		return true;

	return false;
}

bool Script::isFunction(Name *scr)
{
	switch(scr->access) {
	case scrFUNCTION:
	case scrLOCAL:
		return true;
	default:
		return false;
	}
}

bool Script::isPrivate(Name *scr)
{
	switch(scr->access) {
	case scrLOCAL:
	case scrPRIVATE:
		return true;
	default:
		return false;
	}
}

unsigned Script::getIndex(const char *id)
{
	unsigned int key = 0;

	while(*id)
		key ^= (key << 1) ^ (*(id++) & 0x1f);

	return key % SCRIPT_INDEX_SIZE;
}

Script::Symbol *Script::deref(Symbol *sym)
{
	while(sym && sym->type == symREF)
		memcpy(&sym, sym->data, sizeof(sym));

	return sym;
}

bool Script::symindex(Symbol *sym, short index)
{
	Array *a;
	if(!sym)
		return false;

	a = (Array *)&sym->data;

	switch(sym->type) {
	case symFIFO:
		if(index < 0)
			index = a->tail + index;
		if(index < 0 || index > a->tail)
			return false;
		a->head = index;
		if(a->head == a->tail)
			a->head = a->tail = 0;
		return true;
	case symSTACK:
		if(index < 0)
			index = a->tail + index;
		if(index < 0 || index > a->tail)
			return false;
		a->tail = index;
		return true;
	case symARRAY:
		if(index < 0)
			index = a->tail + index;
		if(index < 0 || index >= a->count)
			return false;
		a->head = index;
		return true;
	default:
		return false;
	}
}

void Script::clear(Symbol *sym)
{
	unsigned dec;
	unsigned pos = 1;
	Array *a = (Array *)&sym->data;
	ScriptProperty *p;

	switch(sym->type) {
	case symARRAY:
	case symSTACK:
	case symFIFO:
		a->head = a->tail = 0;
		sym->data[sizeof(Array)] = 0;
		return;
	case symBOOL:
		sym->data[0] = 'n';
		sym->data[1] = 0;
		return;
	case symNUMBER:
		dec = sym->size - 11;
		if(dec)
			++dec;
		sym->data[0] = '0';
		if(dec) {
			sym->data[pos++] = decimal;
			while(pos < dec)
				sym->data[pos++] = '0';
		}
		sym->data[pos] = 0;
		return;
	case symPROPERTY:
		memcpy(&p, &sym->data, sizeof(p));
		p->clear(sym->data + sizeof(p), sym->size - sizeof(p));
		return;
	case symMODIFIED:
		sym->type = symORIGINAL;
	case symORIGINAL:
	case symLOCK:
	case symNORMAL:
		if(!stricmp(sym->id, "script.error"))
			strcpy(sym->data, "none");
		else
			sym->data[0] = 0;
		return;
	case symSEQUENCE:
		sym->data[sym->size] = 0;
		return;
	case symCOUNTER:
		sym->data[0] = '0';
		sym->data[1] = 0;
		return;
	case symTIMER:
		sym->data[0] = 0;
	default:
		return;
	}
}

unsigned Script::count(Symbol *sym)
{
	Array *a = (Array *)&sym->data;

	switch(sym->type) {
	case symCONST:
	case symCOUNTER:
	case symTIMER:
	case symLOCK:
		return 0;
	case symARRAY:
		return a->count;
	case symSTACK:
	case symFIFO:
		return a->count - 1;
	default:
		return 1;
	}
}

unsigned Script::storage(Symbol *sym)
{
	Array *a = (Array *)&sym->data;

	switch(sym->type) {
	case symARRAY:
	case symSTACK:
	case symFIFO:
		return a->rec;
	case symPROPERTY:
		return sym->size - sizeof(ScriptProperty *);
	case symORIGINAL:
	case symMODIFIED:
	case symINITIAL:
	case symNORMAL:
		return sym->size;
	default:
		return 0;
	}
}

const char *Script::extract(Symbol *sym)
{
	long value;
	const char *data;
	unsigned short pos, len;
	Array *a;
	time_t now;

	if(!sym)
		return NULL;

	a = (Array *)&sym->data;

	switch(sym->type) {
	case symLOCK:
		data = strchr(sym->data, ':');
		if(data)
			return ++data;
		return NULL;
	case symPROPERTY:
		return sym->data + sizeof(ScriptProperty *);
	case symORIGINAL:
	case symMODIFIED:
	case symNORMAL:
	case symCONST:
	case symNUMBER:
	case symBOOL:
		return sym->data;
	case symTIMER:
		if(sym->data[0]) {
			time(&now);
			snprintf(sym->data + 12, 12, "%ld",  now - atol(sym->data));
		}
		else
			setString(sym->data + 12, 12, "0");
		return sym->data + 12;
	case symCOUNTER:
		value = atoi(sym->data);
		snprintf(sym->data, sym->size + 1, "%ld", ++value);
		return sym->data;
		case symARRAY:
		pos = a->head;
		if(pos >= a->count || pos >= a->tail)
			return "";

		return sym->data + sizeof(Array) + pos * (a->rec + 1);
		case symSTACK:
		if(a->tail == a->head) {
			a->tail = a->head = 0;
			return "";
		}

		pos = a->tail;
		if(a->tail == 0)
			a->tail = a->count - 1;
		else
			--a->tail;

		return sym->data + sizeof(Array) + pos * (a->rec + 1);
		case symSEQUENCE:
		len = sym->size / sizeof(const char *);
		pos = sym->data[sym->size];
		memcpy(&data, &sym->data[pos * sizeof(data)], sizeof(data));
		if(++pos >= len)
			pos = 0;
		sym->data[sym->size] = (unsigned char)pos;
		return data;
		case symFIFO:
		if(a->head == a->tail)
			return "";

		data = sym->data + a->head * (a->rec + 1) + sizeof(Array);
		if(++a->head >= a->count)
			a->head = 0;
		return data;
	default:
		return NULL;
	}
}

bool Script::append(Symbol *sym, const char *value)
{
	switch(sym->type) {
	case symBOOL:
	case symNUMBER:
	case symPROPERTY:
	case symCOUNTER:
	case symTIMER:
		return false;
	case symORIGINAL:
		sym->type = symMODIFIED;
		sym->data[0] = 0;
	case symMODIFIED:
		addString(sym->data, sym->size + 1, value);
		return true;
	case symNORMAL:
	case symINITIAL:
		addString(sym->data, sym->size + 1, value);
		sym->type = symNORMAL;
		return true;
	default:
		return commit(sym, value);
	}
}

bool Script::commit(Symbol *sym, const char *value)
{
	Array *a;
	long val;
	unsigned short pos, npos, len;
	int dec = 0;
	char *dp;
	const char *sp;
	long v1, v2;
	char vbuf[12];
	ScriptProperty *p;
	time_t now;

	if(!sym)
		return false;

	a = (Array *)&sym->data;

	switch(sym->type) {
	case symBOOL:
		if(atol(value))
			sym->data[0] = 'y';
		else switch(*value)
		{
		case '0':
		case 'n':
		case 'N':
		case 'f':
		case 'F':
			sym->data[0] = 'n';
		default:
			sym->data[0] = 'y';
		};
		sym->data[1] = 0;
		return true;
	case symNUMBER:
		v1 = atol(value);
		sp = strchr(value, '.');
		if(!sp)
			sp = strchr(value, decimal);
		if(sp)
			v2 = atol(++sp);
		else
			v2 = 0;
		dp = NULL;
		if(sym->size > 11) {
			dec = sym->size - 12;
			snprintf(vbuf, sizeof(vbuf), "%ld", v2);
			len = (unsigned short)strlen(vbuf);
			sp = vbuf;
			if(len > dec && vbuf[dec] >= '5') {
				while(--dec > -1) {
					if(vbuf[dec] < '9') {
						++vbuf[dec];
						break;
					}
					vbuf[dec] = '0';
				}
				if(dec < 0 && v1 < 0)
					--v1;
				else if(dec < 0)
					++v1;
			}
			dec = sym->size - 12;
			snprintf(sym->data, 12, "%ld", v1);
					len = (unsigned short)strlen(sym->data);
			sym->data[len++] = decimal;
			dp = sym->data + len;
			while(dec--)
				sym->data[len++] = '0';
			sym->data[len] = 0;
		}
		else {
			if(sp && *sp >= '5') {
				if(v1 > 0)
					++v1;
				else
					--v1;
			}
			snprintf(sym->data, sym->size + 1, "%ld", v1);
		}
		if(sp && dp) {
			len = (unsigned short)strlen(sp);
			if(len > sym->size - 12)
				len = sym->size - 12;
			memcpy(dp, sp, len);
		}
		return true;
	case symARRAY:
		if(a->head >= a->count)
			return false;

		setString(sym->data + sizeof(Array) + a->head * (a->rec + 1), a->rec + 1, value);
		if(++a->head > a->count)
			a->head = a->count;
		if(a->head > a->tail)
			a->tail = a->head;
		return true;
	case symFIFO:
	case symSTACK:
		len = a->rec + 1;
		pos = a->tail;
		if(sym->type == symSTACK)
			npos = ++pos;
		else {
			npos = pos;
			++npos;
		}
		if(npos >= a->count)
			npos = 0;
		if(npos == a->head)
			return false;	// full

		a->tail = npos;
		setString(sym->data + sizeof(Array) + (pos * len), len,	value);
		return true;
	case symTIMER:
		if(sym->data[0] == 0) {
			time(&now);
			val = (long)now + atol(value);
		}
		else
			val = atol(sym->data) + atol(value);
		snprintf(sym->data, sym->size + 1, "%ld", val);
		return true;
	case symCOUNTER:
		val = atoi(value);
		snprintf(sym->data, sym->size + 1, "%ld", --val);
		return true;
	case symORIGINAL:
		sym->type = symMODIFIED;
	case symMODIFIED:
		setString(sym->data, sym->size + 1, value);
		return true;
	case symNORMAL:
	case symINITIAL:
		setString(sym->data, sym->size + 1, value);
		sym->type = symNORMAL;
		return true;
	case symPROPERTY:
		memcpy(&p, &sym->data, sizeof(p));
		p->set(value, sym->data + sizeof(p), sym->size - sizeof(p));
		return true;
	default:
		return false;
	}
}

bool Script::use(const char *name)
{
	Package *pkg = Package::first;
	char buffer[256];
	const char *dpath = plugins;
	const char *alt = altplugins;
	const char *ns = name;

retry:
	if(strchr(name, '/'))
		return false;

#ifdef	WIN32
	if(strchr(name, '\\'))
		return false;

	if(strchr(name, ':'))
		return false;

#endif
	snprintf(buffer, sizeof(buffer), "%s/%s" RLL_SUFFIX, dpath, name);
	name = buffer;

	while(pkg) {
		if(!strcmp(pkg->filename, name))
			return true;
		pkg = pkg->next;
	}

	if(!canAccess(name)) {
		if(alt) {
			dpath = alt;
			alt = NULL;
			name = ns;
			goto retry;
		}
		slog.error() << "use: cannot find " << name << std::endl;
		return false;
	}


	pkg = new Package(name);
	if(pkg->isValid())
		return true;

	slog.error() << "use: cannot load " << name << std::endl;
	delete pkg;
	return false;
}

Script::Package::Package(const char *name) :
DSO(name)
{
	filename = newString(name);
	next = first;
	first = this;
}

void Script::addFunction(const char *id, unsigned args, Script::Function handler)
{
	Script::Fun *rf = new Script::Fun;
	rf->id = id;
	rf->args = args;
	rf->fn = handler;
	rf->next = ifun;
	ifun = rf;
}

void Script::addConditional(const char *id, Script::Cond handler)
{
	Script::Test *rt = new Script::Test;

	rt->id = id;
	rt->handler = handler;
	rt->next = test;
	test = rt;
}

