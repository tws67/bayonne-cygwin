// Copyright (C) 1995-1999 David Sugar, Tycho Softworks.
// Copyright (C) 1999-2005 Open Source Telecom Corp.
// Copyright (C) 2005-2008 David Sugar, Tycho Softworks.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <config.h>
#include <ucommon/ucommon.h>
#include <ucommon/export.h>
#include <ccscript.h>
#include <ctype.h>

using namespace UCOMMON_NAMESPACE;

static const char *getArgument(script::line_t *line, unsigned *index)
{
	const char *cp;

	for(;;) {
		if(*index >= line->argc)
			return NULL;
		cp = line->argv[*index];
		++*index;
		if(*cp == '=') {
			++*index;
			continue;
		}
		return cp;
	}
}

static const char *getKeyword(const char *kw, script::line_t *line)
{
	unsigned index = 0;
	const char *cp;

	while(index < line->argc) {
		cp = line->argv[index++];
		if(*cp == '=' && String::equal(kw, ++cp))
			return line->argv[index];	
	}
	return NULL;
}

static unsigned getArguments(script::line_t *line)
{
	unsigned count = 0;
	unsigned index = 0;
	const char *cp;

	while(index < line->argc) {
		cp = line->argv[index++];
		if(*cp == '=') {
			++index;
			continue;
		}
		++count;
	}
	return count;
}

static unsigned getRequired(script::line_t *line)
{
	unsigned count = 0;
	unsigned index = 0;
	const char *cp;

	while(index < line->argc) {
		cp = line->argv[index++];
		if(*cp == '=') {
			++index;
			continue;
		}
		if(String::equal(cp, "required"))
			++count;
	}
	return count;
}

bool script::checks::isText(const char *text)
{
	while(*text) {
		if(!isalnum(*text))
			return false;
		++text;
	}
	return true;
}

bool script::checks::isValue(const char *text)
{
	switch(*text)
	{
	case '-':
	case '+':
		if(text[1] == '.')
			return true;
	case '.':
		if(isdigit(text[1]))
			return true;
		return false;
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	case '&':
	case '%':
	case '$':
		return true;
	default:
		return false;
	}
}

const char *script::checks::chkApply(script *img, script::header *scr, script::line_t *line)
{
	if(line->argc < 1)
		return "template define script required";

	if(line->argc > 1)
		return "only one template define can be applied";

	if(scr->first) 
		return "apply must be first statement of a new script";

	if(scr->events)
		return "only one apply statement can be used";

	if(!isalnum(line->argv[0][0]))
		return "must apply using valid name of a defined script";

	script::header *tmp = script::find(img, line->argv[0]);
	if(!tmp)
		return "invalid or unknown script applied";

	scr->methods = tmp->methods;
	scr->events = tmp->events;
	return NULL;
}

const char *script::checks::chkPrevious(script *img, script::header *scr, script::line_t *line)
{
	method_t method = img->looping();

	if(method == (method_t)NULL)
		return "cannot be called outside loop";

	if(method == (method_t)&script::methods::scrForeach)
		goto valid;

	return "cannot be called outside for or foreach block";

valid:
	if(line->argc)
		return "command has no arguments";

	return NULL;
}

const char *script::checks::chkContinue(script *img, script::header *scr, script::line_t *line)
{
	method_t method = img->looping();

	if(method == (method_t)NULL)
		return "cannot be called outside loop";

	if(method == (method_t)&script::methods::scrDo)
		goto valid;

	if(method == (method_t)&script::methods::scrWhile)
		goto valid;

	if(method == (method_t)&script::methods::scrForeach)
		goto valid;

	return "cannot be called from conditional block";

valid:
	if(line->argc)
		return "command has no arguments";

	return NULL;
}

const char *script::checks::chkStrict(script *img, script::header *scr, script::line_t *line)
{
	unsigned index = 0;
	const char *cp;

	if(!String::equal(scr->name, "_init_"))
		return "strict can only be used in initialization section";

	if(scr->first) 
		return "strict must be defined at start of initialization section";

	if(!line->argc) {
		script::strict::createGlobal(img, "error");
		script::strict::createGlobal(img, "index");
		return NULL;
	}

	while(index < line->argc) {
		cp = line->argv[index++];
		if(!isalpha(*cp))
			return "strict must only declare names of defined internal global vars";
		script::strict::createGlobal(img, cp);	
	}
	return NULL;
}

const char *script::checks::chkBreak(script *img, script::header *scr, script::line_t *line)
{
	method_t method = img->looping();

	if(method == (method_t)NULL)
		return "cannot be called outside loop";

	if(line->argc)
		return "command has no arguments";

	return NULL;
}

const char *script::checks::chkRef(script *img, script::header *scr, script::line_t *line)
{
	if(line->argc != 1)
		return "one symbol argument for referencing";

	const char *cp = line->argv[0];
	if(*cp != '$')
		return "only field operations can be referenced alone";

	return NULL;
}

const char *script::checks::chkWhen(script *img, script::header *scr, script::line_t *line)
{
	if(!line->argc)
		return "missing conditional expression";

	return chkConditional(img, scr, line);
}

const char *script::checks::chkIf(script *img, script::header *scr, script::line_t *line)
{
	if(!img->push(line))
		return "analysis overflow for if command";

	if(img->thencheck)
		return "cannot nest if in if-then clause";

	return chkConditional(img, scr, line);
}

const char *script::checks::chkWhile(script *img, script::header *scr, script::line_t *line)
{
	if(String::equal(scr->name, "_init_"))
		return "this command cannot be used to initialize";

	if(img->thencheck)
		return "cannot use while in if-then clause";

	if(!img->push(line))
		return "stack overflow for while command";

	return chkConditional(img, scr, line);
}

const char *script::checks::chkExpand(script *img, script::header *scr, script::line_t *line)
{
	unsigned index = 1;
	const char *cp;

	if(line->argc < 1)
		return "no value to expand";

	if(line->argc < 2)
		return "no symbols to assign";

	if(!isValue(line->argv[0]))
		return "cannot expand non-value";

	while(line->argv[index]) {
		cp = line->argv[0];
		switch(*cp) {
		case '&':
			return "cannot assign literal";
		case '=':
			return "no keywords are used in this command";
		case '$':
			return "cannot assign to format";
		case '@':
			return "cannot assign to label";
		case '^':
			return "cannot assign to event";
		case '?':
			return "cannot assign to expression";
		}		
		strict::createAny(img, scr, line->argv[index++]);
	}
	return NULL;
}

const char *script::checks::chkForeach(script *img, script::header *scr, script::line_t *line)
{
	const char *cp;

	if(String::equal(scr->name, "_init_"))
		return "this command cannot be used to initialize";

	if(img->thencheck)
		return "cannot use for in if-then clause";

	if(!img->push(line))
		return "stack overflow for do command";

	if(!line->argc)
		return "no symbols to assign";

	if(line->argc < 2 || line->argc > 3)
		return "assign from only one source";

	cp = line->argv[0];
	switch(*cp) {
	case '&':
		return "cannot assign literal";
	case '=':
		return "no keywords are used in this command";
	case '$':
		return "cannot assign to format";
	case '@':
		return "cannot assign to label";
	case '^':
		return "cannot assign to event";
	case '?':
		return "cannot assign to expression";
	}		
	strict::createAny(img, scr, line->argv[0]);
	
	if(!isValue(line->argv[1]))
		return "cannot assign from label or expression";

	if(line->argc == 3 && !isValue(line->argv[2]))
		return "skip must be a value or symbol";

	return NULL;
}

const char *script::checks::chkDo(script *img, script::header *scr, script::line_t *line)
{
	if(String::equal(scr->name, "_init_"))
		return "this command cannot be used to initialize";

	if(img->thencheck)
		return "cannot use do in if-then clause";

	if(line->argc)
		return "no arguments for do command";

	if(!img->push(line))
		return "stack overflow for do command";

	return NULL;
}

const char *script::checks::chkEndif(script *img, script::header *scr, script::line_t *line)
{
	script::method_t method = img->pull();

	if(img->thencheck)
		return "cannot endif in if-then clause";

	if(method != (method_t)&script::methods::scrIf && method != (method_t)&script::methods::scrElse)
		return "endif not within an if block";

	if(line->argc)
		return "endif has no arguments";

	return NULL;
}

const char *script::checks::chkEndcase(script *img, script::header *scr, script::line_t *line)
{
	script::method_t method = img->pull();

	if(img->thencheck)
		return "cannot endcase in if-then clause";

	if(String::equal(scr->name, "_init_"))
		return "this command cannot be used to initialize";

	if(method != (method_t)&script::methods::scrCase && method != (method_t)&script::methods::scrOtherwise)
		return "endcase not within a case block";

	if(line->argc)
		return "endcase has no arguments";

	--line->loop;
	return NULL;
}

const char *script::checks::chkLoop(script *img, script::header *scr, script::line_t *line)
{
	script::method_t method = img->pull();
	
	if(img->thencheck)
		return "can not end loop in if-then clause";

	if(String::equal(scr->name, "_init_"))
		return "this command cannot be used to initialize";

	if(method == (method_t)&script::methods::scrWhile)
		goto valid;

	if(method == (method_t)&script::methods::scrDo)
		goto valid;

	if(method == (method_t)&script::methods::scrForeach)
		goto valid;

	if(method == (method_t)NULL)
		return "not called from within loop";
		
	return "not called from valid loop";

valid:
	if(line->argc)
		return "loop has no arguments";

	return NULL;
}

const char *script::checks::chkCase(script *img, script::header *scr, script::line_t *line)
{
	script::method_t method = img->looping();

	if(img->thencheck)
		return "cannot create case in if-then clause";

	if(String::equal(scr->name, "_init_"))
		return "this command cannot be used to initialize";

	if(method == (method_t)&script::methods::scrOtherwise) 
		return "cannot have case after otherwise";

	if(method != (method_t)&script::methods::scrCase) {
		if(!img->push(line))
			return "stack overflow for do command";
	}
	else {
		img->pull();
		img->push(line);
		--line->loop;
	}

	return chkConditional(img, scr, line);
}

const char *script::checks::chkElif(script *img, script::header *scr, script::line_t *line)
{
	script::method_t method = img->looping();

	if(img->thencheck)
		return "cannot have elif in if-then clause";

	if(method == (method_t)&script::methods::scrElse) {
		return "cannot have more if conditions after else";
	}

	if(method != (method_t)&script::methods::scrIf) {
		return "cannot have elif outside of if block";
	}

	return chkConditional(img, scr, line);
}

const char *script::checks::chkElse(script *img, script::header *scr, script::line_t *line)
{
	script::method_t method = img->looping();

	if(img->thencheck)
		return "cannot have else in if-then clause";

	if(method == (method_t)&script::methods::scrElse) {
		return "cannot have multiple else statements";
	}

	if(method != (method_t)&script::methods::scrIf) {
		return "cannot have else outside of if block";
	}

	// replace loop with else member to prevent duplication...
	img->pull();
	img->push(line);

	if(line->argc)
		return "otherwise has no arguments";

	return NULL;
}

const char *script::checks::chkOtherwise(script *img, script::header *scr, script::line_t *line)
{
	script::method_t method = img->looping();

	if(img->thencheck)
		return "cannot have otherwise if-then clause";

	if(String::equal(scr->name, "_init_"))
		return "this command cannot be used to initialize";

	if(method != (method_t)&script::methods::scrCase) {
		return "cannot have otherwise outside of case block";
	}

	// replace loop with otherwise member to prevent duplication...
	img->pull();
	img->push(line);
	--line->loop;

	if(line->argc)
		return "otherwise has no arguments";

	return NULL;
}

const char *script::checks::chkUntil(script *img, script::header *scr, script::line_t *line)
{
	script::method_t method = img->pull();

	if(img->thencheck)
		return "cannot have until in if-then clause";

	if(String::equal(scr->name, "_init_"))
		return "this command cannot be used to initialize";

	if(method == (method_t)NULL || method != (method_t)&script::methods::scrDo)
		return "not called from within do loop";

	return chkConditional(img, scr, line);
}

const char *script::checks::chkNop(script *img, script::header *scr, script::line_t *line)
{
	if(line->argc)
		return "arguments are not used for this command";

	return NULL;
}

const char *script::checks::chkExit(script *img, script::header *scr, script::line_t *line)
{
	if(line->argc)
		return "arguments are not used for this command";

	if(String::equal(scr->name, "_init_"))
		return "this command cannot be used to initialize";

	return NULL;
}

const char *script::checks::chkInvoke(script *img, script::header *scr, script::line_t *line)
{
	script::line_t *sub = line->sub->first;
	unsigned required = getRequired(sub);
	unsigned limit = getArguments(sub);
	unsigned count = getArguments(line);
	unsigned index = 0;
	const char *cp, *kw;
		
	if(String::equal(scr->name, "_init_"))
		return "this command cannot be used to initialize";

	if(count < required)
		return "too few arguments for invoked command";

	if(count > limit)
		return "too many arguments for invoked command";

	index = 0;
	while(index < sub->argc) {
		kw = sub->argv[index++];
		if(*kw == '=') {
			cp = sub->argv[index++];
			if(String::equal(cp, "required") && !getKeyword(++kw, line))
				return "required keyword missing";
		}
	}
	index = 0;
	while(index < line->argc) {
		kw = line->argv[index++];
		if(*kw == '=') {
			if(!getKeyword(++kw, sub))
				return "unknown or invalid keyword used";
			++index;
		}
	}
	return NULL;
}

const char *script::checks::chkDefine(script *img, script::header *scr, script::line_t *line)
{
	unsigned count = 0;
	unsigned index = 0;
	const char *cp;
	char idbuf[8];

	if(img->thencheck)
		return "cannot define in if-then clause";

	if(!line->argc)
		return NULL;

	while(index < line->argc) {
		cp = line->argv[index++];
		if(*cp == '=') {
			if(strchr(cp, ':'))
				return "no size or type set for referencing";
			strict::createVar(img, scr, cp);
			continue;
		}

		if(isalpha(*cp)) {
			if(String::equal(cp, "optional")) 
				line->argv[index - 1] = (char *)"&";	// rewrite to null
			else if(!String::equal(cp, "required"))
				return "invalid keyword used in prototype";
		}
		else if(!isValue(cp))
			return "cannot assign from label or expression";

		if(img->isStrict()) {
			snprintf(idbuf, sizeof(idbuf), "%d", ++count);
			strict::createVar(img, scr, img->dup(idbuf));
		}
	}

	return NULL;
}

const char *script::checks::chkPack(script *img, script::header *scr, script::line_t *line)
{
	unsigned index = 1;
	const char *cp;

	if(!line->argc)
		return "no symbols to assign";

	if(line->argc < 2)
		return "no values to assign";

	cp = line->argv[0];
	switch(*cp) {
	case '&':
		return "cannot assign literal";
	case '=':
		return "cannot assign members before symbol name";
	case '$':
		return "cannot assign to format";
	case '@':
		return "cannot assign to label";
	case '^':
		return "cannot assign to event";
	case '?':
		return "cannot assign to expression";
	}		

	script::strict::createAny(img, scr, cp);

	while(index < line->argc) {
		cp = line->argv[index++];
		if(*cp == '=')
			continue;
		if(!isValue(cp))
			return "cannot assign from label or expression";
	}

	return NULL;
}

const char *script::checks::chkPush(script *img, script::header *scr, script::line_t *line)
{
	const char *cp;

	if(line->argc < 1)
		return "no symbol to push";

	if(line->argc > 3)
		return "only use value or key and value";

	cp = line->argv[0];
	switch(*cp) {
	case '&':
		return "cannot assign literal";
	case '=':
		return "no keywords are used in this command";
	case '$':
		return "cannot assign to format";
	case '@':
		return "cannot assign to label";
	case '^':
		return "cannot assign to event";
	case '?':
		return "cannot assign to expression";
	}		
	script::strict::createSym(img, scr, cp);
	if(!isValue(line->argv[1]) && !isText(line->argv[1]))
		return "cannot push from label or expression";

	if(line->argv[2] && !isValue(line->argv[2]) && !isText(line->argv[1]))
		return "cannot push value from label or expression";

	return NULL;
}

const char *script::checks::chkSet(script *img, script::header *scr, script::line_t *line)
{
	unsigned drop = 0;
	unsigned index = 1;
	const char *cp;

	if(!line->argc)
		return "no symbols to assign";

	if(line->argc > 2) {
		if(String::equal(line->argv[1], ":="))
			drop = 1;
		else if(String::equal(line->argv[1], "+=")) {
			drop = 1;
			line->method = (method_t)&methods::scrAdd;
		}
	}

	if(drop) {
		--line->argc;
		while(drop && drop < line->argc) {
			line->argv[drop] = line->argv[drop + 1];
			++drop;
		}
		line->argv[drop] = NULL;
	}

	if(line->argc < 2)
		return "no values to assign";

	cp = line->argv[0];
	switch(*cp) {
	case '&':
		return "cannot assign literal";
	case '=':
		return "no keywords are used in this command";
	case '$':
		return "cannot assign to format";
	case '@':
		return "cannot assign to label";
	case '^':
		return "cannot assign to event";
	case '?':
		return "cannot assign to expression";
	}		

	script::strict::createSym(img, scr, cp);
	
	while(index < line->argc) {
		cp = line->argv[index++];
		if(*cp == '=')
			return "no keywords are used in this command";
		if(!isValue(cp))
			return "cannot assign to format";
	}
	return NULL;
}

const char *script::checks::chkClear(script *img, script::header *scr, script::line_t *line)
{
	unsigned index = 0;
	const char *cp;

	if(!line->argc)
		return "no symbols to clear";

	while(index < line->argc) {
		cp = line->argv[index++];
		if(*cp != '%')
			return "invalid symbol reference or syntax";
		if(strchr(cp, ':'))
			return "invalid size usage for symbol";
	}
	return NULL;
}

const char *script::checks::chkError(script *img, script::header *scr, script::line_t *line)
{
	unsigned index = 0;
	const char *cp;

	if(!line->argc)
		return "no error message";

	while(index < line->argc) {
		cp = line->argv[index++];
		if(*cp == '=')
			return "no keywords used in error";

		if(*cp == '^' || *cp == '@')
			return "cannot use label for error";
	}
	return NULL;
}

const char *script::checks::chkConst(script *img, script::header *scr, script::line_t *line)
{
	unsigned index = 0;
	const char *cp;

	if(!line->argc)
		return "no constants to assign";

	while(index < line->argc) {
		cp = line->argv[index++];
		if(*cp != '=')
			return "cannot assign data or use uninitialized symbol as const";

		if(strchr(cp, ':'))
			return "cannot alter size of const symbol";

		script::strict::createVar(img, scr, cp);

		cp = line->argv[index++];
		if(*cp == '=')
			return "invalid assignment of const";
		if(!isValue(cp))
			return "cannot use label or expression for const";
	}
	return NULL;
}

const char *script::checks::chkGoto(script *img, script::header *scr, script::line_t *line)
{
	unsigned index = 0;
	const char *cp;

	if(String::equal(scr->name, "_init_"))
		return "this command cannot be used to initialize";

	if(line->argc < 1)
		return "goto requires at least one label or event handler";

	while(index < line->argc) {
		cp = line->argv[index++];
		if(*cp != '@' && *cp != '^')
			return "goto only allows scripts and handlers";
	}
	return NULL;
}

const char *script::checks::chkGosub(script *img, script::header *scr, script::line_t *line)
{
	unsigned index = 0;
	const char *cp;

	if(String::equal(scr->name, "_init_"))
		return "this command cannot be used to initialize";

	if(line->argc < 1)
		return "gosub requires at least one label";

	while(index < line->argc) {
		cp = line->argv[index++];
		if(*cp != '@' && !isalnum(*cp))
			return "gosub only allows script sections and named methods";
	}

	return NULL;
}

const char *script::checks::chkConditional(script *img, script::header *scr, script::line_t *line)
{
	unsigned index = 0;
	const char *cp;

	while(index < line->argc) {
		cp = line->argv[index++];
		if((*cp == '-' || *cp == '!') && isalpha(cp[1])) {
			if(index >= line->argc)
				return "missing test value";
			cp = line->argv[index++];
			if(*cp == '?')
				return "cannot test operator";
		}
		else {
			if(*cp == '?')
				return "cannot use operator as element of expression";

			if(index >= line->argc)
				return "missing operator in expression";

			cp = line->argv[index++];

			if(*cp != '?' && !String::equal(cp, "eq") && !String::equal(cp, "ne") && !String::equal(cp, "lt") && !String::equal(cp, "gt") && !String::equal(cp, "le") && !String::equal(cp, "ge") && !String::equal(cp, "is") && !String::equal(cp, "isnot") && !String::equal(cp, "in") && !String::equal(cp, "notin"))
				return "invalid operator in expression";

			if(index >= line->argc)
				return "missing value from expression";

			cp = line->argv[index++];
			if(*cp == '?')
				return "cannot use operator as element of expression";
		}
		if(index == line->argc)
			return NULL;

		cp = line->argv[index++];
		if(String::equal("?&&", cp) || String::equal("?||", cp) || String::equal("and", cp) || String::equal("or", cp))
			continue;

		return "invalid expression joiner statement";
	}
	return "conditional expression missing";
}
	
const char *script::checks::chkIndex(script *img, script::header *scr, script::line_t *line)
{
	unsigned index = 0;
	const char *cp;
	method_t method = img->looping();

	if(method == (method_t)NULL)
		return "cannot be called outside loop";

	if(method == (method_t)&script::methods::scrForeach)
		goto valid;

	return "cannot be called outside for or foreach block";

valid:
	if(!line->argc)
		return "requires at least position";

	while(index < line->argc) {
		cp = getArgument(line, &index);
		if(!isValue(cp))
			return "malformed expression";

		cp = getArgument(line, &index);
		if(!cp)
			break;
		if(!String::equal(cp, "*") && !String::equal(cp, "/") && !String::equal(cp, "+") && !String::equal(cp, "-") && !String::equal(cp, "#"))
			return "invalid expression used";
		if(index == line->argc)
			return "incomplete expression";
	}

	return NULL;
}

const char *script::checks::chkExpr(script *img, script::header *scr, script::line_t *line)
{
	unsigned index = 0;
	const char *cp;

	if(line->argc < 3)
		return "no variable to assign";

	while(index < line->argc) {
		cp = line->argv[index++];
		if(*cp == '=') {
			if(!String::equal(cp, "=decimals"))
				return "invalid keyword for expression";
			++index;
		}
	}

	index = 0;
	cp = getArgument(line, &index);
	if(!cp)
		return "no assignment";

	if(*cp == '&')
			return "cannot assign literal as symbol";

	if(*cp == '$')
		return "cannot assign format as symbol";

	if(*cp == '@' || *cp == '^')
		return "cannot assign label as symbol";

	if(*cp == '?')
		return "cannot assign expression as symbol";

	script::strict::createVar(img, scr, cp);
	
	cp = getArgument(line, &index);
	if(!cp)
		return "expression incomplete";

	if(!String::equal(cp, ":=") && !String::equal(cp, "?=") && !String::equal(cp, "+=") && !String::equal(cp, "-=") && !String::equal(cp, "*=") && !String::equal(cp, "/="))
		return "expression must start with assignment expression";

	while(index < line->argc) {
		cp = getArgument(line, &index);
		if(!isValue(cp))
			return "malformed expression";

		cp = getArgument(line, &index);
		if(!cp)
			break;
		if(!String::equal(cp, "*") && !String::equal(cp, "/") && !String::equal(cp, "+") && !String::equal(cp, "-") && !String::equal(cp, "#"))
			return "invalid expression used";
		if(index == line->argc)
			return "incomplete expression";
	}
	return NULL;
}

const char *script::checks::chkVar(script *img, script::header *scr, script::line_t *line)
{
	unsigned index = 0;
	const char *cp;

	if(!line->argc)
		return "no variables to assign";

	while(index < line->argc) {
		cp = line->argv[index++];
		if(*cp == '&')
			return "cannot assign literal as symbol";

		if(*cp == '$')
			return "cannot assign format as symbol";

		if(*cp == '@' || *cp == '^')
			return "cannot assign label as symbol";

		if(*cp == '?')
			return "cannot assign expression as symbol";

		if(*cp != '=')
			continue;

		script::strict::createVar(img, scr, cp);
		cp = line->argv[index++];
		if(*cp == '=')
			return "invalid assignment of variables";
		if(!isValue(cp))
			return "cannot assign from label or expression";
	}
	return NULL;
}
		
const char *script::checks::chkIgnore(script *img, script::header *scr, script::line_t *line)
{
	return NULL;
}
