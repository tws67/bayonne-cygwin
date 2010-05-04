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
#include <cstdarg>

using namespace ost;
using namespace std;

unsigned long Bayonne::Traffic::stamp = 0;
ScriptImage **Bayonne::localimages = NULL;
BayonneSession **Bayonne::timeslots = NULL;
Bayonne::timeslot_t Bayonne::ts_count = 0;
Bayonne::timeslot_t Bayonne::ts_used = 0;
Bayonne::timeslot_t Bayonne::ts_limit = 0;
ScriptCommand *Bayonne::server = NULL;
const char *Bayonne::path_prompts;
const char *Bayonne::path_tmpfs;
const char *Bayonne::path_tmp;
std::ostream *Bayonne::logging = &cerr;
char *Bayonne::status = NULL;
timeout_t Bayonne::step_timer = 20;
timeout_t Bayonne::reset_timer = 60;
timeout_t Bayonne::exec_timer = 60000;
unsigned Bayonne::ts_trk = 0;
unsigned Bayonne::ts_ext = 0;
Bayonne::RPCNode *Bayonne::RPCNode::first = NULL;
unsigned Bayonne::RPCNode::count = 0;
Bayonne::Traffic Bayonne::total_call_attempts;
Bayonne::Traffic Bayonne::total_call_complete;
Mutex Bayonne::Ring::locker;
Mutex Bayonne::serialize;
Bayonne::Ring *Bayonne::Ring::free = NULL;
ThreadLock Bayonne::reloading;
char Bayonne::sla[64] = {0};
unsigned Bayonne::idle_count = 0;
unsigned Bayonne::idle_limit = 0;
bool Bayonne::shutdown_flag = false;
time_t Bayonne::start_time = 0;
time_t Bayonne::reload_time = 0;
AtomicCounter Bayonne::libexec_count = 0;
unsigned Bayonne::compile_count;
volatile bool Bayonne::image_loaded = false;
volatile unsigned short Bayonne::total_active_calls = 0;
const char *Bayonne::trap_community = "public";
SOCKET Bayonne::trap_so4 = INVALID_SOCKET;
unsigned Bayonne::trap_count4 = 0;
struct sockaddr_in Bayonne::trap_addr4[8];
char Bayonne::dtmf_keymap[256];

#ifdef	CCXX_IPV6
SOCKET Bayonne::trap_so6 = INVALID_SOCKET;
unsigned Bayonne::trap_count6 = 0;
struct sockaddr_in6 Bayonne::trap_addr6[8];
#endif

BayonneTranslator *Bayonne::init_translator = NULL;
const char *Bayonne::init_voicelib = "none/prompts";

Bayonne::statetab Bayonne::states[] = {
	{"initial", &BayonneSession::stateInitial, ' '},
	{"idle", &BayonneSession::stateIdle, '-'},
	{"reset", &BayonneSession::stateReset, '@'},
	{"release", &BayonneSession::stateRelease, '@'},
	{"busy", &BayonneSession::stateBusy, '^'},
	{"standby", &BayonneSession::stateStandby, '*'},
	{"ringing", &BayonneSession::stateRinging, '!'},
	{"pickup", &BayonneSession::statePickup, 'o'},
	{"seize", &BayonneSession::stateSeize, 'o'},
	{"answer", &BayonneSession::stateAnswer, 'a'},
	{"run", &BayonneSession::stateRunning, 'x'},
	{"exec", &BayonneSession::stateLibexec, 'e'},
	{"thread", &BayonneSession::stateThreading, 't'},
	{"clear", &BayonneSession::stateClear, 'c'},
	{"inkey", &BayonneSession::stateInkey, 'c'},
	{"input", &BayonneSession::stateInput, 'c'},
	{"read", &BayonneSession::stateRead, 'c'},
	{"collect", &BayonneSession::stateCollect, 'c'},
	{"dial", &BayonneSession::stateDial, 'd'},
	{"xfer", &BayonneSession::stateXfer, 'd'},
	{"refer", &BayonneSession::stateRefer, 'w'},
	{"hold", &BayonneSession::stateHold, 'd'},
	{"recall", &BayonneSession::stateRecall, 'd'},
	{"tone", &BayonneSession::stateTone, 't'},
	{"dtmf", &BayonneSession::stateDTMF, 't'},
	{"play", &BayonneSession::statePlay, 'p'},
	{"record", &BayonneSession::stateRecord, 'r'},
	{"join", &BayonneSession::stateJoin, 'j'},
	{"accept", &BayonneSession::stateWait, 'w'},
	{"calling", &BayonneSession::stateCalling, 'd'},
	{"connect", &BayonneSession::stateConnect, 'd'},
	{"reconnect", &BayonneSession::stateReconnect, '#'},
	{"hunting", &BayonneSession::stateHunting, 'o'},
	{"sleep", &BayonneSession::stateSleep, 's'},
	{"start", &BayonneSession::stateStart, 's'},
	{"hangup", &BayonneSession::stateHangup, 'h'},
	{"reset", &BayonneSession::stateLibreset, 'e'},
	{"keywait", &BayonneSession::stateWaitkey, 'c'}, 
	{"wait", &BayonneSession::stateLibwait, 's'},
	{"reset", &BayonneSession::stateIdleReset, '@'},
	{"final", &BayonneSession::stateFinal, ' '},
	{NULL, NULL, ' '}};

Bayonne::Ring *Bayonne::Ring::find(Ring *r, BayonneSession *s)
{
	while(r)
	{
		if(r->session == s)
			return r;
		r = r->next;
	}
	return NULL;
}

void Bayonne::Ring::start(Ring *r, BayonneSession *sp)
{
	Event event;
	const char *cp;

	while(r)
	{
		if(!r->driver || r->session != NULL || !r->script)
			goto next;

		cp = r->driver->getLast("type");
		if(cp && !stricmp(cp, "proto"))
			r->session = r->driver->getIdle();
		else
			r->session = getSession(r->driver->getFirst() + atoi(r->ring_id));

		if(!r->session)
			goto next;

		if(r->count)
		{
			--r->count;
			goto next;
		}

		memset(&event, 0, sizeof(event));
		event.id = START_RINGING;
		event.start.img = sp->getImage();
		event.start.scr = r->script;
		event.start.dialing = r->ring_id;
		event.start.parent = sp;
		r->session->postEvent(&event);	
next:
		r = r->next;
	}
}

void Bayonne::Ring::detach(Ring *r)
{
	Event event;
	Ring *n;
	while(r)
	{
		if(r->session)
		{
			memset(&event, 0, sizeof(event));
			event.id = CANCEL_CHILD;
			r->session->queEvent(&event);
		}
		n = r->next;
		locker.enter();
		r->next = free;
		free = r;
		locker.leave();
		r = n;
	}
}

Bayonne::Ring *Bayonne::Ring::attach(BayonneDriver *d, const char *id, Ring *list)
{
	Ring *r;
	locker.enter();
	r = list;

	// make sure we only attach new and unique entries...

	while(r)
	{
		if(d == r->driver && !stricmp(id, r->ring_id))
		{
			locker.leave();
			return NULL;
		}
		r = r->next;
	}
	if(free)
	{
		r = free;
		free = r->next;
	}
	else
		r = new Ring();
	locker.leave();
	r->next = list;
	r->ring_id = id;
	r->driver = d;
	r->session = NULL;
	r->script = NULL;
	r->count = 0;
	return r;
};

Bayonne::RPCNode::RPCNode(const char *id, RPCDefine *table)
{
	prefix = id;
	methods = table;
	next = first;
	first = this;

	while(table && table->name)
	{
		++count;
		++table;
	}
}
	
Bayonne::Traffic::Traffic()
{
	time_t ts;
	if(!stamp)
	{
		time(&ts);
		stamp = (unsigned long)ts;
	}

	iCount = oCount = 0;
}

bool Bayonne::getUserdata(void)
{
#ifdef	WIN32
	return false;
#else
	if(!getuid())
		return false;
	if(!Process::getEnv("HOME"))
		return false;
	return true;
#endif
}

const char *Bayonne::getRegistryId(const char *id)
{
	const char *cp;

	if(!id || !*id)
		return NULL;

	cp = strchr(id, '/');
	if(cp)
		return ++cp;

	cp = strrchr(id, ':');
	if(cp)
		return ++cp;

	return id;
}

void Bayonne::waitLoaded(void)
{
	while(!image_loaded)
	{
		Thread::sleep(100);
		Thread::yield();
	}
}

void Bayonne::snmptrap(unsigned id, const char *descr)
{
	static unsigned char header1_short[] = {
    	0x06, 0x08, 0x2b, 0x06, 0x01, 0x04, 0x01, 0x81, 0xc7, 0x42,
    	0x40, 0x04, 0xc0, 0xa8, 0x3b, 0xcd};

	static unsigned char header1_long[] = {
    	0x06, 0x08, 0x2b, 0x06, 0x01, 0x04, 0x01, 0x81, 0xc7, 0x42,
    	0x40, 0x04, 0xc0, 0xa8, 0x3b, 0xcd};

	static unsigned char header2[] = {
    	0x06, 0x08, 0x2b, 0x06, 0x01, 0x02, 0x01, 0x01, 0x01, 0x00, 0x04};

	unsigned idx;
	socklen_t alen;
	unsigned char buf[128];
	unsigned id1 = id, id2 = 0;
	unsigned len;
	long timestamp = uptime() * 100l;
	unsigned offset1 = 7 + strlen(trap_community);
	unsigned offset2 = offset1 + sizeof(header1_long);
	unsigned lo1 = 1;
	unsigned lo2 = offset1 + 1;

	if(id1 > 6)
	{
		id2 = id1;
		id1 = 6;
	}

    buf[0] = 0x30;
    buf[2] = 0x02;
    buf[3] = 0x01;
    buf[4] = 0x00;
    buf[5] = 0x04;
    buf[6] = strlen(trap_community);

    strcpy((char *)(buf + 7), trap_community);
    buf[offset1] = 0xa4;

    if(descr)
        memcpy(buf + offset1 + 2, header1_long, sizeof(header1_long));
    else
        memcpy(buf + offset1 + 2, header1_short, sizeof(header1_short));

    buf[offset2] = 0x02;
    buf[offset2 + 1 ] = 0x01;
    buf[offset2 + 2] = id1;
    buf[offset2 + 3] = 0x02;
    buf[offset2 + 4] = 0x01;
    buf[offset2 + 5] = id2;

    buf[offset2 + 6] = 0x43;
    buf[offset2 + 7] = 0x04;
    buf[offset2 + 8] = timestamp / 0x1000000l;
    buf[offset2 + 9] = (timestamp / 0x10000l) & 0xff;
    buf[offset2 + 10] = (timestamp / 0x100l) & 0xff;
    buf[offset2 + 11] = timestamp & 0xff;
    buf[offset2 + 12] = 0x30;

    if(!descr) {
        buf[offset2 + 13] = 0x00;
        len = offset2 + 14;
        goto send;
    }

    buf[offset2 + 13] = strlen(descr) + 14;
    buf[offset2 + 14] = 0x30;
    buf[offset2 + 15] = strlen(descr) + 12;
    memcpy(buf + offset2 + 16, header2, sizeof(header2));
    offset2 += 16 + sizeof(header2);
    buf[offset2] = strlen(descr);
    strcpy((char *)(buf + offset2 + 1), descr);
    len = offset2 + 1 + strlen(descr);

send:
    buf[lo1] = len - 2;
    buf[lo2] = len - 15;

	alen = sizeof(struct sockaddr_in);
	if(trap_so4 != INVALID_SOCKET && trap_count4)
		for(idx = 0; idx < trap_count4; ++idx)
			sendto(trap_so4, buf, len, 0, 
				(struct sockaddr *)&trap_addr4[idx], alen);

#ifdef	CCXX_IPV6
        alen = sizeof(struct sockaddr_in6);
        if(trap_so6 != INVALID_SOCKET && trap_count6)
                for(idx = 0; idx < trap_count6; ++idx)
                        sendto(trap_so6, buf, len, 0, 
				(struct sockaddr *)&trap_addr6[idx], alen);
#endif
}

void Bayonne::addTrap4(const char *addr)
{
	char abuf[128];
	char *tok;
	socklen_t alen = sizeof(sockaddr_in);
	InetAddress ia;
	int on = 1;

	setString(abuf, sizeof(abuf), addr);
	addr = strtok_r(abuf, " ;,\r\n\t", &tok);
	if(trap_so4 == INVALID_SOCKET)
	{
		trap_so4 = socket(AF_INET, SOCK_DGRAM, 0);
		setsockopt(trap_so4, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on));
	}

	while(addr && trap_count4 < 8)
	{
		ia = addr;
		memset(&trap_addr4[trap_count4], 0, alen);
		trap_addr4[trap_count4].sin_family = AF_INET;
		trap_addr4[trap_count4].sin_port = htons(162);
		trap_addr4[trap_count4].sin_addr = ia.getAddress();	
		++trap_count4;
		addr = strtok_r(NULL, " ;,\r\n\t", &tok);
	}
}

#ifdef	CCXX_IPV6
void Bayonne::addTrap6(const char *addr)
{
	char abuf[128];
	char *tok;
	socklen_t alen = sizeof(sockaddr_in6);
	IPV6Address ia;
	int on = 1;

	setString(abuf, sizeof(abuf), addr);
	addr = strtok_r(abuf, " ;,\r\n\t", &tok);
	if(trap_so6 == INVALID_SOCKET)
	{
		trap_so6 = socket(AF_INET6, SOCK_DGRAM, 0);
		setsockopt(trap_so6, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on));
	}

	while(addr && trap_count6 < 8)
	{
		ia = addr;
		memset(&trap_addr6[trap_count6], 0, alen);
		trap_addr6[trap_count6].sin6_family = AF_INET6;
		trap_addr6[trap_count6].sin6_port = htons(162);
		trap_addr6[trap_count6].sin6_addr = ia.getAddress();	
		++trap_count6;
		addr = strtok_r(NULL, " ;,\r\n\t", &tok);
	}
}

#endif

bool Bayonne::loadPlugin(const char *path)
{
        char pathbuf[256];
        const char *cp, *kv;
	const char *prefix = NULL;
        DSO *dso;

#ifdef	WIN32
	if(!prefix)
		prefix = "C:\\Program Files\\Common Files\\GNU Telephony\\Bayonne Plugins";
#else
	if(!prefix)
		prefix = LIBDIR_FILES;
#endif

#ifdef  WIN32
        snprintf(pathbuf, sizeof(pathbuf), "%s/%s." RLL_SUFFIX, prefix, path);
#else
        snprintf(pathbuf, sizeof(pathbuf), "%s/%s.dso", prefix, path);
#endif  
        cp = path;
        path = pathbuf;

        kv = server->getLast(path);
        if(kv)
        {
                if(!stricmp(kv, "loaded"))
                        return true;
		return false;
	}

        if(!canAccess(path))
        {
                errlog("access", "cannot load %s", path);
                return false;
        }

        dso = new DSO(path);
        if(!dso->isValid())
        {
                kv = dso->getError();
                server->setValue(path, kv);
                errlog("error", "cannot initialize %s", path);
                return false;
        }
        server->setValue(path, "loaded");
        return true;                
}

bool Bayonne::loadMonitor(const char *path)
{
        char pathbuf[256];
        const char *cp, *kv;
	const char *prefix = NULL;
        DSO *dso;

#ifdef	WIN32
	if(!prefix)
		prefix = "C:\\Program Files\\Common Files\\GNU Telephony\\Bayonne Monitors";
#else
	if(!prefix)
		prefix = LIBDIR_FILES;
#endif

#ifdef  WIN32
        snprintf(pathbuf, sizeof(pathbuf), "%s/%s." RLL_SUFFIX, prefix, path);
#else
        snprintf(pathbuf, sizeof(pathbuf), "%s/%s.mon", prefix, path);
#endif  
        cp = path;
        path = pathbuf;

        kv = server->getLast(path);
        if(kv)
        {
                if(!stricmp(kv, "loaded"))
                        return true;
		return false;
	}

        if(!canAccess(path))
        {
                errlog("access", "cannot load %s", path);
                return false;
        }

        dso = new DSO(path);
        if(!dso->isValid())
        {
                kv = dso->getError();
                server->setValue(path, kv);
                errlog("error", "cannot initialize %s", path);
                return false;
        }
        server->setValue(path, "loaded");
        return true;                
}

bool Bayonne::loadAudio(const char *cp)
{
	return false;
}

Bayonne::Handler Bayonne::getState(const char *id)
{
	unsigned pos = 0;
	while(states[pos].name)
	{
		if(!stricmp(states[pos].name, id))
			return states[pos].handler;
		++pos;
	}
	return (Handler)NULL;
}

Audio::Encoding Bayonne::getEncoding(const char *cp)
{
	const char *encoding = cp;

	if(*encoding == '.')
		++encoding;
	
        if(!strnicmp(encoding, "g.", 2))
                encoding += 2;
        else if(encoding[0] == 'g' && isdigit(encoding[1]))
                ++encoding;

        if(!stricmp(encoding, "stereo"))
                return Audio::pcm16Stereo;
        if(!stricmp(encoding, "726-40") || !stricmp(encoding, "a40"))
                return Audio::g723_5bit;
        if(!stricmp(encoding, "726-32") || !stricmp(encoding, "a32"))
                return Audio::g721ADPCM;
        if(!stricmp(encoding, "726-24") || !stricmp(encoding, "a24"))
                return Audio::g723_3bit;
        if(!stricmp(encoding, "726-16") || !stricmp(encoding, "a16"))
                return Audio::g723_2bit;
        if(!stricmp(encoding, "729"))
                return Audio::g729Audio;
        if(!stricmp(encoding, "721"))
                return Audio::g721ADPCM;
        if(!stricmp(encoding, "pcmu"))
                return Audio::mulawAudio;
        if(!stricmp(encoding, "pcma"))
                return Audio::alawAudio;

	return Audio::getEncoding(cp);
}
	
bool Bayonne::matchDigits(const char *digits, const char *match, bool partial)
{
	unsigned len = strlen(match);
	unsigned dlen = 0;
	bool inc;
	const char *d = digits;
	char dbuf[32];

	if(*d == '+')
		++d;

	while(*d && dlen < sizeof(dbuf) - 1)
	{
		if(isdigit(*d) || *d == '*' || *d == '#')
		{
			dbuf[dlen++] = *(d++);
			continue;
		}

		if(*d == ' ' || *d == ',')
		{
			++d;
			continue;
		}

		if(*d == '!')
			break;

		if(!stricmp(digits, match))
			return true;

		return false;
	}	

	if(*d && *d != '!')
		return false;

	digits = dbuf;
	dbuf[dlen] = 0;	

	if(*match == '+')
	{
		++match;
		--len;
		if(dlen < len)
			return false;
		digits += (len - dlen);
	}

	while(*match && *digits)
	{
		inc = true;
		switch(*match)
		{
		case 'x':
		case 'X':
			if(!isdigit(*digits))
				return false;
			break;
		case 'N':
		case 'n':
			if(*digits < '2' || *digits > '9')
				return false;
			break;
		case 'O':
		case 'o':
			if(*digits && *digits != '1')
				inc = false;
			break;
		case 'Z':
		case 'z':
			if(*digits < '1' || *digits > '9')
				return false;
			break;
		case '?':
			if(!*digits)
				return false;
			break;
		default:
			if(*digits != *match)
				return false;
		}
		if(*digits && inc)
			++digits;
		++match;	
	}
        if(*match && !*digits)
                return partial;

        if(*match && *digits)
                return false;

	return true;
}

void Bayonne::allocateLocal(void)
{
	if(localimages)
		delete[] localimages;

	localimages = new ScriptImage *[ts_count];
	memset(localimages, 0, sizeof(ScriptImage *) * ts_count);
}

void Bayonne::allocate(timeslot_t max, ScriptCommand *cmd, timeslot_t overdraft)
{
	if(localimages)
	{
		delete[] localimages;
		localimages = NULL;
	}

	if(timeslots)
		delete[] timeslots;

	if(status)
		delete[] status;

	if(cmd)
	{
		server = cmd;
		path_prompts = server->getLast("prompts");
		path_tmpfs = server->getLast("tmpfs");
		path_tmp = server->getLast("tmp");
	}

	ts_limit = max;
	max += overdraft;

	status = new char[max + 1];
	timeslots = new BayonneSession*[max];
	memset(timeslots, 0, sizeof(BayonneSession*) * max);
	memset(status, 0x20, max);
	status[max] = 0;
	ts_count = max;
	ts_used = 0;
}

Bayonne::timeslot_t Bayonne::toTimeslot(const char *id)
{
	char buffer[16];
	char *cp;
	unsigned spid;
	timeslot_t ts;
	BayonneDriver *driver;
	BayonneSpan *span;
	BayonneSession *session;

	if(strchr(id, '-'))
	{
		ts = atoi(id);
		session = getSession(ts);
		if(!session)
			return NO_TIMESLOT;

		if(!stricmp(session->var_sid, id))
			return ts;

		return NO_TIMESLOT;
	}

	if(strchr(id, '+'))
	{
                ts = atoi(id);
                session = getSession(ts);
                if(!session)
                        return NO_TIMESLOT;

                if(!stricmp(session->var_tid, id))
                        return ts;

                return NO_TIMESLOT;
	}

	if(isdigit(*id))
	{
		ts = atoi(id);
		if(ts >= ts_used)
			return NO_TIMESLOT;
		return ts;
	}

	setString(buffer, sizeof(buffer), id);
	cp = strchr(buffer, '/');
	if(!cp)
		return NO_TIMESLOT;

	*(cp++) = 0;	
	driver = BayonneDriver::get(buffer);
	if(driver)
	{
		ts = atoi(cp);
		if(ts >= driver->getCount())
			return NO_TIMESLOT;

		return driver->getFirst() + ts;
	}
	spid = atoi(cp);
	cp = strchr(cp, ',');
	if(!cp)
		return NO_TIMESLOT;

	if(stricmp(buffer, "span"))
		return NO_TIMESLOT;

	ts = atoi(++cp);
	span = BayonneSpan::get(spid);
	if(!span)
		return NO_TIMESLOT;

	if(ts >= span->getCount())
		return NO_TIMESLOT;

	return span->getFirst() + ts;
}

BayonneSession *Bayonne::getSid(const char *id)
{
	timeslot_t ts = toTimeslot(id);
	return getSession(ts);
}

ScriptImage **Bayonne::getLocalImage(timeslot_t timeslot)
{
	if(!timeslots || !localimages)
		return NULL;

	if(timeslot >= ts_count)
		return NULL;

	return &localimages[timeslot];
}

BayonneSession *Bayonne::getSession(timeslot_t timeslot)
{
	if(!timeslots)
		return NULL;

	if(timeslot == NO_TIMESLOT)
		return NULL;

	if(timeslot >= ts_count)
		return NULL;

	return timeslots[timeslot];
}

const char *Bayonne::getRunLevel(void)
{
	const char *cp = strrchr(sla, ':');
	if(cp)
		return ++cp;
	else
		return "up";
}

void Bayonne::addConfig(const char *file)
{
	char path[128];
	snprintf(path, sizeof(path), "%s/%s",
		server->getLast("config"), file);

	server->setValue(file, path);
}

bool Bayonne::service(const char *id)
{
	bool rtn = false;
	ScriptImage *img;
	Name *scr = NULL;

	if(!server)
		return false;

	server->enter();

	if(!stricmp(id, "up") || !stricmp(id, "default"))
	{
		sla[0] = 0;
		rtn = true;
		goto done;
	}

	img = server->getActive();
	if(!img)
		goto done;

	snprintf(sla, sizeof(sla), "runlevel::%s", id);
	scr = img->getScript(id);

	if(scr && scr->access != scrPUBLIC)
		scr = NULL;

	if(!scr)
	{
		sla[0] = 0;
		slog.warn("%s: unknown or invalid service run level", id);
		goto done;
	}

	rtn = true;

done:
	server->leave();
	return rtn;
}

void Bayonne::down(void)
{
#ifndef	WIN32
	if(idle_count == idle_limit)
		raise(SIGTERM);
	else
		shutdown_flag = true;
#endif
}

ScriptCompiler *Bayonne::reload(void)
{
	static unsigned trap_id = 0;

	ScriptCompiler *img;

	if(!server)
		return NULL;

	snmptrap(trap_id, "bayonne server");
	if(!trap_id)
		++trap_id;
	
	compile_count = 0;

	reloading.writeLock();
	DynamicKeydata::reload();
	img = BayonneBinder::getCompiler();
	img->setValue("text.cr", "\r");
	img->setValue("text.nl", "\n");
	img->setValue("text.dl", "\n\n");
	img->setValue("text.qt", "\"");
	img->setValue("text.crnl", "\r\n");
	img->setValue("text.tab", "\t");
	BayonneConfig::rebuild(img);	
	ScriptBinder::rebuild(img);
	BayonneDriver::reload();
	img->commit();
	time(&reload_time);
	reloading.unlock();
	if(compile_count)
		image_loaded = true;
	return img;
}

static char digit[16] = {
	'0', '1', '2', '3',
        '4', '5', '6', '7',
        '8', '9', '*', '#',
        'a', 'b', 'c', 'd'};

char Bayonne::getChar(int dig)
{
	if(dig < 0 || dig > 15)
		return 0;

	return digit[dig];
}

int Bayonne::getDigit(char dig)
{
        int i;

	static char digit[16] = {
        	'0', '1', '2', '3',
        	'4', '5', '6', '7',
        	'8', '9', '*', '#',
        	'a', 'b', 'c', 'd'};

        dig = tolower(dig);
        for(i = 0; i < 16; ++i)
        {
                if(digit[i] == dig)
                        return i;
        }
        return -1;
}

ScriptImage *Bayonne::useImage(void)
{
	ScriptImage *img;

	if(!server || !image_loaded)
		return NULL;

	server->enter();
	img = server->getActive();
	if(!img)
	{
		server->leave();
		return NULL;
	}
	img->incRef();
	server->leave();
	return img;
}

void Bayonne::endImage(ScriptImage *img)
{
	if(!img)
		return;

	server->enter();
	img->decRef();
	if(!img->isRef() && img != server->getActive())
		delete img;
        server->leave();
}

unsigned long Bayonne::uptime(void)
{
	time_t now;

	if(!start_time)
		return 0;

	time(&now);
	return (long)(now - start_time);
}

void Bayonne::errlog(const char *level, const char *fmt, ...)    
{
        char *m;
        char buffer[256];
        va_list args;

        va_start(args, fmt);
        vsnprintf(buffer, sizeof(buffer) - 1, fmt, args);

        m = strchr(buffer, '\n');
        if(m)
                *m = 0;
 
        if(!stricmp(level, "debug"))
        {
                slog.debug() << buffer << endl;
                return;
        } 
        else if(!stricmp(level, "missing"))
                slog.warn() << buffer << endl;
        else if(!stricmp(level, "access"))
                slog.warn() << buffer << endl;
        else if(!stricmp(level, "notice"))
                slog.notice() << buffer << endl;
        else if(!strnicmp(level, "warn", 4))
        {
                level = "warn";
                slog.warn() << buffer << endl;
        } 
        else if(!strnicmp(level, "crit", 4))
        {
                level = "fatal";
                slog.critical() << buffer << endl;
        }
        else
                slog.error() << buffer << endl;

	if(Bayonne::server) 	                                                 
		Bayonne::server->errlog(level, buffer);
}
	
BayonneSession *Bayonne::startDialing(const char *dial, 
	const char *name, const char *caller, const char *display, 
	BayonneSession *parent, const char *manager, const char *proxy)
{
	ScriptImage *img;
	BayonneDriver *d = BayonneDriver::getTrunking();
	BayonneSession *session;
	BayonneSpan *span;
	Name *scr = NULL;
	Line *sel = NULL;
	char buffer[256];
	char *sp;
	const char *cp;
	Event event;
	unsigned idx = 0, count;
	timeslot_t pos;
	bool ext = false;

	if(!caller || !*caller)
	{
		display = "bayonne";
		caller = "none";
	}

	if(!display || !*display)
		display = caller;

	if(!strnicmp(dial, "pstn:", 5))
	{
		dial += 5;
		goto trunking;
	}

	if(!strnicmp(dial, "trunk:", 6))
	{
		dial += 6;
		goto trunking;
	}

	if(!strnicmp(dial, "tel:", 4))
		dial += 4;

	setString(buffer, sizeof(buffer), dial);
	sp = strchr(buffer, ':');
	if(!sp)
		sp = strchr(buffer, '/');
	if(!sp)
	{
		setString(buffer, sizeof(buffer), name);
		sp = strchr(buffer, ':');
		if(!sp)
			sp = strchr(buffer, '/');
	}
	
	if(!sp)
	{
		if(d)
			goto trunking;

		d = BayonneDriver::getPrimary();		
		if(!d)
			return NULL;
		goto protocol;
	}
	if(sp)
		*sp = 0;
	if(stricmp(buffer, "trunk"))
		d = BayonneDriver::get(buffer);

	if(!d)
		return NULL;

	if(strchr(dial, ':'))
		dial = strchr(dial, ':') + 1;
	else if(strchr(dial, '/'))
		dial = strchr(dial, '/') + 1;
	cp = d->getLast("type");
	if(cp && !stricmp(cp, "proto"))
		goto protocol;

trunking:
	img = useImage();
	if(strstr(name, "::"))
		scr = img->getScript(name);
	else
	{
		snprintf(buffer, sizeof(buffer), "select.%s", name);
		scr = (Name *)img->getPointer(buffer);
	}

	if(!scr || !scr->select || scr->access != scrPUBLIC)
	{
		endImage(img);
		return NULL;
	}

	sel = scr->select;
	if(!sel)
	{
		session	= d->getIdle();
		if(!session)
		{
			endImage(img);
			return NULL;
		}
		goto start;
	}

        while(sel)
        {
                idx = 0;
                cp = strchr(sel->cmd, '.');
                if(cp && !stricmp(cp, ".span"))
                     while(idx < sel->argc && NULL != (cp = sel->args[idx++]))
                {
                        span = BayonneSpan::get(atoi(cp));
                        if(!span)
                                continue;

                        pos = span->getFirst();
                        count = span->getCount();
                        while(count--)
                        {
                                session = getSession(pos++);
                                if(!session)
                                        continue;
                                session->enter();
                                if(session->isIdle())
                                        goto start;
                                session->leave();
                        }
                }
                else while(idx < sel->argc && NULL != (cp = sel->args[idx++]))
                {
                        session = getSid(cp);
                        if(!session)
                                continue;

                        session->enter();
                        if(session->isIdle())
                                goto start;
                        session->leave();
                }
                sel = sel->next;
	}
	endImage(img);
	return NULL;

protocol:	
	img = useImage();
	if(strstr(name, "::"))
		scr = img->getScript(name);
	else
	{
		snprintf(buffer, sizeof(buffer), "start-%s.%s", d->getName(), name);
		scr = (Name *)img->getPointer(buffer);
	}

	if(!scr || scr->access != scrPUBLIC)
	{
		endImage(img);
		return NULL;
	}

	session = d->getIdle();
	if(!session)
	{
		endImage(img);
		return NULL;
	}

	if(!strchr(dial, '@'))
		sel = scr->select;

	cp = d->getLast("urlprefix");
	if(cp && *cp && !strnicmp(dial, cp, strlen(cp)))
		dial += strlen(cp);

	if(d->isExternal(dial))
	{
		ext = true;
		sel = NULL;
		cp = "";
	}
	else if(sel)
	{
moreproxies:
		while(idx < sel->argc && sel->args[idx])
		{
			if(d->isReachable(sel->args[idx]))
				break;
			++idx;
		}
		if(idx >= sel->argc || !sel->args[idx])
		{
			if(!sel->next)
				return NULL;
			sel = sel->next;
			idx = 0;
			goto moreproxies;
		}
	}

	if(sel)
		snprintf(buffer, sizeof(buffer), "%s%s@%s", cp, dial, sel->args[idx]); 
	else
		snprintf(buffer, sizeof(buffer), "%s%s", cp, dial);

	dial = buffer;
	goto start;

start:
	memset(&event, 0, sizeof(event));
	if(ext)
		event.id = START_RINGING;
	else
		event.id = START_OUTGOING;
	event.start.img = img;
	event.start.scr = scr;
	event.start.dialing = dial;
	event.start.parent = parent;

	if(manager)
	{
		session->setConst("session.manager", manager);
		if(strchr(manager, '/') || strchr(manager, ':'))
			session->setConst("session.authorized", manager);
	}
	session->setConst("session.caller", caller);
	session->setConst("session.display", display);
	if(proxy)
		session->setConst("session.proxyauth", proxy);

        if(!session->postEvent(&event))
        {
                session->leave();
                endImage(img);
                return false;
        }

        return session;
}
