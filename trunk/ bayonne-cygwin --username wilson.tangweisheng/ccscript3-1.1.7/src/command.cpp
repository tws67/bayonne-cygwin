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

using namespace std;
using namespace ost;

#ifdef	HAVE_STRCASECMP
#ifndef	stristr
#define	stristr(x, y) strcasestr(x, y)
#endif
#endif

#ifdef	WIN32
#define	stristr(x,y)	strstr(x,y)
#endif

ScriptCommand::ScriptCommand(ScriptCommand *ini) :
Keydata(), Mutex()
{
	memcpy(&keywords, &ini->keywords, sizeof(keywords));
	memcpy(&traps, &ini->traps, sizeof(traps));
	active = NULL;
	keyword_count = ini->keyword_count;
	trap_count = ini->trap_count;
	imask = ini->imask;
	dbcount = 0;
	dbc = NULL;
	tq = NULL;
	ripple = ini->ripple;
}

ScriptCommand::ScriptCommand() :
Keydata(), Mutex()
{
	imask = 0;
	unsigned i;
	memset(&keywords, 0, sizeof(keywords));
	for(i = 0; i < TRAP_BITS; ++i)
		traps[i] = "<undefined>";

	ripple = false;
	active = NULL;
	keyword_count = 0;
	trap_count = 0;
	dbcount = 0;
	dbc = NULL;
	tq = NULL;
}

const char *ScriptCommand::getExternal(const char *opt)
{
	return NULL;
}

bool ScriptCommand::isInput(Line *line)
{
	return false;
}

void ScriptCommand::errlog(const char *level, const char *msg)
{
}

bool ScriptCommand::control(char **args)
{
	ScriptBinder *module;
	ScriptImage *img;

	enter();
	module = ScriptBinder::first;
	img = active;

	while(module) {
		if(module->control(img, args))
			break;
		module = module->next;
	}

	leave();
	if(module)
		return true;

	return false;
}

Script::Method ScriptCommand::getHandler(const char *keyword)
{
	Keyword *key;
	char keybuf[33];
	int len = 0;
	char *kw = keybuf;

	while(len++ < 32 && *keyword && *keyword != '.')
		*(kw++) = (*keyword++);
	*kw = 0;
	keyword = keybuf;

	key = keywords[Script::getIndex(keyword)];

	while(key) {
		if(!stricmp(key->keyword, keyword))
			return key->method;

		key = key->next;
	}
	return (Method)NULL;
}

bool ScriptCommand::isInitial(const char *keyword)
{
	Keyword *key;
	char keybuf[33];
	int len = 0;
	char *kw = keybuf;

	while(len++ < 32 && *keyword && *keyword != '.')
		*(kw++) = (*keyword++);
	*kw = 0;
	keyword = keybuf;

	key = keywords[Script::getIndex(keyword)];

	while(key) {
		if(!stricmp(key->keyword, keyword))
			return key->init;

		key = key->next;
	}
	return false;
}

const char *ScriptCommand::check(char *keyword, Line *line, ScriptImage *img)
{
	Keyword *key;
	char keybuf[33];
	int len = 0;
	char *kw = keybuf;

	while(len++ < 32 && *keyword && *keyword != '.')
		*(kw++) = *(keyword++);

	*kw = 0;
	keyword = keybuf;
	key = keywords[Script::getIndex(keyword)];

	while(key) {
		if(!stricmp(key->keyword, keyword))
			return check(key->check, line, img);

		key = key->next;
	}
	return "unknown command";
}

void ScriptCommand::aliasModule(const char *id, const char *use)
{
	char temp[65];

	snprintf(temp, sizeof(temp), "use.%s", id);
	setValue(temp, use);
}

unsigned long ScriptCommand::getTrapDefault(void)
{
	return 0x03;
}

unsigned long ScriptCommand::getTrapHandler(Name *scr)
{
	return getTrapDefault();
}

unsigned long ScriptCommand::getTrapModifier(const char *trapname)
{
	return getTrapMask(trapname);
}

const char *ScriptCommand::check(Check chk, Line *line, ScriptImage *img)
{
	return (this->*(chk))(line, img);
}

unsigned ScriptCommand::getTrapId(const char *trapname)
{
	unsigned i;

	for(i = 0; i < TRAP_BITS; ++i)
	{
		if(!stricmp(traps[i], trapname))
			return i;
	}
	return 0;
}

unsigned long ScriptCommand::getTrapMask(unsigned id)
{
	return 1 << id;
}

unsigned long ScriptCommand::getTrapMask(const char *trapname)
{
	unsigned long mask = 1;
	unsigned i;

	for(i = 0; i < TRAP_BITS; ++i)
	{
		if(!stricmp(traps[i], trapname))
			return mask;

		mask = mask << 1;
	}
	return 0;
}

void ScriptCommand::load(Script::Define *keydefs)
{
	size_t len;
	int key;
	Keyword *script;

	for(;;)
	{
		if(!keydefs->keyword)
			break;

		len = strlen(keydefs->keyword) + 1;
		key = Script::getIndex(keydefs->keyword);
		script = (Keyword *)alloc(sizeof(Keyword) + len - 1);
		setString(script->keyword, len, keydefs->keyword);
		script->method = keydefs->method;
		script->init = keydefs->init;
		script->check = keydefs->check;
		script->next = keywords[key];
		keywords[key] = script;
		++keydefs;
	}
}

const char *ScriptCommand::getTrapName(unsigned id)
{
	if(id < trap_count)
		return traps[id];

	return NULL;
}

int ScriptCommand::trap(const char *trapname, bool inherited)
{
	if(inherited)
		imask |= (1 << trap_count);

	traps[trap_count++] = alloc((char *)trapname);
	return trap_count;
}

bool ScriptCommand::isInherited(unsigned id)
{
	if(imask & (1 << id))
		return true;

	return false;
}

bool ScriptCommand::useMember(Line *line, const char *list)
{
	const char *cp = getMember(line);

	if(cp && !list)
		return false;

	if(!cp)
		return true;

	if(stristr(list, cp))
		return true;

	return false;
}

unsigned ScriptCommand::getCount(Line *line)
{
	unsigned idx = 0;
	unsigned count = 0;
	while(idx < line->argc) {
		if(*(line->args[idx++]) == '=')
			++idx;
		else
			++count;
	}
	return count;
}

bool ScriptCommand::useKeywords(Line *line, const char *list)
{
	unsigned idx = 0;
	const char *cp;

	while(idx < line->argc) {
		cp = line->args[idx++];
		if(*cp != '=')
			continue;

		if(!list)
			return false;

		if(!stristr(list, cp))
			return false;
		++idx;
	}
	return true;
}

bool ScriptCommand::hasKeywords(Line *line)
{
	unsigned idx = 0;

	if(!stricmp(line->cmd, "_keydata_"))
		return true;

	while(idx < line->argc)
		if(*line->args[idx++] == '=')
			return true;

	return false;
}

const char *ScriptCommand::findKeyword(ScriptImage *img, Line *line, const char *keyword)
{
	unsigned idx = 0;
	const char *cp;
	char namebuf[128];

	while(idx < line->argc) {
		cp = line->args[idx++];
		if(*cp == '=') {
			if(!stricmp(++cp, keyword))
				return line->args[idx];
			++idx;
		}
	}
	if(img) {
		snprintf(namebuf, sizeof(namebuf), "%s.%s", img->getCurrent()->name, keyword);
		return img->getLast(namebuf);
	}
	return NULL;
}

const char *ScriptCommand::findKeyword(Line *line, const char *keyword)
{
	unsigned idx = 0;
	const char *cp;

	while(idx < line->argc) {
		cp = line->args[idx++];
		if(*cp == '=') {
			if(!stricmp(++cp, keyword))
				return line->args[idx];
			++idx;
		}
	}
	return NULL;
}

const char *ScriptCommand::getMember(Line *line)
{
	const char *cp = strchr(line->cmd, '.');

	if(cp)
		++cp;

	return cp;
}

const char *ScriptCommand::getOption(Line *line, unsigned *idx)
{
	const char *cp;

	for(;;)
	{
		if(*idx >= line->argc)
			return NULL;

		cp = line->args[*idx];
		++*idx;
		if(*cp == '=') {
			++*idx;
			continue;
		}
		else if(*cp == '{')
			return ++cp;
		else
			return cp;
	}
}
