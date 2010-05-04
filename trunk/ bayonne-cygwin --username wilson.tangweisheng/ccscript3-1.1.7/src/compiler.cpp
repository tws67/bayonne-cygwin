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

ScriptCompiler::ScriptCompiler(ScriptCommand *cmd, const char *symset) :
ScriptImage(cmd, symset)
{
	inccount = 0;
	mlist = NULL;
	scrStream = (istream *)&scrSource;
}

void ScriptCompiler::fastBranch(ScriptInterp *interp)
{
	Line *line;
	ScriptInterp::Frame *frame;
	Method m;
	unsigned maxstep = fastStepping;
	bool nongoto = false;

	frame = interp->getFrame();
	if(frame->line == frame->script->first)
		nongoto = true;

	while(maxstep-- && NULL != (line = interp->getLine())) {
		m = line->scr.method;
		if(line->loop == 0xffff) {
			if(interp->getTrace())
				return;
			interp->setFrame();
			interp->execute(m);
		}
		else if(m == (Method)&ScriptMethods::scrBegin ||
			m == (Method)&ScriptMethods::scrReturn)
		{
			interp->setFrame();
			interp->execute(m);
			return;
		}
		else if(m == (Method)&ScriptMethods::scrGoto ||
			m == (Method)&ScriptMethods::scrRestart)
		{
			if(nongoto)
				return;
			interp->setFrame();
			interp->execute(m);
			return;
		}
		else
			return;
	}
}

bool ScriptCompiler::checkSegment(Name *scr)
{
	return true;
}

const char *ScriptCompiler::preproc(const char *token)
{
	return "unknown keyword";
}

const char *ScriptCompiler::getDefined(const char *token)
{
	return getLast(token);
}

void ScriptCompiler::commit(void)
{
	Name *scr, *target;
	const char *last = "";
	NamedEvent *ev, *from;
	unsigned ecount;
	const char *en;
	char pbuf[65];
	char *ep;

	while(inccount)
		include(incfiles[--inccount]);

	while(mlist) {
		scr = getScript(mlist->source);
		if(!scr) {
			if(mlist->source != last)
				slog.error("include from %s not found", mlist->source);
			goto cont;
		}
		ecount = 0;
		en = mlist->prefix;
		if(!*en)
			en = "*:";

		target = mlist->target;
		from = scr->events;
		while(from) {
			if(!strnicmp(from->name, mlist->prefix, strlen(mlist->prefix))) {
				ev = (NamedEvent *)alloc(sizeof(NamedEvent));
				ev->line = from->line;
				ev->name = from->name;
				ev->type = from->type;
				ev->next = target->events;
				target->events = ev;
				++ecount;
			}

			from = from->next;
		}
		if(ecount) {
			setString(pbuf, sizeof(pbuf), en);
			ep = strchr(pbuf, ':');
			if(ep)
				*ep = 0;
			slog.debug("included %s from %s; %d events", pbuf, mlist->source, ecount);
		}
cont:
		last = mlist->source;
		mlist = mlist->next;
	}

	ScriptImage::commit();
}

char *ScriptCompiler::getToken(char **pre)
{
	static char temp[513];
	char *cp = temp + 1;
	char *base = temp + 1;
	char q;
	int level;

	if(pre)
		if(*pre) {
			cp = *pre;
			*pre = NULL;
			return cp;
		}

	if(*bp == '=') {
		++bp;
	   		if(*bp == '{') {
					level = -1;
					while(*bp) {
				if(*bp == '}' && !level)
					break;
				if(*bp == '{')
					++level;
				else if(*bp == '}')
					--level;
				*(cp++) = *(bp++);
					}
					if(*bp == '}')
				++bp;
					*cp = 0;
			*base = 0x01;	// special token
					return base;
			}

		if(*bp == ' ' || *bp == '\t' || !*bp)
			return "";
		if(*bp == '\"' || *bp == '\'') {
			q = *(bp++);
			while(q) {
				switch(*bp) {
				case '\\':
					++bp;
					if(!*bp) {
						q = 0;
						break;
					}
					switch(*bp) {
					case 't':
						*(cp++) = '\t';
						++bp;
						break;
					case 'b':
						*(cp++) = '\b';
						++bp;
						break;
					case 'n':
						*(cp++) = '\n';
						++bp;
						break;
					default:
						*(cp++) = *(bp++);
					}
					break;
				case 0:
					q = 0;
					break;
				default:
					if(*bp == q) {
						++bp;
						q = 0;
					}
					else
						*(cp++) = *(bp++);
				}
			}
			*cp = 0;
			return base;
		}
		while(*bp != ' ' && *bp != '\t' && *bp)
			*(cp++) = *(bp++);
		*cp = 0;
		return base;
	}

	if(!quote)
		while(*bp == ' ' || *bp == '\t')
			++bp;

	if(!quote && *bp == '#' && (!bp[1] || bp[1] == '!' || isspace(bp[1])))
		return NULL;

	if(!quote && bp[0] == '%' && bp[1] == '%' && (!bp[2] || isspace(bp[2])))
		return NULL;


	if(!*bp) {
		paren = 0;
		quote = false;
		return NULL;
	}

	if(*bp == '\"' && !quote) {
		++bp;
		quote = true;
	}

	if(!quote && *bp == '{') {
		level = -1;
		while(*bp) {
			if(*bp == '}' && !level)
				break;
			if(*bp == '{')
				++level;
			else if(*bp == '}')
				--level;
			*(cp++) = *(bp++);
		}
		if(*bp == '}')
			++bp;
		*cp = 0;
		return base;
	}

	if(!quote) {
		if(*bp == ',' && paren) {
			++bp;
			return ",";
		}
retry:
		while(*bp && !isspace(*bp) && *bp != ',' && *bp != '=' && *bp != '(' && *bp != ')' )
			*(cp++) = *(bp++);

		if(*bp == '(')
			++paren;
		else if(*bp == ')')
			--paren;

		if(*bp == '=' && cp == base) {
			*(cp++) = *(bp++);
			goto retry;
		}

		if(*bp == '=' && cp == base + 1 && ispunct(*base)) {
			*(cp++) = *(bp++);
			goto retry;
		}

		if((*bp == '(' || *bp == ')') && cp == base)
			*(cp++) = *(bp++);

		*cp = 0;
		if(*bp == ',' && !paren)
			++bp;
		else if(*bp == '=')
			*(--base) = *(bp);
		if(!strcmp(base, "=="))
			return ".eq.";
		if(!strcmp(base, "="))
			return "-eq";
		if(!strncmp(base, "!=", 2))
			return ".ne.";
		if(!strncmp(base, "<>", 2))
			return "-ne";
		if(!strcmp(base, "<"))
			return "-lt";
		if(!strcmp(base, "<="))
			return "-le";
		if(!strcmp(base, ">"))
			return "-gt";
		if(!strcmp(base, ">="))
			return "-ge";
		return base;
	}

	if(*bp == '\\' && (bp[1] == '%' || bp[1] == '&' || bp[1] == '.' || bp[1] == '#')) {
		++bp;
		*(cp++) = '{';
		*(cp++) = *(bp++);
	}

requote:
	if(isalnum(*bp) || strchr("~/:,. \t\'", *bp)) {
		while(isalnum(*bp) || strchr("~=/:,. \t\'", *bp))
			*(cp++) = *(bp++);
	}
	else while(!isspace(*bp) && *bp && *bp != '\"')
		*(cp++) = *(bp++);

	if(*bp == '\n' || !*bp)
		paren = 0;

	if(*bp == '\n' || !*bp || *bp == '\"')
		quote = false;

	if(*bp == '\\' && bp[1]) {
		++bp;
		switch(*bp) {
		case 0:
			break;
		case 't':
			*(cp++) = '\t';
			++bp;
			break;
		case 'b':
			*(cp++) = '\b';
			++bp;
			break;
		case 'n':
			*(cp++) = '\n';
			++bp;
			break;
		default:
			*(cp++) = *(bp++);
		}
		goto requote;
	}

	if(*bp == '\n' || *bp == '\"')
		++bp;

	*cp = 0;
	return base;
}

int ScriptCompiler::compile(const char *scrname)
{
	char buffer[129];
	char *token;
	char *ext;

#ifdef	WIN32
	char *l1, *l2;
	setString(buffer, sizeof(buffer), scrname);
	l1 = strrchr(buffer, '/');
	l2 = strrchr(buffer, '\\');
	if(l1 > l2)
		token = l1;
	else
		token = l2;
#else
	setString(buffer, sizeof(buffer), scrname);
	token = strrchr(buffer, '/');
#endif

	if(!token)
		token = buffer;
	else
		++token;

	ext = strrchr(token, '.');
	if(ext) {
		if(strstr(exec_extensions, ext) == NULL)
			*ext = 0;
	}

	return compile(scrname, token);
}

Script::Name *ScriptCompiler::include(const char *token)
{
	char buffer[256];
	const char *local = cmds->getLast("binclude");
	const char *prefix = cmds->getLast("include");
	const char *cp;
	Name *inc = getScript(token);

	if(inc)
		return inc;

	if(!prefix)
		return NULL;

	snprintf(buffer, sizeof(buffer), "virtual.%s", token);
	cp = cmds->getLast(buffer);

	if(local) {
		if(cp)
			snprintf(buffer, sizeof(buffer),
				"%s/%s_%s.mac", local, token, cp);
		else
			snprintf(buffer, sizeof(buffer),
				"%s/%s.mac", local, token);
		if(!isFile(buffer) || !canAccess(buffer))
			local = NULL;
	}

	if(!local) {
		if(cp)
			snprintf(buffer, sizeof(buffer),
				"%s/%s_%s.mac", prefix, token, cp);
		else
			snprintf(buffer, sizeof(buffer),
				"%s/%s.mac", prefix, token);
		if(!isFile(buffer) || !canAccess(buffer))
			return NULL;
	}

	compile(buffer, (char *)token);
	return getScript(token);
}

int ScriptCompiler::compileDefinitions(const char *filename)
{
	char buffer[128];
	int rtn;

	const char *cp = strrchr(filename, '.');
	if(!cp || stricmp(cp, ".def"))
		return 0;

	cp = strrchr(filename, '/');
#ifdef	WIN32
	if(!cp)
		cp = strrchr(filename, '\\');
	if(!cp)
		cp = strchr(filename, ':');
#endif

	if(!cp) {
		cp = cmds->getLast("include");
		if(cp) {
			snprintf(buffer, sizeof(buffer), "%s/%s",
				cp, filename);
			filename = buffer;
		}
	}

	if(!isFile(filename) || !canAccess(filename))
		return 0;

	scrSource.open(filename);
	if(!scrSource.is_open())
		return 0;

	Script::use_definitions = true;

	rtn = compile((istream *)&scrSource, "definitions", filename);
	scrSource.close();
	scrSource.clear();
	return rtn;
}

int ScriptCompiler::compile(const char *scrname, char *name)
{
	int rtn;

	scrSource.open(scrname);
	if(!scrSource.is_open())
		return 0;

	rtn = compile((istream *)&scrSource, name, scrname);
	scrSource.close();
	scrSource.clear();
	return rtn;
}

int ScriptCompiler::compile(istream *str, char *name, const char *scrname)
{
	const char *errmsg = NULL;
	const char *basename = name;
	char filename[65];
	unsigned lnum = 0;
	char namebuf[256];
	char gvarname[128];
	char path[128];
	char csname[128];
	char *command, *token, *pretoken = NULL;
	char *args[SCRIPT_MAX_ARGS + 1];
	int maxargs = SCRIPT_MAX_ARGS;
	const char *err;
	char *cp = (char *)strrchr(scrname, '.');
//	const char *var = cmds->getLast("datafiles");
	bool trapflag;
	int argc, key, tlen, initkey = 0, initcount = 0;
	unsigned i;
	unsigned short count, number, total = 0;
	size_t gvarlen;
	NamedEvent *events, *esave;
	Name *script, *ds;
	Line *line, *last;
	unsigned long addmask, submask, trapmask, mask, cmask = 0;
	Method handler;
	unsigned char loopid, looplevel;
	size_t offset = 0;
	Name *base = NULL, *init = NULL;
	bool sub = false;
	streampos pos;
	bool ignore;
	scrAccess access = scrPUBLIC;
	char temp[64];
	char *pmode = NULL;
	char *ftoken = NULL;
	char *filter;
	bool bm = false;
	bool first = true;
	unsigned long addPmask = 0, subPmask = (unsigned long)~0;
	unsigned long addPmask1 = 0, subPmask1 = (unsigned long)~0;
	char catchname[256];
	const char *embed = NULL;
	bool mscmd = false, execflag = false;
	bool apps = false;
	bool defs = false;
	bool ripple = cmds->ripple;
	bool wrapper = false;
	bool keydata = false;
	merge_t *merge;
	char *tok;

	if(strstr(".bat.cmd", cp))
		mscmd = true;

	if(strstr(apps_extensions, cp))
		apps = true;
	else if(strstr(exec_extensions, cp))
	{
		snprintf(namebuf, sizeof(namebuf), "%s:%s", exec_prefix, name);
		name = namebuf;
		embed = exec_token;
		wrapper = true;
	}

	if(!strnicmp(name, exec_prefix, strlen(exec_prefix)))
		wrapper = true;

	buffer = (char *)malloc(512);
	bufsize = 512;

	scrStream = str;

	if(cp && !stricmp(cp, ".mac")) {
		bm = true;
		ripple = false;
	}

	if(cp && !stricmp(cp, ".scr"))
		ripple = false;

#ifdef	WIN32
	if(cp && !stricmp(cp, ".ini"))
		ripple = true;
#endif

	if(cp && !stricmp(cp, ".conf"))
		ripple = true;

	if(cp && !stricmp(cp, ".def")) {
		ripple = false;
		defs = true;
	}

	if(bm || defs)
		access = scrPROTECTED;

	gvarname[0] = '%';

	snprintf(gvarname + 1, 56, "%s.", name);

	gvarlen = strlen(gvarname);

#ifdef	WIN32
	const char *l1 = strrchr(scrname, '/');
	const char *l2 = strrchr(scrname, '\\');
	if(l1 > l2)
		cp = (char *)l1;
	else
		cp = (char *)l2;
#else
	cp = (char *)strrchr(scrname, '/');
#endif
	if(cp)
		++cp;
	else
		cp = (char *)scrname;
	snprintf(filename, sizeof(filename), "%s", cp);

compile:
	trapflag = false;
	count = number = 0;
	last = NULL;
	addmask = submask = trapmask = 0;
	handler = NULL;
	loopid = looplevel = 0;
	bool then = false;
	events = NULL;
	keydata = false;

	setString(csname, sizeof(csname), name);
	cp = strstr(csname, "::");

	if(cp && !stricmp(cp, "::main") && !ripple) {
		initkey = SCRIPT_INDEX_SIZE;
		*cp = 0;
	}

	if(ripple && cp)
		initkey = SCRIPT_INDEX_SIZE;

	key = Script::getIndex(csname);

	if(first && (bm || defs))
		initkey = SCRIPT_INDEX_SIZE;
	else if(first)
		initkey = key;

	current = script = (Name *)alloc(sizeof(Name));
	memset(script, 0, sizeof(Name));
	script->name = alloc(csname);
	script->mask = 0;
	script->events = NULL;
	addPmask1 = addPmask;
	subPmask1 = subPmask;
	addPmask = 0;
	subPmask = (unsigned long)~0;

	if(!first)
		script->next = index[key];

	script->filename = alloc(filename);

	if(first)
		init = script;

	if(pmode) {
		if(!strnicmp(pmode, "pub", 3) || !stricmp(pmode, "program"))
			script->access = scrPUBLIC;
		else if(!strnicmp(pmode, "priv", 4) || !stricmp(pmode, "state"))
			script->access = scrPRIVATE;
		else if(!strnicmp(pmode, "prot", 4))
			script->access = scrPROTECTED;
		else if(!strnicmp(pmode, "fun", 3))
			script->access = scrFUNCTION;
		else if(!stricmp(pmode, "local"))
			script->access = scrLOCAL;
		else
			script->access = access;
	}
	else
		script->access = access;

	if(!first)
		index[key] = script;

	pmode = NULL;

	if(!base)
		base = script;

	if(sub) {
		sub = false;
		memcpy(script->trap, base->trap, sizeof(base->trap));
	}

	for(;;)
	{
		if(ftoken)
			goto first;

		quote = false;
		paren = 0;
		if(!then) {
			cmask = 0;

			if(offset > bufsize - 160) {
				bufsize += 512;
				buffer = (char *)realloc(buffer, bufsize);
			}

			scrStream->getline(buffer + offset, bufsize - 1 - offset);
			if(scrStream->eof()) {
				if(keydata && !script->first)
					goto keytoken;
				else if(wrapper && !execflag)
				{
					wrapper = false;
					execflag = true;
					embed = NULL;
					apps = false;
					setString(buffer, 16, "exec");
				}
				else if(wrapper || (!total && !count && !offset))
				{
					wrapper = false;
					embed = NULL;
					if(ripple && !strchr(name, ':'))
						break;
					setString(buffer, 16, exit_token);
				}
				else
					break;
			}
			++lnum;
			bp = strrchr(buffer, '\\');
			if(bp) {
				++bp;
				if(isspace(*bp) || !*bp) {
					if(!embed)
						offset = bp - buffer - 1;
					continue;
				}
			}

			bp = buffer;
			while(isspace(*bp))
				++bp;

			if(!*bp && keydata)
				continue;

			if(*bp == '#' && keydata)
				continue;

			if(!strnicmp(bp, "%%", 2) && keydata)
				continue;

			if(isalpha(*bp) && ripple && !script->first) {
				while(isalnum(*bp))
					++bp;

				while(isspace(*bp))
					++bp;

				if(*bp == '=') {
					snprintf(namebuf, sizeof(namebuf), "%s.%s",
						script->name, strtok_r(buffer, " \t\r\n=", &tok));
					cp = ++bp;

					while(isspace(*cp))
						++cp;

					bp = bp + strlen(bp) - 1;
					while(bp > cp && isspace(*bp))
						*(bp--) = 0;

					if(*cp == '\"' && cp[strlen(cp) - 1] == '\"') {
						cp[strlen(cp) - 1] = 0;
						++cp;
					}
					else if(*cp == '\'' && cp[strlen(cp) - 1] == '\'')
					{
						cp[strlen(cp) - 1] = 0;
						++cp;
					}
					else if(*cp == '{' && cp[strlen(cp) - 1] == '}')
					{
						cp[strlen(cp) - 1] = 0;
						++cp;
					}

					setValue(namebuf, cp);
					keydata = true;
					continue;
				}
			}

keytoken:
			if(keydata && !script->first) {
				command = "_keydata_";
				handler = cmds->getHandler("_keydata_");
				keydata = false;
			}
			else
				handler = NULL;

			if(handler) {
				line = (Line *)alloc(sizeof(Line));
				line->cmd = command;
				line->line = number++;
				line->lnum = lnum;
				line->cmask = cmask;
				line->mask = script->mask;
				line->next = NULL;
				line->args = (const char **)alloc(sizeof(char *));
				line->argc = 0;
				line->args[0] = NULL;
				line->scr.method = handler;

				err = cmds->check(command, line, this);
				if(err) {
					if(*err)
						slog.error("%s(%d): %s: %s", filename, lnum, command, err);
				}
				else {
					// second copy in case compile-time registration
					if(line->scr.method != handler) {
						line = (Line *)memcpy(alloc(sizeof(Line)), line, sizeof(Line));
						line->scr.method = handler;
					}
					++count;
					if(!script->first)
						script->first = line;
					else
						last->next = line;
					last = line;
				}
			}

			bp = buffer;
			while(isspace(*bp) && keydata)
				++bp;

			if(!*bp && keydata)
				continue;

			bp = buffer;
			if(embed) {
				if(!mscmd) {
					if(*bp != '#')
						continue;

					++bp;
					if(strnicmp(embed, bp, strlen(embed)))
						continue;
				}

				if(mscmd) {
					while(isspace(*bp))
						++bp;

					if(*bp == '@')
						++bp;

					if(stricmp(bp, "rem"))
						continue;

					bp += 3;
					while(isspace(*bp))
						++bp;

					if(strnicmp(embed, bp, strlen(embed)))
						continue;
				}

				bp += strlen(embed);
				if(isalnum(*bp)) {
					--bp;
					*bp = ' ';
				}
			}
		}
		else
			then = false;

		offset = 0;
		++number;
first:
		while(NULL != (token = getToken(&ftoken))) {
			if(*token == '~') {
		                esave = (NamedEvent *)alloc(sizeof(NamedEvent));
				esave->name = alloc(token + 1);
				esave->line = NULL;
				esave->next = events;
				esave->type = '~';
				events = esave;
				continue;
			}
			if(*token == '@' || *token == '{') {
				esave = (NamedEvent *)alloc(sizeof(NamedEvent));
				esave->name = alloc(token + 1);
				esave->line = NULL;
				esave->next = events;
				esave->type = '@';
				events = esave;
				continue;
			}

			if(!ripple && !embed && !apps)
  			  if(!stricmp(token, "private") || !stricmp(token,
"protected") || !stricmp(token, "public") || !stricmp(token, "program") ||
!stricmp(token, "module"))
			{
				if(!stricmp(token, "module")) {
					ftoken = "fconst";
					token = "program";
				}
				pmode = alloc(token);
repname1:
				name = getToken();

				if(!name)
					break;
				if(*name == '+') {
					if(!stricmp(name, "+dtmf"))
						addPmask |= 0x08;
					else
						addPmask |= cmds->getTrapModifier(name + 1);
					goto repname1;
				}

				if(*name == '-') {
					subPmask &= ~cmds->getTrapModifier(name + 1);
					goto repname1;
				}

				token = "::";
				break;
			}

			if(!strnicmp(token, "func", 4) && !ripple && !embed && !apps && !strchr(token, ':')) {
				ftoken = "fconst";
				pmode = "function";
				goto repname1;
			}


			if(!stricmp(token, "macro") && !ripple && !embed && !apps && !strchr(token, ':')) {
				ftoken = "fconst";
				pmode = "function";
				goto repname1;
			}

			if(!stricmp(token, "local") && !ripple && !embed && !apps && !strchr(token, ':')) {
				ftoken = "fconst";
				pmode = "local";
				goto repname1;
			}

			if(!strnicmp(token, "proc", 4) && !ripple && !embed && !apps && !strchr(token, ':')) {
				pmode = "function";
				goto repname1;
			}

			if(!stricmp(token, "catch") && !ripple && !embed && !apps) {
				ftoken = "fconst";
				name = getToken();
				if(*name == '^')
					snprintf(catchname, sizeof(catchname),
						"-catch-signal:%s", ++name);
				else if(*name == '@')
					snprintf(catchname, sizeof(catchname),
						"-catch-%s", ++name);
				else
					snprintf(catchname, sizeof(catchname),
						"-catch-%s", name);
				name = catchname;
				pmode = "function";
				token = "::";
				break;
			}

			if((ripple || apps) && *token == '[') {
				if(use_funcs)
					pmode = "function";
				else
					pmode = "public";

				name = token + 1;
				cp = strchr(name, ']');
				if(cp)
					*cp = 0;
				token = "::";
				break;
			}

			tlen = (int)strlen(token);
			if(token[tlen - 1] == ':' && !ripple && !apps && !embed) {
				token[tlen - 1] = 0;
				name = token;
				token = "::";
				break;
			}


			if(*token == '^') {
				if(!trapflag) {
					trapmask = 0;
					trapflag = true;
				}
			}

			if(!stricmp(token, "->") && !last) {
				token = "goto";
				break;
			}

			if(*token != '^' && *token != '+' && *token != '-' && *token != '?')
				break;

			if(*token == '^')
				mask = cmds->getTrapMask(token + 1);
			else
				mask = cmds->getTrapModifier(token + 1);

			if(!mask) {
				slog.error("%s(%d): %s: unknown trap id", filename, lnum, token + 1);
				continue;
			}

			switch(*token) {
			case '^':
				last = NULL;
				script->mask |= mask | cmds->getTrapDefault();
				trapmask |= mask;
				break;
			case '+':
				addmask |= mask;
				break;
			case '-':
				submask |= mask;
				break;
			case '?':
				cmask |= mask;
			}
		}

		if(!token)
			continue;

		if(!stricmp(token, "::") && !embed)
			break;

		if(!strnicmp(token, "exec.", 5) && (embed || apps))
			execflag = true;

		if(!stricmp(token, "exec") && (embed || apps))
			execflag = true;

		if(!strnicmp(token, "exec.", 5) && !execflag && exec_token)
			goto noexec;

		if(!stricmp(token, "exec") && !execflag && exec_token) {
noexec:
			slog.error("%s(%d): exec only used in embedded", filename, lnum);
			continue;
		}

		if(!stricmp(token, "disuse") && defs) {
			if(NULL != (token = getToken())) {
				snprintf(temp, sizeof(temp), "use.%s", token);
				cmds->setValue(temp, "none");
				continue;
			}
			token = "disuse";
		}

		if(!stricmp(token, "use")) {
			if(NULL != (token = getToken())) {
				snprintf(temp, sizeof(temp), "use.%s", token);
				cp = (char *)cmds->getLast(temp);
				if(cp)
					token = cp;

				if(!stricmp(token, "none"))
					continue;

				if(!Script::use(token))
					slog.warn("%s(%d): %s: package missing", filename, lnum, token);
			}
			else {
				slog.error("%s(%d): use: name missing", filename, lnum);
				continue;
			}
			snprintf(catchname, sizeof(catchname), "use.%s", token);
			token = catchname;
		}
		else if(!stricmp(token, "virtual") && !embed && (!ripple || use_macros) && scrStream == (istream *)&scrSource)
		{
			token = getToken();
			if(!token)
				continue;

			if(strchr(token, '/'))
				continue;

			if(inccount > 255)
				continue;

			snprintf(temp, sizeof(temp), "virtual.%s", token);
			if(!cmds->getLast(temp)) {
				cp = getToken();
				if(!cp || !*cp)
					cp = "none";
				cmds->setValue(temp, "cp");
			}

			incfiles[inccount++] = alloc(token);
			continue;
		}
		else if(!stricmp(token, "include") && use_merge && !apps && !embed)
		{
			token = getToken();
			if(!token)
				continue;

			if(!strchr(token, ':')) {
				snprintf(temp, sizeof(temp), name);
				cp = strchr(temp, ':');
				if(cp)
					*cp = 0;
				addString(temp, sizeof(temp), "::");
				addString(temp, sizeof(temp), token);
				token = temp;
			}
			token = alloc(token);

			filter = getToken();
			for(;;)
			{
				if(filter) {
					snprintf(temp, sizeof(temp), "%s", filter);
					cp = strchr(temp, ':');
					if(cp)
						*cp = 0;
					addString(temp, sizeof(temp), ":");
					filter = alloc(temp);
				}
				else
					filter = "";

				merge = (merge_t *)alloc(sizeof(merge_t));
				merge->next = mlist;
				merge->target = script;
				merge->source = token;
				merge->prefix = filter;
				mlist = merge;
				filter = getToken();
				if(!filter)
					break;
			}
			continue;
		}
		else if((!stricmp(token, "requires") || !stricmp(token, "import")) && scrStream == (istream *)&scrSource && (!ripple || use_macros) && !apps && !embed)
		{
			token = getToken();
			if(!token)
				continue;

			if(strchr(token, '/'))
				continue;


			if(inccount > 255)
				continue;
			incfiles[inccount++] = alloc(token);
   			continue;
		}

		ignore = false;

		if(!token)
			continue;

		if(*token == '%' && !ripple) {
			pretoken = token;
			token = "expr";
		}
		else if(*token == '&' && !ripple && !apps)
		{
			pretoken = token;
			token = "call";
		}
		else if(!strnicmp(token, "*::", 3) && !ripple && !apps)
		{
			pretoken = token + 3;
			token = "call";
		}
		else if(!strnicmp(token, "::", 2) && !ripple && !apps)
		{
			pretoken = token + 2;
			token = "call";
		}
		else if(strstr(token, "::") && (!ripple || use_macros) && !apps)
		{
			pretoken = token;
			token = "call";
		}
		else
			pretoken = NULL;

		if(*token == '@') {
			ignore = true;
			++token;
		}

		trapflag = false;
		handler = cmds->getHandler(token);
		if(handler == (Method)NULL && use_definitions) {
			snprintf(temp, sizeof(temp), "definitions::%s", token);
			ds = getScript(temp);
			if(ds && ds->access == scrFUNCTION) {
				pretoken = alloc(temp);
				token = "call";
				handler = cmds->getHandler(token);
			}
		}
		if(handler == (Method)NULL) {
			errmsg = preproc(token);
			if(!errmsg)
				continue;

			addmask = submask = 0;
			if(!ignore)
				slog.error("%s(%d): %s: %s", filename, lnum, token, errmsg);
			continue;
		}

		command = alloc(token);
		argc = 0;
		while(argc < maxargs && NULL != (token = getToken(&pretoken))) {
			if(token[0] == '$' && token[1] == '{' && token[strlen(token) - 1] == '}') {
				token[strlen(token) - 1] = 0;
				++token;
				*token = '$';
				goto insert;

			}
			if(token[0] == '$' && isalpha(token[1])) {
insert:
				if(!stricmp(token, "$script.name"))
					token = alloc((char *)basename);
				else if(!stricmp(token, "$script.file"))
					token = alloc((char *)name);
				else if(!stricmp(token, "$script.line"))
				{
					sprintf(temp, "%d", number);
					token = alloc(temp);
				}
				else {
					if(!strchr(++token, '.')) {
						snprintf(path, sizeof(path), "%s.%s", basename, token);
						token = path;
					}
					token = (char *)getDefined(token);
				}

				if(!token)
					token = "";
			}
			else if(token[0] == 0x01)
			{
				token[0]='{';
				token = alloc(token);
			}
			else if(token[0] == '.' && isalpha(token[1]))
			{
				cp = token + strlen(token) - 1;
				if(*cp != '.') {
					gvarname[0] = '%';
					snprintf(gvarname + gvarlen, sizeof(gvarname) - gvarlen, "%s", token + 1);
					token = alloc(gvarname);
				}
				else
					token = alloc(token);
			}
			else if(!strnicmp(token, "=.", 2))
			{
				gvarname[0] = '=';
				snprintf(gvarname + gvarlen, sizeof(gvarname) - gvarlen, "%s", token + 2);
				token = alloc(gvarname);
			}
			else if(!strnicmp(token, "#.", 2))
			{
				gvarname[0] = '#';
				snprintf(gvarname + gvarlen, sizeof(gvarname) - gvarlen, "%s", token + 2);
				token = alloc(gvarname);
			}
			else if(!strnicmp(token, "&.", 2) || !strnicmp(token, ">.", 2))
			{
				gvarname[0] = '&';
				snprintf(gvarname + gvarlen,sizeof(gvarname) - gvarlen, "%s", token + 2);
				token = alloc(gvarname);
			}
			else if(!strnicmp(token, "@.", 2))
			{
				gvarname[0] = '@';
				snprintf(gvarname + gvarlen,sizeof(gvarname) - gvarlen, "%s", token + 2);
				token = alloc(gvarname);
			}
			else if(!strnicmp(token, "%.", 2))
			{
				gvarname[0] = '%';
				snprintf(gvarname + gvarlen, sizeof(gvarname) - gvarlen, "%s", token + 2);
				token = alloc(gvarname);
			}
			else if(*token == '>' && isalnum(token[1]))
			{
				token = alloc(token);
				*token = '&';
			}
			else
				token = alloc(token);


			args[argc++] = token;

			if(!stricmp(token, "then") && handler == (Method)&ScriptMethods::scrIf) {
				--bp;
				*bp = ' ';
				then = true;
				handler = (Method)&ScriptMethods::scrIfThen;
				break;
			}
		}

		args[argc++] = NULL;
		line = (Line *)alloc(sizeof(Line));
		line->line = number;
		line->lnum = lnum;
		line->cmask = cmask;
		line->mask = ((~0 & ~trapmask) | addmask) & ~submask;
		if(script->mask)
			line->mask &= cmds->getTrapHandler(script);
		if(!trapmask) {
			line->mask |= addPmask1;
			line->mask &= subPmask1;
		}
		line->next = NULL;
		line->args = (const char **)alloc(sizeof(char *) * argc);
		line->argc = --argc;
		line->scr.method = handler;
		line->cmd = command;
		line->loop = 0;
		if(cmds->isInitial(line->cmd))
			line->loop = 0xffff;

		addmask = submask = 0;

		if(!stricmp(command, "repeat") || !stricmp(command, "for") || !stricmp(command, "do") || !stricmp(command, "foreach")) {
			if(!looplevel)
				++loopid;
			++looplevel;
			line->loop = loopid * 128 + looplevel;
		}

		if(!stricmp(command, "loop") || !strnicmp(command, "loop.", 5)) {
			line->loop = loopid * 128 + looplevel;
			if(!looplevel) {
				slog.error("%s(%d): loop nesting error", filename, line->lnum);
				continue;
			}
			else
				--looplevel;
		}

		memcpy(line->args, &args, sizeof(char *) * argc);

		err = cmds->check(command, line, this);
		if(err) {
			if(*err)
				slog.error("%s(%d): %s: %s", filename, lnum, command, err);
			continue;
		}

		++count;
		script->mask |= trapmask;
		if(!script->first)
			script->first = line;

		while(events) {
			esave = events->next;
			events->line = line;
			events->next = script->events;
			script->events = events;
			events = esave;
		}

		if(trapmask && !last) {
			for(i = 0; i < TRAP_BITS; ++i)
			{
				if((1l << i) & trapmask) {
					if(!script->trap[i])
						script->trap[i] = line;
				}
			}
		}

		if(last)
			last->next = line;

		last = line;
	}
	line = script->first;
	if(!script->mask)
		script->mask = cmds->getTrapDefault();
	script->mask |= addPmask1;
	script->mask &= subPmask1;

	addPmask1 = 0;
	subPmask1 = (unsigned long)~0;
	while(line) {
		line->mask &= script->mask;
		line->mask |= ((~script->mask) & cmds->imask);
		line = line->next;
	}
	total += count;

	checkSegment(script);

	if(first && count)
		initcount = count;
	else if(count)
		slog.info("compiled %s; %d steps", script->name, count);
	first = false;

	if(!scrStream->eof()) {
		execflag = false;
		if(apps && apps_prefix)
			snprintf(namebuf, sizeof(namebuf), "%s::%s", apps_prefix, name);
		else if(strstr(name, "::") || (ripple && !use_prefix) || apps)
			snprintf(namebuf, sizeof(namebuf), "%s", name);
		else
			snprintf(namebuf, sizeof(namebuf), "%s::%s", basename, name);
		name = namebuf;
		if(!strnicmp(name, "exec:", 5) && exec_funcs)
			pmode = "function";
		goto compile;
	}
	scrSource.close();
	scrSource.clear();
	if(init) {
		line = last = NULL;
		if(initkey == SCRIPT_INDEX_SIZE && initcount)
			line = init->first;

		while(line) {
			if(line->loop != 0xffff) {
				slog.error("%s(%d): %s: not allowed as initializer",filename, line->lnum, line->cmd);
				if(last)
					last->next = line->next;
				else
					first = true;
			}
			line = line->next;
		}
		if(initkey == SCRIPT_INDEX_SIZE && initcount)
			slog.info("compiled %s initializer; %d steps", init->name, initcount);
		else if(initcount)
			slog.info("compiled %s; %d steps", init->name, initcount);
		init->next = index[initkey];
		index[initkey] = init;
	}
	free(buffer);
	return total;
}


