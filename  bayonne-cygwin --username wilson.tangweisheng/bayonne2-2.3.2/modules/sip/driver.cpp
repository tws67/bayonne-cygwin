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

#include "driver.h"

#ifdef  WIN32
#define CONFIG_FILES    "C:/Program Files/GNU Telephony/Bayonne Config"
#define	strdup	ost::strdup
#else
#include <private.h>
#endif

#ifndef	OSIP2_LIST_PTR
#define	OSIP2_LIST_PTR
#endif

namespace sipdriver {
using namespace ost;
using namespace std;

static Keydata::Define driver[] = {
	{"type", "proto"},
	{"proto", "sip"},
	{"driver", "exosip2"},
	{"stack", "0"},
	{"events", "128"},
	{"priority", "0"},
	{"encoding", "mulaw"},
	{"peering", "any"},
	{"framing", "20"},
	{"timer", "500"},
	{"payload", "0"},
	{"dtmf", "101"},
	{"inband", "false"},
	{"silence", "500"},
	{"duration", "3m"},
	{"invite", "60000"},
	{"pickup", "2000"},
	{"hangup", "250"},
	{"accept", "0"},
	{"audio", "60"},
	{"jitter", "2"},
	{"filler", "false"},
	{"urlprefix", "sip:"},
	{"immediate", "yes"},
	{"zeroconf", "yes"},
	{"regtype", "friend"},	// default registration type
	{"starting", "delayed"},
	{"threads", "5"},
	{"realm", "Bayonne"},
	{"hostid", "false"},
	{"nonce", "456345766asd345"},
#ifdef	WIN32		// use localhost default if not in conf for w32
	{"interface", "127.0.0.1:5070"},
#endif
        {NULL, NULL}};

static char *remove_quotes(char *c)
{
	char *o = c;
	char *d = c;
	if(*c != '\"')
		return c;

	++c;

	while(*c)
		*(d++) = *(c++); 

	*(--d) = 0;
	return o;
}

SIPThread *SIPThread::first = NULL;
SIPTraffic *SIPTraffic::first = NULL;
Driver Driver::sip;
unsigned Driver::instance = 0;

const char *Image::dupString(const char *str)
{
	char *np = (char *)alloc(strlen(str) + 1);
	strcpy(np, str);
	return (const char *)np;
}

bool Registry::isActive(void)
{
	if(!stricmp(type, "anon"))
		return true;

	if(!proxy)
		return false;

	if(!traffic->active)
		return false;

	if(traffic->getTimer() > 0)
		return true;

	return false;
}

void Registry::add(ScriptImage *img)
{
        Registry *first = (Registry *)img->getPointer("_sip_registry_");
        next = first;
        img->setPointer("_sip_registry_", this);
}

SIPThread::SIPThread() :
Thread(atoi(Driver::sip.getLast("priority")), atoi(Driver::sip.getLast("stack")) * 1024 + 8192)
{
	next = first;
	first = this;
}

void SIPThread::run(void)
{
	Driver::sip.run();
}

Driver::Driver() :
BayonneDriver(driver, "/bayonne/driver/sip", "sip", false),
BayonneZeroconf("_sip._udp", ZEROCONF_IPV4),
Thread(atoi(getLast("priority")), atoi(getLast("stack")) * 1024 + 8192),
Assoc()
{
	const char *cp, *p;
	char buf[1024];

	sip_port = 5060;

	cp = getLast("bind");
	if(!cp)
		cp = getLast("interface");

	if(!cp)
	{
		gethostname(buf, sizeof(buf) - 1);
		InetAddress host(buf);
		snprintf(buf, sizeof(buf), "%s:5060", 
			inet_ntoa(host.getAddress()));
		if(!stricmp(buf, "0.0.0.0:5060"))
			setString(buf, sizeof(buf), "127.0.0.1:5060");
		setValue("interface", buf);
		cp = getLast("interface");
	}
	else
		setValue("interface", cp);

	p = strchr(cp, ':');
	if(p)
		sip_port = atoi(++p);
	else
		sip_port = atoi(cp);		

	rtp_port = sip_port + 2;

	cp = getLast("rtp");
	if(cp)
		rtp_port = atoi(cp);

	silence = atoi(getLast("silence"));

	cp = getLast("immediate");
	send_immediate = false;

	switch(*cp)
	{
	case 'y':
	case 'Y':
	case 't':
	case 'T':
	case '1':
		send_immediate = true;
		break;
	}

	starting = START_DELAYED;

	cp = getLast("starting");
	switch(*cp)
	{
	case 'i':
	case 'I':
		starting = START_IMMEDIATE;
		break;
	case 'a':
	case 'A':
		starting = START_ACTIVE;
		break;
	}

	memset(&info, 0, sizeof(info));
	registry = false;
	addConfig("sip.conf");
	Driver::updateConfig(NULL);
}

void Driver::updateConfig(Keydata *key)
{
	timeout_t pickup = updatedMsecTimer("ack");
	timeout_t hangup = updatedMsecTimer("hangup");
	const char *dtmf = updatedString("dtmf");
	const char *cp = updatedString("encoding");

    if(!stricmp(cp, "a32"))
        cp = "adpcm";
    else if(!stricmp(cp, "a40"))
        cp = ".a40";
    else if(!stricmp(cp, "a24"))
        cp = ".a24";
    else if(!stricmp(cp, "a16"))
        cp = ".a16";
    info.encoding = Bayonne::getEncoding(cp);
    info.rate = rate8khz;
    cp = updatedString("rate");
    if(cp)
        info.rate = atol(cp);
    info.setFraming(updatedMsecTimer("framing"));

	audio_timer = updatedMsecTimer("audio");
	accept_timer = updatedMsecTimer("accept");

	if(!pickup)
		pickup = updatedMsecTimer("acktimer");
	if(!pickup)
		pickup = updatedMsecTimer("pickup");

	if(pickup < 10)
		pickup *= 1000;

	if(pickup)
		pickup_timer = pickup;

	if(hangup && audio_timer < hangup)
		hangup_timer = hangup - audio_timer;

	data_negotiate = updatedValue("payload");
	dtmf_negotiate = updatedValue("dtmf");
	if(dtmf && !stricmp(dtmf, "info"))
		info_negotiate = true;
	else if(dtmf && !stricmp(dtmf, "sipinfo"))
		info_negotiate = true;
	else
		info_negotiate = false;

	dtmf_inband = updatedBoolean("inband");
	data_filler = updatedBoolean("filler");
	jitter = updatedValue("jitter");
}

void *Driver::getMemory(size_t size)
{
	return alloc(size);
}

SIPTraffic *Driver::getTraffic(const char *id)
{
	SIPTraffic *traffic = (SIPTraffic *)getPointer(id);
	if(!traffic)
	{
		traffic = (SIPTraffic *)getMemory(sizeof(SIPTraffic));
		memset(traffic, 0, sizeof(SIPTraffic));
		setPointer(id, traffic);
		traffic->next = traffic->first;
		traffic->first = traffic;
	}
	++traffic->refcount;
	return traffic;
}

void Driver::reloadDriver(void)
{
	SIPTraffic *traffic = SIPTraffic::first;

	while(traffic)
	{
		if(traffic->refcount)
			--traffic->refcount;
		else 
			memset(traffic, 0, sizeof(SIPTraffic));
		traffic = traffic->next;
	}
}

bool Driver::reregister(const char *id, const char *uri, const char *secret, timeout_t expires)
{
    ScriptImage *img;
    Registry *reg;
    char buf[32];
    bool rtn = false;
    osip_message_t *msg = NULL;
	const char *cp;

    if(!strnicmp(id, "sip:", 4))
        id += 4;

	if(!deregister(id) || !secret)
		return false;
		
    snprintf(buf, sizeof(buf), "uri.%s", id);
    img = useImage();
    if(!img)
		return false;

    reg = (Registry *)img->getPointer(buf);
	if(!reg)
		goto final;

	if(!strnicmp(uri, "sip:", 4))
		setString(reg->traffic->address, sizeof(reg->traffic->address), uri);
	else
		snprintf(reg->traffic->address, sizeof(reg->traffic->address), 
			"sip:%s", uri);

	reg->proxy = strchr(reg->traffic->address, '@');
	if(reg->proxy)
		++reg->proxy;
	else
		goto final;

	reg->uri = reg->traffic->address;
	setString(reg->traffic->secret, sizeof(reg->traffic->secret), secret);
	reg->secret = reg->traffic->secret;
	reg->duration = expires;

    eXosip_lock();
    reg->regid = eXosip_register_build_initial_register((char *)reg->uri, (char *)reg->proxy, (char *)reg->contact, reg->duration / 1000, &msg);
    eXosip_unlock();

	if(reg->regid < 0)
		goto final;

    osip_message_set_header(msg, "Event", "Registration");
    osip_message_set_header(msg, "Allow-Events", "presence");
    cp = getLast("transport");
    if(cp && !stricmp(cp, "tcp"))
        eXosip_transport_set(msg, "TCP");
    else if(cp && !stricmp(cp, "tls"))
        eXosip_transport_set(msg, "TLS");
    eXosip_lock();
    eXosip_register_send_register(reg->regid, msg);
    eXosip_unlock();
	rtn = true;
	snprintf(buf, sizeof(buf), "sipreg.%d", reg->regid);
	img->setPointer(buf, reg);

final:
	if(reg && !rtn)
	{
		reg->proxy = NULL;
		reg->duration = 0;
		reg->regid = -1;
	}

	endImage(img);
	return rtn;
}

bool Driver::deregister(const char *id)
{
	ScriptImage *img;
	Registry *reg;
	char buf[32];
	bool rtn = false;

	if(!strnicmp(id, "sip:", 4))
		id += 4;

	snprintf(buf, sizeof(buf), "uri.%s", id);
	img = useImage();
	if(!img)
		return false;

	reg = (Registry *)img->getPointer(buf);
	if(!reg || strnicmp(reg->type, "dyn", 3))
		goto final;

	rtn = true;
	if(reg->regid < 0)
		goto final;

	updateExpires(reg, 0);
	snprintf(buf, sizeof(buf), "sipreg.%d", reg->regid);
	img->setPointer(buf, NULL);
	reg->regid = -1;
	reg->proxy = NULL;	
	reg->duration = 0;

final:
	endImage(img);
	return rtn;
}

bool Driver::isReachable(const char *proxy)
{
	char buf[128];
	bool rtn = false;
	ScriptImage *img;
	Registry *reg;
	char *p;

	if(!proxy)
		return false;

	if(strchr(proxy, '@'))
		proxy = strchr(proxy, '@') + 1;
	else if(!strnicmp(proxy, "sip:", 4))
		proxy += 4;

	img = useImage();
	if(!img)
		return false;

	snprintf(buf, sizeof(buf), "sip.%s", proxy);
	p = strrchr(buf, ':');
	if(p && !stricmp(p, ":5060"))
		*p = 0;
	
	reg = (Registry *)img->getPointer(buf);
	if(reg && reg->isActive())
	{
		if(!reg->call_limit || reg->traffic->active_calls < reg->call_limit)
			rtn = true;
	}

	endImage(img);
	return rtn;
}

bool Driver::isAuthorized(const char *userid, const char *secret)
{
	char buf[65];
	ScriptImage *img;
	Registry *reg;
	bool rtn = false;

	if(!userid)
		return false;

	if(strchr(userid, '@'))
		return false;

	img = useImage();
	if(!img)
		return false;

	if(!strnicmp(userid, "sip:", 4) || !strnicmp(userid, "sip/", 4))
		userid += 4;

	snprintf(buf, sizeof(buf), "uri.%s",  userid);
	reg = (Registry *)img->getPointer(buf);
	if(reg)
	{
		if(!stricmp(reg->type, "ext"))
			if(!strcmp(reg->secret, secret))
				rtn = true;
	}
	endImage(img);
	return rtn;
}	

bool Driver::isExternal(const char *ext)
{
	char buf[65];
	ScriptImage *img;
	Registry *reg;
	bool rtn = false;
	Name *scr;

	if(!ext)
		return false;

	if(strchr(ext, '@'))
		return false;

	img = useImage();
	if(!img)
		return false;

	if(!strnicmp(ext, "sip:", 4) || !strnicmp(ext, "sip/", 4))
		ext += 4;

	snprintf(buf, sizeof(buf), "uri.%s", ext);
	reg = (Registry *)img->getPointer(buf);
	if(!reg)
	{
		snprintf(buf, sizeof(buf), "sip::%s", ext);
		scr = img->getScript(buf);
		if(scr && scr->select)
		{
			snprintf(buf, sizeof(buf), "uri.%s", scr->select->args[0]);
			reg = (Registry *)img->getPointer(buf);
		}
	}
	if(reg && !strnicmp(reg->type, "ext", 3))
		rtn = true;
	endImage(img);
	return rtn;
}

bool Driver::isRegistered(const char *ext)
{
	char buf[65];
	ScriptImage *img;
	Registry *reg;
	bool rtn = false;
	Name *scr;

	if(!ext)
		return false;

	if(strchr(ext, '@'))
		return false;

	img = useImage();
	if(!img)
		return false;

	if(!strnicmp(ext, "sip:", 4) || !strnicmp(ext, "sip/", 4))
		ext += 4;

	snprintf(buf, sizeof(buf), "uri.%s", ext);
	reg = (Registry *)img->getPointer(buf);
	if(!reg)
	{
		snprintf(buf, sizeof(buf), "sip::%s", ext);
		scr = img->getScript(buf);
		if(scr && scr->select)
		{
			snprintf(buf, sizeof(buf), "uri.%s", scr->select->args[0]);
			reg = (Registry *)img->getPointer(buf);
		}
	}
	if(reg && !strnicmp(reg->type, "ext", 3))
		rtn = reg->isActive();
	else if(reg && !stricmp(reg->type, "gw"))
		rtn = reg->isActive();
	else if(reg && !strnicmp(reg->type, "dyn", 3))
		rtn = reg->isActive();
	endImage(img);
	return rtn;
}

bool Driver::isAvailable(const char *ext)
{
	char buf[65];
	ScriptImage *img;
	Registry *reg;
	bool rtn = false;
	Name *scr;
	bool regext = false, gw = false;

	if(!ext)
		return false;

	if(strchr(ext, '@'))
		return false;

	img = useImage();
	if(!img)
		return false;

	if(!strnicmp(ext, "sip:", 4) || !strnicmp(ext, "sip/", 4))
		ext += 4;

	snprintf(buf, sizeof(buf), "uri.%s", ext);
	reg = (Registry *)img->getPointer(buf);
	if(!reg)
	{
		snprintf(buf, sizeof(buf), "sip::%s", ext);
		scr = img->getScript(buf);
		if(scr && scr->select)
		{
			snprintf(buf, sizeof(buf), "uri.%s", scr->select->args[0]);
			reg = (Registry *)img->getPointer(buf);
		}
	}
	if(reg && !strnicmp(reg->type, "ext", 3))
		regext = true;
	else if(reg && !stricmp(reg->type, "gw"))
		gw = true;
	if((regext || gw) && reg->isActive())
	{
		if(!reg->call_limit)
			rtn = true;
		else if(reg->call_limit > reg->traffic->active_calls)
			rtn = true;
		if(regext)
			switch(reg->traffic->presence)
			{
			case SIPTraffic::P_BUSY:
			case SIPTraffic::P_AWAY:
			case SIPTraffic::P_DND:
				rtn = false;
			default:
				break;
			}
	}
	endImage(img);
	return rtn;
}

Registry *Driver::getAnonymous(ScriptImage *img, const char *hid, const char *dialed)
{
	Registry *reg;
	char buf[128];

	if(dialed && *dialed)
	{
		snprintf(buf, sizeof(buf), "uri.%s", dialed);
		reg = (Registry *)img->getPointer(buf);
		if(reg)
			return reg;
	}

	if(!hid || !*hid)
		return NULL;

	snprintf(buf, sizeof(buf), "sipanon.%s", hid);
	reg = (Registry *)img->getPointer(buf);
	if(!reg)
		reg = (Registry *)img->getPointer("sipanon.*");
	return reg;
}

bool Driver::isLocalName(const char *hid)
{
	char names[128];
	char *tok, *np;

	if(!getString("localnames", names, sizeof(names)))
		return true;

	if(!hid)
		return true;

	np = strtok_r(names, " ,;:\t\r\n", &tok);
	while(np)
	{
		if(!stricmp(np, hid))
			return true;

		np = strtok_r(NULL, " ,;:\t\r\n", &tok);
	}
	return false;
}

unsigned Driver::getRegistration(regauth_t *data, unsigned count, const char *id)
{
	unsigned idx = 0;
	ScriptImage *img = useImage();
	Registry *reg = (Registry *)img->getPointer("_sip_registry_");
	bool ext, gw, anon;
	const char *uid;

	while(idx < count && reg)
	{
		anon = false;
		ext = false;
		gw = false;
		if(!strnicmp(reg->type, "ext", 3))
			ext = true;
		else if(!stricmp(reg->type, "gw"))
			gw = true;
		else if(!stricmp(reg->type, "anon"))
			anon = true;

		uid = reg->localid;
		if(!uid)
			uid = reg->userid;
		if(!uid && !strnicmp(reg->type, "dyn", 3))
			uid = "unknown";

		if(id && !uid)
			goto skip;

		if(id && stricmp(id, uid))
			goto skip;

		if(anon)
			data[idx].remote = "*";
		else if((ext || gw) && !reg->traffic->active)
			data[idx].remote = "-";
		else if(!strnicmp(reg->proxy, "sip:", 4))
			data[idx].remote = reg->proxy + 4;
		else
			data[idx].remote = reg->proxy;
		data[idx].userid = uid;
		data[idx].type = reg->type;
		if(!strnicmp(reg->type, "dyn", 3))
		{
			if(reg->proxy)
				data[idx].status = "active";
			else
			{
				data[idx].status = "unused";
				data[idx].remote = "none";
			}
		}
		else if(anon)
			data[idx].status = "active";
		else if((ext || gw) && !reg->traffic->active)
			data[idx].status = "offline";
		else if(ext && reg->traffic->getTimer() > 0)
		{
			switch(reg->traffic->presence)
			{
			case SIPTraffic::P_AWAY:
				data[idx].status = "away";
				break;
			case SIPTraffic::P_BUSY:
				data[idx].status = "busy";
				break;
			case SIPTraffic::P_DND:
				data[idx].status = "dnd";
				break;
			default:
				if(!reg->call_limit || reg->traffic->active_calls < reg->call_limit)
					data[idx].status = "ready";
				else
					data[idx].status = "busy";
			}
		}
		else if(gw && reg->traffic->getTimer() > 0)
		{
			if(!reg->call_limit || reg->call_limit > reg->traffic->active_calls)
				data[idx].status = "online";
			else
				data[idx].status = "busy";
		}
		else if(!reg->proxy || !reg->userid)
			data[idx].status = "invalid";
		else if(!reg->traffic->active)
			data[idx].status = "failed";
		else if(reg->traffic->getTimer() > 0)
		{
			if(!reg->call_limit || reg->traffic->active_calls < reg->call_limit)
				data[idx].status = "active";
			else
				data[idx].status = "full";
		}
		else
			data[idx].status = "expired";
		data[idx].updated = reg->traffic->updated;
		data[idx].attempts_iCount = reg->traffic->call_attempts.iCount;
		data[idx].attempts_oCount = reg->traffic->call_attempts.oCount;
		data[idx].complete_iCount = reg->traffic->call_complete.iCount;
		data[idx].complete_oCount = reg->traffic->call_complete.oCount;
		data[idx].active_calls = reg->traffic->active_calls;
		data[idx].call_limit = reg->call_limit;
		++idx;
skip:
		reg = reg->next;
	}

	endImage(img);
	return idx;
}

bool Driver::getAuthentication(Session *s, osip_message_t *resp)
{
	char buf[256];
	ScriptImage *img = s->getImage();
	const char *cp = s->getSymbol("session.registry");
	char *p;
	const char *proxy = s->getSymbol("session.proxyauth");
	Registry *reg;
	bool rtn = false;
	const char *realm = NULL;
        osip_proxy_authenticate_t *prx_auth = NULL;
        osip_www_authenticate_t *www_auth = NULL;


        if(resp)
        {
		prx_auth = (osip_proxy_authenticate_t*)osip_list_get(OSIP2_LIST_PTR resp->proxy_authenticates, 0);
                www_auth = (osip_proxy_authenticate_t*)osip_list_get(OSIP2_LIST_PTR resp->www_authenticates,0);
        }

	if(proxy && strchr(proxy, ':') && (prx_auth != NULL|www_auth != NULL))
	{
        if(prx_auth)
                realm = osip_proxy_authenticate_get_realm(prx_auth);
        else if(www_auth)
                realm = osip_www_authenticate_get_realm(www_auth);

		setString(buf, sizeof(buf), s->getSymbol("session.caller"));
		p = strchr(buf, ':');
		if(p)		
			*(p++) = 0;

		eXosip_lock();
		if(eXosip_add_authentication_info(buf, buf, p, NULL, realm))
			slog.error("sip: proxy authentication failed");
		else
			rtn = true;
		eXosip_unlock();
		goto done;
	}

	if(!cp)
		goto done;

	reg = (Registry *)img->getPointer(cp);
	if(!reg)
		goto done;

        if(prx_auth)
                realm = osip_proxy_authenticate_get_realm(prx_auth);
        else if(www_auth)
                realm = osip_www_authenticate_get_realm(www_auth);
	else
		realm = reg->realm;

	if(!reg->userid)
		goto done;

	setString(buf, sizeof(buf), reg->userid);
	p = strchr(buf, '@');
	if(p)
		*(p++) = 0;

	eXosip_lock();
	if(eXosip_add_authentication_info(buf, buf, reg->secret, NULL, realm))
		slog.error("sip: authentication failed; host=%s", reg->proxy);
	else
	{
		rtn = true;
	        slog.debug("sip: authenticating to %s using %s", reg->proxy, reg->userid);
	}
	eXosip_unlock();
done:
	return rtn;
}

void Driver::setAuthentication(Registry *reg, osip_message_t *resp)
{
        osip_proxy_authenticate_t *prx_auth = NULL;
        osip_www_authenticate_t *www_auth = NULL;
	char uname[65];
	const char *realm = reg->realm;
	char *p;

	if(resp)
	{
		prx_auth = (osip_proxy_authenticate_t*)osip_list_get(OSIP2_LIST_PTR resp->proxy_authenticates, 0);
        	www_auth = (osip_proxy_authenticate_t*)osip_list_get(OSIP2_LIST_PTR resp->www_authenticates,0);
	}

	if(prx_auth)
		realm = osip_proxy_authenticate_get_realm(prx_auth);
	else if(www_auth)
		realm = osip_www_authenticate_get_realm(www_auth);

	setString(uname, sizeof(uname), reg->userid);
	p = strchr(uname, '@');
	if(p)
		*p = 0;

	eXosip_lock();
	eXosip_add_authentication_info(uname, uname, reg->secret, NULL, realm);
	eXosip_unlock();
}
		
void Driver::startDriver(void)
{
	char cbuf[65];
	unsigned tries = 0;
	char *p;
	const char *cp;
	unsigned max_count = ts_limit;
	cp = getLast("timeslots");
	int rtn;

	exiting = false;
	timeslot = ts_used;
	msgport = new BayonneMsgport(this);
	unsigned threads = 0;
	SIPThread *thread;
	
	if(cp)
		max_count = atoi(cp);

	while(ts_used < ts_limit && max_count--)
		new Session(ts_used);

	count = ts_used - timeslot;
	if(!count)
		return;

	eXosip_init();

	for(;;)
	{
		cp = getLast("transport");
		if(!cp)
			cp = "udp";

		if(!stricmp(cp, "tcp"))
		{
			zeroconf_type = "_sip._tcp";
			rtn = eXosip_listen_addr(IPPROTO_TCP, NULL, sip_port, AF_INET, 0);
		}
		else if(!stricmp(cp, "tls"))
		{
			zeroconf_type = "_sip._tcp";
			rtn = eXosip_listen_addr(IPPROTO_TCP, NULL, sip_port, AF_INET, 1);
		}
		else
		{
			zeroconf_type = "_sip._udp";
			rtn = eXosip_listen_addr(IPPROTO_UDP, NULL, sip_port, AF_INET, 0);
		}

		if(!rtn)
			break;

		slog.error("sip: failed to bind %d", sip_port);
		if(tries++ > 3)
		{
			eXosip_quit();
			return;
		}
		sip_port += 10;
		if(!getLast("rtp"))
			rtp_port += 10;
	}

	cp = getLast("zeroconf");
	switch(*cp)
	{
	case '1':
	case 'y':
	case 'Y':
	case 't':
	case 'T':
		zeroconf_port = sip_port;
	}

	threads = atoi(getLast("threads"));

	cp = getLast("interface");
	if(!cp)
		cp = "all";

	setString(cbuf, sizeof(cbuf) - 8, getLast("interface"));
	p = strrchr(cbuf, ':');
	snprintf(p + 1, 8, "%d", sip_port);
	setValue("interface", cbuf);
	*p = 0;
	setValue("localip", cbuf);
	
        osip_trace_initialize_syslog(TRACE_LEVEL0, "bayonne");
/*        osip_trace_enable_level(TRACE_LEVEL1);
        osip_trace_enable_level(TRACE_LEVEL2);
        osip_trace_enable_level(TRACE_LEVEL3);
        osip_trace_enable_level(TRACE_LEVEL4);
        osip_trace_enable_level(TRACE_LEVEL5);
        osip_trace_enable_level(TRACE_LEVEL6);
        osip_trace_enable_level(TRACE_LEVEL7);
*/
	switch(info.encoding)
	{
	case mulawAudio:
		if(!data_negotiate)
			setValue("payload", "0");
		snprintf(cbuf, sizeof(cbuf), "%d PCMU/%ld", 
			data_negotiate, info.rate);
		break;
	case alawAudio:
		if(!data_negotiate)
		{
			data_negotiate = 8;
			setValue("payload", "8");
		}
		snprintf(cbuf, sizeof(cbuf), "%d PCMA/%ld",
			data_negotiate, info.rate);
		break;
	case pcm16Mono:
		snprintf(cbuf, sizeof(cbuf), "%d L16/%ld",
			data_negotiate, info.rate);
		break;
	case pcm16Stereo:
		snprintf(cbuf, sizeof(cbuf), "%d L16_2CH/%ld",
			data_negotiate, info.rate);
		break;
	case speexVoice:
		if(!data_negotiate)
		{
			data_negotiate = 97;
			setValue("payload", "97");
		}
		snprintf(cbuf, sizeof(cbuf), "%d SPEEX/%ld",
			data_negotiate, info.rate);
		break;
	case g721ADPCM:
		if(!data_negotiate)
		{
			data_negotiate = 5;
			setValue("payload", "5");
		}
		snprintf(cbuf, sizeof(cbuf), "%d G726-32/%ld",
			data_negotiate, info.rate);
		break;
	case g723_2bit:
                if(!data_negotiate)
                {
                        data_negotiate = 94;
                        setValue("payload", "94");
                }
                snprintf(cbuf, sizeof(cbuf), "%d G726-16/%ld",
                        data_negotiate, info.rate);
                break;
	case g723_3bit:
		if(!data_negotiate)
		{
			data_negotiate = 96;
			setValue("payload", "96");
		}
		snprintf(cbuf, sizeof(cbuf), "%d G726-24/%ld",
			data_negotiate, info.rate);
		break;
        case g723_5bit:
                if(!data_negotiate)
                {
                        data_negotiate = 95;
                        setValue("payload", "95");
                }
                snprintf(cbuf, sizeof(cbuf), "%d G726-40/%ld",
                        data_negotiate, info.rate);
                break;
	case gsmVoice:
		if(!data_negotiate)
		{
			data_negotiate = 3;
			setValue("payload", "3");
		}
		snprintf(cbuf, sizeof(cbuf), "%d GSM/%ld",
			data_negotiate, info.rate);
		break;
	default:
		slog.error("sip: unsupported sdp audio encoding");
		return;
	}

	slog.debug("sip: adding sdp encoding %s", cbuf);
	info.annotation = strdup(cbuf);
	eXosip_set_user_agent("GNU Bayonne");
	
#ifdef	EXOSIP2_OPTION_SEND_101
	int val=1;
	eXosip_set_option(EXOSIP_OPT_DONT_SEND_101, &val); 
#endif

	slog.debug("sip: bound to port %d", sip_port);
	msgport->start();
	Thread::start();

	while(threads-- > 1)
	{
		thread = new SIPThread();
		thread->start();
	}

	Thread::sleep(100);
	BayonneDriver::startDriver();	

	switch(starting)
	{
	case START_DELAYED:
		relistIdle();
	default:
		break;
	}
}

void Driver::stopDriver(void)
{
	ScriptImage *img;
	Registry *reg;
	SIPThread *thread;
	unsigned c = count;
	timeslot_t ts = timeslot;
	BayonneSession *s;
	bool extgw;

	if(running)
	{
		exiting = true;
		img = useImage();
		if(img)
		{
			reg = (Registry *)img->getPointer("_sip_registry_");
			while(reg)
			{
				extgw = false;
				if(!strnicmp(reg->type, "ext", 3))
					extgw = true;
				else if(!stricmp(reg->type, "gw"))
					extgw = true;
				else if(!stricmp(reg->type, "peer"))
					extgw = true;
				if(!extgw)
					updateExpires(reg, 0);
				reg = reg->next;
			}
			endImage(img);
		}
		thread = SIPThread::first;
		while(thread)
		{
			thread->terminate();
			thread = thread->next;
		}
		SIPThread::first = NULL;
		Driver::sip.terminate();
		eXosip_quit();

		while(c--)
		{
			s = getSession(ts++);
			if(!s)
				continue;
//			s->makeIdle();
		}
		
		Thread::sleep(120);
		BayonneDriver::stopDriver();
	}
}

const char *Driver::assignScript(ScriptImage *img, Line *line)
{
	Name *scr = img->getCurrent();
	char buffer[1024];
	char cbuf[65];
	char uname[65];
	char rname[76];
	unsigned idx = 0;
	const char *cp;
	char *ep, *contact;
	Registry *reg;
	const char *host, *port;

	for(;;)
	{
		cp = line->args[idx++];
		if(!cp && idx > 1)
			break;

		reg = (Registry *)img->getMemory(sizeof(Registry));
		memset(reg, 0, sizeof(Registry));
		reg->add(img);
		reg->proxy = NULL;
		reg->protocol = "sip";
		reg->iface = "none";
		reg->uri = cp;
		reg->hostid = "none";
		reg->userid = "none";
		reg->realm = NULL;
		reg->type = "none";
		reg->dtmf = NULL;
		reg->scr = scr;
		reg->secret = NULL;
		reg->line = line;
		reg->duration = 0;		

		setString(cbuf, sizeof(cbuf), scr->name);
		ep = strchr(cbuf, ':');
		if(ep)
		{
			*(ep++) = 0;
			*ep = '.';
		}
		else
			ep = "";

                host = strchr(cp, '@');
                if(host)
                {
                        ++host;
                        ep = strrchr((char *)host, ':');
                        if(ep)
                                *ep = 0;
                        reg->address = host;
                        setString(uname, sizeof(uname), cp);
                        ep = strchr(uname, '@');
                        if(ep)
                                *ep = 0;
                }
                else if(cp)
		{
			reg->address = NULL;
			setString(uname, sizeof(uname), cp);
		}
		else
                {
                        reg->address = NULL;
                        snprintf(uname, sizeof(uname), "%s%s", cbuf, ep);
                }

		snprintf(rname, sizeof(rname), "uri.%s", uname);
                host = getLast("interface");
                if(reg->address)
                {
                        port = strrchr(host, ':');
                        host = reg->address;
                }
                else
                        port = "";

                snprintf(buffer, sizeof(buffer), "sip:%s@%s%s",
                        uname, host, port);

		ep = strrchr(buffer, ':');
		if(ep && !stricmp(ep, ":5060"))
			*ep = 0;	
		contact = (char *)img->getMemory(strlen(buffer) + 1);
		strcpy(contact, buffer);
		reg->contact = (const char *)contact;
		if(!reg->uri)
			reg->uri = reg->contact;
                reg->localid = (char *)img->getMemory(strlen(uname) + 1);
                strcpy((char *)reg->localid, uname);
		slog.debug("sip: assigning %s to %s", scr->name, buffer);
		reg->traffic = Driver::sip.getTraffic(rname);
		reg->traffic->active = true;
		img->setPointer(rname, reg);
		if(!cp)
			break;
	}
	return "";
}

void Driver::requestAuth(eXosip_event_t *sevent)
{
	osip_message_t *reply = NULL;
        const char *realm = getLast("realm");
        const char *nonce = getLast("nonce");
	char buf[256];

	eXosip_lock();
	eXosip_call_build_answer(sevent->tid, 407, &reply);
        snprintf(buf, sizeof(buf),
                "Digest realm=\"%s\", nonce=\"%s\"", realm, nonce);
        osip_message_set_header(reply, "Proxy-Authenticate", buf);
        eXosip_call_send_answer(sevent->tid, 407, reply);
        eXosip_unlock();
}

bool Driver::getRegistry(eXosip_event_t *sevent, Registry **reg, ScriptImage *img)
{
	char a1[256], a2[256], a1_hash[256], a2_hash[256], resp[256], resp_hash[256];
	osip_proxy_authorization_t *auth;
	char buf[256];

	*reg = NULL; 

	if(osip_message_get_proxy_authorization(sevent->request, 0, &auth) != 0)
		return true;

	if(!auth)
		return true;

	if(!auth->username)
		return false;

        remove_quotes(auth->username);
        remove_quotes(auth->uri);
        remove_quotes(auth->nonce);
        remove_quotes(auth->response);
        snprintf(buf, sizeof(buf), "uri.%s", auth->username);
	*reg = (Registry *)img->getPointer(buf);

	if(!*reg)
		return false;

	snprintf(a1, sizeof(a1), "%s:%s:%s",
                auth->username, getLast("realm"), (*reg)->secret);
        snprintf(a2, sizeof(a2), "INVITE:%s", auth->uri);
        md5_hash(a1_hash, a1);
        md5_hash(a2_hash, a2);
        snprintf(resp, sizeof(resp), "%s:%s:%s",
                a1_hash, auth->nonce, a2_hash);
        md5_hash(resp_hash, resp);
	if(!stricmp(auth->response, resp_hash))
		return true;
	return false;
}

void Driver::updateExpires(Registry *reg, timeout_t expires)
{
	osip_message_t *msg = NULL;

	reg->duration = expires;
	eXosip_lock();
	eXosip_register_build_register(reg->regid, (int)(expires / 1000), &msg);
	if(msg)
		eXosip_register_send_register(reg->regid, msg);
	eXosip_unlock();
}

void Driver::publishRequest(eXosip_event_t *sevent, const char *target)
{
	osip_authorization_t *auth = NULL;
	char buffer[65];
	Registry *reg;
	ScriptImage *img;

	if(!osip_message_get_authorization(sevent->request, 0, &auth))
		goto registerAuth;

	if(!target)
		goto registerAuth;

	snprintf(buffer, sizeof(buffer), "uri.%s", target);
	server->enter();
	img = server->getActive();
	reg = (Registry *)img->getPointer(buffer);	
	if(reg && reg->insecure_publish)
	{
		publishUpdate(sevent, reg);
		server->leave();
		return;
	}

	server->leave();
registerAuth:
	registerRequest(sevent);
}

void Driver::publishUpdate(eXosip_event_t *sevent, Registry *reg)
{
	const char *tmp;
	bool online = true;
	osip_body_t *mbody = NULL;
	osip_message_get_body(sevent->request, 0, &mbody);
	bool presence = false;

	if(!mbody)
		return;

	tmp = mbody->body;

	while(*tmp)
	{
		if(!strnicmp(tmp, "<presence", 9))
			presence = true; 

		if(!strnicmp(tmp, "online", 6) && presence)
			break;

		else if(!strnicmp(tmp, "busy", 4) && presence)
		{
			reg->traffic->presence = SIPTraffic::P_BUSY;
			online = false;
			break;
		}
		else if(!strnicmp(tmp, "away", 4) && presence)
		{
			reg->traffic->presence = SIPTraffic::P_AWAY;
			online = false;
			break;
		}
		++tmp;
	}

	if(online && presence)
		reg->traffic->presence = SIPTraffic::P_ONLINE;
}

void Driver::requestOptions(eXosip_event_t *sevent)
{
	osip_message_t *reply = NULL;

	eXosip_lock();
	if(eXosip_options_build_answer(sevent->tid, 200, &reply) == 0)
		eXosip_options_send_answer(sevent->tid, 200, reply);
	eXosip_unlock();
}
	
void Driver::registerRequest(eXosip_event_t *sevent)
{
	char a1[256], a2[256], a1_hash[256], a2_hash[256], resp[256], resp_hash[256];
	osip_message_t *reply = NULL;
	osip_contact_t *contact = NULL;
	osip_authorization_t *auth = NULL;
	osip_header_t *exp = NULL;
	osip_uri_param_t *param = NULL;
	const char *realm = getLast("realm");
	const char *nonce = getLast("nonce");
	char buf[256];
	ScriptImage *img;
	Registry *reg;	
	int pos = 0;
	char *cp;
	InetAddress addr;
	const char *extgw = NULL;

	if(osip_message_get_authorization(sevent->request, 0, &auth) != 0)
		goto challenge;

	if(!auth || !auth->username || !auth->response)
		goto challenge;

	remove_quotes(auth->username);
	remove_quotes(auth->uri);
	remove_quotes(auth->nonce);
	remove_quotes(auth->response);
	snprintf(buf, sizeof(buf), "uri.%s", auth->username);

	server->enter();
	img = server->getActive();
	reg = (Registry *)img->getPointer(buf);	
	if(reg && !strnicmp(reg->type, "ext", 3))
		extgw = "external";
	else if(reg && !stricmp(reg->type, "gw"))
		extgw = "gateway";
	else if(reg && !stricmp(reg->type, "peer"))
		extgw = "peer";

	if(!reg || extgw == NULL)
	{
fail401:

		server->leave();
		slog.debug("rejecting invalid %s", auth->username); 
		eXosip_lock();
		eXosip_message_build_answer(sevent->tid, 401, &reply);
		eXosip_message_send_answer(sevent->tid, 401, reply);
		eXosip_unlock();		
		return;
	}
	snprintf(a1, sizeof(a1), "%s:%s:%s",
		auth->username, getLast("realm"), reg->secret);
	snprintf(a2, sizeof(a2), "REGISTER:%s", auth->uri);
	md5_hash(a1_hash, a1);
	md5_hash(a2_hash, a2);
	snprintf(resp, sizeof(resp), "%s:%s:%s",
		a1_hash, auth->nonce, a2_hash);
	md5_hash(resp_hash, resp);
	if(stricmp(auth->response, resp_hash))
		goto fail401;

	while(osip_list_eol(OSIP2_LIST_PTR sevent->request->contacts, pos) == 0)
	{
		contact = (osip_contact_t *)osip_list_get(OSIP2_LIST_PTR sevent->request->contacts, pos++);
		if(contact && contact->url)
			break;
	}

	if(!contact || !contact->url)
		goto fail401;

	if(!stricmp(reg->type, "peer"))
		goto sip200;

	addr = contact->url->host;
	snprintf(buf, sizeof(buf), "%s:%s", 
		inet_ntoa(addr.getAddress()), contact->url->port);

	if(!reg->proxy || stricmp(buf, reg->proxy))
	{
		setString(reg->traffic->address, sizeof(reg->traffic->address), buf);
		reg->proxy = reg->traffic->address;
	}

	if(!reg->userid || stricmp(contact->url->username, reg->userid))
	{
		cp = (char *)img->getMemory(strlen(contact->url->username) + 1);
		strcpy(cp, contact->url->username);
		reg->userid = cp;
	}

sip200:
	if(MSG_IS_PUBLISH(sevent->request))
	{
		publishUpdate(sevent, reg);
		goto answer200;
	}
	osip_message_get_expires(sevent->request, 0, &exp);
	if(contact)
		osip_contact_param_get_byname(contact, "expires", &param);

	if(exp && exp->hvalue)
		pos = osip_atoi(exp->hvalue);
	else if(param && param->gvalue)
		pos = osip_atoi(param->gvalue);
	else
		pos = 600;

	if(pos < 0)
		pos = 600;

	if(pos)
	{
		time(&reg->traffic->updated);
		if(!reg->traffic->getTimer())
			reg->traffic->presence = SIPTraffic::P_ONLINE;

		reg->traffic->setTimer(pos * 1000);
		reg->traffic->active = true;
		slog.debug("registered %s %s from %s; expires=%d", 
			extgw, auth->username, buf, pos);
	}
	else
	{
		reg->traffic->updated = 0l;
		reg->traffic->active = false;
		reg->traffic->setTimer(0);
		reg->traffic->presence = SIPTraffic::P_AVAILABLE;
		slog.debug("unregister %s %s from %s", 
			extgw, auth->username, buf);
	}
answer200:
	server->leave();

        eXosip_lock();
        eXosip_message_build_answer(sevent->tid, 200, &reply);
        eXosip_message_send_answer(sevent->tid, 200, reply);
	eXosip_unlock();
	return;

challenge:
	eXosip_lock();
	eXosip_message_build_answer(sevent->tid, 401, &reply);
	snprintf(buf, sizeof(buf), 
		"Digest realm=\"%s\", nonce=\"%s\"", realm, nonce);
	osip_message_set_header(reply, "WWW-Authenticate", buf);
	eXosip_message_send_answer(sevent->tid, 401, reply);
	eXosip_unlock();
}

#ifdef	SCRIPT_RIPPLE_KEYDATA
#define	regKeyword(l, k)	ScriptChecks::findKeyword(img, l, k)
#else
#define	regKeyword(l, k)	ScriptChecks::findKeyword(l, k)
#endif

const char *Driver::registerScript(ScriptImage *img, Line *line)
{
	Name *scr = img->getCurrent();
	Registry *reg = (Registry *)img->getMemory(sizeof(Registry));
	Registry *preg;
	const char *dur = regKeyword(line, "timeout");
	const char *insecure = regKeyword(line, "insecure");
	const char *lcp, *cp;
	char buf[1024];
	char nbuf[32];
	unsigned len, pos;
	char *p, *sp, *tok;
	char *uri;
	long mult = 1000;
	osip_message_t *msg = NULL; 
	InetAddress host;
	bool ext = false, gw = false;
	Image *ip = (Image *)img;
	const char *gateways = NULL;

	memset(reg, 0, sizeof(Registry));

	if(!strnicmp(line->cmd, "ext", 3))
		ext = true;
	else if(!stricmp(line->cmd, "gateway"))
		gw = true;

	if(ext || gw)
	{
		if(!ScriptChecks::useKeywords(line, 
			"=reconnect=peering=secret=encoding=framing=dtmf=user=count"
			"=info=answer=accept=hold=transfer=update=group"))
				return "invalid keywords used for sip external";
	}
	else if(!ScriptChecks::useKeywords(line, 
		"=uri=timeout=server=proxy=userid=secret=type=realm=dtmf=reconnect"
		"=limit=public=encoding=framing=local=caller=service=peering=answer=gateways"
		"=accept=hold=transfer=update"))
			return "invalid keywords for sip registry";

	reg->protocol = "sip";
	reg->iface = getLast("interface");
	reg->uri = regKeyword(line, "uri");
	reg->address = regKeyword(line, "public");
	reg->hostid = regKeyword(line, "proxy");
	reg->userid = regKeyword(line, "userid");
	reg->secret = regKeyword(line, "secret");
	reg->realm = regKeyword(line, "realm");
	reg->type = regKeyword(line, "type");
	reg->dtmf = regKeyword(line, "dtmf");
	reg->peering = regKeyword(line, "peering");
	reg->encoding = regKeyword(line, "encoding");
	reg->caller = regKeyword(line, "caller");
	reg->reconnect = regKeyword(line, "reconnect");
	reg->answer = regKeyword(line, "answer");
	reg->update = regKeyword(line, "update");
	reg->hold = regKeyword(line, "hold");
	reg->transfer = regKeyword(line, "transfer");
	reg->group = regKeyword(line, "group");
	gateways = regKeyword(line, "gateways");

	if(reg->type && (!strnicmp(reg->type, "rem", 3)
		|| !stricmp(reg->type, "opx")))
	{
		ext = true;
		reg->type = "ext";
		if(!reg->reconnect)
			reg->reconnect = getLast("remote.reconnect");
		if(!reg->peering)
			reg->peering = "no";
		cp = getLast("remote.encoding");
		if(cp && !reg->encoding && reg->reconnect && stristr(reg->reconnect, cp))
			reg->encoding = cp;
		if(!reg->encoding)
			reg->encoding = getLast("message.encoding");
	}

	if(!stricmp(line->cmd, "_keydata_"))
	{
		if(reg->type && !strnicmp(reg->type, "ext", 3))
			ext = true;
		else if(reg->type && !stricmp(reg->type, "gateway"))
			gw = true;
	}
		
	if(!reg->group)
		reg->group = "nobody";

	cp = regKeyword(line, "accept");
	if(cp)
		reg->accept = atol(cp);

	if(ext || gw)
	{
		if(!reg->reconnect && !reg->peering && !reg->encoding)
			reg->reconnect = img->getLast("local.reconnect");
		if(!reg->encoding && !reg->peering && reg->reconnect)
		{
			cp = img->getLast("local.encoding");
			if(cp && stristr(reg->reconnect, cp))
			{
				reg->encoding = cp;
				reg->peering = "yes";
			}
		}
		if(!reg->encoding)
			reg->encoding = img->getLast("message.encoding");
			
		reg->localid = regKeyword(line, "user");
	}
	if(gw && !reg->localid)
		reg->localid = regKeyword(line, "userid");

	if(!ext)
	{
		reg->service = regKeyword(line, "service");
		reg->localid = regKeyword(line, "local");

		if(reg->service && !reg->localid)
			reg->localid = reg->service;
	}

	if(!gw && reg->type && !stricmp(reg->type, "gateway"))
		gw = true;

	if(!gw && reg->type && !stricmp(reg->type, "gw"))
		gw = true;

	if(gw)
	{
		gateways = NULL;
		reg->type = "gw";
	}

	if(!reg->service)
	{
		reg->service = scr->name;
		if(!strnicmp(reg->service, "sip::", 5))
			reg->service += 5;
	}

	cp = regKeyword(line, "limit");
	if(cp)
		reg->call_limit = atoi(cp);
	cp = regKeyword(line, "count");
	if(cp)
		reg->call_limit = atoi(cp);

	cp = regKeyword(line, "framing");

	if(!cp || !reg->encoding)
		cp = "0";
	reg->framing = atol(cp);
	
	if(ext)
	{
		if(!reg->hold)
			reg->hold = "send";
		if(!reg->transfer)
			reg->transfer = "send";
	}
	if(!reg->update)
			reg->update = "invite";
	if(reg->hold && !strnicmp(reg->hold, "no", 2))
		reg->hold = NULL;
	if(reg->transfer && !strnicmp(reg->transfer, "no", 2))
		reg->transfer = NULL;

	if(reg->peering)
	{
		if(!stricmp(reg->peering, "yes"))
			reg->peering = getLast("peering");

		if(!reg->peering)
			reg->peering = "any";

		if(!strnicmp(reg->peering, "no", 2))
			reg->peering = NULL;
	}

	while(insecure && *insecure)
	{
		if(!strnicmp(insecure, "publish", 7))
			reg->insecure_publish = true;
		++insecure;
	} 

	reg->add(img);

	if(!reg->dtmf)
		reg->dtmf = getLast("dtmf");

	if(!reg->dtmf)
		reg->dtmf = "auto";

	if(!reg->hostid || !*reg->hostid)
		reg->hostid = regKeyword(line, "server");

	if(!reg->hostid || !*reg->hostid)
	{
		if(!reg->type && ext)
			reg->type = "extern";
		else if(!gw)
		{
			reg->hostid = getLast("proxy");
			if(!reg->hostid || !*reg->hostid)
				reg->hostid = server->getLast("proxy");
		}
	}

	if(!reg->type)
		reg->type = getLast("regtype");

	if(!reg->hostid || !*reg->hostid)
		reg->hostid = server->getLast("server");

	if(!reg->hostid || !*reg->hostid)
		reg->hostid = "localhost";

	if(!reg->userid)
		reg->userid = getLast("userid");

	if(!reg->userid)
		reg->userid = server->getLast("userid");

	if(!*reg->userid)
		reg->userid = NULL;

	if(!reg->secret)
		reg->secret = getLast("secret");

	if(!reg->secret)
		reg->secret = server->getLast("secret");

	if(!*reg->secret)
		reg->secret = NULL;	

	if(!dur)
		dur = getLast("duration");
	cp = dur;

	lcp = cp + strlen(cp) - 1;
	switch(*lcp)
	{
	case 'm':
	case 'M':
		mult = 60000;
		break;
	case 's':
	case 'S':
		mult = 1000;
		break;
	case 'h':
	case 'H':
		mult = 360000;
		break;
	}

	reg->duration = mult * atol(cp);
	reg->line = line;
	reg->scr = scr;

	// min 3 minutes, as automatic resend is minute prior

	if(reg->duration < 180000)
		reg->duration = 180000;

        cp = scr->name;
        if(!strnicmp(cp, "sip::", 5))
                cp += 5; 

	snprintf(buf, sizeof(buf) - 2, "sip:%s", cp);
	p = strchr(buf + 4, ':');
	if(p)
	{
		*(p++) = '.';
		cp = strrchr(scr->name, ':');
		strcpy(p, cp + 1);
	}
	
	len = strlen(buf) - 3;
	if(!reg->localid)
	{
		reg->localid = (const char *)img->getMemory(len);
		strcpy((char *)reg->localid, buf + 4);
	}
	else
		setString(buf + 4, sizeof(buf) - 4, reg->localid);

        if(!reg->userid)
                reg->userid = reg->localid; 

	len = strlen(buf);
	buf[len++] = '@';
	buf[len] = 0;
	pos = len;

        if(reg->address)
        {
                cp = strrchr(reg->iface, ':');
                if(!cp)
                        cp = "";
                snprintf(buf + pos, sizeof(buf) - pos, "%s%s",
                        reg->address, cp);
        }
        else
		setString(buf + pos, sizeof(buf) - pos, reg->iface);

	len = strlen(buf) + 1;
	reg->contact = (const char *)img->getMemory(len);
	strcpy((char *)reg->contact, buf);

        setString(buf + pos, sizeof(buf) - pos, reg->hostid);
	p = strrchr(buf + 5, ':');
	if(p)
		*p = 0;
	len = 8 + strlen(reg->userid) + strlen(reg->hostid);
        uri = (char *)img->getMemory(len);
	snprintf(uri, len, "sip:%s@%s", reg->userid, reg->hostid);

	snprintf(buf, sizeof(buf), "sip:%s", reg->hostid);

	len = strlen(buf) + 1;
	reg->proxy = (const char *)img->getMemory(len);
	strcpy((char *)reg->proxy, buf);

	if(!reg->uri)
		reg->uri = uri;

	if(!strnicmp(reg->type, "anon", 4))
	{
		reg->type = "anon";
		slog.debug("register %s for anonymous calling", reg->localid);
		if(reg->address)
		{
			snprintf(buf, sizeof(buf), "sipanon.%s", reg->address);
			img->setPointer(buf, reg);
			if(!img->getPointer("sipanon.*"))
				img->setPointer("sipanon.*", reg);
		}
		else
			img->setPointer("sipanon.*", reg);
		goto add;
	}
	else if(!strnicmp(reg->type, "pub", 3))
		reg->type = "anon";

	if(ext)
		reg->type = "extern";

	if(ext)
	{
		slog.debug("register %s externally", reg->localid);
		goto add;
	}
	else if(gw)
	{
		slog.debug("register %s gateway", reg->localid);
		snprintf(buf, sizeof(buf), "sip.%s", reg->localid);
		setPointer(buf, reg);
		reg->proxy = NULL;
		goto add;
	}

	slog.debug("register %s as %s on %s", reg->localid, reg->userid, reg->proxy);

	if(!registry)
		registry = true;

	if(!strnicmp(reg->type, "dyn", 3))
	{
		reg->regid = -1;
		reg->proxy = NULL;
		goto add;
	}

	eXosip_lock();
	reg->regid = eXosip_register_build_initial_register((char *)reg->uri, (char *)reg->proxy, (char *)reg->contact, reg->duration / 1000, &msg);
	eXosip_unlock();

	if(reg->regid < 0)
		return "cannot create sip registration";

//	if(reg->secret)
//		setAuthentication(reg, NULL);

//	osip_message_set_supported(msg, "path");   
	osip_message_set_header(msg, "Event", "Registration");
	osip_message_set_header(msg, "Allow-Events", "presence");

	cp = getLast("transport");
	if(cp && !stricmp(cp, "tcp"))
		eXosip_transport_set(msg, "TCP"); 
	else if(cp && !stricmp(cp, "tls"))
		eXosip_transport_set(msg, "TLS");

/*
	reg->route = regKeyword(line, "route");
	if(reg->route && reg->route[0])
	{
		char *hdr = osip_strdup(reg->route);
		osip_message_set_multiple_header(msg, hdr, reg->route);
		osip_free(hdr);
	}
*/
	eXosip_lock();
	eXosip_register_send_register(reg->regid, msg);
	eXosip_unlock();
	
	if(!strnicmp(reg->proxy, "sip:", 4))
		snprintf(buf, sizeof(buf), "sip.%s", reg->proxy + 4);
	else
		snprintf(buf, sizeof(buf), "sip.%s", reg->proxy);
	p = strrchr(buf, ':');
	if(p && !stricmp(p, ":5060"))
		*p = 0;
	img->setPointer(buf, reg);
	p = strchr(buf, ':');
	if(p)
		*p = 0;
	host = (buf + 4);
	if(p)
		*p = ':';
	else
		p = "";

	snprintf(buf, sizeof(buf), "sipreg.%d", reg->regid);
	img->setPointer(buf, reg);

        snprintf(nbuf, sizeof(nbuf), "sip.%s%s",
                inet_ntoa(host.getAddress()), p);
        img->setPointer(nbuf, reg);

add:
	snprintf(buf, sizeof(buf), "uri.%s", reg->localid);
	reg->traffic = Driver::sip.getTraffic(buf);
	if(reg->traffic->secret[0])
		reg->secret = reg->traffic->secret;

	if(reg->traffic->address[0])
	{
		reg->proxy = reg->traffic->address;
		cp = strchr(reg->proxy, '@');
		if(cp)
			reg->proxy = ++cp;
	} 
	img->setPointer(buf, reg);
	img->addRegistration(line);
	line->scr.registry = reg;
	if(ext)
	{
		snprintf(buf, sizeof(buf), "uid.%s", reg->localid);
		img->setPointer(buf, line);
		snprintf(buf, sizeof(buf), "ext.%s", reg->localid);
		img->setPointer(buf, &Driver::sip);
	}

	if(gateways)
	{
		setString(buf + 5, sizeof(buf) - 5, gateways);
		gateways = strtok_r(buf, ",;\t\r\n", &tok);
	}
	while(gateways)
	{
		p = strrchr(gateways, ':');
		if(p && !stricmp(p, ":5060"))
			*p = 0;

		sp = (char *)(buf - 4);
		sp[0] = 's';
		sp[1] = 'i';
		sp[2] = 'p';
		sp[3] = '.';
		if(img->getPointer(sp))
			goto ngw;

		preg = (Registry *)img->getMemory(sizeof(Registry));
		memcpy(preg, reg, sizeof(Registry));
		preg->proxy = ip->dupString(gateways);
		setPointer(sp, preg);
		
		host = gateways;
		if(p)
			*p = ':';
		else
			p = "";

		snprintf(nbuf, sizeof(nbuf), "sip.%s%s",
			inet_ntoa(host.getAddress()), p);
		img->setPointer(nbuf, reg);
		
ngw:
		gateways = strtok_r(NULL, ",;\t\r\n", &tok);
	}

	return "";
}

Session *Driver::getCall(int callid)
{
	timeslot_t ts = 0;
	Session *session;
	BayonneSession *bs;

	while(ts < count)
	{
		bs = getTimeslot(ts++);
		if(!bs)
			continue;
		session = (Session *)bs;
		if(session->cid == callid && session->offhook)
			return session;
	}
	return NULL;
}

void Driver::run(void)
{
        eXosip_event_t *sevent;
        Event event;
	char buf[1024];
	char cbuf[256];
	Registry *reg, *peer, *proxy, *sreg;
	Name *scr;
	ScriptImage *img;
	const char *cp;
	const char *caller = NULL;
	const char *dialed = NULL;
	const char *display = NULL;
	BayonneSession *s = NULL;
	Session *session;
	char *k, *v, *l, *p, *line;
	osip_from_t *from;
	osip_to_t *to;
	char digit[10];
	char duration[10];
	osip_body_t *mbody;
	osip_header_t *header;
	const char *auth = "anon";
	timeout_t event_timer = atoi(getLast("timer"));
	osip_message_t *msg = NULL;
	char *remote_uri = NULL;
	char *local_uri = NULL;
	sdp_message_t *remote_sdp, *local_sdp;
	sdp_connection_t *conn;
	osip_content_type_t *ct;
	sdp_media_t *remote_media = NULL, *local_media, *audio_media;
	char *tmp, *audio_payload, *adp;
	const char *data_rtpmap = NULL;
	int pos, mpos, rtn;
	sdp_attribute_t *attr;
	char *sdp;
	bool reg_inband;
	uint8 reg_payload;
	timeout_t expires;
	unsigned thread = ++instance;
	unsigned short sinstance;
	time_t now;
	time_t last = 0;
	bool inactive;
	InetAddress hostid;

	Bayonne::waitLoaded();

	for(;;)
        {
		sevent = eXosip_event_wait(0, event_timer);
		Thread::yield();

		if(exiting)
			Thread::sync();

		if(!sevent)
		{
			eXosip_lock();
			time(&now);
			if(now != last)
				eXosip_automatic_action();
			last = now;
			eXosip_unlock();
			continue;
		}

		slog.debug("sip: event %04x; cid=%d, did=%d, rid=%d, instance=%d",
			sevent->type, sevent->cid, sevent->did, sevent->rid, thread);

		sreg = NULL;
		sinstance = 0;
		switch(sevent->type)
		{
		case EXOSIP_REGISTRATION_NEW:
			slog.info("sip: new registration");
			break;
		case EXOSIP_REGISTRATION_SUCCESS:
			snprintf(buf, sizeof(buf), "sipreg.%d", sevent->rid);
			server->enter();
			img = server->getActive();
			reg = (Registry *)img->getPointer(buf);	
			header = NULL;
			osip_message_get_expires(sevent->request, 0, &header);
			expires = 0;
			if(header)
				expires = osip_atoi(header->hvalue) * 1000l;
			else if(reg)
				expires = reg->duration;
			if(reg && !expires)
			{
				reg->traffic->active = false;
				slog.debug("registration for %s rejected", reg->service);
			}
			else if(reg)
			{
				reg->traffic->active = true;
				reg->traffic->setTimer(expires);
				if(expires != reg->duration)
				{
					updateExpires(reg, expires);
					slog.debug("registration for %s updated for %d seconds",
						reg->service, reg->duration / 1000);			
				}
				else
					slog.debug("registration for %s confirmed for %d seconds", 
						reg->service, expires / 1000);
			}
			else
				slog.warn("unknown sip registration confirmed; rid=%s", sevent->rid);
			server->leave();
			break;
		case EXOSIP_REGISTRATION_FAILURE:
			snprintf(buf, sizeof(buf), "sipreg.%d", sevent->rid);
			server->enter();
			img = server->getActive();
			reg = (Registry *)img->getPointer(buf);	
			if(reg && sevent->response && sevent->response->status_code == 401)
			{
				setAuthentication(reg, sevent->response);
				server->leave();
				eXosip_lock();
				time(&last);
				eXosip_automatic_action();
				eXosip_unlock();
				break;
			}
			if(reg)
			{
				reg->traffic->setTimer(0);
				reg->traffic->active = false;
				slog.info("registration for %s failed", reg->service);
			}
			else
				slog.warn("unknown sip registration failed; rid=%s", sevent->rid);
			server->leave();
			break;
		case EXOSIP_REGISTRATION_REFRESHED:
			slog.info("sip registration refreshed");
			break;
		case EXOSIP_REGISTRATION_TERMINATED:
			slog.info("sip registration terminated");
			break;	
		case EXOSIP_CALL_RELEASED:
                        session = getCall(sevent->cid);
                        if(session)
				goto exitcall;
			break;
		case EXOSIP_CALL_MESSAGE_NEW:
			if(!sevent->request)
				break;
			session = getCall(sevent->cid);
			if(!session)
			{
				slog.warn("sip: got info for non-existant call");
				goto killit;
			}
			remote_media = NULL;
			local_media = NULL;
			conn = NULL;
			remote_sdp = NULL;
			local_sdp = NULL;
			display = NULL;
			caller = NULL;
			dialed = NULL;
			local_uri = NULL;
			remote_uri = NULL;
			from = NULL;
			to = NULL;
			sdp = NULL;
			msg = NULL;

			if(MSG_IS_REFER(sevent->request))
			{
				session->enterMutex();
				if(!session->holding)
				{
failrefer:
					session->leaveMutex();
					eXosip_lock();
					eXosip_call_build_answer(sevent->tid, 503, &msg);
					if(msg)
						eXosip_call_send_answer(sevent->tid, 503, msg);
					eXosip_unlock();
					break;
				}
				if(!session->isJoined())
					goto failrefer;
				if(!session->state.join.refer)
					goto failrefer;
				session->leaveMutex();					
		
				rtn = 503;
				osip_message_header_get_byname(sevent->request, "refer-to", 0, &header);
				if(!header || !header->hvalue)
					goto norefer;
				osip_from_to_str(sevent->request->from, &remote_uri);
				if(!remote_uri)
					goto norefer;
				osip_from_init(&from);
				osip_from_parse(from, remote_uri);
				if(!from || !from->url)
					goto norefer; 
				osip_to_init(&to);
				osip_to_parse(to, header->hvalue);
				if(!to || !to->url)
					goto norefer;

				if(stricmp(to->url->scheme, "sip"))
				{
					snprintf(buf, sizeof(buf), "%s:%s@%s:%s",
						to->url->scheme, to->url->username, to->url->host, to->url->port);
					goto dorefer;
				}

				if(!from->url->port)
					from->url->port = "5060";

				if(!to->url->port)
					to->url->port = "5060";

				if(!stricmp(from->url->host, to->url->host))
					if(!stricmp(from->url->port, to->url->port))
					{
						setString(buf, sizeof(buf), to->url->username);
						goto dorefer;
					}
						
				if(to->url->port && stricmp(to->url->port, "5060"))
					snprintf(buf, sizeof(buf), "sip:%s@%s:%s",
						to->url->username, to->url->host, to->url->port);
				else
					snprintf(buf, sizeof(buf), "sip:%s@%s",
						to->url->username, to->url->host);

dorefer:
				memset(&event, 0, sizeof(event));
				event.id = START_REFER;
				event.dialing = buf;
				session->tid = sevent->tid;
				if(session->postEvent(&event))
					rtn = 100;

norefer:
				eXosip_lock();
				eXosip_call_build_answer(sevent->tid, rtn, &msg);
				if(msg)
					eXosip_call_send_answer(sevent->tid, rtn, msg);
				eXosip_unlock();
				msg = NULL;
				goto cleanup;
			}
			if(MSG_IS_BYE(sevent->request))
			{
				memset(&event, 0, sizeof(event));
				event.id = STOP_DISCONNECT;
				session->queEvent(&event);
				eXosip_lock();
				msg = NULL;
				eXosip_call_build_answer(sevent->tid, 200, &msg);
				if(msg)
					eXosip_call_send_answer(sevent->tid, 200, msg);
				eXosip_unlock();
				msg = NULL;
				break;
			}
			if(!MSG_IS_INFO(sevent->request))
			{
				slog.debug("%s: in dialog %s currently unsupported", session->logname, sevent->request->sip_method);
				break;
			}
			ct = sevent->request->content_type;
			if(!ct || !ct->type || !ct->subtype)
				break;

			if(stricmp(ct->type, "application") || stricmp(ct->subtype, "dtmf-relay"))
				break;

			mbody = NULL;
			osip_message_get_body(sevent->request, 0, &mbody);
			line=mbody->body;
			digit[0] = 0;
			duration[0] = 0;
			while(line && *line)
			{
				k = v = line;
				while(*v && *v != '=' && *v != '\r' && *v != '\n')
					++v;

				if(*v != '=')
					break;

				++v;

				while(*v == ' ' || *v == '\b')
					++v;

				if(!*v || *v == '\r' || *v == '\n')
					break;

				l = v;
				while(*l && *l != ' ' && *l != '\r' && *l != '\n')
					++l;

				if(!*l)
					break;

				if(!strnicmp(k, "signal", 6))
					strncpy(digit, v, l-v);
				else if(!strnicmp(k, "duration", 8))
					strncpy(duration, v, l-v);

				++l;
				while(*l == ' ' || *l == '\n' || *l == '\r')
					++l;

				line = l;
				if(!*line)
					break;
			}

			msg = NULL;
			eXosip_lock();
			eXosip_call_build_answer(sevent->tid, 200, &msg);
			eXosip_call_send_answer(sevent->tid, 200, msg);
			eXosip_unlock();

			if(!digit[0] || !duration[0])
				break;

			memset(&event, 0, sizeof(event));
			event.id = DTMF_KEYUP;
			session->dtmf_inband = false;
			switch(*digit)
			{
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
                                event.dtmf.digit = *digit - '0';
                                event.dtmf.duration = atoi(duration);
				session->postEvent(&event);
				break;
			case '*':
                                event.dtmf.digit = 10;
                                event.dtmf.duration = atoi(duration);
                                session->postEvent(&event);
                                break;
                        case '#':
                                event.dtmf.digit = 11;
                                event.dtmf.duration = atoi(duration);
                                session->postEvent(&event);
                                break;
			}
			break;				
		case EXOSIP_CALL_CLOSED:
			session = getCall(sevent->cid);
			if(session)
				goto exitcall;
			break;
		case EXOSIP_CALL_CANCELLED:
			session = getCall(sevent->cid);
			if(!session)
			{
				slog.warn("sip: got bye for non-existant call");
				break;
			}
exitcall:
			memset(&event, 0, sizeof(event));
			slog.debug("%s: closing call", session->logname);
			event.id = STOP_DISCONNECT;	
			session->did = 0;
			session->postEvent(&event);
			break;
		case EXOSIP_CALL_PROCEEDING:
			session = getCall(sevent->cid);
			if(!session)
				break;
			memset(&event, 0, sizeof(event));
			session->did = sevent->did;
			event.id = CALL_PROCEEDING;
			session->postEvent(&event);
			break;
                case EXOSIP_CALL_RINGING:
                        session = getCall(sevent->cid);
                        if(!session)
                                break;
                        memset(&event, 0, sizeof(event));
			session->did = sevent->did;
                        event.id = CALL_RINGING;
                        session->postEvent(&event);
                        break;
                case EXOSIP_CALL_ANSWERED:
                        session = getCall(sevent->cid);
                        if(!session)
                                break;
			session->did = sevent->did;
			msg = NULL;
			sdp = NULL;
			local_sdp = NULL;
			remote_sdp = NULL;
			conn = NULL;
			if(sevent->request && sevent->response)
			{
				local_sdp = eXosip_get_sdp_info(sevent->request);
				remote_sdp = eXosip_get_sdp_info(sevent->response);
			}

			// we should verify sdp stuff...

			if(remote_sdp)
			{
				conn = eXosip_get_audio_connection(remote_sdp);
				remote_media = eXosip_get_audio_media(remote_sdp);
			}
			cp = session->getSymbol("session.rtp_remote");
			if(cp && !*cp)
				cp = NULL;
			if(conn && remote_media && !cp)
			{
				session->remote_address = conn->c_addr;
				session->remote_port = atoi(remote_media->m_port);
				snprintf(cbuf, sizeof(cbuf), "%s:%d",
					inet_ntoa(session->remote_address.getAddress()),
					session->remote_port);
				session->setConst("session.rtp_remote", cbuf);
				snprintf(cbuf, sizeof(cbuf), "%s:%d",
					getLast("localip"),
					session->getLocalPort());
				session->setConst("session.rtp_local", cbuf);
			}

                        memset(&event, 0, sizeof(event));
                        event.id = CALL_ANSWERED;
                        if(session->postEvent(&event))
			{
				eXosip_lock();
				eXosip_call_build_ack(sevent->did, &msg);
				rtn = eXosip_call_send_ack(sevent->did, msg);
				eXosip_unlock();
				memset(&event, 0, sizeof(event));
				if(rtn)
					event.id = CALL_FAILURE;
				else
					event.id = CALL_ACCEPTED;
				session->queEvent(&event);
			}
			if(sdp)
				delString(sdp);

			if(local_sdp)
				sdp_message_free(local_sdp);
			if(remote_sdp)
				sdp_message_free(remote_sdp);
                        break;
		case EXOSIP_CALL_GLOBALFAILURE:
		case EXOSIP_CALL_SERVERFAILURE:
                        session = getCall(sevent->cid);
                        if(!session)
                                break;
                        memset(&event, 0, sizeof(event));
                        event.id = CALL_FAILURE;
                        session->postEvent(&event);
                        break;
		case EXOSIP_CALL_ACK:
			session = getCall(sevent->cid);
			if(!session)
			{
				slog.warn("sip: call ack for non-existant call");
killit:
				eXosip_lock();
				eXosip_call_terminate(sevent->cid, sevent->did);
				eXosip_unlock();
				break;
			}
			session->did = sevent->did;
			memset(&event, 0, sizeof(event));
			event.id = CALL_ACCEPTED;
			session->postEvent(&event);
			break;

		case EXOSIP_CALL_MESSAGE_REQUESTFAILURE:
		case EXOSIP_CALL_REQUESTFAILURE:
			session = getCall(sevent->cid);
			if(!session)
				break;

			memset(&event, 0, sizeof(event));
			slog.debug("sip: failure result code %d", sevent->response->status_code);
			switch(sevent->response->status_code)
			{
			case AUTH_REQUIRED:
				getAuthentication(session, sevent->response);
				eXosip_lock();
				time(&last);
				eXosip_automatic_action();
				eXosip_unlock();
				break;
			case UNAUTHORIZED:
				getAuthentication(session, sevent->response);
				eXosip_lock();
				time(&last);
				eXosip_automatic_action();
				eXosip_unlock();
				break;
			case BUSY_HERE:
				event.id = DIAL_BUSY;
				break;
			case REQ_TIMEOUT:
				event.id = DIAL_TIMEOUT;
				break;
			case NOT_FOUND:
			case NOT_ALLOWED:
			case NOT_ACCEPTABLE:
			case FORBIDDEN:
			case BAD_REQ:
			case TEMP_UNAVAILABLE:
				event.id = DIAL_INVALID;
			default:
				event.id = DIAL_FAILED;
			}	
			if(event.id)
				session->postEvent(&event);
			break;
		case EXOSIP_MESSAGE_NEW:
                        local_media = NULL;
                        conn = NULL;
                        remote_sdp = NULL;
                        local_sdp = NULL;
                        display = NULL;
                        caller = NULL;
                        dialed = NULL;
                        local_uri = NULL;
                        remote_uri = NULL;
                        from = NULL;
                        to = NULL;
                        sdp = NULL;
                        msg = NULL;
                        session = NULL;


			if(!sevent->request)
				break;
			if(MSG_IS_REGISTER(sevent->request))
				registerRequest(sevent);
			else if(MSG_IS_OPTIONS(sevent->request))
				requestOptions(sevent);
			else if(MSG_IS_UPDATE(sevent->request))
				goto answer;
			else if(MSG_IS_PRACK(sevent->request))
			{
				if(eXosip_get_sdp_info(sevent->request))
					goto answer;
				eXosip_lock();
				eXosip_call_send_answer(sevent->tid, 200, NULL);
				eXosip_unlock();
			}
			else if(MSG_IS_PUBLISH(sevent->request))
			{
				eXosip_call_get_referto(sevent->did, buf, sizeof(buf));
				osip_to_to_str(sevent->request->to, &local_uri);
				if(!local_uri)
					goto cleanup;
				osip_from_to_str(sevent->request->from, &remote_uri);
				if(!remote_uri)
					goto cleanup;
				osip_to_init(&to);
				osip_to_parse(to, local_uri);
				if(!to)
					goto cleanup;

				osip_from_init(&from);
				osip_from_parse(from, remote_uri);
				if(!from)
					goto cleanup;

				if(!from->url || !to->url)
					goto cleanup;

				if(!from->url->username || !to->url->username)
					goto cleanup;

				if(stricmp(from->url->username, to->url->username))
					goto cleanup;

				if(stricmp(to->url->port, from->url->port))
					goto cleanup;

				if(stricmp(to->url->host, from->url->host))
					goto cleanup;

				publishRequest(sevent, to->url->username);
				goto cleanup;
			}
			else
				slog.debug("sip: out of dialog msg %s", sevent->request->sip_method);
			break;
answer:
		case EXOSIP_CALL_REINVITE:
                        if(!sevent->request)
                                break;
                        if(sevent->cid < 1 && sevent->did < 1)
                                break;
			session = getCall(sevent->cid);
			if(!session)
			{
				slog.warn("sip: got re-invite for non-existant call");
				break;
			}

                        remote_media = NULL;
                        local_media = NULL;
                        conn = NULL;
                        remote_sdp = NULL;
                        local_sdp = NULL;
                        display = NULL;
                        caller = NULL;
                        dialed = NULL;
                        local_uri = NULL;
                        remote_uri = NULL;
                        from = NULL;
                        to = NULL;
                        sdp = NULL;
                        msg = NULL;

			cp = session->getSymbol("session.ip_public");
			sdp = newString("", 4096);
			snprintf(sdp, 4096,
				"v=0\r\n"
				"o=bayonne 0 0 IN IP4 %s\r\n"
				"s=conversation\r\n"
				"c=IN IP4 %s\r\n"
				"t=0 0\r\n", cp, cp);
			
			mpos = 0;
			data_rtpmap = session->sdpEncoding();
			audio_payload = NULL;
			remote_sdp = eXosip_get_sdp_info(sevent->request);
			if(!remote_sdp)
			{
				eXosip_lock();
				eXosip_call_send_answer(sevent->tid, 400, NULL);
				eXosip_unlock();
				goto cleanup;
			}
			conn = eXosip_get_audio_connection(remote_sdp);
			remote_media = eXosip_get_audio_media(remote_sdp);

			inactive = false;

			if(!remote_media)
			{
				eXosip_lock();
				eXosip_call_send_answer(sevent->tid, 415, NULL);
				eXosip_unlock();
				goto cleanup;
			}
			while(!osip_list_eol(OSIP2_LIST_PTR remote_sdp->m_medias, mpos))
			{
				remote_media = (sdp_media_t *)osip_list_get(OSIP2_LIST_PTR remote_sdp->m_medias, mpos++);
				if(!remote_media)
					continue;

				if(!stricmp(remote_media->m_media, "audio") && !audio_payload)
					audio_media = remote_media;
				else
					audio_media = NULL;

				audio_payload = NULL;
				tmp = NULL;
				pos = 0;

				while(!osip_list_eol(OSIP2_LIST_PTR remote_media->a_attributes, pos))
				{
					attr = (sdp_attribute_t *)osip_list_get(OSIP2_LIST_PTR remote_media->a_attributes, pos++);
					if(!attr)
						continue;

					tmp = attr->a_att_field;
					if(tmp && !stricmp(tmp, "sendonly"))
						inactive = true;
					else if(tmp && !stricmp(tmp, "inactive"))
						inactive = true;
					tmp = attr->a_att_value;
					if(!tmp)
						continue;

					if(stricmp(attr->a_att_field, "rtpmap"))
						continue;

					adp = tmp;
					tmp = strchr(tmp, ' ');
					if(!tmp)
					{
						adp = NULL;
						continue;
					}
					while(isspace(*tmp))
						++tmp;
					if(!strnicmp(tmp, "telephone-event/", 16))
					{
						if(session->dtmf_payload)
							session->dtmf_payload = atoi(adp);
						continue;
					}
					if(!strnicmp(tmp, data_rtpmap, strlen(tmp)))
					{
						audio_payload = adp;
						session->data_payload = atoi(adp);
					}
				}
			}				
			if(!audio_payload)
			{
				eXosip_lock();
				eXosip_call_send_answer(sevent->tid, 415, NULL);
				eXosip_unlock();
				goto cleanup;
			}
					
                        if(session->dtmf_payload)
                                snprintf(cbuf, sizeof(cbuf),
                                        "m=audio %d RTP/AVP %d %d\r\n"
                                        "%s\r\n",
                                                session->getLocalPort(),
                                                session->data_payload,
                                                session->dtmf_payload,
                                                data_rtpmap);
                        else
                                snprintf(cbuf, sizeof(cbuf),
                                        "m=audio %d RTP/AVP %d\r\n"
                                        "%s\r\n",
                                                session->getLocalPort(),
                                                session->data_payload,
                                                data_rtpmap);

			addString(sdp, 4096, cbuf);
                        if(session->dtmf_payload)
                        {
                                snprintf(cbuf, sizeof(cbuf),
                                        "a=rtpmap:%d telephone-event/8000\r\n"
					"a=fmtp:%d 0-15\r\n",                                        
					session->dtmf_payload,
					session->dtmf_payload);
                                addString(sdp, 4096, cbuf);
                        }
			data_rtpmap = Session::getAttributes(session->info.encoding);
			if(data_rtpmap)
			{
				snprintf(cbuf, sizeof(cbuf),
					"a=fmtp:%d %s\r\n",
					session->data_payload,
					data_rtpmap);
				addString(sdp, 4096, cbuf);
			}
			if(session->info.framing)
			{
				snprintf(cbuf, sizeof(cbuf),
					"a=ptime:%ld\r\n",
					session->info.framing);
				addString(sdp, 4096, cbuf);
			}		
			memset(&event, 0, sizeof(event));
			if(*conn->c_addr == '0' || inactive)
				event.id = CALL_HOLD;
			else if(session->holding)
				event.id = CALL_NOHOLD;
			else
				event.id = AUDIO_RECONNECT;
			session->enter();
			if(!session->rtp || !session->postEvent(&event))
			{
				session->leave();
				eXosip_lock();
				eXosip_call_send_answer(sevent->tid, 404, NULL);
				eXosip_unlock();
				goto cleanup;
			}

			osip_message_header_get_byname(sevent->request, "session-expires", 0, &header);
			if(header && header->hvalue)
			{
				time(&now);
				expires = osip_atoi(header->hvalue) * 1000;
				now += (expires / 1000);
				session->session_timer = now;
			}
			else
			{
				expires = 0;
				session->session_timer = 0;
			}

			if(expires)
				slog.debug("%s: reconnecting %s:%d; expires=%ld seconds",
					session->logname, conn->c_addr, atoi(remote_media->m_port), expires / 1000);
			else
				slog.debug("%s: reconnecting %s:%d",
					session->logname, conn->c_addr, atoi(remote_media->m_port));
			session->rtp->forgetDestination(session->remote_address, session->remote_port);
			session->remote_address = conn->c_addr;
			session->remote_port = atoi(remote_media->m_port);
			session->rtp->addDestination(session->remote_address, session->remote_port);
			session->leave();
			eXosip_lock();
			eXosip_call_build_answer(sevent->tid, 200, &msg);
			if(!msg)
			{
				eXosip_call_send_answer(sevent->tid, 404, NULL);
				eXosip_unlock();
				goto cleanup;
			}
                        osip_message_set_require(msg, "100rel");
                        osip_message_set_header(msg, "RSeq", "1");
                        osip_message_set_body(msg, sdp, strlen(sdp));
                        osip_message_set_content_type(msg, "application/sdp");

                        eXosip_call_send_answer(sevent->tid, 183, msg);

                        eXosip_unlock();
			goto cleanup;		
		case EXOSIP_CALL_INVITE:
			if(!sevent->request)
				break;
			if(sevent->cid < 1 && sevent->did < 1)
				break;

			inactive = false;
			remote_media = NULL;
			local_media = NULL;
			conn = NULL;
			remote_sdp = NULL;
			local_sdp = NULL;
			display = NULL;
			caller = NULL;
			dialed = NULL;
			local_uri = NULL;
			remote_uri = NULL;
			from = NULL;
			to = NULL;
			sdp = NULL;
			msg = NULL;
			session = NULL;

			osip_to_to_str(sevent->request->to, &local_uri);

			if(!local_uri)
				goto clear;

			osip_from_to_str(sevent->request->from, &remote_uri);
			if(!remote_uri)
				goto clear;

			osip_to_init(&to);
			osip_to_parse(to, local_uri);
			if(!to)
				goto clear;

			if(to->url != NULL && to->url->username != NULL)
				dialed = to->url->username;

			osip_from_init(&from);
			osip_from_parse(from, remote_uri);		
			if(!from)
				goto clear;

			if(from->url != NULL && from->url->username != NULL)
				caller = from->url->username;					
			
                        display = osip_from_get_displayname(from);  
                        if(display && *display == '\"')
                        {
                                ++display;
                                p = (char *)strrchr(display, '\"');
                                if(p)
                                        *p = 0; 
                        }
			else if(!display)
				display = caller;

			img = useImage();

                        if(to->url->port && stricmp(to->url->port, "5060"))
                                     snprintf(buf, sizeof(buf), "sip.%s:%s",
                                        to->url->host, to->url->port);
                        else
                                snprintf(buf, sizeof(buf), "sip.%s",
                                        to->url->host); 
                        proxy = (Registry *)img->getPointer(buf);
                        if(proxy && !proxy->isActive())
                                goto sip404;

			if(from->url->port && stricmp(from->url->port, "5060"))
				snprintf(buf, sizeof(buf), "sip.%s:%s",
					from->url->host, from->url->port);
			else
				snprintf(buf, sizeof(buf), "sip.%s", from->url->host);
			peer = (Registry *)img->getPointer(buf);
			
			if(peer && !peer->isActive())
				goto sip404;

			reg = NULL;
			if(from->url)
				cp = from->url->host;
			else
				cp = NULL;

			if(!isLocalName(cp))
			{
				if(to->url)
					cp = to->url->host;
				else
					cp = NULL;
				reg = getAnonymous(img, cp, dialed);
				if(!reg)
					goto sip403;
			}
			else
			{
				reg = getAnonymous(img, NULL, dialed);
				if(!reg && !getRegistry(sevent, &reg, img))
					goto sip401;
			}
			reg_inband = dtmf_inband;
			reg_payload = dtmf_negotiate;

			if(!reg)
			{
				snprintf(buf, sizeof(buf), "uri.%s", dialed);
				reg = (Registry *)img->getPointer(buf);
			}
			else	
			{
				peer = reg;
				if(!strnicmp(reg->type, "ext", 3))
				{
					auth = "peer";
					goto validate;
				}	
				if(!stricmp(reg->type, "peer"))
				{
					auth = "peer";
					goto validate;
				}
				if(!stricmp(reg->type, "gw"))
				{
					auth = "peer";
					goto validate;
				}
				if(!stricmp(reg->type, "anon"))
				{
					auth = "anon";
					goto validate;
				}
				goto sip404;
			}

			auth = "none";
			if(peer && !reg)
				auth = peer->type;
			else if(reg)
				auth = reg->type;
			if(!strnicmp(auth, "ext", 3))
				auth = "peer";
			if(!stricmp(auth, "gw"))
				auth = "peer";
			if(!stricmp(auth, "friend") && peer)
				auth = "peer";
			else if(!stricmp(auth, "friend") && proxy)
				auth = "proxy";
			else if(!stricmp(auth, "friend"))
				reg = NULL;
			if(proxy && !reg && !stricmp(auth, "proxy"))
				reg = proxy;
			if(peer && !reg && !stricmp(auth, "peer"))
				reg = peer;
			if(reg && !peer && !stricmp(auth, "peer"))
				reg = NULL;
			if(reg && !peer && !stricmp(auth, "user"))
				reg = NULL;
                        if(reg && !proxy && !stricmp(auth, "proxy"))
                                reg = NULL; 

			if(!reg && !peer)
			{
				snprintf(buf, sizeof(buf), "sipanon.%s", to->url->host);
				reg = (Registry *)img->getPointer(buf);
				if(reg)
					goto validate;
				endImage(img);
				requestAuth(sevent);
				goto clear;
			}

			if(reg && !stricmp(reg->type, "peer"))
			{
				endImage(img);
				requestAuth(sevent);
				goto clear;
			}

validate:
			if(reg)
			{
				if(!reg->isActive())
					goto sip404;
	
				sreg = reg;
				sinstance = ++reg->traffic->active_calls;
				if(reg->call_limit && sinstance > reg->call_limit)
					goto sip480;

				scr = img->getScript(reg->scr->name);

				cp = reg->dtmf;
				if(cp && *cp)
				{
					if(!stricmp(cp, "info") || !stricmp(cp, "sipinfo"))
						reg_inband = false;
					else if(atoi(cp) < 255 && atoi(cp) > 10)
						reg_payload = atoi(cp);
					else if(stricmp(cp, "2833") && stricmp(cp, "rfc2833"))
						reg_payload = 0;
				}		
			}
			else
				goto sip404;

			if(!scr)
			{
				slog.warn("sip: unknown uri %s dialed", dialed);
sip404:
				endImage(img);
				eXosip_lock();
				eXosip_call_send_answer(sevent->tid, 404, NULL);
				eXosip_unlock();				
				goto clear;
sip403:
                endImage(img);
                eXosip_lock();
                eXosip_call_send_answer(sevent->tid, 403, NULL);
                eXosip_unlock();
                goto clear;
sip401:
				endImage(img);
				eXosip_lock();
				eXosip_call_send_answer(sevent->tid, 401, NULL);
				eXosip_unlock();
				goto clear;
			}

			s = getIdle();
			if(!s)
			{
				slog.warn("sip: no timeslots available");
sip480:
				endImage(img);
				eXosip_lock();
				eXosip_call_send_answer(sevent->tid, 480, NULL);
				eXosip_unlock();
				goto clear;
			}
		
			session = (Session *)(s);

			session->session_timer = 0;
			eXosip_lock();
			osip_message_header_get_byname(sevent->request, "session-expires", 0, &header);
			if(header && header->hvalue)
			{
				time(&now);
				now += osip_atoi(header->hvalue);
				session->session_timer = now;
			}
			eXosip_unlock();

			session->dtmf_sipinfo = info_negotiate;
			if(reg->dtmf && !stricmp(reg->dtmf, "info"))
				session->dtmf_sipinfo = true;
			else if(reg->dtmf && !stricmp(reg->dtmf, "sipinfo"))
				session->dtmf_sipinfo = true;
			else if(reg->dtmf)
				session->dtmf_sipinfo = false;

                        cp = getLast("localip");
                        if(reg->address)
                        {
                                InetAddress host(reg->address);
                                snprintf(cbuf, sizeof(cbuf), "%s",
                                        inet_ntoa(host.getAddress()));
                                session->setConst("session.ip_public", cbuf);
                        }
                        else
                                session->setConst("session.ip_public", cp);

                        session->setConst("session.ip_local", cp);

			session->cid = sevent->cid;
			session->did = sevent->did;
			session->setConst("session.info", auth);
			session->setConst("session.dialed", dialed);
			session->setConst("session.caller", caller);
			session->setConst("session.display", display);

			if(from->url->port && stricmp(from->url->port, "5060"))
				snprintf(buf, sizeof(buf), "sip:%s@%s:%s",
					from->url->username,
					from->url->host,
					from->url->port);
			else
				snprintf(buf, sizeof(buf), "sip:%s@%s",
					from->url->username,
					from->url->host);
					
			if(reg)
			{
				++reg->traffic->call_attempts.iCount;
				++reg->traffic->call_complete.iCount;
				if(!stricmp(reg->type, "anon") || 
				   !strnicmp(reg->type, "ext", 3) || 
				   !stricmp(reg->type, "gw")) 
					snprintf(cbuf, sizeof(cbuf), "uri.%s", reg->localid);
				else
					snprintf(cbuf, sizeof(cbuf), "sipreg.%d", reg->regid);
				session->setConst("session.registry", cbuf);
				session->setConst("session.uri_server", reg->proxy);
				session->setConst("session.uri_local", reg->contact);

				if(reg->encoding)
					session->setEncoding(reg->encoding, reg->framing);

				if(!strnicmp(reg->type, "ext", 3))
				{
					snprintf(buf, sizeof(buf), "sip:%s", reg->localid);
					session->setConst("session.authorized", buf);
					session->setConst("session.identity", buf);
					session->setConst("session.group", reg->group);
				}
				else if(!strnicmp(reg->type, "anon", 4))
				{
					snprintf(buf, sizeof(buf), "sip:%s", reg->localid);
					session->setConst("session.anonymous", buf);
				}
				else
					session->setConst("session.service", reg->service);
			
				snprintf(digit, sizeof(digit), "%d", sinstance);
				session->setConst("session.instance", digit);
			}
			else
			{
				auth = "anon";
				snprintf(buf, sizeof(buf), "sip:anon@%s", getLast("interface"));
				p = strrchr(buf, ':');
				if(p && !stricmp(p, ":5060"))
					*p = 0;
				session->setConst("session.uri_local", buf);
				session->setConst("session.service", "anonymous");
			}
                        snprintf(digit, sizeof(digit), "%d", sinstance);                                
			session->setConst("session.instance", digit);

			sinstance = 0;
			sreg = NULL;
				
                        session->local_address = getLast("localip");
                        snprintf(buf, sizeof(buf), "%s:%d",
                                inet_ntoa(session->local_address.getAddress()),
                                session->getLocalPort());
                        session->setConst("session.rtp_local", buf);    

			memset(&event, 0, sizeof(event));
			
			if(!stricmp(auth, "peer"))
				event.id = START_INCOMING;
			else
				event.id = START_DIRECT;

			event.start.img = img;
			event.start.scr = scr;

			eXosip_lock();
			remote_sdp = eXosip_get_sdp_info(sevent->request);
			eXosip_unlock();
			if(!remote_sdp)
			{
				eXosip_lock();
				eXosip_call_send_answer(sevent->tid, 400, NULL);
				eXosip_unlock();
				goto clear;
			}		
			eXosip_lock();
			conn = eXosip_get_audio_connection(remote_sdp);
			remote_media = eXosip_get_audio_media(remote_sdp);	
			eXosip_unlock();
			if(!remote_media || !remote_media->m_port)
			{
sip415:
				eXosip_lock();
				eXosip_call_send_answer(sevent->tid, 415, NULL);
				eXosip_unlock();
				goto clear;
			}
			session->remote_address = conn->c_addr;			
			session->remote_port = atoi(remote_media->m_port);

                        snprintf(buf, sizeof(buf), "%s:%d",
                                inet_ntoa(session->remote_address.getAddress()),                                session->getRemotePort());
                        session->setConst("session.rtp_remote", buf);  

                        cp = session->getSymbol("session.ip_public");
			sdp = newString("", 4096);
			snprintf(sdp, 4096, 
				"v=0\r\n"
				"o=bayonne 0 0 IN IP4 %s\r\n"
				"s=conversation\r\n"
				"c=IN IP4 %s\r\n"
				"t=0 0\r\n", cp, cp);

			session->data_payload = data_negotiate;
			session->dtmf_payload = 0;

			mpos = 0;
			data_rtpmap = session->sdpEncoding();
			session->data_payload = session->sdpPayload();
			audio_payload = NULL;
			while(!osip_list_eol(OSIP2_LIST_PTR remote_sdp->m_medias, mpos))
			{
				remote_media = (sdp_media_t *)osip_list_get(OSIP2_LIST_PTR remote_sdp->m_medias, mpos++);
				if(!remote_media)
					continue;

				if(!stricmp(remote_media->m_media, "audio") && !audio_payload)
					audio_media = remote_media;
				else
					audio_media = NULL;

				audio_payload = NULL;
				tmp = NULL;
				pos = 0;
				inactive = false;				

				while(!osip_list_eol(OSIP2_LIST_PTR remote_media->a_attributes, pos))
				{
					attr = (sdp_attribute_t *)osip_list_get(OSIP2_LIST_PTR remote_media->a_attributes, pos++);
					if(!attr)
						continue;
					tmp = attr->a_att_field;
					if(tmp && !stricmp(tmp, "inactive"))
						inactive = true;
					tmp = attr->a_att_value;
					if(!tmp)
						continue;
				
					if(stricmp(attr->a_att_field, "rtpmap"))
						continue;
 	
					adp = tmp;
					tmp = strchr(tmp, ' ');
					if(!tmp)
					{
						adp = NULL;
						continue;
					}
					
					while(isspace(*tmp))
						++tmp;
					if(!strnicmp(tmp, "telephone-event/", 16))
					{
						session->dtmf_payload = atoi(adp);
						continue;
					}
					if(!strnicmp(tmp, "G721", 4))
						tmp = "G726-32/8000";
					if(!strnicmp(tmp, data_rtpmap, strlen(tmp)))
					{
						audio_payload = adp;
						session->data_payload = atoi(adp);
					}
 				}
			
				pos = 0;
				while(!osip_list_eol(OSIP2_LIST_PTR remote_media->m_payloads, pos) && !audio_payload)
				{
					tmp = (char *)osip_list_get(OSIP2_LIST_PTR remote_media->m_payloads, pos++);
					if(!tmp || !*tmp)
						continue;

					if(reg_payload && atoi(tmp) == reg_payload)
					{
						session->dtmf_payload = dtmf_negotiate;
						reg_inband = false;
					}

					if(audio_media && atoi(tmp) == session->data_payload)
						audio_payload = tmp;
				}
			}
			
			session->dtmf_inband = reg_inband;		

			if(!audio_payload)
				goto sip415;

			snprintf(cbuf, sizeof(cbuf), "a=rtpmap:%d %s",
				session->data_payload, data_rtpmap);
			session->setConst("session.sdp_encoding", cbuf);
			data_rtpmap = session->getSymbol("session.sdp_encoding");

			if(reg && reg->peering)
				session->setPeering(reg->peering);
			if(reg && reg->reconnect)
				session->setConst("session.sdp_reconnect", reg->reconnect);

			if(session->dtmf_payload)
				snprintf(cbuf, sizeof(cbuf),
					"m=audio %d RTP/AVP %d %d\r\n"
					"%s\r\n",
						session->getLocalPort(),
						session->data_payload, 
						session->dtmf_payload,
						data_rtpmap);
			else
				snprintf(cbuf, sizeof(cbuf),
					"m=audio %d RTP/AVP %d\r\n"
					"%s\r\n",
						session->getLocalPort(),
						session->data_payload,
						data_rtpmap);
				
			addString(sdp, 4096, cbuf);

			if(to->url && to->url->host)
			{
				cp = strchr(to->url->host, '.');
				if(cp)
					session->setConst("session.hostname", to->url->host);
				else
				{
					snprintf(buf, sizeof(buf), "%s.localdomain", to->url->host);
					session->setConst("session.hostname", buf);
				}
			}
			if(from->url && from->url->host)
				session->setConst("session.peername", from->url->host);

			if(session->dtmf_sipinfo)
				session->setConst("session.dtmfmode", "info");
			else if(session->dtmf_payload)
				session->setConst("session.dtmfmode", "2833");
			else
				session->setConst("session.dtmfmode", "none");

			if(session->dtmf_payload)
			{
				snprintf(cbuf, sizeof(cbuf),
					"a=rtpmap:%d telephone-event/8000",
					session->dtmf_payload);
				session->setConst("session.sdp_events", cbuf);
				snprintf(cbuf, sizeof(cbuf),
					"a=rtpmap:%d telephone-event/8000\r\n"
					"a=fmtp:%d 0-15\r\n",
					session->dtmf_payload,
					session->dtmf_payload);
				addString(sdp, 4096, cbuf);
			}

			data_rtpmap = Session::getAttributes(session->info.encoding);
			if(data_rtpmap)
			{
				snprintf(cbuf, sizeof(cbuf),
					"a=fmtp:%d %s\r\n",
					session->data_payload, data_rtpmap);
				addString(sdp, 4096, cbuf);
			}
			if(session->info.framing)
			{
				snprintf(cbuf, sizeof(cbuf),
					"a=ptime:%ld\r\n",
					session->info.framing);
				addString(sdp, 4096, cbuf);
			}

                        if(!s->postEvent(&event))
                                goto sip404;


			cp = NULL;
			if(reg)
				cp = reg->answer;
			if(!cp && inactive)
				cp = "183";
			else if(!cp)
				cp = "200";

			if(!stricmp(cp, "no"))
				cp = "183";

			if(!stricmp(cp, "yes") || !stricmp(cp, "fast"))
				cp = "200";

			if(!stricmp(cp, "100"))
			{
				eXosip_lock();
				eXosip_call_send_answer(sevent->tid, 100, NULL);
				eXosip_unlock();
				cp = "200";
			}

			if(!stricmp(cp, "ims") || !strnicmp(cp, "3g", 2))
				cp = "183";

			if(!strnicmp(cp, "ring", 4))
			{
				eXosip_lock();
				eXosip_call_send_answer(sevent->tid, 100, NULL);
				eXosip_unlock();
				cp = "180";
			}
			// maybe we have "100,xxx" as in "100,182"
			else if(strstr(cp, "100"))
			{
				eXosip_lock();
				eXosip_call_send_answer(sevent->tid, 100, NULL);
				eXosip_unlock();
			}
			if(strstr(cp, "183") && strstr(cp, "200"))
			{
				eXosip_lock();
				eXosip_call_send_answer(sevent->tid, 183, NULL);
				eXosip_unlock();
				cp = "200";
			}
			if(strstr(cp, "180"))
			{
				eXosip_lock();
				eXosip_call_send_answer(sevent->tid, 180, NULL);	
				eXosip_unlock();
				Thread::yield();
				if(!strstr(cp, "200"))
				{
					session->tid = sevent->tid;
					session->sip_answered = false;
					session = NULL;
					goto clear;
				}
			}

			eXosip_lock();
			if(strstr(cp, "183") || strstr(cp, "182"))
			{
				session->tid = sevent->tid;
				session->sip_answered = false;
				eXosip_call_build_answer(sevent->tid, atoi(cp), &msg);
			}
			else
			{
				session->sip_answered = true;				
				eXosip_call_build_answer(sevent->tid, 200, &msg);
			}

			if(!msg)
				goto sip404;
			
			osip_message_set_require(msg, "100rel");
			osip_message_set_header(msg, "RSeq", "1");
			osip_message_set_body(msg, sdp, strlen(sdp));
			osip_message_set_content_type(msg, "application/sdp");

			eXosip_call_send_answer(sevent->tid, 183, msg); 
			
			eXosip_unlock();
			session = NULL;
			goto clear;
		
clear:
			if(session)
				add(session);

cleanup:
			if(sreg && sinstance)
				--sreg->traffic->active_calls;

			sinstance = 0;
			sreg = NULL;
			eXosip_lock();
//			if(msg)
//				osip_message_free(msg);

			if(sdp)
				delString(sdp);

			if(remote_sdp)
				sdp_message_free(remote_sdp);

			if(local_sdp)
				sdp_message_free(local_sdp);

			if(from) 	                                                             
				osip_from_free(from);

			if(to)
				osip_to_free(to);

			if(remote_uri)
				osip_free(remote_uri);

			if(local_uri)
				osip_free(local_uri);

			eXosip_unlock();
		default:
			break;
		}			
		eXosip_event_free(sevent);
	}
}

} // namespace
