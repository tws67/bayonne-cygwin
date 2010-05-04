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

#include "module.h"
#include "private.h"

namespace binder {
using namespace ost;
using namespace std;

ParseImage::ParseImage() :
#ifdef	WIN32
ScriptImage(server, "/bayonne/bayonnexml")
#else
ScriptImage(server ,"/bayonne/server/bayonnexml")
#endif
{
}

unsigned ParseImage::getList(const char **args, const char *text, unsigned len, unsigned max)
{
	char buf[512];
	unsigned cpos = 0;
	bool var = false;
	bool lchar = 0;
	unsigned count = 0;

	--max;

	if(!len)
		len = strlen(text);

	while(len-- && cpos < 511)
	{
		if(*text == '\r' || *text == '\n')
			break;
		else if(isspace(*text) && var && cpos)
		{
			buf[cpos] = 0;
			*(args++) = dupString(buf);
			if(!--max)
			{
				*args = NULL;
				return count;
			}	
			var = false;
			cpos = 0;
		}
		else if((*text == '%' || *text == '&') && cpos && len && isalpha(text[1]))
		{
			buf[cpos] = 0;
			*(args++) = dupString(buf);
			if(!--max)
			{
				*args = NULL;
				return count;
			}
			var = true;
			cpos = 0;
		}

		if(!cpos && (*text != '%' && *text != '&'))
			buf[cpos++] = '{';

		lchar = *text;
		buf[cpos++] = *(text++);
		++count;
	}
	if(cpos)
	{
		buf[cpos] = 0;
		*(args++) = dupString(buf);
	}
	*args = NULL;
	return count;
}

const char *ParseImage::dupString(const char *text)
{
	char *str = (char *)alloc(strlen(text) + 1);
	strcpy(str, text);
	return (const char *)str;
}

void ParseImage::postCompile(Compile *cc, unsigned long mask)
{
	Name *scr;
	Line *line;
	unsigned count = 0;

	if(!cc->script)
		return;

	scr = cc->script;
	line = scr->first;

	if(!mask)
	{
		mask = 0x03 | cc->addmask | cc->addterm;
		mask &= ~cc->submask;
	}
	scr->mask = mask;

	while(line)
	{
		++count;
		if(!line->mask)
			line->mask = mask;

		line = line->next;
	}

	memset(cc, 0, sizeof(cc));
	slog.debug("%s: parsed %s; %d steps", cc->logname, scr->name, count);
}

void ParseImage::addCompile(Compile *cc, unsigned mask, const char *cmd, const char **args)
{
	Name *scr = cc->script;
	Line *line = (Line *)alloc(sizeof(Line));
	unsigned trap = cc->trap;
	static const char *empty[] = {NULL};

	memset(line, 0, sizeof(line));

	if(trap && !((1 << (trap - 1)) & cc->addterm))
	{
		cc->addmask |= (1 << (trap - 1));
		if(trap > 4 && trap < 21)
			cc->addmask |= 0x08;
	}

	if(cc->last[trap])
		cc->last[trap]->next = line;
	else if(!trap)
		scr->first = line;
	else
		scr->trap[trap - 1] = line;

	cc->last[trap] = line;

	if(trap)
	{
		if(!mask)
			mask = 0x03;
		
		mask &= ~(1 << (trap - 1));
	}

	line->mask = mask;		
	line->lnum = ++cc->lnum;
	line->loop = 0;

	if(!stricmp(cmd, "-assign") || !stricmp(cmd, "set") || !stricmp(cmd, "add") || !stricmp(cmd, "clear"))
		line->loop = 0xffff;

	line->scr.method = getHandler(cmd);
	line->cmd = dupString(cmd);
	line->argc = 0;

	if(!stricmp(cmd, "repeat") || !stricmp(cmd, "for") || !stricmp(cmd, "do") || !stricmp(cmd, "foreach"))
	{
		if(!cc->looplevel[trap])
			++cc->loopid[trap];
		++cc->looplevel[trap];
		line->loop = cc->loopid[trap] * 128 + cc->looplevel[trap];
	}
	else if(!stricmp(cmd, "loop"))
	{
		line->loop = cc->loopid[trap] * 128 + cc->looplevel[trap];
		if(cc->looplevel[trap])
			--cc->looplevel[trap];
	}

	if(!args)
	{
		line->args = empty;
		line->argc = 0;
		return;
	}

	while(args[line->argc])
		++line->argc;

	line->args = (const char **)alloc((line->argc + 1) * sizeof(char *));
	memcpy(line->args, args, sizeof(char *) * (line->argc + 1));
}	

void ParseImage::getCompile(Compile *cc, const char *name)
{
	Name *scr;

	scr = (Name *)alloc(sizeof(Name));
	unsigned key;

	memset(cc, 0, sizeof(Compile));
	memset(scr, 0, sizeof(Name));
	scr->name = (char *)alloc(strlen(name) + 2);
	strcpy((char *)scr->name, name);
	key = Script::getIndex(scr->name);
	scr->mask = 0;
	scr->next = index[key];
	index[key] = scr;

	cc->script = scr;
	cc->trap = 0;
}

} // end namespace
