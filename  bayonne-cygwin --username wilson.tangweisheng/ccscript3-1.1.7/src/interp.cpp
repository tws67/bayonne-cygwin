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

ScriptInterp::ScriptInterp() :
Mutex(), ScriptSymbols()
{
	stack = 0;
	cmd = NULL;
	image = NULL;
	memset(temps, 0, sizeof(temps));
	tempidx = 0;
	session = this;
	thread = NULL;
	trace = false;
	lock = NULL;
	sequence = 0;

	setString(logname, sizeof(logname), "ccscript");
}

unsigned ScriptInterp::getTempSize(void)
{
	return symsize + 1;
}

char *ScriptInterp::getTemp(void)
{
	char *tmp = temps[tempidx++];

	if(tempidx >= SCRIPT_TEMP_SPACE)
		tempidx = 0;

	return tmp;
}

ScriptInterp *ScriptInterp::getInterp(const char *id)
{
	if(!atoi(id))
		return this;

	return NULL;
}

unsigned ScriptInterp::getId(void)
{
	return 0;
}

bool ScriptInterp::isLocked(const char *id)
{
	if(!strnicmp(id, "script.", 7) && initialized)
		return true;

	if(!strnicmp(id, "initial.", 8) && initialized)
		return true;

	return false;
}

void ScriptInterp::setFrame(void)
{
	frame[stack].index = 0;
	updated = false;
}

const char *ScriptInterp::remapLocal(void)
{
	return "script";
}

const char *ScriptInterp::getExternal(const char *opt)
{
	char *cp, *p;
	Line *line;
	unsigned idx;

	if(!cmd)
		return NULL;

	if(!stricmp(opt, "script.id")) {
		cp = getTemp();
		snprintf(cp, symsize, "%d", getId());
		return cp;
	}
	else if(!stricmp(opt, "script.index"))
	{
		if(!stack)
			return "";
		line = frame[stack - 1].line;
		idx = frame[stack - 1].index;
		cp = getTemp();
		snprintf(cp, symsize, "%u", idx - 1);
		return cp;
	}
	else if(!stricmp(opt, "script.basename"))
	{
		cp = getTemp();
		setString(cp, symsize, frame[stack].script->name);
		p = strstr(cp, "::");
		if(p)
			*p = 0;
		return cp;
	}
	else if(!stricmp(opt, "script.subname"))
	{
		cp = getTemp();
		setString(cp, symsize, frame[stack].script->name);
		p = strstr(cp, "::");
		if(p)
			return p + 2;
		return cp;
	}
	else if(!stricmp(opt, "script.name"))
		return frame[stack].script->name;
	else if(!stricmp(opt, "script.stack"))
	{
		cp = getTemp();
		snprintf(cp, symsize, "%d", stack);
		return cp;
	}
	else if(!stricmp(opt, "script.base"))
	{
		cp = getTemp();
		snprintf(cp, symsize, "%d", frame[stack].base);
	}
	return cmd->getExternal(opt);
}

void ScriptInterp::initialize(void)
{
}

bool ScriptInterp::execute(Method method)
{
	return (this->*(method))();
}

void ScriptInterp::branching(void)
{
}

Script::Name *ScriptInterp::getScript(const char *name)
{
	if(!image)
		return NULL;

	Name *scr = image->getScript(name);

	return scr;
}

void ScriptInterp::release(void)
{
	if(lock) {
		lock->leaveMutex();
		lock = NULL;
	}
}

ScriptSymbols *ScriptInterp::getSymbols(const char *id)
{
	if(strchr(id, '.') && session != this) {
		if(lock)
			lock->leaveMutex();
		session->enterMutex();
		lock = dynamic_cast<Mutex *>(session);
	}
	else
		release();

	if(strchr(id, '.'))
		return dynamic_cast<ScriptSymbols*>(session);
	if(!frame[stack].local)
		return dynamic_cast<ScriptSymbols*>(this);

	return frame[stack].local;
}

ScriptSymbols *ScriptInterp::getLocal(void)
{
	if(frame[stack].local)
		return frame[stack].local;

	return dynamic_cast<ScriptSymbols*>(this);
}

bool ScriptInterp::setConst(const char *id, const char *value)
{
	MutexLock lock(*this);
	Symbol *sym;
	unsigned short len;

	if(!value)
		return false;

	len = (unsigned short)strlen(value);

	if(!len)
		++len;

	sym = mapSymbol(id, len);

	if(!sym)
		return false;

	if(sym->type != symINITIAL)
		return false;

	sym->type = symCONST;
	setString(sym->data, sym->size + 1, value);
	return true;
}

char ScriptInterp::getPackToken(void)
{
	const char *sym = extract(mapSymbol("script.token"));
	if(!sym)
		sym = ",";

	if(!*sym)
		sym = ",";

	return *sym;
}

Script::Symbol *ScriptInterp::mapSymbol(const char *id, unsigned short size)
{
	Symbol *sym;

	if(*id != '@')
		return mapDirect(id, size);

	sym = mapDirect(++id);
	if(!sym)
		return NULL;

	id = extract(sym);
	if(!id)
		return NULL;

	if(!*id)
		return NULL;

	return mapDirect(id, size);
}

Script::Symbol *ScriptInterp::mapDirect(const char *id, unsigned short size)
{
	Symbol *sym;
	ScriptSymbols *syms;
	unsigned count = 1;
	char partial[70];
	char *cp;
	const char *p;

	if(!id)
		return NULL;

	if(*id == '%' || *id == '&')
		++id;

	if(*id == '.' && frame[stack].script) {
		cp = (char *)strchr(frame[stack].script->filename, '.');
		if(cp && !stricmp(cp, ".mod"))
			setString(partial, sizeof(partial), "mod.");
		else
			setString(partial, sizeof(partial), "scr.");
		addString(partial, sizeof(partial), frame[stack].script->name);
 		cp = (char *)strstr("::", partial);
		if(cp)
			*cp = 0;
		addString(partial, sizeof(partial), id);
		id = partial;
	}

retry:
	if(!isalnum(*id) && *id != '_') {
		logmissing(id, "invalid");
		return NULL;
	}

	while(count < 64) {
		if(!id[count])
			break;

		if(!strchr("abcdefghijklmnopqrstuvwxyz01234567890._", tolower(id[count]))) {
			logmissing(id, "invalid");
			return NULL;
		}
		++count;
	}

	if(count == 64) {
		logmissing(id, "invalid");
		return NULL;
	}

	if(size && isLocked(id))
		size = 0;

	syms = getSymbols(id);
	if(!syms)
		return NULL;

	sym = deref(syms->find(id, size));
	if(!sym && !strchr(id, '.')) {
		p = remapLocal();
		if(!p)
			return NULL;

		snprintf(partial, sizeof(partial), "%s.%s", p, id);
		id = partial;
		goto retry;
	}
	return sym;
}

bool ScriptInterp::setNumber(const char *id, const char *value, unsigned dec)
{
	Symbol *sym;

	if(!dec)
		dec = 11;
	else
		dec += 12;

	sym = mapSymbol(id, dec);
	if(!sym)
		return false;

	if(!value)
		return true;

	if(sym->type == symINITIAL)
		sym->type = symNUMBER;

	return commit(sym, value);
}

bool ScriptInterp::setSymbol(const char *id, const char *value, unsigned short size)
{
	Symbol *sym;

	if(!size)
		size = symsize;

	sym = mapSymbol(id, size);

	if(!sym)
		return false;

	if(!value)
		return true;

	return commit(sym, value);
}

void ScriptInterp::trap(const char *trapid)
{
	unsigned trap = cmd->getTrapId(trapid);
	if(!trap) {
		if(!image)
			return;

		if(!stricmp(trapid, "first") || !stricmp(trapid, "top")) {
			frame[stack].tranflag = frame[stack].caseflag = false;
			frame[stack].line = frame[stack].first;
			return;
		}
	}
	ScriptInterp::trap(trap);
}

void ScriptInterp::trap(unsigned id)
{
	Line *trap = NULL;
	unsigned base = frame[stack].base;

	if(!image)
		return;

	// we can inherit traps at lower levels

	for(;;)
	{
		trap = frame[stack].script->trap[id];
		if(trap == frame[stack].first) {
			advance();
			return;
		}
		if(!trap && !cmd->isInherited(id)) {
			advance();
			return;
		}
		if(trap || stack == base)
			break;

		pull();
	}

	// when doing a trap, always unwind loop or recursive stack frames

	clearStack();
	frame[stack].tranflag = frame[stack].caseflag = false;
	frame[stack].line = frame[stack].first = trap;
	if(!id) {
		if(!trap)
			redirect("::exit");
		exiting = true;
	}
}

bool ScriptInterp::pull(void)
{
	if(!stack) {
		error("stack-underflow");
		return false;
	}

	if(frame[stack].local && frame[stack - 1].local != frame[stack].local)
		delete frame[stack].local;

	--stack;
	return true;
}

bool ScriptInterp::push(void)
{
	if(stack >= (SCRIPT_STACK_SIZE - 1)) {
		error("stack-overflow");
		return false;
	}

	frame[stack + 1] = frame[stack];
	frame[stack + 1].caseflag = frame[stack + 1].tranflag = false;
	++stack;
	return true;
}

void ScriptInterp::clearStack(void)
{
	unsigned indexes[SCRIPT_STACK_SIZE];
	unsigned idx = 0, len = 0;
	char values[SCRIPT_STACK_SIZE * 6];

	while(stack) {
		if(frame[stack - 1].script != frame[stack].script)
			break;
		pull();
		indexes[idx++] = frame[stack].index;
	}
	snprintf(values, 3, "%d", idx);
	setSymbol("script.stack", values, 4);
	values[1] = 0;
	while(idx--) {
		snprintf(values + len, sizeof(values) - len, ",%d", indexes[idx]);
		len = (unsigned)strlen(values);
	}
	setSymbol("script.index", values + 1, 3);
}

void ScriptInterp::skip(void)
{
	frame[stack].line = frame[stack].line->next;
}

void ScriptInterp::advance(void)
{
	if(updated)
		return;

	frame[stack].line = frame[stack].line->next;
	updated = true;
}

void ScriptInterp::error(const char *errmsg)
{
	char evtname[128];

	setSymbol("script.error", errmsg);
	snprintf(evtname, sizeof(evtname), "error:%s", errmsg);
	if(scriptEvent(evtname))
		return;

	if((frame[stack].script->mask & 0x02) && frame[stack].script->trap[1]) {
		trap(1);
		return;
	}
	advance();
}

bool ScriptInterp::tryCatch(const char *id)
{
	Name *scr;
	char namebuf[160];
	char *cp;
	unsigned stk = frame[stack].base;

	setString(namebuf, sizeof(namebuf), frame[stk].script->name);
	cp = strstr(namebuf, "::");
	if(cp)
		*(cp + 2) = 0;
	else
		addString(namebuf, sizeof(namebuf), "::");
	addString(namebuf, sizeof(namebuf), id);
	scr = getScript(namebuf);
	if(!scr || !push())
		return false;

	branching();

	frame[stack].script = scr;
	frame[stack].line = frame[stack].first = scr->first;
	frame[stack].caseflag = frame[stack].tranflag = 0;
	frame[stack].index = 0;
	frame[stack].mask = getMask();
	image->fastBranch(this);
	return true;
}

void ScriptInterp::gotoEvent(NamedEvent *evt)
{
	clearStack();
	branching();
	frame[stack].tranflag = frame[stack].caseflag = false;
	frame[stack].line = frame[stack].first = evt->line;
	image->fastBranch(this);
}

bool ScriptInterp::scriptEvent(const char *name, bool inhereted)
{
	char evtname[128];
	const char *savname = name;
	NamedEvent *evt, *top = frame[stack].script->events;
	unsigned base = frame[stack].base;
	const char *chkname = name;
	unsigned current = stack;
	bool found = false;
#ifdef	HAVE_REGEX_H
	regex_t *regex;
#endif

retry:
	evt = frame[current].script->events;
	while(evt) {
		switch(evt->type) {
		case '@':
			if(!stricmp(evt->name, chkname))
				found = true;
			break;
#ifdef	HAVE_REGEX_H
		case '~':
			regex = new regex_t;
					memset(regex, 0, sizeof(regex_t));

					if(!regcomp(regex, evt->name, REG_ICASE|REG_NOSUB|REG_NEWLINE))
				if(!regexec(regex, chkname, 0, NULL, 0))
					found = true;

			regfree(regex);
			delete regex;
			break;
#endif
		}

		if(found)
			break;

		evt = evt->next;
	}

	if(!evt && NULL != (chkname = strchr(chkname, ':'))) {
		++chkname;
		goto retry;
	}

	if(evt) {
		while(stack > current)
			pull();

		gotoEvent(evt);
		return true;
	}


	while(current > base && frame[current].script->events == top && inhereted)
		--current;

	if(frame[current].script->events != top) {
		top = frame[current].script->events;
		chkname = name;
		goto retry;
	}

	if(*savname == '@')
		++savname;
	snprintf(evtname, sizeof(evtname), "-catch-%s", savname);

	if(tryCatch(evtname))
		return true;

	return false;
}

void ScriptInterp::initRuntime(Name *scr)
{
	MutexLock lock(*this);

	while(stack)
		pull();
	frame[stack].script = scr;
	frame[stack].line = frame[stack].first = frame[stack].script->first;
	frame[stack].index = 0;
	frame[stack].caseflag = frame[stack].tranflag = false;
	frame[stack].decimal = 0;
	frame[stack].base = 0;
	frame[stack].mask = frame[stack].script->mask;
}

bool ScriptInterp::attach(ScriptCommand *cmdref, const char *scrname)
{
	Name *scr;
	char msg[65];

	cmd = cmdref;
	enterMutex();

	purge();

	cmd->enterMutex();
	image = cmd->active;

	if(!image) {
		cmd->leaveMutex();
		leaveMutex();
		return false;
	}

	scr = getScript(scrname);
	if(!scr || scr->access != scrPUBLIC) {
		snprintf(msg, sizeof(msg), "%s: attach failed", scrname);
		if(!image->getLast(msg)) {
			image->setValue(msg, "missing");
			cmd->errlog("missing", msg);
		}
		cmd->leaveMutex();
		leaveMutex();
		logerror("missing; attach failed", scrname);
		snprintf(msg, sizeof(msg), "%s: attach failed", scrname);
		return false;
	}

	++image->refcount;
	cmd->leaveMutex();
	attach(cmd, image, scr);
	return true;
}

void ScriptInterp::attach(ScriptCommand *cmdref, ScriptImage *img, Name *scr)
{
	const char *scrname = scr->name;
	ScriptImage::InitialList *ilist;
	ScriptBinder *mod;
	Name *init;
	Line *line;
	stack = 0;
	cmd = cmdref;
	exiting = initialized = false;
	session = this;
	thread = NULL;
	bool selected = false;
	Symbol *sym;

	image = img;

	frame[stack].local = NULL;

	for(tempidx = 0; tempidx < SCRIPT_TEMP_SPACE; ++tempidx)
		temps[tempidx] = (char *)alloc(symsize + 1);
	tempidx = 0;

	ilist = image->ilist;
	while(ilist) {
		setSymbol(ilist->name, ilist->value, ilist->size);
		ilist = ilist->next;
	}

	sym = mapSymbol("script.authorize", 0);
	if(sym)
		sym->type = symTIMER;

	setSymbol("script.home", scrname);
	mod = ScriptBinder::first;
	while(mod) {
		mod->attach(this);
		mod = mod->next;
	}
	initialize();

	init = image->index[SCRIPT_INDEX_SIZE];
	leaveMutex();
	while(init) {
		initRuntime(init);
		while(step())
			Thread::yield();
		init = init->next;
	}

	initialized = true;

	enterMutex();
	initRuntime(scr);

	mod = ScriptBinder::first;
	while(mod && !selected) {
		selected = mod->select(this);
		mod = mod->next;
	}

	if(selected)
		goto finish;

	if(fastStart) {
		image->fastBranch(this);
		goto finish;
	}

	line = getLine();
	if(!line)
		goto finish;

	if(!stricmp(line->cmd, "options"))
		execute(line->scr.method);

finish:
	leaveMutex();
}

void ScriptInterp::detach(void)
{
	ScriptBinder *mod = ScriptBinder::first;
	char scrname[65];
	char *sp;

	++sequence;

	snprintf(scrname, sizeof(scrname), "%s", frame[0].script->name);
	sp = strchr(scrname, ':');
	if(sp)
		*sp = 0;

	if(!image)
		return;

	if(thread) {
		delete thread;
		thread = NULL;
	}

	while(mod) {
		mod->detach(this);
		mod = mod->next;
	}

	enterMutex();
	cmd->enterMutex();
	--image->refcount;

	if(image)
		if(!image->refcount && image != cmd->active)
			delete image;

	cmd->leaveMutex();
	image = NULL;

	while(stack)
		pull();

	purge();
	leaveMutex();
}

bool ScriptInterp::step(void)
{
	bool rtn = false;
	Line *next, *line;
	unsigned count = autoStepping;

	if(!image)
		return true;

	enterMutex();

	if(!frame[stack].line)
		goto exit;

trans:
	updated = false;
	frame[stack].index = 0;
	line = getLine();
	next = line->next;
	rtn = execute(line->scr.method);
	release();

	if(!rtn || !frame[stack].line)
		goto exit;

	if((frame[stack].tranflag && !trace)) {
		count = 0;
		goto trans;
	}

	if(count-- && frame[stack].line == next && !trace)
		goto trans;

exit:
	while(!frame[stack].line && stack) {
		if(frame[stack - 1].local == frame[stack].local)
			break;
		pull();
		if(frame[stack].line)
			advance();
	}

	if(!frame[stack].line) {
		if(initialized)
			exit();
		rtn = false;
	}
	else if(!rtn && thread)
	{
		release();
		startThread();
	}
	else
		release();

	leaveMutex();

	return rtn;
}

void ScriptInterp::ripple(void)
{
	char namebuf[256];
	char *label;
	Name *scr = frame[stack].script;
	Line *line;
	const char *name = getValue(NULL);

	snprintf(namebuf, sizeof(namebuf), "%s", name);

	label = strchr(namebuf, ':');
	if(!label) {
		label = namebuf;
		goto find;
	}
	*(label++) = 0;
	scr = getScript(namebuf);
	if(!scr) {
		logmissing(name, "missing", "script");
		error("label-missing");
		return;
	}

find:
	if(!label || !*label) {
		line = scr->first;
		goto done;
	}

	line = scr->first;
	while(line) {
		if(!stricmp(line->cmd, "label"))
			if(!stricmp(line->args[0], label))
				break;
		line = line->next;
	}

	if(!line) {
		logmissing(name, "missing", "script");
		error("label-missing");
		return;
	}

done:
	frame[stack].caseflag = false;
	frame[stack].script = scr;
	frame[stack].first = scr->first;
	frame[stack].line = line;
	frame[stack].index = 0;
	frame[stack].mask = scr->mask;
	updated = true;
}

bool ScriptInterp::redirect(bool evflag)
{
	char namebuf[256];

	const char *label = getValue(NULL);
	char *ext;
	size_t len;
	bool pvt = true;
	Name *scr = frame[stack].script;
	bool shortflag = true;
	unsigned base = frame[stack].base;
	bool fun = false;
	bool isfun = false;
	unsigned long mask = frame[stack].line->mask & frame[stack].mask & cmd->imask;

	if(!stricmp(frame[stack].line->cmd, "call"))
		fun = true;

	isfun = isFunction(scr);

	if(!label) {
		logmissing(label, "missing", "script");
		error("branch-failed");
		return true;
	}

	if(*label == '&')
		++label;

	if(strstr(label, "::"))
		shortflag = false;

	len = strlen(label);

retry:
	if(shortflag) {
		snprintf(namebuf, sizeof(namebuf), "%s", frame[stack].script->name);
		ext = strstr(namebuf, "::");
		if(ext)
			*ext = 0;
		len = strlen(namebuf);
		snprintf(namebuf + len, sizeof(namebuf) - len, "::%s", label);
		scr = getScript(namebuf);
		if(scr) {
			pvt = false;
			goto script;
		}
		shortflag = false;
		goto retry;

	}
	else if(!strncmp(label, "::", 2))
	{
		pvt = false;
		setString(namebuf, sizeof(namebuf), frame[stack].script->name);
		ext = strstr(namebuf, "::");
		if(ext)
			setString(ext, sizeof(namebuf) + namebuf - ext, label);
		else
			addString(namebuf, sizeof(namebuf), label);
		label = namebuf;
	}
	else if(fun || isfun)
	{
		setString(namebuf, sizeof(namebuf), frame[stack].script->name);
		ext = strstr(namebuf, "::");
		if(ext)
			setString(ext + 2, sizeof(namebuf) + namebuf - ext - 2, label);
		else {
			addString(namebuf, sizeof(namebuf), "::");
			addString(namebuf, sizeof(namebuf), label);
		}
		scr = getScript(namebuf);
		if(scr) {
			pvt = false;
			goto script;
		}
	}
	scr = getScript(label);
script:
	if(!scr) {
		logmissing(label, "missing", "script");
		error("script-not-found");
		return true;
	}
	if(pvt && isPrivate(scr)) {
		logmissing(label, "access", "script");
		error("script-private");
		return true;
	}
	if(!isfun && !fun && isFunction(scr) && scr != frame[stack].script) {
		logmissing(label, "access", "script");
		error("script-function");
		return true;
	}

	if(!isFunction(scr))
		isfun = false;

	if(isfun && evflag)
		clearStack();
	else while(evflag && stack > base)
		pull();

	frame[stack].caseflag = false;
	frame[stack].script = scr;
	frame[stack].line = frame[stack].first = scr->first;
	frame[stack].index = 0;
	if(evflag && isfun && stack) {
		mask = frame[stack - 1].line->mask & frame[stack - 1].mask & cmd->imask;
		frame[stack].mask = (mask | scr->mask);
	}
	else if(evflag)
		frame[stack].mask = getMask();
	else
		frame[stack].mask = (mask | scr->mask);

	updated = true;
	return true;
}

bool ScriptInterp::redirect(const char *scriptname)
{
	Name *scr;
	char namebuf[128];
	char *ext;

	if(!strncmp(scriptname, "::", 2)) {
		setString(namebuf, sizeof(namebuf), frame[stack].script->name);
		ext = strstr(namebuf, "::");
		if(ext)
			*ext = 0;
		addString(namebuf, sizeof(namebuf), scriptname);
	}
	else
		setString(namebuf, sizeof(namebuf), scriptname);

	scr = getScript(namebuf);
	if(scr) {
		clearStack();
		frame[stack].script = scr;
		frame[stack].line = frame[stack].first = scr->first;
		frame[stack].mask = getMask();
		return true;
	}
	else
		logmissing(namebuf, "missing", "script");
	return false;
}

unsigned long ScriptInterp::getMask(void)
{
	unsigned sp = frame[stack].base;
	unsigned long mask = 0;

	while(sp < stack) {
		mask |= (frame[sp].script->mask & frame[sp].line->mask & cmd->imask);
		++sp;
	}
	mask |= frame[stack].script->mask;
	return mask;
}

void ScriptInterp::enterThread(ScriptThread *thr)
{
}

bool ScriptInterp::eventThread(const char *evt, bool flag)
{
	if(updated)
		return false;

	if(scriptEvent(evt, flag))
		updated = true;

	return updated;
}

void ScriptInterp::exitThread(const char *msg)
{
	if(updated)
		return;

	if(msg)
		error(msg);
	else
		advance();

	updated = true;
}

void ScriptInterp::startThread(void)
{
	thread->start();
}

void ScriptInterp::waitThread(void)
{
	timeout_t timer = getTimeout();
	if(!timer)
		return;

	Thread::sleep(timer);
	enterMutex();
	delete thread;
	thread = NULL;
	if(!updated)
		error("timeout");
	updated = true;
	leaveMutex();
}

bool ScriptInterp::exit(void)
{
	if(exiting)
		return false;

	exiting = true;
	trap((unsigned)0);
	if(frame[stack].line)
		return true;
	return redirect("::exit");
}

bool ScriptInterp::signal(const char *trapname)
{
	unsigned long mask, cmask;
	Line *line = getLine();
	if(!image)
		return true;

	MutexLock lock(*this);

	cmask = mask = cmd->getTrapMask(trapname);

	mask &= line->mask;

	mask &= frame[stack].mask;

	if(frame[stack].line)
		mask &= frame[stack].line->mask;

	if(exiting)
		mask &= ~1;

	if(!mask)
		return false;

//	stop(mask);
	trap(trapname);
	branching();

	image->fastBranch(this);
	return true;
}

bool ScriptInterp::signal(unsigned id)
{
	unsigned long mask, cmask;
	if(!image)
		return true;

	if(!id && exiting)
		return false;

	MutexLock lock(*this);

	if(id >= TRAP_BITS)
		return false;

	cmask = mask = cmd->getTrapMask(id);
	mask &= frame[stack].mask;
	if(frame[stack].line)
		mask &= frame[stack].line->mask;

	if(!mask)
		return false;

//	stop(mask);

	trap(id);
	branching();

	image->fastBranch(this);
	return true;
}

const char *ScriptInterp::hasOption(void)
{
	for(;;)
	{
		if(frame[stack].index >= frame[stack].line->argc)
			return NULL;

		if(*frame[stack].line->args[frame[stack].index] != '=')
			break;

		frame[stack].index += 2;
	}
	return frame[stack].line->args[frame[stack].index];
}

const char *ScriptInterp::getOption(const char *def)
{
	register const char *cp;
	unsigned current;

get:
	for(;;)
	{
		if(frame[stack].index >= frame[stack].line->argc)
			return (char *)def;

		cp = frame[stack].line->args[frame[stack].index];

		if(stack && !stricmp(cp, "%*"))
			goto expand;

		if(*cp != '=')
			break;

		frame[stack].index += 2;
	}

	++frame[stack].index;
	return cp;

expand:
	current = stack;
	while(stack && frame[stack].local == frame[current].local)
		--stack;
	if(frame[stack].local == frame[current].local) {
		stack = current;
		goto get;
	}
	if(frame[stack].index >= frame[stack].line->argc)
		frame[stack].index = 0;
	cp = getOption(NULL);
	if(!cp || frame[stack].index >= frame[stack].line->argc)
		++frame[current].index;
	stack = current;
	if(!cp)
		goto get;
	return cp;
}

const char *ScriptInterp::getSymbol(const char *id)
{
	const char *val = getExternal(id);
	Symbol *sym;

	if(val)
		return val;

	sym = mapSymbol(id);

	if(!sym)
		return NULL;

	return extract(sym);
}

Script::Symbol *ScriptInterp::getSymbol(unsigned short size)
{
	const char *id = getOption(NULL);

	if(!id)
		return NULL;

	if(*id != '%' && *id != '&' && *id != '@')
		return NULL;

	return mapSymbol(id, size);
}

const char *ScriptInterp::getMember(void)
{
	const char *cp = strchr(frame[stack].line->cmd, '.');

	if(cp)
		++cp;

	return cp;
}

const char *ScriptInterp::getKeyoption(const char *kw)
{
	unsigned idx = 0;
	Line *line = frame[stack].line;
	const char *opt;
	while(idx < line->argc) {
		opt = line->args[idx++];
		if(*opt == '=') {
			if(!strnicmp(kw, opt + 1, strlen(kw)))
				return line->args[idx];
			++idx;
		}
	}
	return NULL;
}

const char *ScriptInterp::getKeyword(const char *kw)
{
	unsigned idx = 0;
	Line *line = frame[stack].line;
	const char *opt;
	while(idx < line->argc) {
		opt = line->args[idx++];
		if(*opt == '=') {
			if(!strnicmp(kw, opt + 1, strlen(kw)))
				return getContent(line->args[idx]);
			++idx;
		}
	}
	return NULL;
}

Script::Symbol *ScriptInterp::getKeysymbol(const char *kw, unsigned size)
{
	const char *opt = getKeyoption(kw);
	Symbol *sym;

	if(!opt)
		return NULL;

	if(*opt != '&')
		return NULL;

	sym = mapSymbol(opt, size);
	if(!sym)
		logmissing(opt);

	return deref(sym);
}

const char *ScriptInterp::getSymContent(const char *opt)
{
	Symbol *sym;

	if(!opt)
		return NULL;

	if(*opt != '&')
		return getContent(opt);

	sym = mapSymbol(++opt);
	if(sym)
		return extract(sym);

	logmissing(opt);
	return NULL;
}

const char *ScriptInterp::getContent(const char *opt)
{
	Symbol *sym;
	const char *val;
	char *tmp;
	Array *a;
	ScriptProperty *p;
	long v;

	if(!opt)
		return NULL;

	if(*opt == '%' && !opt[1])
		return opt;

	if(*opt == '{')
		return ++opt;

	if(*opt == '#') {
		tmp = getTemp();
		val = getExternal(++opt);
		if(val) {
			snprintf(tmp, 11, "%ld", (long)strlen(val));
			return tmp;
		}
		sym = mapSymbol(opt);
		if(!sym) {
			logmissing(opt);
			return NULL;
		}
		tmp = getTemp();
		a = (Array *)&sym->data;
		switch(sym->type) {
		case symBOOL:
			switch(sym->data[0]) {
			case '0':
			case 'n':
			case 'N':
			case 'f':
			case 'F':
				tmp[0] = '0';
				break;
			default:
				tmp[0] = '1';
				break;
			}
			tmp[1] = 0;
			return tmp;
		case symARRAY:
			snprintf(tmp, 11, "%d", a->tail);
			return tmp;
		case symCONST:
		case symNORMAL:
			snprintf(tmp, 11, "%ld", (long)strlen(sym->data));
			return tmp;
		case symCOUNTER:
			snprintf(tmp, 11, "%ld", atol(sym->data));
			return tmp;
		case symTIMER:
			if(!sym->data[0]) {
				setString(tmp, 11, "999999999");
				return tmp;
			}
			v = atol(extract(sym));
			if(v < 0)
				snprintf(tmp, 11, "%ld", -v);
			else
				setString(tmp, 11, "0");
			return tmp;
		case symINITIAL:
			return "0";
		case symPROPERTY:
			opt = sym->data + sizeof(ScriptProperty *);
			memcpy(&p, &sym->data, sizeof(p));
			snprintf(tmp, 11, "%ld", p->getValue(opt));
			return tmp;
	        case Script::symSTACK:
			case Script::symFIFO:
					if(a->tail >= a->head)
				snprintf(tmp, 11, "%d", a->tail - a->head);
					else
				snprintf(tmp, 11, "%d", a->count - (a->tail - a->head));
					return tmp;
		default:
			return NULL;
		}
	}

	if(*opt != '%' && *opt != '@')
		return opt;

	if(*opt != '@') {
		++opt;
		val = session->getExternal(opt);
		if(val)
			return val;
	}

	sym = mapSymbol(opt);
	if(sym)
		return extract(sym);

	logmissing(opt);
	return NULL;
}

const char *ScriptInterp::getValue(const char *def)
{
	const char *opt = getOption(NULL);

	if(!opt)
		return def;

	opt = getContent(opt);
	if(!opt)
		return def;

	return opt;
}

void ScriptInterp::logerror(const char *msg, const char *scrname)
{
	if(!scrname && frame[stack].script)
		scrname = frame[stack].script->name;

	if(scrname)
		slog.error() << logname << ": " << scrname << ": " << msg << endl;
	else
		slog.error() << logname << ": " << msg << endl;
}

void ScriptInterp::logmissing(const char *sym, const char *reason, const char *group)
{
	char msg[65];

	if(*sym == '@' || *sym == '%' || *sym == '&')
		++sym;

	if(!frame[stack].line)
		return;

	slog.warn() << logname << ": " << frame[stack].script->filename
		<< "(" << frame[stack].line->lnum << "): " << group << " "
		<< sym << " " << reason << endl;

	snprintf(msg, sizeof(msg), "%s(%d): %s %s",
		frame[stack].script->filename, frame[stack].line->lnum, group, sym);

	cmd->enterMutex();
	if(image->getLast(msg)) {
		cmd->leaveMutex();
		return;
	}

	image->setValue(msg, reason);
	if(!stricmp(reason, "undefined"))
		reason = "missing";
	cmd->errlog(reason, msg);
	cmd->leaveMutex();
}

bool ScriptInterp::done(void)
{
	if(!stack && !frame[stack].line && exiting)
		return true;

	return false;
}

bool ScriptInterp::catSymbol(const char *id, const char *value, unsigned short size)
{
	Symbol *sym;

	if(!id)
		return false;

	if(!value)
		return true;

	while(*id == '%' || *id == '&' || *id == '@')
		++id;

	if(!*id)
		return false;

	MutexLock local(*this);
	if(strchr(id, '.') && session != this)
		MutexLock sess(*session);

	sym = mapSymbol(id, size);
	if(!sym)
		return false;

	return append(sym, value);
}

bool ScriptInterp::putSymbol(const char *id, const char *value, unsigned short size)
{
	Symbol *sym;

	if(!id)
		return false;

	if(!value)
		value = "";

	while(*id == '&' || *id == '%' || *id == '@')
		++id;

	if(!*id)
		return false;

	MutexLock local(*this);

	if(strchr(id, '.') && session != this)
		MutexLock sess(*session);

	sym = mapSymbol(id, size);
	if(!sym)
		return false;

	return commit(sym, value);
}

bool ScriptInterp::getSymbol(const char *id, char *buffer, unsigned short size)
{
	Symbol *sym;
	const char *cp;

	if(!id)
		return false;

	if(!buffer)
		return false;

	while(*id == '&' || *id == '%' || *id == '@')
		++id;

	if(!*id)
		return false;

	MutexLock local(*this);

	if(strchr(id, '.') && session != this)
		MutexLock sess(*session);

	sym = mapSymbol(id);
	if(!sym)
		return false;

	*buffer = 0;
	cp = extract(sym);
	if(!cp)
		return false;

	setString(buffer, size, cp);
	return true;
}

timeout_t ScriptInterp::getTimeout(void)
{
	if(!thread)
		return 0;

	return thread->getTimeout();
}


