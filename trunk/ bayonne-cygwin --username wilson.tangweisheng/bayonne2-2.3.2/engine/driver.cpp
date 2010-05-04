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

bool BayonneDriver::stopping = false;
bool BayonneDriver::protocols = true;
BayonneDriver *BayonneDriver::firstDriver = NULL;
BayonneDriver *BayonneDriver::lastDriver = NULL;
BayonneDriver *BayonneDriver::trunkDriver = NULL;
BayonneDriver *BayonneDriver::protoDriver = NULL;
Semaphore BayonneDriver::oink(0);

BayonneDriver::BayonneDriver(Keydata::Define *pairs, const char *cfg, const char *id, bool virt) :
ReconfigKeydata(cfg, pairs), Mutex()
{
	name = id;
	running = false;
	msgport = NULL;
	logevents = NULL;

	avail = 0;
	firstIdle = lastIdle = highIdle = NULL;

	flash_timer = 250;
	pickup_timer = hangup_timer = 160;
	seize_timer = 1000;
	hunt_timer = 16000;
	ring_timer = 6000;
	reset_timer = 60000;
	release_timer = 60000;
	interdigit_timer = 80;
	answer_count = 1;
	active_calls = 0;

	audio_priority = 0;
	audio_stack = 0;
	audio_level = 100;

	if(firstDriver)
	{
		lastDriver->nextDriver = this;
		lastDriver = this;
	}
	else
		firstDriver = lastDriver = this;

	nextDriver = NULL;

	timeslot = getTimeslotsUsed();
	count = 0;
	span = getSpansUsed();
	spans = 0;
}

BayonneDriver::~BayonneDriver()
{
	if(running)
		stopDriver();

	if(msgport)
	{
		delete msgport;
		msgport = NULL;
	}
}

bool BayonneDriver::deregister(const char *id)
{
	return false;
}

bool BayonneDriver::reregister(const char *id, const char *uri, const char *secret, timeout_t duration)
{
	return false;
}

bool BayonneDriver::suspend(void)
{
	return false;
}

bool BayonneDriver::resume(void)
{
	return false;
}

void BayonneDriver::reregister(void)
{
}

bool BayonneDriver::isExternal(const char *ext)
{
	return false;
}

bool BayonneDriver::isAuthorized(const char *userid, const char *secret)
{
	return false;
}

bool BayonneDriver::isRegistered(const char *ext)
{
	return isExternal(ext);
}

bool BayonneDriver::isAvailable(const char *ext)
{
	return isExternal(ext);
}

bool BayonneDriver::isReachable(const char *proxy)
{
	return true;
}

void BayonneDriver::relistIdle(void)
{
	highIdle = firstIdle;
	firstIdle = lastIdle = NULL;
}

unsigned BayonneDriver::getRegistration(regauth_t *data, unsigned count, const char *id)
{
        return 0;
}

void BayonneDriver::del(BayonneSession *session)
{
	BayonneDriver *driver = session->driver;
	BayonneSpan *span = session->span;

	if(!session->isAvail)
		return;

	driver->enter();
	if(driver->firstIdle == session)
		driver->firstIdle = session->nextIdle;
	if(driver->lastIdle == session)
		driver->lastIdle = session->prevIdle;
	if(session->nextIdle)
		session->nextIdle->prevIdle = session->prevIdle;
	if(session->prevIdle)
		session->prevIdle->nextIdle = session->nextIdle;
	session->isAvail = false;
	--driver->avail;
	--idle_count;
	if(span)
		++span->used;
	driver->leave();
}

void BayonneDriver::add(BayonneSession *session)
{
	BayonneDriver *driver = session->driver;
	BayonneSpan *span = session->span;

	if(session->isAvail)
		return;

	session->prevIdle = driver->lastIdle;
	session->nextIdle = NULL;

	driver->enter();
	if(!driver->firstIdle)
		driver->firstIdle = driver->lastIdle = session;
	else
	{
		driver->lastIdle->nextIdle = session;
		driver->lastIdle = session;
	}
	++driver->avail;
	session->isAvail = true;
	if(span && span->used > 0)
		--span->used;
	driver->leave();
	if(++idle_count > idle_limit)
		idle_limit = idle_count;
	if(idle_count == idle_limit && shutdown_flag)
	{
		shutdown_flag = false;
#ifndef	WIN32
		raise(SIGTERM);
#endif
	}		
}

bool BayonneDriver::isSpanable(unsigned spid)
{
	BayonneSpan *span = getSpan(spid);

	if(!span)
		return false;

	if(span->driver != this)
		return false;

	if(span->used >= span->count)
		return false;

	return true;
}

bool BayonneDriver::getDestination(const char *server, const char *user, char *buf, size_t size)
{
	return false;
}

BayonneDriver *BayonneDriver::loadProtocol(const char *id, unsigned timeslots)
{
	char buf[65];
	char val[16];
	BayonneDriver *d;
	const char *cp;

	if(!id || !*id || !stricmp(id, "none"))
		return NULL;

	if(!protocols)
	{
		slog.error("cannot load %s; protocols disabled", id);
		return NULL;
	}

	snprintf(val, sizeof(val), "%d", timeslots);
	snprintf(buf, sizeof(buf), "slots.%s", id);
	if(timeslots)
		server->setValue(buf, val);

	d = loadDriver(id);
	if(!d)
		return NULL;

	cp = d->getLast("type");
	if(!cp || strnicmp(cp, "proto", 5))
	{
		slog.error("loading driver %s as protocol; rejecting", id);
		return NULL;
	}			
	return d;
}

BayonneDriver *BayonneDriver::loadTrunking(const char *id)
{
	BayonneDriver *d;
	const char *cp;

	if(!id || !*id || !stricmp(id, "none"))
		return NULL;

	if(trunkDriver)
	{
		slog.error("trunk driver %s disabled; trunking already loaded", id);
		return NULL;
	}

	d = loadDriver(id);
	if(!d)
		return NULL;

	cp = d->getLast("type");
	if(!cp)
		cp = "none";

	if(!strnicmp(cp, "proto", 5))
	{
		slog.error("loading protocol %s for trunking; rejecting", id);
		return NULL;
	}
		 
	if(strnicmp(cp, "peer", 4))
	{
		protocols = false;
		slog.warn("driver %s incapable of peering; no protocols will be loaded", id);
	}

	return d;
}

BayonneDriver *BayonneDriver::loadDriver(const char *id)
{
	DSO *dso;
	BayonneDriver *d;
	char pathbuf[256];
	const char *cp;
	const char *path;
	const char *prefix = NULL;

#ifdef	WIN32
	if(!prefix)
		prefix="C:\\Program Files\\Common Files\\GNU Telephony\\Bayonne Drivers";
#else
	if(!prefix)
		prefix=LIBDIR_FILES;
#endif

#ifdef	WIN32
	snprintf(pathbuf, sizeof(pathbuf), "%s/%s." RLL_SUFFIX, prefix, id);
#else
	snprintf(pathbuf, sizeof(pathbuf), "%s/%s.ivr", prefix, id);
#endif
	path = pathbuf;	

	d = get(id);
	if(d)
		goto check;

	if(!canAccess(path))
	{
		errlog("access", "cannot load %s", path);
		return NULL;
	}

	dso = new DSO(path);
	if(!dso->isValid())
	{
		errlog("error", "%s: %s", path, dso->getError());
		return NULL;
	}
	d = get(id);
check:
	if(!d)
		return NULL;

	cp = d->getLast("type");
	if(!strnicmp(cp, "proto", 5))
		protoDriver = d;
	else
		trunkDriver = d;

	return d;
}
	
BayonneSpan *BayonneDriver::getSpan(unsigned id)
{
	if(id >= spans)
		return NULL;

	return BayonneSpan::get(span + id);
}

BayonneSession *BayonneDriver::getTimeslot(timeslot_t ts)
{
	if(ts >= count)
		return NULL;

	return getSession(timeslot + ts);
}

const char *BayonneDriver::registerScript(ScriptImage *img, Line *line)
{
	return "selected driver does not support registration";
}

const char *BayonneDriver::assignScript(ScriptImage *img, Line *line)
{
	return "selected driver does not support assignments";
}

void BayonneDriver::reloadDriver(void)
{
}

void BayonneDriver::startDriver(void)
{
	oink.wait();		// wait for the pig to squeal
	running = true;
	time(&start_time);
}

void BayonneDriver::stopDriver(void)
{
	Event event;
	BayonneSession *session;

	if(!running)
		return;

        if(msgport)
        {
                delete msgport;
                msgport = NULL;
        }

	while(count--)
	{
		session = getSession(timeslot + count);
		if(session)
		{
			memset(&event, 0, sizeof(event));
			event.id = SYSTEM_DOWN;
			session->enter();
			session->postEvent(&event);
			session->leave();
		}
	}

	count = 0;
	running = false;
}

BayonneDriver *BayonneDriver::authorize(const char *userid, const char *secret)
{
	BayonneDriver *driver = firstDriver;

	if(!userid)
		userid = "anonymous";

	if(!secret)
		secret = "";

	while(driver)
	{
		if(driver->isAuthorized(userid, secret))
			break;

		driver = driver->nextDriver;
	}
	return driver;
}

BayonneDriver *BayonneDriver::get(const char *id)
{
	BayonneDriver *driver = firstDriver;
	const char *cp;

	if(!id)
		return driver;

	while(driver)
	{
		if(!stricmp(driver->name, id))
			break;
		cp = driver->getLast("type");
		if(cp && !strnicmp(cp, "proto", 5))
		{
			cp = driver->getLast("proto");
			if(!cp)
				cp = driver->getLast("protocol");
		}
		else
			cp = NULL;
		if(cp && !stricmp(cp, id))
			break;
		driver = driver->nextDriver;
	}
	return driver;
}

unsigned BayonneDriver::list(char **items, unsigned max)
{
	BayonneDriver *driver = firstDriver;
	unsigned count = 0;

	while(driver && max--)
	{
		items[count++] = (char *)driver->name;
		driver = driver->nextDriver;
	}
	items[count] = NULL;
	return count;	
}
	
void BayonneDriver::start(void)
{
	BayonneDriver *driver = firstDriver;
	while(driver)
	{
		if(!driver->running)
			driver->startDriver();
		driver = driver->nextDriver;
	}
	BayonneSpan::allocate();
}

void BayonneDriver::reload(void)
{
	BayonneDriver *driver = firstDriver;

	while(driver)
	{
		driver->reloadDriver();
		driver = driver->nextDriver;
	}
}

void BayonneDriver::stop(void) 
{
	BayonneSession *s;
        BayonneDriver *driver = firstDriver;
	unsigned ts = 0;
	Event event;
	timeout_t hangup_timer = 0;

	slog.notice("driver(s) stopping...");

	stopping = true;

	while(ts < ts_used)
	{
		s = getSession(ts++);
		if(!s)
			continue;
		s->enter();
		if(!s->isIdle())
		{
			if(s->driver->hangup_timer > hangup_timer)
				hangup_timer = s->driver->hangup_timer;

			hangup_timer = s->driver->hangup_timer;
			memset(&event, 0, sizeof(event));
			event.id = STOP_SCRIPT;
			s->postEvent(&event);
		}
		s->leave();
	}

	if(hangup_timer)
		Thread::sleep(hangup_timer + 60);
	
        while(driver)
        {
		if(driver->running)
	                driver->stopDriver();

		driver->running = false;
                driver = driver->nextDriver; 
        }
}

BayonneSession *BayonneDriver::getIdle(void)
{
	BayonneSession *session = NULL;

	if(stopping)
		return NULL;

	enter();
	if(firstIdle)
	{
		session	= firstIdle;
		firstIdle = session->nextIdle;
		session->isAvail = false;
	}
	else if(highIdle)
	{
		session = highIdle;
		highIdle = session->nextIdle;
		session->nextIdle = session->prevIdle = NULL;
	}
	leave();
	return session;
}

	
