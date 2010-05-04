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

static long tens[] = {1, 10, 100, 1000, 10000, 100000, 1000000, 10000000};

long ScriptInterp::getRealValue(double d, unsigned prec)
{
	char buf[20];
	char *cp;
	char lval[9];
	unsigned count;
	long rval;
	snprintf(buf, sizeof(buf), "%f", d);

	rval = atol(buf) * tens[prec];
	cp = strchr(buf, '.');
	if(!cp)
		return rval;
	count = (unsigned)strlen(++cp);
	if(count > prec)
		count = prec;
	strcpy(lval, "00000000");
	strncpy(lval, cp, count);
	lval[prec] = 0;
	if(rval < 0)
		return rval - atol(lval);
	return rval + atol(lval);
}

long ScriptInterp::getIntValue(const char *text, unsigned prec, ScriptProperty *p)
{
	Fun *fun = ifun;
	unsigned count;
	const char *arg;
	long *ival, rval;
	char lval[9];
	const char *orig = text;

	if(p && p->isProperty(text)) {
		rval = p->getValue(text);
		return rval * tens[prec];
	}

	if(!isalpha(*text)) {
		if(!strnicmp("0x", text, 2)) {
			rval = strtol(text, NULL, 16);
			return rval * tens[prec];
		}
		rval = atol(text);
		rval *= tens[prec];
		text = strchr(text, '.');
		if(!text)
			text = strrchr(orig, decimal);
		if(!text)
			return rval;
		count = (unsigned)strlen(++text);
		if(count > prec)
			count = prec;
		strcpy(lval, "00000000");
		strncpy(lval, text, count);
		lval[prec] = 0;
		if(rval < 0)
			return rval - atol(lval);

		return rval + atol(lval);
	}

	while(NULL != fun) {
		if(!stricmp(fun->id, text))
			break;
		fun = fun->next;
	}

	if(!fun)
		return 0;

	if(!fun->args)
		return fun->fn(NULL, prec);

	arg = getValue(NULL);
	if(!arg)
		return 0;

	if(stricmp(arg, "("))
		return 0;

	ival = new long[fun->args];

	count = numericExpression(ival, fun->args, prec);
	if(count != fun->args)
		return 0;

	rval = fun->fn(ival, prec);
	delete[] ival;
	return rval;
}

double ScriptInterp::getDouble(long value, unsigned prec)
{
	return (double)value / (double)tens[prec];
}

long ScriptInterp::getInteger(long value, unsigned prec)
{
	return value / tens[prec];
}

long ScriptInterp::getTens(unsigned prec)
{
	return tens[prec];
}

int ScriptInterp::numericExpression(long *vals, int max, unsigned prec, ScriptProperty *p)
{
	char estack[32];
	unsigned esp = 0;
	long vstack[32], val;
	const char *value;
	char **expr;
	int count = 0;
	long nval;

	static char *elist[] = {"*", "+", "-", "/", "%", NULL};

	vstack[0] = 0;

	while(NULL != (value = getValue(NULL))) {
		expr = elist;
		while(*expr) {
			if(!stricmp(*expr, value))
				break;
			++expr;
		}
		if(*expr)
			estack[esp] = *value;
		else
			estack[esp] = 0;

		if(!stricmp(value, "("))
		{
			if(esp < 31) {
				++esp;
				vstack[esp] = 0;
				continue;
			}
			return -1;
		}

		if(!stricmp(value, ",")) {
			if(esp)
				return -1;
			if(count < max)
				*(vals++) = vstack[0];
			vstack[0] = 0;
			++count;
			continue;
		}
		if(!stricmp(value, ")"))
		{
			if(!esp) {
				if(count < max)
					*(vals++) = vstack[0];
				return ++count;
			}

			switch(estack[--esp]) {
			case '+':
				vstack[esp] = vstack[esp] + vstack[esp + 1];
				break;
			case '*':
				vstack[esp] = vstack[esp] * vstack[esp + 1] / tens[prec];
				break;
			case '/':
				if(vstack[esp + 1] == 0)
					return -1;
				vstack[esp] = vstack[esp] * tens[prec] / vstack[esp + 1];
				break;
			case '-':
				vstack[esp] = vstack[esp] - vstack[esp + 1];
				break;
			case '%':
				vstack[esp] = vstack[esp] % vstack[esp + 1];
				break;
			default:
				vstack[esp] = vstack[esp + 1];
			}
			if(p) {
				nval = p->adjustValue(getInteger(vstack[esp], prec));
				vstack[esp] = nval * tens[prec];
			}
			continue;
		}

		if(!*expr) {
			vstack[esp] = getIntValue(value, prec, p);
			continue;
		}

		value = getValue("0");
		if(!stricmp(value, "("))
		{
			if(esp > 31)
				return -1;
			vstack[++esp] = 0;
			continue;
		}

		val = getIntValue(value, prec, p);

		switch(estack[esp]) {
		case '+':
			vstack[esp] = vstack[esp] + val;
			break;
		case '-':
			vstack[esp] = vstack[esp] - val;
			break;
		case '/':
			if(!val)
				return -1;
			vstack[esp] = vstack[esp] * tens[prec] / val;
			break;
		case '*':
			vstack[esp] = vstack[esp] * val / tens[prec];
			break;
		case '%':
			vstack[esp] = vstack[esp] % atol(value);
			break;
		}
		if(p) {
			nval = p->adjustValue(getInteger(vstack[esp], prec));
			vstack[esp] = nval * tens[prec];
		}
	}
	if(count < max) {
		if(p)
			*(vals++) = p->adjustValue(getInteger(vstack[esp], prec));
		else
			*(vals++) = vstack[esp];
	}
	if(!esp)
		return ++count;
	else
		return -1;
}

bool ScriptInterp::conditionalExpression(void)
{
#ifdef	HAVE_REGEX_H
	regex_t *regex;
	bool rtn;
#endif

	Symbol *sym;
	Test *node = NULL;
	const char *v1, *op = NULL, *v2;
	char n1[12], n2[12];
	size_t l1, l2;
	long ival;
	time_t now;
	Array *a;
	char buf[65];

	// no first parm, invalid

	v1 = getOption(NULL);
	if(!v1)
		return false;

	if(*v1 == '-' || *v1 == '!') {
		node = Script::test;
		while(node) {
			if(!stricmp(node->id, v1 + 1))
				break;
			node = node->next;
		}
		if(node)
			op = (char *)node->id;
	}

	if(!strcmp(v1, "("))
	{
		numericExpression(&ival, 1, frame[stack].decimal);
		snprintf(n1, sizeof(n1), "%ld", ival);
		v1 = n1;
	}
	else if(!stricmp(v1, "-script") || !stricmp(v1, "!script"))
		op = v1;
	else if(!stricmp(v1, "-defined") || !stricmp(v1, "!defined"))
		op = v1;
	else if(!stricmp(v1, "-const") || !stricmp(v1, "!const"))
		op = v1;
	else if(!stricmp(v1, "-expired") || !stricmp(v1, "!expired"))
		op = v1;
	else if(!stricmp(v1, "-modify") || !stricmp(v1, "!modify"))
		op = v1;
	else if(!stricmp(v1, "-array") || !stricmp(v1, "!array"))
		op = v1;
	else if(!stricmp(v1, "-queue") || !stricmp(v1, "!queue"))
		op = v1;
	else if(!stricmp(v1, "-empty") || !stricmp(v1, "!empty"))
		op = v1;
	else if(!stricmp(v1, "-active") || !stricmp(v1, "!active"))
		op = v1;
	else if(!stricmp(v1, "-module") || !stricmp(v1, "-command") || !stricmp(v1, "-has"))
		op = "-module";
	else if(!stricmp(v1, "!module") || !stricmp(v1, "!command"))
		op = "!module";
	else
		v1 = getContent(v1);

	// sym/label by itself, invalid

	if(!op)
		op = getValue(NULL);

	if(!op) {
		frame[stack].index = 0;
		if(v1 && *v1)
			return true;
		return false;
	}

	// ifdef sym ... format assumed

	v2 = getOption(NULL);

	if(!v2) {
		frame[stack].index = 1;
		if(v1 && *v1)
			return true;
		return false;
	}

	if(node) {
		if(*v1 == '!')
			return !node->handler(this, getContent(v2));
		else
			return node->handler(this, getContent(v2));
	}

	if(!stricmp(op, "-script")) {
		if(getScript(getContent(v2)))
			return true;

		return false;
	}

	if(!stricmp(op, "!script")) {
		if(!getScript(getContent(v2)))
			return true;

		return false;
	}

	if(!stricmp(op, "-module"))
	{	v2 = getContent(v2);
		if(!v2)
			return false;
		if(cmd->getHandler(v2))
			return true;
		snprintf(buf, sizeof(buf), "definitions::%s", v2);
		if(getScript(buf))
			return true;
		return false;
	}

	if(!stricmp(op, "!module")) {
		v2 = getContent(v2);
		if(!v2)
			return false;
		if(cmd->getHandler(v2))
			return false;
		snprintf(buf, sizeof(buf), "definitions::%s", v2);
		if(getScript(buf))
			return false;
		return true;
	}

	if(!stricmp(op, "!expired")) {
		sym = mapSymbol(v2, 0);
		if(!sym)
			return false;

		if(sym->type != symTIMER)
			return false;

		if(!sym->data[0])
			return true;

		time(&now);
		if((long)now >= atol(sym->data))
			return false;
		return true;
	}

	if(!stricmp(op, "-expired")) {
		sym = mapSymbol(v2, 0);
		if(!sym)
			return false;

		if(sym->type != symTIMER)
			return false;

		if(!sym->data[0])
			return false;

		time(&now);
		if((long)now >= atol(sym->data))
			return true;

		return false;
	}



	if(!stricmp(op, "-modify") || !stricmp(op, "!const")) {
		sym = mapSymbol(v2, 0);
		if(!sym)
			return false;

		switch(sym->type) {
		case symLOCK:
		case symCONST:
		case symSEQUENCE:
			return false;
		default:
			return true;
		}
	}

	if(!stricmp(op, "!modify") || !stricmp(op, "-const")) {
		sym = mapSymbol(v2, 0);
		if(!sym)
			return true;

		switch(sym->type) {
				case symLOCK:
				case symCONST:
				case symSEQUENCE:
			return true;
		default:
			return false;
		}
	}

	if(!stricmp(op, "-queue")) {
		sym = mapSymbol(v2, 0);
		if(!sym)
			return false;

		switch(sym->type) {
				case symSTACK:
		case symFIFO:
			return true;
		default:
			return false;
		}
	}

	if(!stricmp(op, "!queue")) {
		sym = mapSymbol(v2, 0);
		if(!sym)
			return false;

		switch(sym->type) {
				case symFIFO:
		case symSTACK:
			return false;
		default:
			return true;
		}
	}

	if(!stricmp(op, "-array")) {
		sym = mapSymbol(v2, 0);
		if(!sym)
			return false;

		switch(sym->type) {
		case symARRAY:
			return true;
		default:
			return false;
		}
	}

	if(!stricmp(op, "!array")) {
		sym = mapSymbol(v2, 0);
		if(!sym)
			return true;

		switch(sym->type) {
				case symARRAY:
			return false;
		default:
			return true;
		}
	}

	if(!stricmp(op, "-defined")) {
		sym = mapSymbol(v2, 0);
		if(sym && sym->type != symINITIAL)
			return true;
		return false;
	}

	if(!stricmp(op, "!defined")) {
		sym = mapSymbol(v2, 0);
		if(!sym || sym->type == symINITIAL)
			return true;
		return false;
	}

	if(!stricmp(op, "-active")) {
		sym = mapSymbol(v2, 0);
		if(!sym)
			return false;

		a = (Array *)&sym->data;
		switch(sym->type) {
		case symARRAY:
			if(!a->tail)
				return false;
			return true;
		case symSTACK:
		case symFIFO:
			if(a->tail == a->head)
				return false;
			return true;
		case symLOCK:
			if(strchr(sym->data, ':'))
				return true;
			return false;
		case symCONST:
		case symSEQUENCE:
		case symNORMAL:
		case symPROPERTY:
		case symORIGINAL:
		case symMODIFIED:
		case symCOUNTER:
			return true;
		case symINITIAL:
			return false;
		case symTIMER:
		case symNUMBER:
		case symBOOL:
			if(!sym->data[0])
				return false;
		default:
			return true;
		}
	}

	if(!stricmp(op, "!active")) {
		sym = mapSymbol(v2, 0);
		if(!sym)
			return false;

		a = (Array *)&sym->data;
		switch(sym->type) {
		case symARRAY:
			if(!a->tail)
				return true;
			return false;
		case symSTACK:
		case symFIFO:
			a = (Array *)&sym->data;
			if(a->tail == a->head)
				return true;
			return false;
		case symLOCK:
			if(strchr(sym->data, ':'))
				return false;
			return true;
		case symCONST:
		case symSEQUENCE:
		case symNORMAL:
		case symPROPERTY:
		case symORIGINAL:
		case symMODIFIED:
		case symCOUNTER:
			return false;
		case symINITIAL:
			return true;
		case symTIMER:
		case symNUMBER:
		case symBOOL:
			if(!sym->data[0])
				return true;
		default:
			return false;
		}
	}


	if(!stricmp(op, "-empty")) {
		v2 = getContent(v2);
		if(!v2)
			return true;

		if(!*v2)
			return true;

		return false;
	}

	if(!stricmp(op, "!empty")) {
		v2 = getContent(v2);
		if(!v2)
			return false;

		if(!*v2)
			return false;

		return true;
	}

	if(!strcmp(v2, "("))
	{
		numericExpression(&ival, 1, frame[stack].decimal);
		snprintf(n2, sizeof(n2), "%ld", ival);
		v2 = n2;
	}
	else
		v2 = getContent(v2);

	if(!v1)
		v1 = "";

	if(!v2)
		v2 = "";

	if(!stricmp(op, "=") || !stricmp(op, "-eq")) {
		if(atol(v1) == atol(v2))
			return true;
		return false;
	}

	if(!stricmp(op, "<>") || !stricmp(op, "-ne")) {
		if(atol(v1) != atol(v2))
			return true;
		return false;
	}

	if(!stricmp(op, "==") || !stricmp(op, ".eq.")) {
		if(!stricmp(v1, v2))
			return true;

		return false;
	}

	if(!stricmp(op, "!=") || !stricmp(op, ".ne.")) {
		if(stricmp(v1, v2))
			return true;

		return false;
	}

	if(!stricmp(op, ".of.") || !stricmp(op, "?")) {
		l1 = strlen(v1);
		while(v2 && *v2) {
			if(!strnicmp(v1, v2, l1)) {
				if(v2[l1] == '=' || v2[l1] == ':')
					return true;
			}
			v2 = strchr(v2, ';');
			if(v2)
				++v2;
		}
		return false;
	}

	if(!stricmp(op, "!?")) {
		l1 = strlen(v1);
		while(v2 && *v2) {
			if(!strnicmp(v1, v2, l1)) {
				if(v2[l1] == '=' || v2[l1] == ':')
					return false;
			}
			v2 = strchr(v2, ';');
			if(v2)
				++v2;
		}
		return true;
	}

	if(!stricmp(op, "$") || !stricmp(op, ".in.")) {
		if(strstr(v2, v1))
			return true;
		return false;
	}

	if(!stricmp(op, "!$")) {
		if(strstr(v2, v1))
			return false;

		return true;
	}

#ifdef	HAVE_REGEX_H
	if(!stricmp(op, "~") || !stricmp(op, "!~")) {
		frame[stack].tranflag = false;
		rtn = false;
		regex = new regex_t;
		memset(regex, 0, sizeof(regex_t));

		if(regcomp(regex, v2, REG_ICASE|REG_NOSUB|REG_NEWLINE)) {
			regfree(regex);
			delete regex;
			return false;
		}

		if(regexec(regex, v1, 0, NULL, 0))
			rtn = false;
		else
			rtn = true;

		regfree(regex);
		delete regex;
		if(*op == '!')
			return !rtn;
		return rtn;
	}
#endif

	if(!stricmp(op, "$<") || !stricmp(op, "$+") || !stricmp(op, ".prefix.")) {
		if(!strnicmp(v1, v2, strlen(v1)))
			return true;
		return false;
	}

	if(!stricmp(op, "$>") || !stricmp(op, "$-") || !stricmp(op, ".suffix.")) {
		l1 = strlen(v1);
		l2 = strlen(v2);
		if(l1 <= l2)
			if(!strnicmp(v1, v2 + l2 - l1, l1))
				return true;

		return false;
	}

	if(!stricmp(op, "<") || !stricmp(op, "-lt")) {
		if(atol(v1) < atol(v2))
			return true;
		return false;
	}

	if(!stricmp(op, ".le.")) {
		if(stricmp(v1, v2) <= 0)
			return true;
		return false;
	}

	if(!stricmp(op, ".ge.")) {
		if(stricmp(v1, v2) >= 0)
			return true;
		return false;
	}

	if(!stricmp(op, "<=") || !stricmp(op, "=<") || !stricmp(op, "-le")) {
		if(atol(v1) <= atol(v2))
			return true;
		return false;
	}

	if(!stricmp(op, ">") || !stricmp(op, "-gt")) {
		if(atol(v1) > atol(v2))
			return true;
		return false;
	}

	if(!stricmp(op, ">=") || !stricmp(op, "=>") || !stricmp(op, "-ge")) {
		if(atol(v1) >= atol(v2))
			return true;
		return false;
	}

	// if no op, assume ifdef format

	frame[stack].index = 1;
	if(*v1)
		return true;

	return false;
}

bool ScriptInterp::conditional(void)
{
	Line *line = getLine();
	const char *joiner;
	bool rtn;
	bool andfalse = false;
	bool ortrue = false;

	for(;;)
	{
		rtn = conditionalExpression();
		if(frame[stack].index < line->argc)
			joiner = line->args[frame[stack].index];
		else
			joiner = "";

		if(!stricmp(joiner, "and")) {
			if(!rtn)
				andfalse = true;
		}
		else if(!stricmp(joiner, "or"))
		{
			if(rtn)
				ortrue = true;
		}
		else
			break;

		++frame[stack].index;
	}
	if(andfalse)
		return false;

	if(ortrue)
		return true;

	return rtn;
}
