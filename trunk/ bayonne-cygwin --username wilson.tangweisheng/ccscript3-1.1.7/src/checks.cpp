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

const char *ScriptChecks::chkKeywords(Line *line, ScriptImage *img)
{
	char proto[80];
	Name *scr = img->getCurrent();
	unsigned idx = 0;
	const char *cp;

	if(getMember(line))
		return "no members defined";

	if(hasKeywords(line))
		return "keywords defined, not used";

	if(!line->argc)
		return "keyword list missing";

	while(NULL != (cp = getOption(line, &idx))) {
		if(!isalpha(*cp) && !isdigit(*cp))
			return "invalid keyword entry";
	}

	snprintf(proto, sizeof(proto), "keywords.%s", scr->name);
	if(img->getPointer(proto))
		return "keywords already defined for this function";
	img->setPointer(proto, line);
	return "";
}

const char *ScriptChecks::chkUse(Line *line, ScriptImage *img)
{
	return ScriptBinder::check(line, img);
}

const char *ScriptChecks::chkRestart(Line *line, ScriptImage *img)
{
	if(getMember(line))
		return "member not used in this command";

	return chkNoArgs(line, img);
}

const char *ScriptChecks::chkCall(Line *line, ScriptImage *img)
{
	const char *cp = getMember(line);
	unsigned idx = 0;
	Line *list = NULL;
	char proto[256];
	unsigned len = 0, diff;
	Name *scr = img->getCurrent();
	char *p;

	if(cp)
		return "members not used in this command";

	cp = getOption(line, &idx);
	if(!cp)
		return "target label missing";

	if(*cp == '&')
		++cp;

	if(strchr(cp, ':') || stricmp(line->cmd, "call")) {
		snprintf(proto, sizeof(proto), "keywords.%s", cp);
		list = (Line *)img->getPointer(proto);
	}
	else if(!stricmp(line->cmd, "call"))
	{
		snprintf(proto, sizeof(proto), "keywords.%s", scr->name);
		p = strchr(proto, ':');
		if(!p)
			p = proto + strlen(proto);
		diff = (unsigned)(p - proto);
		snprintf(proto + diff, sizeof(proto) - diff, "::%s", cp);
		list = (Line *)img->getPointer(proto);
	}

	if(*cp == '^' || *cp == '@')
		return "invalid label used";

	if(!list)
		return NULL;

	idx = 0;
	while(NULL != (cp = getOption(list, &idx))) {
		snprintf(proto + len, sizeof(proto) - len, "=%s", cp);
		len = (unsigned)strlen(proto);
	}

	if(len) {
		if(!useKeywords(line, proto))
			return "invalid keyword used for function call";
	}

	return NULL;
}

const char *ScriptChecks::chkSession(Line *line, ScriptImage *img)
{
	if(getMember(line))
		return "member not used in this command";

	if(!line->argc)
		return NULL;

	if(line->argc > 1)
		return "only one session allowed";

	return NULL;
}

const char *ScriptChecks::chkLock(Line *line, ScriptImage *img)
{
	const char *cp;

	if(getMember(line))
		return "member not used in this command";

	if(!line->argc)
		return "lock symbol missing";

	if(line->argc > 1)
		return "only one lock symbol allowed";

	cp = line->args[0];
	if(*cp != '%' && *cp != '&')
		return "lock target must be symbol";

	return NULL;
}

const char *ScriptChecks::chkSignal(Line *line, ScriptImage *img)
{
	const char *cp = getMember(line);

	if(cp)
		return "member not used in this command";

	if(!line->argc)
		return "target handler missing";

	if(line->argc > 1)
		return "only single target handler allowed";

	cp = line->args[0];
	if(*cp != '^')
		return "target must refer to signal handler";

	return NULL;
}

const char *ScriptChecks::chkThrow(Line *line, ScriptImage *img)
{
	const char *cp = getMember(line);

	if(cp)
		return "member not used in this command";

	if(!line->argc)
		return "target handler missing";

	if(line->argc > 1)
		return "only single target handler allowed";

	cp = line->args[0];
	if(*cp != '@' && *cp != '{')
		return "target must refer to event handler";

	return NULL;
}

const char *ScriptChecks::chkGoto(Line *line, ScriptImage *img)
{
	unsigned opt = 0;

	if(getMember(line))
		return "goto has no member";

	if(!getOption(line, &opt))
		return "goto label missing";

	if(getOption(line, &opt))
		return "only one goto label";

	return NULL;
}

const char *ScriptChecks::chkLabel(Line *line, ScriptImage *img)
{
	const char *cp = getMember(line);

	if(cp)
		return "member not used in this command";

	if(!line->argc)
		return "target label missing";

	if(line->argc > 1)
		return "only single target label allowed";

	cp = line->args[0];
	if(*cp == '^' || *cp == '{' || *cp == '@')
		return "invalid label used";

	return NULL;
}

const char *ScriptChecks::chkReturn(Line *line, ScriptImage *img)
{
	const char *cp = getMember(line);

	if(cp)
		return "member not used in this command";

	return NULL;
}

const char *ScriptChecks::chkIgnore(Line *line, ScriptImage *img)
{
	return NULL;
}

const char *ScriptChecks::chkDecimal(Line *line, ScriptImage *img)
{
	if(getMember(line))
		return "member not used for this command";

	if(line->argc != 1)
		return "decimal argument missing";

	return NULL;
}

const char *ScriptChecks::chkOnlyCommand(Line *line, ScriptImage *img)
{
	if(getMember(line))
		return "member not used for this command";

	return chkNoArgs(line, img);
}



const char *ScriptChecks::chkConditional(Line *line, ScriptImage *img)
{
	if(getMember(line))
		return "member not used for this command";

	if(hasKeywords(line))
		return "keywords not used for this command";

	return chkExpression(line, img);
}

const char *ScriptChecks::chkError(Line *line, ScriptImage *img)
{
	if(getMember(line))
		return "member not used for this command";

	return chkOnlyArgs(line, img);
}

const char *ScriptChecks::chkNoArgs(Line *line, ScriptImage *img)
{
	if(line->argc)
		return "arguments not used for this command";

	return NULL;
}

const char *ScriptChecks::chkSlog(Line *line, ScriptImage *img)
{
	const char *member = getMember(line);

	if(member) {
		if(!stricmp(member, ".debug"))
			member = NULL;
		else if(!stricmp(member, ".info"))
			member = NULL;
		else if(!stricmp(member, ".notice"))
			member = NULL;
		else if(!strnicmp(member, ".warn", 5))
			member = NULL;
		else if(!strnicmp(member, ".err", 4))
			member = NULL;
		else if(!strnicmp(member, ".crit", 5))
			member = NULL;
		else if(!stricmp(member, ".alert"))
			member = NULL;
		else if(!strnicmp(member, ".emerg", 6))
			member = NULL;
	}

	if(member)
		return "invalid or unknown log level used";

	return chkHasArgs(line, img);
}

const char *ScriptChecks::chkHasArgs(Line *line, ScriptImage *img)
{
	if(!line->argc)
		return "arguments missing";

	return NULL;
}

const char *ScriptChecks::chkOnlyArgs(Line *line, ScriptImage *img)
{
	if(!line->argc)
		return "arguments missing";

	if(hasKeywords(line))
		return "keywords not used for this command";

	return NULL;
}

const char *ScriptChecks::chkOnlyOneArg(Line *line, ScriptImage *img)
{
	if(line->argc > 1)
		return "too many arguments";

	return chkOnlyArgs(line, img);
}

const char *ScriptChecks::chkRepeat(Line *line, ScriptImage *img)
{
	if(getMember(line))
		return "member not used for this command";

	if(hasKeywords(line))
		return "keywords not used for this command";

	if(line->argc < 1)
		return "at least repeat value required";

	return chkExpression(line, img);
}

const char *ScriptChecks::chkIndex(Line *line, ScriptImage *img)
{
	if(getMember(line))
		return "member not used for this command";

	if(hasKeywords(line))
		return "keywords not used for this command";

	return chkExpression(line, img);
}

const char *ScriptChecks::chkExpr(Line *line, ScriptImage *img)
{
	const char *cp = getMember(line);

	if(cp && !isdigit(cp[0])) {
		cp = chkProperty(line, img);

		if(cp)
			return cp;
	}

	if(cp && atoi(cp) > 6)
		return "numbers only valid to 6 decimal places";

	if(hasKeywords(line))
		return "keywords not used in this command";

	return chkExpression(line, img);
}

const char *ScriptChecks::chkChar(Line *line, ScriptImage *img)
{
	if(getMember(line))
		return "char always size 1";

	if(hasKeywords(line))
		return "no keywords used in char";

	return chkAllVars(line, img);
}

const char *ScriptChecks::chkString(Line *line, ScriptImage *img)
{
	const char *cp = getMember(line);

	if(cp && !isdigit(cp[0]))
		return "member when used must be size";

	if(!useKeywords(line, "=size"))
		return "invalid keyword used for this command";

	return chkAllVars(line, img);
}

const char *ScriptChecks::chkDefine(Line *line, ScriptImage *img)
{
	unsigned idx = 0;
	const char *cp = line->cmd;

	if(!line->argc)
		return "define requires arguments";

	while(idx < line->argc) {
		cp = line->args[idx++];

		if(*cp == '=') {
			++cp;
			++idx;
		}

		if(*cp == '%' || *cp == '&')
			continue;

		if(*cp == '.')
			++cp;

		cp = strchr(cp, ':');
		if(cp) {
			++cp;
			if(!isdigit(*cp))
				return "invalid field size used";
		}
	}
	return NULL;
}

const char *ScriptChecks::chkVarType(Line *line, ScriptImage *img)
{
	if(getMember(line))
		return "no members in type";

	if(hasKeywords(line))
		return "no keywords in type";

	return chkAllVars(line, img);
}

const char *ScriptChecks::chkVar(Line *line, ScriptImage *img)
{
	const char *cp = getMember(line);

	if(cp && !isdigit(cp[0])) {
		cp = chkProperty(line, img);
		if(!cp)
			return "property invalid for var";
	}
	else
		cp = NULL;

	if(!useKeywords(line, "=size=value"))
		return "invalid keyword used";

	return chkAllVars(line, img);
}


const char *ScriptChecks::chkNumber(Line *line, ScriptImage *img)
{
	const char *cp = getMember(line);

	if(cp && !isdigit(cp[0]))
		return "member must be decimal place";

	if(cp && atoi(cp) > 6)
		return "numbers supported only to 6 decimal places";

	if(!useKeywords(line, "=decimal"))
		return "invalid keyword used";

	return chkAllVars(line, img);
}

const char *ScriptChecks::chkConstruct(Line *line, ScriptImage *img)
{
	unsigned int idx = 0;
	const char *cp = getOption(line, &idx);
	if(getMember(line))
		return "no members for this command";

	if(!cp)
		return "destination for contruct is missing";

	if(*cp != '%' && *cp != '&')
		return "destination of contruct must be symbol";

	if(getOption(line,&idx))
		return "only one target for contruct";

	return NULL;
}

const char *ScriptChecks::chkPack(Line *line, ScriptImage *img)
{
	const char *cp = getMember(line);
	if(cp && stricmp(cp, "struct"))
		return "only .struct may be used for this command";

	if(!stricmp(cp, "struct"))
		if(!useKeywords(line, "=size"))
			return "invalid keyword used for pack.struct";

	if(!useKeywords(line, "=field=offset=token=size=quote=prefix=suffix"))
		return "invalid keyword used for this command";

	return chkAllVars(line, img);
}

const char *ScriptChecks::chkClear(Line *line, ScriptImage *img)
{
	if(getMember(line))
		return "member not used for this command";

	if(hasKeywords(line))
		return "keywords not used for this command";

	return chkAllVars(line, img);
}

const char *ScriptChecks::chkExpression(Line *line, ScriptImage *img)
{
	unsigned idx = 0;
	unsigned paren = 0;
	const char *cp;

	while(NULL != (cp = getOption(line, &idx))) {
		if(*cp == '(')
			++paren;
		else if(*cp == ')')
			--paren;

		if(paren < 0)
			return "unbalanced parenthesis in expression";
	}

	if(paren)
		return "unbalanced parenthesis in expression";

	return NULL;
}

const char *ScriptChecks::chkAllVars(Line *line, ScriptImage *img)
{
	unsigned idx = 0;
	const char *cp;

	while(NULL != (cp = getOption(line, &idx))) {
		switch(*cp) {
		case '%':
		case '@':
		case '&':
			break;
		default:
			return "arguments must be symbols";
		}
	}

	return chkHasArgs(line, img);
}

const char *ScriptChecks::chkType(Line *line, ScriptImage *img)
{
	if(getMember(line))
		return "type does not use member";

	if(hasKeywords(line))
		return "type does not use keyword";

	if(line->argc < 1)
		return "type requires at least one var";

	return chkAllVars(line, img);
}

const char *ScriptChecks::chkFirstVar(Line *line, ScriptImage *img)
{
	const char *cp;
	unsigned idx = 0;

	cp = getOption(line, &idx);
	if(!cp)
		return "too few arguments";

	if(*cp != '%' && *cp != '@' && *cp != '&')
		return "first argument must be symbol";

	return chkProperty(line, img);
}

const char *ScriptChecks::chkArray(Line *line, ScriptImage *img)
{
	const char *cp = getMember(line);
	if(cp && !isdigit(*cp))
		return "invalid member used";

	if(!useKeywords(line, "=count=size"))
		return "invalid keywords used";

	if(!getMember(line))
		if(!findKeyword(line, "count"))
			return "requires count either in member or keyword";

	return chkAllVars(line, img);
}

const char *ScriptChecks::chkSet(Line *line, ScriptImage *img)
{
	const char *cp = getMember(line);

	if(cp && !isdigit(*cp))
		cp = chkProperty(line, img);
	else
		cp = NULL;

	if(cp)
		return cp;

	if(!useKeywords(line, "=size=offset"))
		return "invalid keyword used";

	return chkFirstVar(line, img);
}

const char *ScriptChecks::chkFor(Line *line, ScriptImage *img)
{
	if(getMember(line))
		return "member not used for this command";

	if(!useKeywords(line, "=index=size"))
		return "invalid keyword used";

	return chkFirstVar(line, img);
}

const char *ScriptChecks::chkForeach(Line *line, ScriptImage *img)
{
	if(getMember(line))
		return "member not used for this command";

	if(!useKeywords(line, "=index=size=token"))
		return "invalid keyword used";

	if(getCount(line) != 2)
		return "incorrect number of arguments";

	return chkAllVars(line, img);
}

const char *ScriptChecks::chkCat(Line *line, ScriptImage *img)
{
	if(getMember(line))
		return "member not used for this command";

	if(hasKeywords(line))
		return "keywords not used for this command";

	return chkFirstVar(line, img);
}

const char *ScriptChecks::chkRemove(Line *line, ScriptImage *img)
{
	if(getMember(line))
		return "member not used for this command";

	if(!useKeywords(line, "=field=token=offset"))
		return "invalid keyword used for this command";

	return chkFirstVar(line, img);
}

const char *ScriptChecks::chkSequence(Line *line, ScriptImage *img)
{
	if(hasKeywords(line))
		return "keywords not used for this command";

	if(getMember(line))
		return "member not used for this command";

	return chkFirstVar(line, img);
}

const char *ScriptChecks::chkConst(Line *line, ScriptImage *img)
{
	const char *cp = chkProperty(line, img);

	if(cp)
		return cp;

	if(hasKeywords(line))
		return "keywords not used for this command";

	return chkFirstVar(line, img);
}

const char *ScriptChecks::chkCounter(Line *line, ScriptImage *img)
{
	const char *cp = getMember(line);

	if(cp && atoi(cp) < 1)
		return "member must be initial value and greater than zero";

	if(hasKeywords(line))
		return "keywords not used for this command";

	return chkAllVars(line, img);
}

const char *ScriptChecks::chkTimer(Line *line, ScriptImage *img)
{
	if(getMember(line))
		return "timer has no member";

	if(!useKeywords(line, "=offset=expires"))
		return "invalid keyword used for this command";

	return chkAllVars(line, img);
}

const char *ScriptChecks::chkRefArgs(Line *line, ScriptImage *img)
{
	if(hasKeywords(line))
		return "keywords not used for this command";

	if(getMember(line))
		return "member not used for this command";

	if(line->argc != 2)
		return "invalid number of arguments";

	if(!isSymbol(line->args[0]))
		return "reference target not symbol";

	if(!isSymbol(line->args[1]))
		return "reference source not symbol";

	return NULL;
}

const char *ScriptChecks::chkProperty(Line *line, ScriptImage *img)
{
	const char *cp = getMember(line);
	ScriptProperty *prop;

	if(!cp)
		return NULL;

	prop = ScriptProperty::find(cp);
	if(!prop)
		return "unknown script property referenced";

	return NULL;
}
