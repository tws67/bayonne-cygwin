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

#include "engine.h"

using namespace ost;
using namespace std;

ScriptSymbols *BayonneSession::globalSyms = NULL;
Mutex BayonneSession::globalLock;
BayonneTranslator BayonneSession::langNone("none");

BayonneSession::BayonneSession(BayonneDriver *d, timeslot_t ts, BayonneSpan *s) :
ScriptInterp(), audio()
{
	driver = d;
	msgport = d->getMsgport();
	logevents = logtrace = NULL;
	state.logstate = (Handler)NULL;
	state.pfd = PFD_INVALID;
	state.libaudio = NULL;
	span = s;
	timeslot = ts;
	timeslots[ts] = this;
	state.handler = (Handler)NULL;
	isAvail = false;
	nextIdle = prevIdle = NULL;
	seq = 0;
	evseq = 0;
	tseq = 0;
	offhook = dtmf = answered = holding = referring = false;
	starttime = 0;
	type = NONE;
	seizure = CHILD_RUNNING;
	voicelib = init_voicelib;
	translator = init_translator;
	state.menu = NULL;
	peer = NULL;
	connecting = false;
	ring = NULL;
	vm = NULL;

	if(!translator)
		translator = &langNone;

	if(ts >= ts_used)
		ts_used = ts + 1;

	snprintf(logname, sizeof(logname), "%s/%d",
		d->getName(), ts - d->getFirst());

	SLOG_DEBUG("%s: session starting", logname);
	setState(STATE_INITIAL);

	snprintf(var_timeslot, sizeof(var_timeslot), "%d", timeslot);

	strcpy(audio.var_position, "00:00:00.000");
	strcpy(var_bankid, "0");
	strcpy(var_pid, "none");
	strcpy(var_recall, "none");
	strcpy(var_joined, "none");
	time_joined = 0;
	var_sid[0] = 0;
	dtmf_digits = NULL;
	digit_count = ring_count = 0;
	iface = IF_NONE;
	bridge = BR_NONE;
	starting = false;

	strcpy(var_rings, "0");
	strcpy(var_callid, "none");

	if(span)
	{
		snprintf(var_spanid, sizeof(var_spanid), "span/%d", span->getId());
		snprintf(var_spantsid, sizeof(var_spantsid), "span/%d,%d",
			span->getId(), timeslot - span->getFirst());
	}
	else
	{
		strcpy(var_spanid, "none");
		strcpy(var_spantsid, "none");
	}

	newTid();
}

BayonneSession::~BayonneSession()
{
	finalize();
}

bool BayonneSession::signalScript(signal_t signal)
{
	if(vm)
		if(vm->signalEngine(signal))
			return true;

	if(ScriptInterp::signal(signal))
	{
		if(vm)
			vm->releaseEngine();
		return true;
	}

	return false;
}

void BayonneSession::enterCall(void)
{
	BayonneService *svc = BayonneService::first;

	BayonneBinder::binder->makeCall(this);

	while(svc)
	{
		svc->enteringCall(this);
		svc = svc->next;
	}
}

void BayonneSession::exitCall(const char *reason)
{
	BayonneService *svc = BayonneService::first;

	setString(var_joined, sizeof(var_joined), "none");
	setSymbol("session.terminated", reason);
	BayonneBinder::binder->dropCall(this);

	while(svc)
	{
		svc->exitingCall(this);
		svc = svc->next;
	}
}

void BayonneSession::incIncomingAttempts(void)
{
	incActiveCalls();

	++call_attempts.iCount;
	++total_call_attempts.iCount;
	++driver->call_attempts.iCount;
	if(span)
		++span->call_attempts.iCount;
}

void BayonneSession::decActiveCalls(void)
{
	--total_active_calls;
	--driver->active_calls;
	if(span)
		--span->active_calls;
}

void BayonneSession::incActiveCalls(void)
{
        ++total_active_calls;
        ++driver->active_calls;
        if(span)
                ++span->active_calls;
}

void BayonneSession::incOutgoingAttempts(void)
{
	incActiveCalls();

	++call_attempts.oCount;
	++total_call_attempts.oCount;
	++driver->call_attempts.oCount;
	if(span)
		++span->call_attempts.oCount;
}

void BayonneSession::incIncomingComplete(void)
{
	++call_complete.iCount;
        ++total_call_complete.iCount;
        ++driver->call_complete.iCount;
        if(span)
                ++span->call_complete.iCount;
}

void BayonneSession::incOutgoingComplete(void)
{
	++call_complete.oCount;
        ++total_call_complete.oCount;
        ++driver->call_complete.oCount;
        if(span)
                ++span->call_complete.oCount;
}

void BayonneSession::initialize(void)
{
	setSid();
}

void BayonneSession::detach(void)
{
	BayonneService *svc = BayonneService::first;

	answered = false;
	digit_count = ring_count = 0;

	disableDTMF();

	while(svc)
	{
		svc->detachSession(this);
		svc = svc->next;
	}

	ScriptInterp::detach();
	var_sid[0] = 0;
	strcpy(var_rings, "0");

	starttime = 0;
	type = NONE;
	seizure = CHILD_RUNNING;
	voicelib = init_voicelib;
	translator = init_translator;

	if(!translator)
		translator = &langNone;

	if(localimages && localimages[timeslot])
	{
		delete localimages[timeslot];
		localimages[timeslot] = NULL;
	}

	state.menu = NULL;
	strcpy(audio.var_position, "00:00:00.000");
}
	
void BayonneSession::initialevent(void)
{
	Event event;

	event.id = ENTER_STATE;
	event.name = "initial";
	event.timeslot = timeslot;
	putEvent(&event);
}

void BayonneSession::finalize(void)
{
	purge();

	SLOG_DEBUG("%s: session stopping", logname);
}

const char *BayonneSession::getDigits(void)
{
	if(!dtmf_digits)
		return "";

	return dtmf_digits;
}

const char *BayonneSession::getWritepath(char *buf, size_t len)
{
	char nbuf[128];
	const char *cp, *ep;
	const char *name = getValue(NULL);

	const char *prefix = getKeyword("prefix");
	if(!prefix || !*prefix)
		prefix = NULL;

	if(!name || !*name)
		return NULL;

	if(strchr(name, '/') || strchr(name, ':'))
	{
		cp = audio.getFilename(name, true);
		goto finish;
	}

	if(!prefix)
		return NULL;

	ep = prefix + strlen(prefix) - 1;
	if(*ep == ':' || *ep == '/')
		snprintf(nbuf, sizeof(nbuf), "%s%s", prefix, name);
	else
		snprintf(nbuf, sizeof(nbuf), "%s/%s", prefix, name);

	cp = audio.getFilename(nbuf, true);
finish:
	if(!cp)
		return NULL;

	if(!buf || !len)
		return cp;

	if(*cp == '/' || cp[1] == ':')
		setString(buf, len, cp);
	else
		snprintf(buf, len, "%s/%s", server->getLast("prefix"), cp);
	return buf;
}
	
unsigned BayonneSession::getId(void)
{
	return (unsigned)timeslot;
}

bool BayonneSession::isRefer(void)
{
        if(state.handler == &BayonneSession::stateRefer)
                return true;
        return false;
}

void BayonneSession::associate(BayonneSession *s)
{
	if(isAssociated())
		exitCall("new association");

	time(&time_joined);
	strcpy(var_pid, s->var_sid);
}

bool BayonneSession::isAssociated(void)
{
	if(time_joined)
		return true;

	if(!strcmp(var_joined, var_pid) && strcmp(var_pid, "none"))
		return true;

	return false;
}

bool BayonneSession::isJoined(void)
{
	if(state.handler == &BayonneSession::stateJoin)
		return true;
	return false;
}

bool BayonneSession::isDisconnecting(void)
{
	if(state.handler == &BayonneSession::stateIdle)
		return true;
	if(state.handler == &BayonneSession::stateHangup)
		return true;
	return false;
}

timeout_t BayonneSession::getJoinTimer(void)
{
	if(state.handler == &BayonneSession::stateStart)
		return state.timeout;

	if(state.handler == &BayonneSession::stateConnect)
		return state.timeout;

	return 0;
}

#ifdef	WIN32
void BayonneSession::libWrite(const char *str)
{
	DWORD len = strlen(str);
	DWORD count;

	if(state.pfd == PFD_INVALID)
		return;

	if(!WriteFile(state.pfd, str, len, &count, NULL))
	{
		CloseHandle(state.pfd);
		state.pfd = PFD_INVALID;
	}
}

void BayonneSession::libClose(const char *str)
{
	libWrite(str);
	if(state.pfd != PFD_INVALID)
	{
		CloseHandle(state.pfd);
		state.pfd = PFD_INVALID;
	}		
}

#else
void BayonneSession::libWrite(const char *str)
{
	size_t len = strlen(str);
	if(state.pfd != PFD_INVALID)
	{
		if(::write(state.pfd, (char *)str, len) < (ssize_t)len)
		{
			::close(state.pfd);
			state.pfd = PFD_INVALID;
		}
	}
}

void BayonneSession::libClose(const char *str)
{
	libWrite(str);
	if(state.pfd != PFD_INVALID)
	{
		::close(state.pfd);
		state.pfd = PFD_INVALID;
	}
}
#endif

uint32 BayonneSession::newTid(void)
{
	if(state.pid)
		libClose("901 TERMINATE\n\n");

	if(state.lib)
	{
		--libexec_count;
		state.lib = NULL;
	}

	state.pid = 0;
	snprintf(var_tid, sizeof(var_tid), "%04d+%08x", timeslot, ++tseq);
	return tseq;
}

void BayonneSession::setSid(void)
{
	time_t now;
	uint32 tstamp;
	time(&now);
	tstamp = (uint32)now;

	snprintf(var_sid, sizeof(var_sid), "%04d-%08x%02x", timeslot, tstamp, seq++);
}

bool BayonneSession::addSymbol(const char *id, const char *value)
{
	Symbol *sym = mapSymbol(id);

	if(!sym)
		return false;

	if(!value)
		return true;

	return append(sym, value);
}

bool BayonneSession::clearSymbol(const char *id)
{
	Symbol *sym = mapSymbol(id);

	if(!sym)
		return false;

	clear(sym);
	return true;
}

const char *BayonneSession::getExternal(const char *id)
{
	time_t now;

	const char *p = strchr(id, '.');
	char *tmp;

	if(!p || ((p - id) < 6))
		return NULL;

	if(!strnicmp("session.", id, 8))
	{
		id += 8;
		if(!stricmp(id, "gid") || !stricmp(id, "id") || !stricmp(id, "sid"))
			return var_sid;
		else if(!stricmp(id, "index"))
		{
			tmp = getTemp();
			snprintf(tmp, 10, "%d", timeslot);
			return tmp;
		}
		else if(!stricmp(id, "timestamp"))
		{
			tmp = getTemp();
			time(&now);
			snprintf(tmp, 16, "%ld", now);
			return tmp;
		}
		else if(!stricmp(id, "uid"))
		{
			tmp = getTemp();
			time(&now);
			snprintf(tmp, 16, "%08lx-%04x", now, timeslot); 
			return tmp;
		}			
		else if(!stricmp(id, "mid"))
		{
			tmp = getTemp();
			time(&now);
			snprintf(tmp, 24, "%08lx-%04x.%s", now, timeslot, audio.libext);
			return tmp;
		}		
		else if(!stricmp(id, "libext"))
			return audio.libext;
		else if(!stricmp(id, "status"))
		{
			if(holding)
				return "holding";
			else if(isJoined())
				return "joined";
			else if(isRefer())
				return "refer";
			else if(isIdle())
				return "idle";
			else
				return "active";
		}	
		else if(!stricmp(id, "line"))
		{
			if(holding)
				return "holding";
			if(offhook)
				return "offhook";
			return "idle";
		}
		else if(!stricmp(id, "pid"))
			return var_pid;
		else if(!stricmp(id, "recall"))
			return var_recall;
		else if(!stricmp(id, "joined") || !stricmp(id, "joinid"))
			return var_joined;
		else if(!stricmp(id, "callid") || !stricmp(id, "crn"))
			return var_callid;
		else if(!stricmp(id, "timeslot"))
			return var_timeslot;
		else if(!stricmp(id, "servertype"))
			return "sa";
		else if(!stricmp(id, "deviceid"))
			return logname;
		else if(!stricmp(id, "voice"))
			return voicelib;
		else if(!stricmp(id, "position"))
			return audio.var_position;
		else if(!stricmp(id, "driverid"))
			return driver->getName();
		else if(!stricmp(id, "spanid"))
			return var_spanid;
		else if(!stricmp(id, "bankid"))
			return var_bankid;
		else if(!stricmp(id, "spantsid"))
			return var_spantsid;
		else if(!stricmp(id, "tid"))
			return var_tid;
		else if(!stricmp(id, "rings"))
			return var_rings;
		else if(!stricmp(id, "date"))
		{
			if(!starttime)
				return NULL;
			return var_date;
		}
		else if(!stricmp(id, "time"))
		{
			if(!starttime)
				return NULL;
			return var_time;
		}
		else if(!stricmp(id, "duration"))
		{
			if(!starttime)
				return "0:00:00";

			time(&now);
			now -= starttime;

			snprintf(var_duration, sizeof(var_duration), "%ld:%02ld:%02ld",
				now / 3600, (now / 60) % 60, now % 60);

			return var_duration;
		}
		else if(!stricmp(id, "type"))
		{
			switch(type)
			{
			case NONE:
				return "none";
			case DIRECT:
				return "direct";
			case INCOMING:
				return "incoming";
			case OUTGOING:
				return "outgoing";
			case FORWARDED:
				return "forward";
			case RECALL:
				return "recall";
			case PICKUP:
				return "pickup";
			case VIRTUAL:
				return "virtual";
			case RINGING:
				return "ringing";
			}
		}
		else if(!stricmp(id, "interface"))
		{
			switch(iface)
			{
			case IF_SPAN:
				return "span";
			case IF_PSTN:
				return "pstn";
			case IF_ISDN:
				return "isdn";
			case IF_INET:
				return "inet";
			default:
				return "none";
			}
		}
		else if(!stricmp(id, "bridge"))
		{
			switch(bridge)
			{
			case BR_TDM:
				return "tdm";
			case BR_SOFT:
			case BR_GATE:
				return "soft";
			default:
				return "none";
			}
		}
		else if(!stricmp(id, "encoding"))
			return audioEncoding();
		else if(!stricmp(id, "extension"))
			return audioExtension();
		else if(!stricmp(id, "framing"))
		{
			tmp = getTemp();
			snprintf(tmp, 10, "%ld", (long)audioFraming());
			return tmp;
		}
		return NULL;
	}

	if(!strnicmp("script.", id, 7)) 
		return ScriptInterp::getExternal(id);
  
	if(!strnicmp("server.", id, 7))
		return ScriptInterp::getExternal(id);
	return NULL;
}

ScriptInterp *BayonneSession::getInterp(const char *id)
{
	BayonneSession *session;
	if(!isdigit(*id))
		return NULL;

	session = getSession(atoi(id));
	if(!session)
		return NULL;

	return dynamic_cast<ScriptInterp *>(session);
}

bool BayonneSession::sizeGlobal(const char *id, unsigned size)
{
	char idg[64];
	Symbol *sym;
	bool rtn = true;

	snprintf(idg, sizeof(idg), "global.%s", id);
	globalLock.enter();
	if(!globalSyms)
		globalSyms = new ScriptSymbols();
	sym = globalSyms->find(idg, size);
	if(!sym)
		rtn = false;
	globalLock.leave();
	return rtn;
}

const char *BayonneSession::getGlobal(const char *id)
{
        char idg[64];
        Symbol *sym;
	const char *cp;

        if(!globalSyms)
                return NULL;

        snprintf(idg, sizeof(idg), "global.%s", id);
        globalLock.enter();
	sym = globalSyms->find(idg, 0);
	cp = ScriptInterp::extract(sym);
	globalLock.leave();
	return cp;
}

bool BayonneSession::setGlobal(const char *id, const char *value)
{
	char idg[64];
	Symbol *sym;
	bool rtn = true;
	
	if(!globalSyms)
		return false;

	snprintf(idg, sizeof(idg), "global.%s", id);
	globalLock.enter();
	sym = globalSyms->find(idg, 0);
	if(!sym)
		rtn = false;
	else
		ScriptInterp::commit(sym, value);
	globalLock.leave();
	return rtn;
}

bool BayonneSession::addGlobal(const char *id, const char *value)
{
	char idg[64];
	Symbol *sym;
	bool rtn = true;

	if(!globalSyms)
		return false;

	snprintf(idg, sizeof(idg), "global.%s", id);
	globalLock.enter();
	sym = globalSyms->find(idg, 0);
	if(!sym)
		rtn = false;
	else
		ScriptInterp::append(sym, value);
	globalLock.leave();
	return rtn;
}

bool BayonneSession::clearGlobal(const char *id)
{
	char idg[64];
	Symbol *sym;
	bool rtn = true;

	if(!globalSyms)
		return false;

	snprintf(idg, sizeof(idg), "global.%s", id);
	globalLock.enter();
	sym = globalSyms->find(idg, 0);
	if(!sym)
		rtn = false;
	else
		ScriptInterp::clear(sym);
	globalLock.leave();
	return rtn;
}


ScriptSymbols *BayonneSession::getSymbols(const char *id)
{
	if(!strnicmp(id, "global.", 7))
	{
		release();
		globalLock.enter();
		lock = &globalLock; 
		if(!globalSyms)
			globalSyms = new ScriptSymbols();
		return globalSyms;
	}

	if(!strnicmp(id, "local.", 6) && frame[stack].base > 0)
	{
		release();
		return frame[frame[stack].base].local;
	}
	return ScriptInterp::getSymbols(id);
}

void BayonneSession::startThread(void)
{
	setState(STATE_THREAD);
}

void BayonneSession::enterThread(ScriptThread *thread)
{
	SLOG_DEBUG("%s: entering thread", logname);
}

void BayonneSession::exitThread(const char *msg)
{
	Event event;
	char *tmp;

	if(msg)
		SLOG_DEBUG("%s: exiting thread; reason=%s", logname, msg);
	else
		SLOG_DEBUG("%s: exiting thread", logname);

	if(msg)
	{
		tmp = getTemp();
		setString(tmp, 64, msg);
		msg = tmp;
	}

	event.id = EXIT_THREAD;
	event.errmsg = msg;
	queEvent(&event);
}			

void BayonneSession::queEvent(Event *event)
{
	Event ev;

	if(!event && msgport)
	{
		msgport->update();
		return;
	}

	if(!event)
	{
		memset(&ev, 0, sizeof(ev));
		ev.id = MSGPORT_WAKEUP;
		event = &ev;
	}

        if(event->id >= ENTER_STATE)
                event->timeslot = timeslot;
        else
                event->timeslot = NO_TIMESLOT;

        if(!msgport)
                putEvent(event);
        else if(!msgport->post(event, 0))
                slog.error("%s: queue event %d lost", logname, event->id);
}

bool BayonneSession::postEvent(Event *event)
{
	return putEvent(event);
}

bool BayonneSession::putEvent(Event *event)
{
	bool rtn;
	Handler prior;
	event_t id;

	enter();
	event->seq = evseq;

retry:
	prior = state.handler;
	rtn = filterPosting(event);

	if(!rtn)
		goto exiting;

	if(logevents)
        {
		serialize.enter();
                if(state.logstate == (Handler)NULL || state.logstate == state.handler)
                        *logevents << logname << ": state=" << state.name
                        << ", event=" << event->id 
			<< ", seq=" << event->seq << endl;
		serialize.leave();
        }

	prior = state.handler;
	id = event->id;

	rtn = (this->*state.handler)(event);

	if(state.handler != prior)
	{
		if(prior == &BayonneSession::stateIdle)
			BayonneDriver::del(this);

		stopTimer();
		event->id = ENTER_STATE;
		event->name = state.name;
		goto retry;
	}

	if(!rtn && id != event->id)
		goto retry;

exiting:
	++evseq;
	release();
	leave();
	return rtn;
}

void BayonneSession::setOffhook(bool state)
{
	offhook = state;
	if(state)
		answered = true;
}

const char *BayonneSession::getKeyString(const char *id)
{
	const char *cp = NULL;

	if(span)
		cp = span->getLast(id);

	if(!cp)
		cp = driver->getLast(id);

	return cp;
}

bool BayonneSession::getKeyBool(const char *id)
{
	const char *cp = getKeyString(id);

	if(!cp)
		cp = "f";

	switch(*cp)
	{
	case 't':
	case 'T':
	case 'y':
	case 'Y':
		return true;
	}
	if(atoi(id) > 0)
		return true;

	return false;
}

long BayonneSession::getKeyValue(const char *id)
{
	const char *cp = getKeyString(id);
	if(!cp)
		cp = "0";

	return atol(cp);
}

timeout_t BayonneSession::getSecTimeout(const char *id)
{
	const char *cp = getKeyString(id);
	if(!cp)
		cp = "0";

	return Audio::toTimeout(cp);
}

timeout_t BayonneSession::getMSecTimeout(const char *id)
{
	const char *cp = getKeyString(id);
	const char *d;

	if(!cp)
		cp = "0";

	d = cp;
	while(*d && isdigit(*d))
		++d;

	if(*d)
		return getSecTimeout(id);

	return atol(cp);
}

timeout_t BayonneSession::getLibexecTimeout(void)
{
	timeout_t timeout;
	Line *line = getLine();

	if(!stricmp(line->cmd, "exec"))
		return TIMEOUT_INF;

	const char *cp = getMember();
	if(cp && isdigit(*cp))
		return atol(cp) * 1000l;

	timeout = getTimeoutKeyword("timeout");
	if(timeout == TIMEOUT_INF || !timeout)
	{
		cp = getMember();
		if(cp)
			return atol(cp) * 1000;
	}
	if(!timeout)
		timeout = TIMEOUT_INF;
	return timeout;
}

bool BayonneSession::isLibexec(const char *tsid)
{     
        if(state.handler != &BayonneSession::stateLibexec)    
                goto error;

        if(!state.pid)
                goto error;

        if(stricmp(var_tid, tsid))
                goto error;

        return true;
error:
        slog.error("libexec transaction id %s invalid", tsid);
        return false;
}  

bool BayonneSession::setLibreset(result_t result)
{
	if(!state.pid)
		return false;

	state.result = result;
	setState(STATE_LIBRESET);
	return true;
}

bool BayonneSession::setLibexec(result_t result)
{
	if(!state.pid)
		return false;

	state.result = result;
	setState(STATE_LIBEXEC);
	return true;
}

void BayonneSession::setConnecting(const char *evname)
{
	char scrbuf[65];
	Name *scr;

	if(evname)
	{
		if(scriptEvent(evname))
		{
			setRunning();
			return;
		}
		snprintf(scrbuf, sizeof(scrbuf), "connect::%s", evname + 5);
		scr = getScript(scrbuf);
		if(connecting && scr)
		{
			redirect(scrbuf);
			return;
		}
	}

	switch(type)
	{
	case RINGING:
		if(!scriptEvent("call:incoming") && connecting)
			redirect("connect::incoming");
		break;
	case OUTGOING:
		if(connecting && !scriptEvent("call:outgoing"))
			redirect("connect::outgoing");
		break;
	default:
		break;
	}
	setRunning();
}

bool BayonneSession::recallReconnect(void)
{
	Event ev;

	memset(&ev, 0, sizeof(ev));
	ev.id = RECALL_RECONNECT;
	if(!enterReconnect(&ev))
		return false;

	setState(STATE_RECONNECT);
	return true;
}

bool BayonneSession::setReconnect(const char *enc, timeout_t framing)
{
	Event ev;

	memset(&ev, 0, sizeof(ev));
	ev.id = ENTER_RECONNECT;
	ev.reconnect.encoding = enc;
	ev.reconnect.framing = framing;
	if(!enterReconnect(&ev))
		return false;

	setState(STATE_RECONNECT);
	return true;
}

void BayonneSession::setRunning(void)
{
	setState(STATE_RUNNING);
	updated = true;
	check();
}

void BayonneSession::setState(state_t id)
{
	state.name = states[id].name;
	state.handler = states[id].handler;
	status[timeslot] = states[id].flag;
}

Script::Name *BayonneSession::attachStart(Event *event)
{
	BayonneSession *parent = event->start.parent;
	BayonneService *svc = BayonneService::first;
	const char *id;
	Name *scr = event->start.scr;
	ScriptImage *img = event->start.img;
	const char *did = NULL, *cid = NULL;
	bool ref = false;
	unsigned pri = 0;
	Line *line;
	struct tm *dt, dtr;
	const char *cp;

	if(!img)
	{
		img = useImage();
		if(!img)
			return NULL;
		ref = true;
	}

	if(parent && event->start.scr)
	{
		scr = event->start.scr;
		goto final;
	}

	cid = getSymbol("session.manager");
	if(cid && *cid && event->start.scr)
	{
		scr = event->start.scr;
		goto final;
	}

	scr = BayonneBinder::binder->getIncoming(img, this, event);
	if(scr && scr->first )
		goto final;

	scr = event->start.scr;

	if(scr && scr->first)
		goto final;		

	id = server->getLast("startup");
	if(id)
	{
		scr = img->getScript(id);
		goto final;
	}

        while(pri < SCRIPT_ROUTE_SLOTS)
        {
                line = img->getRoute(pri++);
                while(line)
                {
			if(matchLine(line))
			{
				scr = line->scr.name;
				pri = SCRIPT_ROUTE_SLOTS;
				break;
			}
                        line = line->next;
                }
        }

	if(!scr)
		scr = img->getScript("runlevel::default");

final:
	event->start.scr = scr;
	
	if(!scr)
	{
		if(ref)
			endImage(img);

		purge();
		return NULL;
	}

	time(&starttime);
	dt = localtime_r(&starttime, &dtr);

	if(dt->tm_year < 1900)
		dt->tm_year += 1900;

	snprintf(var_date, sizeof(var_date), "%04d-%02d-%02d",
		dt->tm_year, dt->tm_mon + 1, dt->tm_mday);

	snprintf(var_time, sizeof(var_time), "%02d:%02d:%02d",
		dt->tm_hour, dt->tm_min, dt->tm_sec);

	enter();	// this attach assumes we have an active lock already
	attach(server, img, scr);

	if(parent)
	{
		state.join.answer_timer = parent->getJoinTimer();
		setConst("session.caller", parent->getSymbol("session.caller"));
		setConst("session.display", parent->getSymbol("session.display"));
		strcpy(var_pid, parent->var_sid);
		strcpy(var_joined, parent->var_sid);
		cp = parent->getSymbol("session.authorized");
		if(cp && *cp)
			setConst("session.authorized", cp);
		cp = parent->getSymbol("session.identity");
		if(cp && *cp)
			setConst("session.associated", cp);
	}
	else
		state.join.answer_timer = 0;

	if(event->id == START_HUNTING)
	{
		state.join.index = 0;
		state.join.select = event->hunt.select;
	}
	else if(!did)
		setConst("session.dialed", event->start.dialing);

	while(svc)
	{
		svc->attachSession(this);
		svc = svc->next;
	}

	return scr;
}
	
bool BayonneSession::matchLine(Line *line)
{
	char sv[65];
	const char *cv, *pat;
	const char **args = line->args;

	if(!line->argc)
		return true;

	while(*args)
	{
		cv = *(args++);
		if(*cv != '=')
			continue;

		++cv;

		if(!strchr(cv, '.'))
		{
			snprintf(sv, sizeof(sv), "session.%s", cv);
			cv = sv;
		}
		pat = getContent(*(args++));
		if(!pat)
			continue;

		cv = getSymbol(cv);
		if(!cv)
			continue;		

		if(matchDigits(cv, pat))
			return true;
	}
	return false;
}	

bool BayonneSession::exit(void)
{
	if(!exiting)
		if(ScriptInterp::exit())
		{
			check();
			return true;
		}

	if(type == PICKUP && offhook)
	{
	}

	setState(STATE_HANGUP);
	return false;
}

bool BayonneSession::requiresDTMF(void)
{
	if(dtmf_digits)
		digit_count = strlen(dtmf_digits);
	else
		digit_count = 0;

	if(dtmf)
		return true;

	dtmf = enableDTMF();
	if(!dtmf)
	{
		if(!signalScript(SIGNAL_FAIL))
			error("dtmf-disabled");
		return false;
	}
	return true;
}

bool BayonneSession::enableDTMF(void)
{
	return true;
}

void BayonneSession::disableDTMF(void)
{
	dtmf = false;
}

void BayonneSession::check(void)
{
	Line *line = getLine();

        if(state.menu)
        {
                dtmf = enableDTMF();
                return;
        }

        if(!image || !line)
        {
                dtmf = false;
                return;
        }

        if(server->isInput(line))
        {
                dtmf = enableDTMF();
                return;
        }

        if((frame[stack].mask & 0x8) && (line->mask & 0x08))
        {
                dtmf = enableDTMF();
                return;
        }

        disableDTMF();
}

bool BayonneSession::digitEvent(const char *evt)
{
        Name *scr = getName();
        NamedEvent *ev = scr->events;
        size_t len = strchr(evt, ':') - evt + 1, clen;
        const char *cp;
        bool partial = false;
        char buffer[32];

	snprintf(buffer, sizeof(buffer), "%s/", server->getLast("location"));
	clen = strlen(buffer);

        while(ev)
        {
#ifdef  HAVE_REGEX_H
                if(ev->type == '~')
                {
                        ev = ev->next;
                        continue;
                }
#endif
                cp = ev->name;
                if(strchr(cp, ':'))
                {
                        if(!strnicmp(cp, evt, len))
                                cp += len;
                        else
                        	goto next;
		}
		if(strchr(cp, '/'))
		{
			if(!strnicmp(buffer, cp, clen))
				cp += clen;
			else
				goto next;
                }
                if(!partial)
                        partial = matchDigits(evt + len, cp, true);

                if(partial)
                        if(matchDigits(evt + len, cp, false))
			{
				gotoEvent(ev);
				return true;
			}
next:
		ev = ev->next;
        }
        strncpy(buffer, evt, len);
        if(partial)
                strcpy(buffer + len, "partial");
        else
                strcpy(buffer + len, "default");
        return scriptEvent(buffer, false);
}

void BayonneSession::makeIdle(void)
{
	BayonneSession *parent, *recall;
	Symbol *sym;
	Event event;
		
	if(ring)
	{
		Ring::detach(ring);
		ring = NULL;
	}

	if(thread)
        {
        	delete thread;
                thread = NULL;
        }

	if(audio.tone)
	{
		delete audio.tone;
		audio.tone = NULL;
	}

	if(offhook)
		setOffhook(false);

	if(peer)
		part(PART_DISCONNECT);

	newTid();
	clrAudio();
        purge();
	disableDTMF();
	dtmf_digits = NULL;
        digit_count = 0;
        ring_count = 0;
        type = NONE;
        strcpy(var_rings, "0");
        var_sid[0] = 0;
	holding = false;
	connecting = false;
	referring = false;
        sym = mapSymbol("session.digits", MAX_DTMF);
        if(sym)
	{
		sym->type = ScriptSymbols::symNORMAL;
		dtmf_digits = sym->data;
		dtmf_digits[0] = 0;
	}

	recall = getSid(var_recall);
	parent = getSid(var_pid);
	if(recall)
	{
		memset(&event, 0, sizeof(event));
		event.peer = this;
		event.id = DROP_REFER;
		recall->queEvent(&event);
		strcpy(var_recall, "none");
	}
	if(parent)
	{
		memset(&event, 0, sizeof(event));
		if(starting)
			event.id = STOP_PARENT;
		else if(seizure == CHILD_RUNNING)
			seizure = CHILD_FAILED;
		else
			event.id = seizure;
		parent->queEvent(&event);
	}
	if(time_joined)
		exitCall("disconnect");

	time_joined = 0;
	strcpy(var_joined, "none");
	strcpy(var_pid, "none");
	starting = false;
	vm = NULL;
}

timeout_t BayonneSession::getTimeoutValue(const char *opt)
{
	opt = getValue(opt);
	if(!opt)
		return 0;

	return Audio::toTimeout(opt);
}

timeout_t BayonneSession::getTimeoutKeyword(const char *kwd)
{
	kwd = getKeyword(kwd);
	if(!kwd)
		return TIMEOUT_INF;

	return Audio::toTimeout(kwd);
}

const char *BayonneSession::getExitKeyword(const char *def)
{
	const char *cp = getKeyword("exit");

	if(!cp)
		return def;

	if(!*cp)
		return NULL;

	if(!stricmp(cp, "none"))
		return NULL;

	if(!stricmp(cp, "any"))
		return "0123456789*#";

	return cp;
}	

const char *BayonneSession::getMenuKeyword(const char *def)
{
        const char *cp = getKeyword("menu");

        if(!cp)
                return def;

        if(!*cp)
                return NULL;

        if(!stricmp(cp, "none"))
                return NULL;

	if(!stricmp(cp, "any"))
                return "0123456789*#";

        return cp;
}

char BayonneSession::getDigit(void)
{
	char dig;
	unsigned c = 0;

	if(!dtmf_digits || !digit_count)
		return 0;

	dig = *dtmf_digits;
		
	while(++c <= digit_count)
		dtmf_digits[c - 1] = dtmf_digits[c];

	return dig;
}

unsigned BayonneSession::getInputCount(const char *digits, unsigned max)
{
	unsigned count = 0;

	if(!digit_count)
		return 0;

	if(!digits)
		goto skip;

	while(count < digit_count && count <= max)
	{
		if(strchr(digits, dtmf_digits[count++]))		
			return count;
	}

skip:
	if(digit_count >= max)
		return max;

	return 0;
}

void BayonneSession::clrAudio(void)
{
	audio.cleanup();
	peer = NULL;
}

const char *BayonneSession::audioEncoding(void)
{
	return "ulaw";
}

timeout_t BayonneSession::audioFraming(void)
{
	return 20;
}

const char *BayonneSession::audioExtension(void)
{
	return ".au";
}

const char *BayonneSession::getAudio(bool live)
{
	const char *cp = getKeyword("encoding");
	const char *ext = getKeyword("extension");
	char *p;
	char bts[8];

	setSymbol("script.error", "none");

	clrAudio();

	if(ext && !*ext)
		ext = NULL;

	if(!cp || !*cp)
		cp = ext;

	if(cp && *cp)
	{
		audio.encoding = Bayonne::getEncoding(cp); 

		if(!ext)
			ext = Audio::getExtension(audio.encoding);
	}
	else
		audio.encoding = Audio::unknownEncoding;

	cp = getKeyword("framing");
	if(cp && *cp)
		audio.framing = atoi(cp);
	else
		audio.framing = 0;

	audio.libext = ".au";
	audio.extension = ext;
	audio.offset = getKeyword("position");
	audio.prefixdir = cp = getKeyword("prefix");

	if(audio.offset && !*audio.offset)
		audio.offset = NULL;

	if(cp && !*cp)
	{
		cp = NULL;
		audio.prefixdir = NULL;
	}

	if(cp)
	{
		if(*cp == '/' || *cp == '\\' || cp[1] == ':')
			return "invalid prefix directory";

		if(strstr(cp, "..") || strstr(cp, "/."))
			return "invalid prefix directory";

		if(!stricmp(cp, "tmp:"))
			audio.prefixdir = path_tmp;
		else if(!stricmp(cp, "ram:"))
			audio.prefixdir = path_tmpfs;
		else
			if(strchr(cp, ':'))
				return "invalid prefix directory";
	}

	cp = getKeyword("voice");
	if(cp && *cp)
	{
		snprintf(bts, sizeof(bts), "%s", cp);
		p = strchr(bts, '/');
		if(p)
			*p = 0;
		
		audio.translator = BayonneTranslator::get(bts);
		if(!audio.translator)
			return "requested language not loaded";
		
		cp = audio.getVoicelib(cp);
		if(!cp)
			return "voice library invalid";
		audio.voicelib = cp;
	}
	else
	{
		audio.translator = translator;
		audio.voicelib = voicelib;
	}

	return checkAudio(live);
}

const char *BayonneSession::checkAudio(bool live)
{
	audio.libext = ".au";

	if(!audio.extension)
		audio.extension = ".au";

	if(!live)
	{
		if(audio.encoding == Audio::unknownEncoding)
			audio.encoding = Audio::mulawAudio;

		audio.framing = Audio::getFraming(audio.encoding, audio.framing);
		if(!audio.framing)
			audio.framing = 10;
		return NULL;
	}

	switch(audio.encoding)
	{
	case Audio::unknownEncoding:
		audio.encoding = Audio::mulawAudio;
		break;
	default:
		if(AudioCodec::isLinear(audio.encoding))
			break;
		if(AudioCodec::load(audio.encoding))
			break;
		return "unsupported audio encoding";
	}
	audio.framing = Audio::getFraming(audio.encoding, audio.framing);
	if(!audio.framing)
		audio.framing = 20;
	return NULL;
}	
	
void BayonneSession::branching(void)
{
	newTid();

	if(state.menu)
	{
		if(stack > state.stack)
			return;
		state.menu = NULL;
	}
}

timeout_t BayonneSession::getToneFraming(void)
{
	return 20;
}

bool BayonneSession::peerLinear(void)
{
	return false;
}

bool BayonneSession::peerAudio(Audio::Encoded encoded)
{
	return false;
}

bool BayonneSession::setPeering(Audio::Encoding encoding, timeout_t framing)
{
	return false;
}

void BayonneSession::part(event_t id)
{
	Event event;

	if(!peer)
		return;

	memset(&event, 0, sizeof(event));
	event.id = id;
	peer->queEvent(&event);
}
