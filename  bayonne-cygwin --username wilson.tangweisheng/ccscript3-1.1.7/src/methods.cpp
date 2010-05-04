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

bool ScriptMethods::scrNop(void)
{
	skip();
	return true;
}

bool ScriptMethods::scrIf(void)
{
	if(conditional()) {
		if(frame[stack].index < frame[stack].line->argc)
			return intGoto();
		skip();
		if(frame[stack].line->scr.method == (Method)&ScriptMethods::scrThen)
			skip();
		return true;
	}
	skip();
	return true;
}

bool ScriptMethods::scrIfThen(void)
{
	if(!conditional())
		skip();

	skip();
	return true;
}

bool ScriptMethods::scrSlog(void)
{
	const char *member = getMember();
	const char *opt;

	if(member) {
		if(!stricmp(member, "debug"))
			slog(Slog::levelDebug);
		else if(!stricmp(member, "info"))
			slog(Slog::levelInfo);
		else if(!stricmp(member, "notice"))
			slog(Slog::levelNotice);
		else if(!strnicmp(member, "warn", 4))
			slog(Slog::levelWarning);
		else if(!strnicmp(member, "err", 3))
			slog(Slog::levelError);
		else if(!strnicmp(member, "crit", 4))
			slog(Slog::levelCritical);
		else if(!stricmp(member, "alert"))
			slog(Slog::levelAlert);
		else if(!strnicmp(member, "emerg", 5))
			slog(Slog::levelEmergency);
		else
			slog(Slog::levelNotice);
	}
	else
		slog(Slog::levelNotice);

	slog() << logname << ": ";
	while(NULL != (opt = getValue(NULL)))
		slog() << opt;
	slog() << endl;
	skip();
	return true;
}

bool ScriptMethods::scrThen(void)
{
	int level = 0;
	Line *line;

	skip();

	while(NULL != (line = frame[stack].line)) {
		skip();

		if(line->scr.method == (Method)&ScriptMethods::scrThen)
			++level;
		else if(line->scr.method == (Method)&ScriptMethods::scrElse)
		{
			if(!level)
				return true;
		}
		else if(line->scr.method == (Method)&ScriptMethods::scrEndif)
		{
			if(!level)
				return true;
			--level;
		}
	}
	return true;
}

bool ScriptMethods::scrElse(void)
{
	int level = 0;
	Line *line;

	skip();

	while(NULL != (line = frame[stack].line)) {
		skip();

		if(line->scr.method == (Method)&ScriptMethods::scrThen)
			++level;
		else if(line->scr.method == (Method)&ScriptMethods::scrEndif)
		{
			if(!level)
				return true;
		}
	}
	return true;
}

bool ScriptMethods::scrEndif(void)
{
	frame[stack].tranflag = false;
	skip();
	return true;
}

bool ScriptMethods::scrInit(void)
{
	const char *cp, *value;
	Line *line = getLine();
	unsigned idx = 0;
	ScriptSymbols *syms;
	Symbol *sym;

	syms = getLocal();

	while(idx < line->argc) {
		cp = line->args[idx++];
		if(*cp != '=')
			continue;

		value = getContent(line->args[idx++]);
		syms = getLocal();
		sym = syms->find(++cp, (unsigned short)strlen(value));
		if(!sym)
			continue;
		if(sym->type != symINITIAL)
			continue;
		setString(sym->data, sym->size + 1, value);
		sym->type = symCONST;
	}
	skip();
	return true;
}

static bool gotField(const char *cp, char pack, unsigned count, const char* match, unsigned len)
{
	while(count-- && cp) {
		cp = strchr(cp, pack);
		if(cp && *cp == pack)
			++cp;
	}
	if(!cp)
		cp = ",";

	if(strnicmp(cp, match, len))
		return false;

	if(cp[len] == pack || !cp[len])
		return true;

	return false;
}

bool ScriptMethods::scrRemove(void)
{
	Symbol *sym;
	Array *a;
	ScriptProperty *p;
	const char *opt, *key;
	char buffer[1024];
	char pack = getPackToken();
	unsigned idx, ndx;
	char *cp, *dp;
	Line *line = getLine();
	bool queue = false;
	unsigned offset = 0, len, count;

	opt = getKeyword("offset");
	if(!opt)
		opt = getKeyword("field");
	if(opt)
		offset = atoi(opt);

	opt = getKeyword("token");
	if(opt && *opt)
		pack = *opt;

	if(!stricmp(line->cmd, "last") || !stricmp(line->cmd, "first"))
		queue = true;

	opt = getOption(NULL);

	sym = mapSymbol(opt);
	if(!sym) {
		error("symbol-missing");
		return true;
	}

	buffer[0] = 0;
	while(NULL != (opt = getValue(NULL)))
		addString(buffer, sizeof(buffer), opt);
	key = opt = buffer;
	count = offset;
	while(key && count--) {
		key = strchr(key, pack);
		if(key && *key == pack)
			++key;
	}
	if(!key)
		key = ",";

	count = 0;
	while(key[count] && key[count] != pack)
		++count;

	a = (Array *)&sym->data;

	switch(sym->type) {
	case symFIFO:
	case symSTACK:
		if(a->head == a->tail)
			break;
		if(sym->type == symFIFO) {
			dp = sym->data + sizeof(Array) + a->head * (a->rec + 1);
			if(gotField(dp, pack, offset, key, count)) {
				++a->head;
				if(a->head >= a->count)
					a->head = 0;
				break;
			}
		}
		if(sym->type == symSTACK) {
			dp = sym->data + sizeof(Array) + a->tail * (a->rec + 1);
			if(gotField(dp, pack, offset, key, count)) {
				if(!a->tail)
					a->tail = a->count;
				--a->tail;
				break;
			}
		}
		idx = a->head;
		if(sym->type == symSTACK)
			if(++idx >= a->count)
				idx = 0;
		for(;;)
		{
			if(++idx >= a->count)
				idx = 0;

			if(idx == a->tail)
				break;

			dp = sym->data + sizeof(Array) + idx * (a->rec + 1);
			if(gotField(dp, pack, offset, key, count))
				break;
		}

		if(idx == a->tail)
			break;

		for(;;)
		{
			ndx = idx + 1;
			if(ndx >= a->count)
				ndx = 0;
			if(ndx == a->tail && sym->type == symFIFO)
				break;
			if(idx == a->tail && sym->type == symSTACK)
				break;

			strcpy(sym->data + sizeof(Array) + idx * (a->rec + 1),sym->data + sizeof(Array) + ndx * (a->rec + 1));

			++idx;
			if(idx >= a->count)
				break;
		}
		a->tail = idx;
		if(sym->type == symSTACK) {
			if(!a->tail)
				a->tail = a->count;
			--a->tail;
		}
		break;
	case symARRAY:
		if(queue) {
			error("invalid-queue-symbol");
			break;
		}
		idx = 0;
		if(!a->tail)
			break;

		len = (unsigned)strlen(opt);
		while(idx < a->tail) {
			dp = sym->data + sizeof(Array) + idx * (a->rec + 1);
			if(gotField(dp, pack, offset, key, count))
				break;
			++idx;
		}
		if(a->head > idx)
			--a->head;
		while(idx < (unsigned)(a->tail - 1)) {
			strcpy(sym->data + sizeof(Array) + idx * (a->rec + 1),
				sym->data + sizeof(Array) + (idx + 1) * (a->rec + 1));
			++idx;
		}
		if(a->tail)
			--a->tail;
		break;
	case symPROPERTY:
		memcpy(&p, &sym->data, sizeof(p));
		if(p->token())
			pack = p->token();
	case symNORMAL:
	case symNUMBER:
		if(queue) {
			error("invalid-queue-symbol");
			break;
		}
		cp = (char *)extract(sym);
		len = (unsigned int)strlen(buffer);
		while(cp && *cp) {
			if(!strncmp(cp, opt, len))
				if(cp[len] == pack || !cp[len])
					break;

			cp = strchr(cp, pack);
			if(cp && *cp == pack)
				++cp;
		}
		if(!cp || !*cp)
			break;

		while(cp[len]) {
			*cp = cp[len];
			++cp;
		}
		*cp = 0;
		break;
	default:
		error("symbol-invalid");
		return true;
	}

	if(!stricmp(line->cmd, "last"))
		append(sym, opt);

	if(!stricmp(line->cmd, "first") && sym->type == symFIFO) {
		idx = a->head;
		if(!idx)
			idx = a->count;
		if(--idx != a->tail) {
			a->head = idx;
			snprintf(sym->data + sizeof(Array) + a->head * (a->rec + 1), a->rec + 1, "%s", opt);
		}
	}

	if(!stricmp(line->cmd, "first") && sym->type == symSTACK) {
		idx = a->tail;
		if(++idx >= a->count)
			idx = 0;
		if(idx != a->tail) {
			a->tail = idx;
			snprintf(sym->data + sizeof(Array)+ a->tail * (a->rec + 1), a->rec + 1, "%s", opt);
		}
	}

	skip();
	return true;
}

bool ScriptMethods::scrArray(void)
{
	unsigned short rec = symsize - 10;
	unsigned char count;
	const char *kw = getKeyword("count");
	const char *mem = getMember();
	Symbol *sym;
	Array *a;
	unsigned size;
	char *err = NULL;
	Line *line = getLine();

	if(kw)
		count = atoi(kw);
	else
		count = atoi(mem);

	kw = getKeyword("size");
	if(kw)
		rec = atoi(kw);

	if(!count || !rec) {
		error("symbol-no-size");
		return true;
	}

	kw = line->cmd;
	if(!strnicmp(kw, "stack", 5) || !strnicmp(kw, "fifo", 4) || !strnicmp(kw, "lifo", 4))
		++count;

	size = (rec + 1) * count + sizeof(Array);

	while(NULL != (mem = getOption(NULL))) {
		sym = mapSymbol(mem, size);
		if(!sym) {
			err = "symbol-invalid";
			continue;
		}
		if(sym->type != symINITIAL || sym->size != size) {
			err = "symbol-already-defined";
			continue;
		}

		if(!strnicmp(kw, "array", 5))
			sym->type = symARRAY;
		else if(!strnicmp(kw, "fifo", 4))
			sym->type = symFIFO;
		else if(!strnicmp(kw, "stack", 5) || !strnicmp(kw, "lifo", 4))
			sym->type = symSTACK;

		a = (Array *)&sym->data;
		a->head = a->tail = 0;
		a->rec = rec;
		a->count = count;
	}
	skip();
	return true;
}

bool ScriptMethods::scrSet(void)
{
	unsigned max = symlimit;
	char buffer[1024];
	const char *id = getOption(NULL);
	const char *opt = getMember();
	ScriptProperty *prop = ScriptProperty::find(opt);
	unsigned short size = symsize;
	Symbol *sym;
	Line *line = getLine();
	bool cat = false;
	bool rtn;
	bool pset = false;
	unsigned ss = stack, len;
	int maxsize = 0, pos = 0;
	char *ep;

	if(opt && isdigit(*opt))
		size = atoi(opt);

	opt = getKeyword("size");
	if(opt) {
		maxsize = atoi(opt);
		size = atoi(opt);
	}

	opt = getKeyword("offset");
	if(opt)
		pos = atoi(opt);

	if(maxsize < 0) {
		pos += maxsize;
		maxsize = -maxsize;
		size = -maxsize;
	}

	if(max > 1024)
		max = 1024;

	if(!id || (*id != '%' && *id != '@' && *id != '&')) {
		error("set-invalid-symbol");
		return true;
	}

	buffer[0] = 0;
	while(NULL != (opt = getValue(NULL)))
		addString(buffer, max + 1, opt);

	if(!stricmp(line->cmd, "pset"))
		pset = true;

	opt = buffer;
	while(pset && stack) {
		--stack;
		if(frame[stack].local != frame[stack + 1].local)
			break;
	}
	sym = mapSymbol(id, size);
	stack = ss;
	if(!stricmp(line->cmd, "cat") || !stricmp(line->cmd, "add"))
		cat = true;
	else if(!strnicmp(line->cmd, "init", 4))
	{
		if(sym && sym->type != symINITIAL && sym->type != symORIGINAL) {
			skip();
			return true;
		}
	}

	if(!sym) {
		error("set-sym-missing");
		return true;
	}

	if(sym->type == symINITIAL && prop)
		sym->type = symPROPERTY;
	else if(sym->type == symINITIAL)
		sym->type = symNORMAL;

	len = (unsigned)strlen(opt);
	if(pos < 0 && -pos > (int)len)
		pos = 0;
	else if(pos < 0)
	{
		opt = opt + len + pos;
		len = -pos;
	}
	else if(pos >= (int)len)
		len = 0;
	else {
		opt += pos;
		len -= pos;
	}

	if(!len)
		opt = "";

	if(maxsize && (int)len > maxsize) {
		ep = (char *)opt + maxsize;
		*ep = 0;
	}

	if(cat)
		rtn = append(sym, opt);
	else
		rtn = commit(sym, opt);

	if(rtn)
		skip();
	else
		error("set-type-invalid");

	return true;
}

bool ScriptMethods::scrConst(void)
{
	unsigned max = symlimit;
	char buffer[1024], pbuf[1024];
	const char *id = getOption(NULL);
	const char *opt;
	ScriptProperty *prop = ScriptProperty::find(getMember());

	if(max > 1024)
		max = 1024;

	if(!id || (*id != '%' && *id != '@' && *id != '&')) {
		error("const-invalid-symbol");
		return true;
	}

	buffer[0] = 0;
	while(NULL != (opt = getValue(NULL)))
		addString(buffer, max + 1, opt);

	if(prop && !buffer[0]) {
		prop->clear(pbuf);
		opt = buffer;
	}
	else if(prop)
	{
		prop->set(pbuf, buffer, max);
		opt = pbuf;
	}
	else
		opt = buffer;

	if(setConst(id, opt))
		skip();
	else
		error("const-already-defined");
	return true;
}

bool ScriptMethods::scrSequence(void)
{
	Line *line = getLine();
	const char *id = getOption(NULL);
	const char *opt;
	unsigned limit = line->argc;
	ScriptSymbols *syms;
	Symbol *sym;
	unsigned size = --limit * sizeof(opt);
	unsigned idx = 0;

	if(!id || (*id != '%' && *id != '@' && *id != '&')) {
		error("sequence-invalid-symbol");
		return true;
	}

	syms = getSymbols(id);
	if(!syms) {
		error("sequence-symbol-invalid");
		return true;
	}
	sym = syms->find(id, size);
	if(!sym) {
		error("sequence-symbol-invalid");
		return true;
	}
	if(sym->type != symINITIAL || sym->size != size) {
		error("sequence-already-defined");
		return true;
	}
	while(idx < limit) {
		opt = syms->cstring(getValue(""));
		memcpy(&sym->data[idx * sizeof(opt)], &opt, sizeof(opt));
		++idx;
	}
	sym->type = symSEQUENCE;
	sym->data[sym->size] = 0;
	skip();
	return true;
}


bool ScriptMethods::scrDecimal(void)
{
	frame[stack].decimal = atoi(getValue("0"));
	skip();
	return true;
}

bool ScriptMethods::scrError(void)
{
	char buffer[256];
	const char *cp;
	buffer[0] = 0;

	while(NULL != (cp = getValue(NULL)))
		addString(buffer, sizeof(buffer), cp);

	error(buffer);
	return true;
}

bool ScriptMethods::scrExit(void)
{
	while(stack)
		pull();

	frame[stack].line = NULL;
	return true;
}

bool ScriptMethods::scrSignal(void)
{
	const char *cp = getOption(NULL);

	if(!cp) {
		error("signal-target-missing");
		return true;
	}

	if(*cp == '^') {
		if(!signal(++cp))
			error("signal-trap-invalid");

		return true;
	}

	error("signal-target-invalid");
	return true;
}

bool ScriptMethods::scrThrow(void)
{
	const char *cp = getOption(NULL);

	if(!cp) {
		error("throw-target-missing");
		return true;
	}

	if(!scriptEvent(++cp))
		advance();
	return true;
}

bool ScriptMethods::scrGoto(void)
{
	unsigned argc = 0;
	Line *line = getLine();
	const char *var, *value;

	while(argc < line->argc) {
		var = line->args[argc++];
		if(*var != '=')
			continue;

		++var;
		value = getContent(line->args[argc++]);
		if(!value)
			continue;

		setSymbol(var, value, 0);
	}

	return intGoto();
}

bool ScriptMethods::intGoto(void)
{
	frame[stack].tranflag = false;
	if(image->isRipple() && frame[stack].local == NULL) {
		ripple();
		return true;
	}
	return redirect(true);
}

bool ScriptMethods::scrRestart(void)
{
	clearStack();
	branching();

	frame[stack].caseflag = frame[stack].tranflag = false;
	frame[stack].line = frame[stack].first = frame[stack].script->first;
	frame[stack].index = 0;
	if(isFunction(frame[stack].script))
		frame[stack].tranflag = true;

	return true;
}

bool ScriptMethods::scrTimer(void)
{
	const char *err = NULL;
	Symbol *sym;
	time_t now;
	const char *cp;
	const char *off = getKeyword("offset");
	const char *exp = getKeyword("expires");

	time(&now);

	while(NULL != (cp = getOption(NULL))) {
		sym = mapSymbol(cp, 23);
		if(!sym) {
err:
			err = "timer-symbol-invalid";
			continue;
		}
		if(sym->type == symINITIAL && sym->size > 22)
			goto set;
		if(sym->type == symTIMER)
			goto set;
		goto err;
set:
		snprintf(sym->data, 12, "%ld", now);
		sym->type = symTIMER;
		if(off)
			commit(sym, off);
		else if(exp && atol(exp) > 0)
			commit(sym, exp);
		else if(exp)
			sym->data[0] = 0;
	}
	if(err)
		error(err);
	else
		skip();
	return true;
}

bool ScriptMethods::scrCounter(void)
{
	unsigned long lval = 0;
	const char *err = NULL;
	const char *cp = getMember();
	Symbol *sym;

	if(cp)
		lval = atol(cp) - 1;

	while(NULL != (cp = getOption(NULL))) {
		sym = mapSymbol(cp, 11);
		if(!sym) {
			err = "var-symbol-invalid";
			continue;
		}
		if(sym->type != symINITIAL) {
			err = "var-already-defined";
			continue;
		}
		snprintf(sym->data, sym->size + 1, "%ld", lval);
		sym->type = symCOUNTER;
	}
	if(err)
		error(err);
	else
		skip();
	return true;
}

bool ScriptMethods::scrSession(void)
{
	const char *cp = getValue(NULL);
	ScriptInterp *interp;

	if(!cp) {
		session = this;
		skip();
		return true;
	}

	interp = getInterp(cp);
	if(interp) {
		session = interp;
		skip();
	}
	else
		error("session-invalid-id");

	return true;
}

bool ScriptMethods::scrLock(void)
{
	Symbol *sym;
	const char *cp = getOption(NULL);
	ScriptInterp *locker = NULL;
	unsigned long lockseq;
	char evtname[65];

	sym = mapSymbol(cp, 23);
	if(!sym) {
		error("lock-symbol-undefined");
		return true;
	}
	if(sym->type == symLOCK) {
		cp = strchr(sym->data, ':');
		if(cp) {
			locker = getInterp(++cp);
			lockseq = atol(sym->data);
		}
		if(locker && locker == this && lockseq == sequence)
			return true;
		if(locker && locker->getSequence() == lockseq) {
			snprintf(evtname, sizeof(evtname), "locked:%s", sym->id);
			if(scriptEvent(evtname))
				return true;
			error("lock-symbol-locked");
			return true;
		}
		sym->type = symINITIAL;
	}
	if(sym->size != 23 || sym->type != symINITIAL) {
		error("lock-symbol-invalid");
		return true;
	}
	snprintf(sym->data, sym->size + 1, "%ld:%u", sequence, getId());
	sym->type = symLOCK;
	skip();
	return true;
}

bool ScriptMethods::scrDeconstruct(void)
{
	const char *cp = getOption(NULL);
	const char *bp;
	char *dp;
	char name[65];
	char value[960];
	Symbol *sym = mapSymbol(cp, 0);
	Symbol *dest;

	if(!sym) {
		error("invalid-symbol");
		return true;
	}

	bp = sym->data;
	while(*bp) {
		dp = name;
		while(*bp && *bp != ':' && *bp != '=')
			*(dp++) = *(bp++);
		*dp = 0;
		if(!*bp)
			break;
		else
			++bp;
		dp = value;
		while(*bp && *bp != ';')
			*(dp++) = *(bp++);
		*dp = 0;
		if(*bp == ';')
			++bp;
		dest = getKeysymbol(name);
		if(!dest)
			continue;
		commit(dest, value);
	}
	skip();
	return true;
}

bool ScriptMethods::scrConstruct(void)
{
	const char *target = getOption(NULL);
	unsigned idx = 0;
	char buffer[1024];
	unsigned count = 0;
	const char *tag, *value;
	Line *line = getLine();
	char *ep;

	++target;
	while(idx < line->argc) {
		tag = line->args[idx++];
		if(!tag || *tag != '=')
			continue;

		++tag;
		value = getContent(line->args[idx++]);
		if(!value)
			continue;

		if(count)
			snprintf(buffer + count, sizeof(buffer) - count, ";%s=%s",
				tag, value);
		else
			snprintf(buffer + count, sizeof(buffer) - count, "%s=%s",
				tag, value);
		while(NULL != (ep = strchr(buffer + count + 1, ';')))
			*ep = ',';
		count = strlen(buffer);
	}
	setConst(target, buffer);
	advance();
	return true;
}

bool ScriptMethods::scrPack(void)
{
	char pack = getPackToken();
	const char *prefix = getKeyword("prefix");
	const char *suffix = getKeyword("suffix");
	const char *cp = getKeyword("token");
	const char *q = getKeyword("quote");
	Symbol *sym;
	unsigned size = symsize;
	char buffer[1024], tokbuf[2];
	ScriptProperty *p;
	bool unpack = false;
	Line *line = getLine();
	char *lp = buffer, *ep;
	unsigned offset = 0;

	if(!stricmp(line->cmd, "unpack"))
		unpack = true;

	if(q && !*q)
		q = NULL;

	if(cp && *cp)
		pack = *cp;

	cp = getKeyword("offset");
	if(!cp)
		cp = getKeyword("field");
	if(cp)
		offset = atoi(cp);

	cp = getKeyword("size");
	if(cp)
		size = atoi(cp);

	cp = getOption(NULL);
	if(unpack)
		sym = mapSymbol(cp);
	else
		sym = mapSymbol(cp, size);

	if(!sym) {
		error("symbol-invalid");
		return true;
	}

	if(sym->type == symPROPERTY) {
		memcpy(&p, &sym->data, sizeof(p));
		if(p->token())
			pack = p->token();
	}

	tokbuf[0] = 0;
	buffer[0] = 0;

	if(!stricmp(line->cmd, "unpack.struct")) {
		ep = sym->data;
		while(ep && *ep) {
			lp = strchr(ep, ':');
			if(!lp)
				lp = strchr(ep, '=');
			if(!lp)
				break;
			*(lp++) = 0;
			cp = (const char *)ep;
			ep = strchr(lp, ';');
			if(ep)
				*(ep++) = 0;
			setConst(cp, lp);
		}
		clear(sym);
		advance();
		return true;
	}

	if(!unpack && prefix)
		setString(buffer, sizeof(buffer), prefix);

	if(unpack) {
		cp = extract(sym);
		if(prefix && !strnicmp(cp, prefix, strlen(prefix)))
			cp += strlen(prefix);
		if(cp && *cp)
			setString(buffer, sizeof(buffer), cp);
		else {
			skip();
			return true;
		}
		if(suffix && !stricmp(buffer + strlen(buffer) - strlen(suffix), suffix))
			buffer[strlen(buffer) - strlen(suffix)] = 0;

		if(!q) {
			if(*lp == '\'')
				q = "'";
			else if(*lp == '"')
				q = "\"";
		}
	}

	while(unpack && NULL != (cp = getOption(NULL))) {
		if(offset) {
			sym = NULL;
			--offset;
		}
		else {
			sym = mapSymbol(cp, size);
			if(!sym) {
				error("symbol-missing");
				return true;
			}
		}

		if(*lp == pack) {
			commit(sym, "");
			++lp;
			continue;
		}

		if(q && !strnicmp(lp, q, strlen(q))) {
			lp += strlen(q);
			ep = strstr(lp, q);
			if(!ep)
				ep = lp + strlen(lp);
			else {
				*ep = 0;
				ep += strlen(q);
			}
			commit(sym, lp);
			if(*ep == pack)
				++ep;
			if(!*ep)
				break;
			lp = ep;
			continue;
		}
		ep = strchr(lp, pack);
		if(ep)
			*(ep++) = 0;
		if(sym)
			commit(sym, lp);
		if(!ep || !*ep)
			break;
		lp = ep;
	}

	while(!unpack) {
		if(offset) {
			cp = "";
			--offset;
		}
		else
			cp = getOption(NULL);

		if(!cp)
			break;
		if(tokbuf[0])
			addString(buffer, sizeof(buffer), tokbuf);
		tokbuf[0] = pack;
		tokbuf[1] = 0;
		if(!stricmp(line->cmd, "pack.struct")) {
			tokbuf[0] = ';';
			addString(buffer, sizeof(buffer), cp + 1);
			addString(buffer, sizeof(buffer), ":");
			lp = buffer + strlen(buffer);
		}
		else
			lp = NULL;
		if(q)
			addString(buffer, sizeof(buffer), q);
		cp = getContent(cp);
		if(cp)
			addString(buffer, sizeof(buffer), cp);
		while(lp && *lp) {
			lp = strchr(lp, ';');
			if(lp)
				*(lp++) = ',';
		}
		if(q)
			addString(buffer, sizeof(buffer), q);
	}

	if(!unpack) {
		if(suffix)
			addString(buffer, sizeof(buffer), suffix);

		if(!commit(sym, buffer)) {
			error("symbol-not-packable");
			return true;
		}
	}

	skip();
	return true;
}

bool ScriptMethods::scrClear(void)
{
	const char *cp;
	Symbol *sym;
	const char *err = NULL;
	ScriptInterp *locker;
	unsigned long lockseq;

	while(NULL != (cp = getOption(NULL))) {
		sym = mapSymbol(cp, 0);
		if(!sym) {
			err = "clear-symbol-undefined";
			continue;
		}
		if(sym->type == symLOCK) {
			locker = NULL;
			cp = strchr(sym->data, ':');
			if(cp)
				locker = getInterp(++cp);
			lockseq = atol(sym->data);
			if(locker && locker != this && lockseq == locker->getSequence()) {
				err = "clear-symbol-locked";
				continue;
			}
		}
		clear(sym);
	}
	if(err)
		error(err);
	else
		skip();
	return true;
}

bool ScriptMethods::scrType(void)
{
	Symbol *tsym, temp, *sym;
	const char *cp = getOption(NULL);

	tsym = mapSymbol(cp, 0);
	if(!tsym) {
		error("symtype-missing");
		return true;
	}

	while(NULL != (cp = getOption(NULL))) {
		sym = mapSymbol(cp, tsym->size);
		if(!sym)
			continue;

		if(sym->type != symINITIAL && sym->type != tsym->type)
			continue;

		if(sym->size != tsym->size)
			continue;

		temp.id = sym->id;
		temp.next = sym->next;

		memcpy(sym, tsym, sizeof(Symbol) + tsym->size);
		sym->id = temp.id;
		sym->next = temp.next;
	}

	advance();
	return true;
}

bool ScriptMethods::scrDefine(void)
{
	char var[128];
	char base[65];
	Symbol *sym;
	const char *cp, *pv;
	const char *prefix = getMember();
	unsigned len;
	char *ep;
	Line *line = getLine();
	Name *scr = getName();
	unsigned idx = 0, vlen;

	if(!prefix && !frame[stack].local) {
		setString(base, sizeof(base), scr->name);
		ep = strchr(base, ':');
		if(ep)
			*ep = 0;
		prefix = base;
	}

	while(idx < line->argc) {
		pv = NULL;
		cp = line->args[idx++];
		if(*cp == '=') {
			pv = line->args[idx++];
			++cp;
			if(*pv == '{')
				++pv;
		}

		if(prefix && !strchr(cp, '.') && *cp != '%' && *cp != '&')
	                snprintf(var, sizeof(var), "%s.%s", prefix, cp);
		else
			setString(var, sizeof(var), cp);

		ep = strrchr(var, ':');
		if(ep) {
			*(ep++) = 0;
			len = atoi(ep);
		}
		else if(pv)
			len = 0;
		else
			len = symsize;

		if(!pv)
			pv = "";

		if(!len)
			vlen = (unsigned)strlen(pv);
		else
			vlen = len;

		if(!vlen)
			++vlen;

		sym = mapSymbol(var, vlen);
		if(!sym || sym->type != symINITIAL)
			continue;

		setString(sym->data, sym->size + 1, pv);
		if(len)
			sym->type = symNORMAL;
		else
			sym->type = symCONST;

	}
	advance();
	return true;
}

bool ScriptMethods::scrVar(void)
{
	unsigned short size = symsize;
	const char *cp = getMember();
	Symbol *sym;
	char *errmsg = NULL;
	Line *line = getLine();
	const char *value = getKeyword("value");

	if(!stricmp(line->cmd, "char") || !stricmp(line->cmd, "bool"))
		size = 1;

	if(cp)
		size = atoi(cp);

	cp = getKeyword("size");
	if(cp)
		size = atoi(cp);

	while(NULL != (cp = getOption(NULL))) {
		sym = mapSymbol(cp, size);
		if(!sym) {
			errmsg = "var-symbol-invalid";
			continue;
		}

		if(sym->type != symINITIAL || sym->size != size) {
			errmsg = "var-already-defined";
			continue;
		}
		if(!stricmp(line->cmd, "bool")) {
			sym->type = symBOOL;
			if(!value)
				commit(sym, "false");
		}
		if(value)
			commit(sym, value);
	}

	if(errmsg)
		error(errmsg);
	else
		skip();

	return true;
}

bool ScriptMethods::scrExpr(void)
{
	unsigned prec = 0;
	Symbol *sym;
	const char *mem = getMember();
	const char *opt;
	ScriptProperty *prop = NULL;
	long iNumber, hNumber, lNumber;
	char result[20];
	char presult[65];
	char fmt[13];
	unsigned len;
	Array *a;

	if(mem) {
		prop = ScriptProperty::find(mem);
		prec = atoi(mem);
		if(prop)
			prec = prop->prec();
	}

	while(NULL != (opt = getOption(NULL))) {
		if(!stricmp(opt, "-eq"))
			break;

		if(*opt == '%' || *opt == '&' || *opt == '@') {
			sym = mapSymbol(opt, 0);
			if(!sym) {
				error("expr-sym-undefined");
				return true;
			}
			if(sym->type == symNUMBER && !prec && !prop && sym->size > 11)
				prec = sym->size - 12;
			continue;
		}

		error("expr-invalid");
		return true;
	}
	if(!opt) {
		error("expr-missing");
		return true;
	}

	if(!prec)
		prec = frame[stack].decimal;

	if(numericExpression(&iNumber, 1, prec, prop) != 1) {
		error("expr-invalid");
		return true;
	}

	snprintf(fmt, sizeof(fmt), "%s%d%s", "%ld.%0", prec, "ld");
	frame[stack].index = 0;
	hNumber = iNumber / tens[prec];
	lNumber = iNumber % tens[prec];
	if(lNumber < 0)
		lNumber = -lNumber;
	if(prec)
		snprintf(result, sizeof(result), fmt, hNumber, lNumber);
	else
		snprintf(result, sizeof(result), "%ld", iNumber);

	while(NULL != (opt = getOption(NULL))) {
		if(!stricmp(opt, "-eq"))
			break;

		sym = mapSymbol(opt, 0);

		if(!sym)
			continue;

		if(sym->type == symNUMBER || !prop) {
			if(!commit(sym, result)) {
				error("expr-cannot-assign");
				return true;
			}
			continue;
		}

		a = (Array *)&sym->data;
		switch(sym->type) {
		case symINITIAL:
			sym->type = symNORMAL;
		case symNORMAL:
		case symPROPERTY:
			prop->setValue(sym->data, sym->size, iNumber);
			break;
		case symARRAY:
		case symSTACK:
		case symFIFO:
			len = a->rec;
			if(len >= sizeof(presult))
				len = sizeof(presult) - 1;
			prop->setValue(presult, len, iNumber);
			commit(sym, presult);
		default:
			error("expr-cannot-assign");
			return true;
		}
	}
	skip();
	return true;
}

bool ScriptMethods::scrIndex(void)
{
	Symbol *sym;
	const char *opt;
	long iNumber;

	while(NULL != (opt = getOption(NULL))) {
		if(!stricmp(opt, "-eq"))
			break;

		if(*opt == '%' || *opt == '&' || *opt == '@') {
			sym = mapSymbol(opt, 0);
			if(!sym) {
				error("index-sym-undefined");
				return true;
			}
			switch(sym->type) {
			case symARRAY:
			case symFIFO:
			case symSTACK:
				break;
			default:
				error("index-not-array");
				return true;
			}
			continue;
		}

		error("index-invalid");
		return true;
	}
	if(!opt) {
		error("index-expr-missing");
		return true;
	}

	if(numericExpression(&iNumber, 1, 0, NULL) != 1) {
		error("index-invalid");
		return true;
	}

	while(NULL != (opt = getOption(NULL))) {
		if(!stricmp(opt, "-eq"))
			break;

		sym = mapSymbol(opt, 0);

		if(!sym)
			continue;

		if(iNumber > 0)
			--iNumber;

		symindex(sym, (short)iNumber);
	}
	skip();
	return true;
}

bool ScriptMethods::scrNumber(void)
{
	unsigned short size = 11;
	const char *cp = getMember();
	Symbol *sym;
	char *errmsg = NULL;
	Line *line = getLine();

	if(!strnicmp(line->cmd, "num", 3) && frame[stack].decimal)
		size = 12 + frame[stack].decimal;

	if(cp)
		size = 12 + atoi(cp);

	cp = getKeyword("decimal");
	if(cp)
		size = 12 + atoi(cp);

	while(NULL != (cp = getOption(NULL))) {
		sym = mapSymbol(cp, size);
		if(!sym) {
			errmsg = "var-symbol-invalid";
			continue;
		}

		if(sym->type != symINITIAL || sym->size != size) {
			errmsg = "var-already-defined";
			continue;
		}

		sym->type = symNUMBER;
		clear(sym);
	}

	if(errmsg)
		error(errmsg);
	else
		skip();

	return true;
}


bool ScriptMethods::scrRef(void)
{
	const char *target = getOption(NULL);
	Symbol *sym = getSymbol();
	ScriptSymbols *syms;

	if(!target) {
		error("ref-target-unknown");
		return true;
	}

	if(!sym) {
		error("ref-missing-source");
		return true;
	}

	syms = getSymbols(sym->id);
	if(syms != dynamic_cast<ScriptSymbols*>(this) && syms != frame[stack].local) {
		error("ref-invalid-source");
		return true;
	}

	if(strchr(target, '.')) {
		error("ref-invalid-target");
		return true;
	}

	if(frame[stack].local)
		syms = frame[stack].local;
	else
		syms = dynamic_cast<ScriptSymbols*>(this);

	if(!syms->setReference(target, sym))
		error("ref-failed");
	else
		skip();

	return true;
}

bool ScriptMethods::scrReturn(void)
{
	Line *line = getLine();
	const char *label = getOption(NULL), *var;
	char *ext;
	char namebuf[256];
	unsigned len = symsize;
	unsigned argc = 0;
	Name *scr;
	char *n;

	if(label && *label != '@' && *label != '{')
		label = getContent(label);

	tempidx = 0;
	while(argc < line->argc) {
		if(*line->args[argc++] != '=')
			continue;

		snprintf(temps[tempidx], symsize + 1,
			"%s", getContent(line->args[argc]));

//		line->args[argc] = temps[tempidx];
		if(tempidx++ >= SCRIPT_TEMP_SPACE)
			tempidx = 0;
		++argc;
	}

	while(stack) {
		if(frame[stack - 1].local == frame[stack].local &&
			frame[stack - 1].script == frame[stack].script)
			pull();
		else
			break;
	}

	if(stack)
		pull();
	else {
		error("return-failed");
		return true;
	}

	argc = 0;
	tempidx = 0;
	while(argc < line->argc) {
		var = line->args[argc++];
		if(*var != '=')
			continue;

		++argc;
		if(*(++var) == '%')
			++var;
		else if(*var == '.')
		{
			snprintf(namebuf, sizeof(namebuf), "%s", frame[stack].script->name);
			n = strchr(namebuf, ':');
			if(n)
				*n = 0;
			snprintf(namebuf + strlen(namebuf), sizeof(namebuf) - strlen(namebuf), "%s", var);
			var = namebuf;
		}
		ext = temps[tempidx++];
		if(tempidx >= SCRIPT_TEMP_SPACE)
			tempidx = 0;
		setSymbol(var, ext, len);
	}

retry:
	if(!label) {
		skip();
		return true;
	}

	if(!*label) {
		skip();
		return true;
	}
	if(*label == '@' || *label == '{') {
		if(scriptEvent(label + 1))
			return true;
	}
	if(*label == '^') {
		if(!signal(++label)) {
			error("trap-invalid");
			return true;
		}
		return true;
	}
	len = (unsigned)strlen(label);
	if(!strncmp(label, "::", 2)) {
		setString(namebuf, sizeof(namebuf), frame[stack].script->name);
		ext = strstr(namebuf, "::");
		if(ext)
			setString(ext, sizeof(namebuf) + namebuf - ext, label);
		else
			addString(namebuf, sizeof(namebuf), label);
		label = namebuf;
	}
	else if(label[len - 1] == ':')
	{
		setString(namebuf, sizeof(namebuf), frame[stack].script->name);
		ext = strstr(namebuf, "::");
		if(ext)
			*ext = 0;

		addString(namebuf, sizeof(namebuf), "::");
		addString(namebuf, sizeof(namebuf), label);

		label = ext = namebuf;
		len = (unsigned)strlen(label);
		ext[len - 1] = 0;
	}

	scr = getScript(label);
	if(!scr) {
		label = getValue(NULL);
		if(label)
			goto retry;
		error("script-not-found");
		return true;
	}
	clearStack();
	frame[stack].caseflag = frame[stack].tranflag = false;
	frame[stack].script = scr;
	frame[stack].line = frame[stack].first = scr->first;
	frame[stack].index = 0;
	frame[stack].mask = getMask();
	return true;
}

bool ScriptMethods::scrCall(void)
{
	Line *line = getLine();
	const char *cmd = line->cmd;
	const char *cp, *vp;
	unsigned idx = 0;
	Symbol *sym;
	unsigned argc = 0;
	char argn[8];

	if(!push()) {
		error("stack-overflow");
		return true;
	}

	frame[stack].tranflag = false;
	frame[stack].index = 0;
	frame[stack].caseflag = false;

	if(!strnicmp(cmd, "source", 6)) {
		frame[stack].tranflag = true;
		return redirect(false);
	}

	frame[stack].local = new ScriptSymbols();

	if(!strnicmp(cmd, "call", 4))
		frame[stack].tranflag = true;
	else if(!strnicmp(cmd, "gosub", 5))
		frame[stack].base = stack;

	while(idx < line->argc) {
		cp = line->args[idx++];
		if(*cp != '=') {
			if(!argc) {
				++argc;
				continue;
			}
			snprintf(argn, sizeof(argn), "%d", argc++);
			if(*cp == '&') {
				vp = cp;
				cp = argn;
				goto byref;
			}
			--stack;
			cp = getContent(cp);
			++stack;
			setConst(argn, cp);
			continue;
		}

		++cp;
		vp = line->args[idx++];
		if(*vp == '&') {
byref:
			--stack;
			sym = mapSymbol(vp);
			++stack;
 			frame[stack].local->setReference(cp, sym);
			continue;
		}
		--stack;
		vp = getKeyword(cp);
		++stack;
		setConst(cp, vp);
	}
	snprintf(argn, sizeof(argn), "%d", --argc);
	setConst("_", argn);
	return redirect(false);
}

bool ScriptMethods::scrBegin(void)
{
	if(frame[stack].tranflag) {
		error("begin-already-in-transaction");
		return true;
	}

	frame[stack].tranflag = true;
	skip();
	return true;
}

bool ScriptMethods::scrEnd(void)
{
	if(!frame[stack].tranflag) {
		error("end-not-in-transaction");
		return true;
	}
	frame[stack].tranflag = false;
	skip();
	return true;
}

bool ScriptMethods::scrCase(void)
{
	unsigned short loop = 0xfffe;
	Line	*line;

	if(!frame[stack].caseflag)
		if(conditional() || !frame[stack].line->argc) {
			frame[stack].caseflag = true;
			skip();
			while(frame[stack].line) {
				if(frame[stack].line->scr.method == (Method)&ScriptMethods::scrCase)
					skip();
				else
					return true;
			}
			return true;
		}

	if(stack && frame[stack].line->loop)
		loop = frame[stack - 1].line->loop;

	skip();
	while(NULL != (line = frame[stack].line)) {
		if(line->loop == loop)
			return true;

		if(line->scr.method == (Method)&ScriptMethods::scrCase && !frame[stack].caseflag)
			return true;

		if(line->scr.method == (Method)&ScriptMethods::scrEndcase)
			return true;

		skip();
	}
	return true;
}

bool ScriptMethods::scrEndcase(void)
{
	frame[stack].caseflag = frame[stack].tranflag = false;
	skip();
	return true;
}

bool ScriptMethods::scrOffset(void)
{
	long offset;
	Method method;
	Line *line;

	numericExpression(&offset, 1, 0);
	--offset;
	if(!stack) {
		error("stack-underflow");
		return true;
	}

	line = frame[stack - 1].line;
	method = line->scr.method;

	if(method != (Method)&ScriptMethods::scrForeach &&
	   method != (Method)&ScriptMethods::scrFor)
	{
		error("offset-not-indexed-loop");
		return true;
	}
	--stack;
	if(offset < 0) {
		if((unsigned)(-offset) >= frame[stack].index)
			frame[stack].index = 1;
		else
			frame[stack].index += (unsigned short)offset;
	}
	else
		frame[stack].index += (unsigned short)offset;
	updated = false;
	return execute(frame[stack].line->scr.method);
}

bool ScriptMethods::scrRepeat(void)
{
	unsigned short loop = frame[stack].line->loop;
	Line *line;
	int index = frame[stack].index;
	long count;

	frame[stack].index = 0;
	numericExpression(&count, 1, 0);

	if(index >= count) {
		line = frame[stack].line->next;
		while(line) {
			if(line->loop == loop) {
				frame[stack].line = line;
				skip();
				return true;
			}
			line = line->next;
		}
		error("loop-overflow");
		return true;
	}

	frame[stack].index = ++index;
	if(!push())
		return true;

	skip();
	return true;
}

bool ScriptMethods::scrFor(void)
{
	Symbol *sym;
	unsigned short loop = frame[stack].line->loop;
	Line *line;
	int index = frame[stack].index;
	unsigned size = symsize;
	const char *opt;
	const char *kw = getKeyword("size");
	char buffer[12];

	if(kw)
		size = atoi(kw);

	frame[stack].index = 0;
	opt = getOption(NULL);

	if(!index) {
		kw = getKeyword("index");
		if(kw)
			index = atoi(kw);
	}

	if(!index)
		++index;

	sym = getKeysymbol("index");
	if(sym) {
		snprintf(buffer, sizeof(buffer), "%d", index);
		commit(sym, buffer);
	}

	sym = mapSymbol(opt, size);

	if(!sym) {
		error("symbol-not-found");
		return true;
	}

	frame[stack].index = index;
	opt = getValue(NULL);

	if(!opt) {
failed:
		line = frame[stack].line->next;
		while(line) {
			if(line->loop == loop) {
				frame[stack].line = line;
				skip();
				return true;
			}
			line = line->next;
		}
		error("loop-overflow");
		return true;
	}

	if(!push())
		goto failed;

	if(commit(sym, opt))
		skip();
	else
		error("for-cannot-set");

	return true;
}

bool ScriptMethods::scrForeach(void)
{
	Symbol *sym, *src;
	unsigned short loop = frame[stack].line->loop;
	Line *line;
	int index = frame[stack].index;
	unsigned size = symsize;
	const char *opt, *val;
	char pack = getPackToken();
	const char *kw = getKeyword("size");
	char buffer[1024];
	char *cp;
	Array *a;
	ScriptProperty *p;

	if(kw)
		size = atoi(kw);

	kw = getKeyword("token");
	if(kw && *kw)
		pack = *kw;

	frame[stack].index = 0;
	opt = getOption(NULL);
	val = getOption(NULL);

	if(!index) {
		kw = getKeyoption("index");
		if(kw)
			kw = getSymContent(kw);
		if(kw)
			index = atoi(kw);
	}

	if(!index)
		++index;

	sym = getKeysymbol("index");
	if(sym) {
		snprintf(buffer, 11, "%d", index);
		commit(sym, buffer);
	}

	sym = mapSymbol(opt, size);
	src = mapSymbol(val, 0);

	if(!sym || !src) {
		error("symbol-not-found");
		return true;
	}

	frame[stack].index = index;
	++frame[stack].index;

	opt = NULL;
	switch(src->type) {
	case symARRAY:
		if(!symindex(src, --index))
			goto failed;
	case symFIFO:
	case symSTACK:
		a = (Array *)&src->data;
		if(a->head == a->tail)
			goto failed;
		opt = extract(src);
	default:
		break;
	case symPROPERTY:
		memcpy(&p, &sym->data, sizeof(p));
		if(p->token())
			pack = p->token();
	case symNORMAL:
	case symNUMBER:
		opt = extract(src);
		while(opt && --index) {
			opt = strchr(opt, pack);
			if(opt && *opt == pack)
				++opt;
		}
		if(opt) {
			snprintf(buffer, sizeof(buffer), "%s", opt);
			opt = buffer;
			cp = (char *)strchr(opt, pack);
			if(cp)
				*cp = 0;
		}
	}

	if(!opt) {
failed:
		line = frame[stack].line->next;
		while(line) {
			if(line->loop == loop) {
				frame[stack].line = line;
				skip();
				return true;
			}
			line = line->next;
		}
		error("loop-overflow");
		return true;
	}

	if(!push())
		goto failed;

	if(commit(sym, opt))
		skip();
	else
		error("for-cannot-set");

	return true;
}

bool ScriptMethods::scrDo(void)
{
	unsigned short loop = frame[stack].line->loop;
	Line *line;

	frame[stack].index = 0;	// always reset

	if(frame[stack].line->argc) {
		if(!conditional()) {
			line = frame[stack].line->next;
			while(line) {
				if(line->loop == loop) {
					frame[stack].line = line;
					skip();
					return true;
				}
				line = line->next;
			}
			error("loop-overflow");
			return true;
		}
	}

	if(!push())
		return true;

	skip();
	return true;
}

bool ScriptMethods::scrLoop(void)
{
	unsigned short loop;

	if(stack < 1) {
		error("stack-underflow");
		return true;
	}

	loop = frame[stack - 1].line->loop;
	if(!loop) {
		error("stack-not-loop");
		return true;
	}

	if(frame[stack].line->argc) {
		if(!conditional()) {
			frame[stack - 1] = frame[stack];
			--stack;
			skip();
			return true;
		}
	}

	--stack;
	updated = false;
	return execute(frame[stack].line->scr.method);
}

bool ScriptMethods::scrContinue(void)
{
	Line *line;
	unsigned short loop;

	if(frame[stack].line->argc) {
		if(!conditional()) {
			skip();
			return true;
		}
	}

	if(stack < 1) {
		error("stack-underflow");
		return true;
	}

	loop = frame[stack - 1].line->loop;
	line = frame[stack].line->next;

	if(!loop) {
		error("stack-not-loop");
		return true;
	}

	while(line) {
		if(line->loop == loop) {
			frame[stack].line = line;
			return true;
		}
		line = line->next;
	}
	error("loop-overflow");
	return true;
}

bool ScriptMethods::scrBreak(void)
{
	Line *line;
	unsigned short loop;

	if(frame[stack].line->argc) {
		if(!conditional()) {
			skip();
			return true;
		}
	}

	if(stack < 1) {
		error("stack-underflow");
		return true;
	}

	loop = frame[stack - 1].line->loop;
	line = frame[stack].line->next;

	if(!loop) {
		error("stack-not-loop");
		return true;
	}

	while(line) {
		if(line->loop == loop) {
			--stack;
			frame[stack].line = line;
			skip();
			return true;
		}
		line = line->next;
	}
	error("loop-overflow");
	return true;
}
