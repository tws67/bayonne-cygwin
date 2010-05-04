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

#include "module.h"

namespace binder {
using namespace ost;
using namespace std;

class Image : public ScriptImage
{
private:
    Image() : ScriptImage(NULL, NULL) {};

public:
    inline void *alloc(size_t size)
		{return ScriptImage::alloc(size);};
};

const char *Checks::chkJoin(Line *line, ScriptImage *img)
{
	if(line->argc)
		return "join uses no keywords or arguments";

	return NULL;
}

const char *Checks::chkRedial(Line *line, ScriptImage *img)
{
	if(line->argc > 1)
		return "redial may have optional variable only";

	return NULL;
}

const char *Checks::chkRecall(Line *line, ScriptImage *img)
{
	if(line->argc)
		return "recall uses no keywords or arguments";

	return NULL;
}


const char *Checks::chkFeature(Line *line, ScriptImage *img)
{
        if(!useKeywords(line, "=encoding=framing"))
                return "invalid keyword used for reconnect";

        return NULL;
}

const char *Checks::chkRing(Line *line, ScriptImage *img)
{
	unsigned idx = 0;

	if(!useKeywords(line, "=coverage=delay=timeout=duration=tone=encoding=framing"))
		return "invalud keyword used for ring";

	if(!getOption(line, &idx))
		return "select at least one ring group script to use";

	return NULL;
}

const char *Checks::chkConnect(Line *line, ScriptImage *img)
{
	unsigned idx = 0;
	
	if(!useKeywords(line, "=session=caller=display=timeout=duration=tone=encoding=framing"))
		return "invalid keyword used for connect";

	if(!getOption(line, &idx))
		return "missing dial option";

	if(!getOption(line, &idx))
		return NULL;

	if(getOption(line, &idx))
		return "too many arguments for connect";

	return NULL;
}

const char *Checks::chkStart(Line *line, ScriptImage *img)
{
        unsigned idx = 0;
	const char *cp;

        if(!useKeywords(line, "=session=caller=display"))
                return "invalid keyword used for start";

        if(!getOption(line, &idx))
                return "no dial command specified";

        if(!getOption(line, &idx))
                return "no script specified";

	while(NULL != (cp = getOption(line, &idx)))
	{
		if(*cp != '&' && *cp != '%')
			return "remaining arguments must be variables";
	}

        return NULL;
}

const char *Checks::chkStop(Line *line, ScriptImage *img)
{
	if(getMember(line))
		return "stop has no members";

	if(hasKeywords(line))
		return "stop uses no keywords";

	if(line->argc < 1)
		return "stop missing session id argument";

	if(line->argc > 1)
		return "stop only has one argument";

	return NULL;
}

const char *Checks::chkEndinput(Line *line, ScriptImage *img)
{
        if(getMember(line))
                return "endinput has no members";

        if(line->argc > 0)
                return "endinput has no arguments";

        return NULL; 
}  

const char *Checks::chkDial(Line *line, ScriptImage *img)       
{
        unsigned idx = 0;

        if(getMember(line))
                return "dial uses no members";  

        if(!useKeywords(line, "=timeout"))
                return "unknown keyword for dial";

        if(!getOption(line, &idx))
                return "dialed number argument missing";

        return NULL;
} 

const char *Checks::chkCDR(Line *line, ScriptImage *img)
{
	char var[65];
	char *p;
	unsigned idx = 0;
	const char *cp;

	Name *scr = img->getCurrent();
	snprintf(var, sizeof(var), "cdr.%s", scr->name);
	p = strchr(var, ':');
	if(p)
		*p = 0;

	if(img->getPointer(var))
		return "cdr already assigned for this script file";

	if(getMember(line))
		return "cdr uses no members";

	if(hasKeywords(line))
		return "cdr has no keywords";

	while(NULL != (cp = getOption(line, &idx)))
	{
		if(!isalpha(*cp) && *cp != '%')
			return "invalid cdr entry used";
	}

	img->setPointer(var, line);
	return "";
}

const char *Checks::chkRegister(Line *line, ScriptImage *img)
{
	const char *cp = getMember(line);
	Name *scr = img->getCurrent();
	BayonneDriver *driver;
	const char *ext = strrchr(scr->filename, '.');
	char dbuf[15];
	char *sp;

	if(scr->access != ScriptInterp::scrPUBLIC)
		return "cannot register non-public script";

	if(scr->first && !stricmp(scr->first->cmd, "_keydata_"))
		return "cannot add additional registration to this entry";

	if(ext && !stricmp(ext, ".conf"))
	{
		if(cp)
			return "cannot register to alternate in config";

		cp = strrchr(scr->filename, '/');
		if(cp)
			++cp;
		else
			cp = scr->filename;

		setString(dbuf, sizeof(dbuf), cp);
		sp = strchr(dbuf, '.');
		if(sp)
			*sp = 0;
		cp = dbuf;
	}

	if(cp)
		driver = BayonneDriver::get(cp);
	else
		driver = BayonneDriver::getProtocol();
		
	if(!driver)
	{
		slog.warn("cannot register %s", scr->name);
		return "";
	}
	return driver->registerScript(img, line);
}

const char *Checks::chkKeydata(Line *line, ScriptImage *ip)
{
	Image *img = (Image *)ip;
	Name *scr = img->getCurrent();
	BayonneDriver *driver;
	const char *cp;
	char dbuf[15];
	char *sp;

	if(scr->access != ScriptInterp::scrPUBLIC)
		return "cannot register non-public script";

	cp = strrchr(scr->filename, '/');
	if(cp)
		++cp;
	else
		cp = scr->filename;

	setString(dbuf, sizeof(dbuf), cp);
	sp = strchr(dbuf, '.');
	if(sp)
		*sp = 0;
	driver = BayonneDriver::get(dbuf);

	if(!driver)
	{
		slog.warn("cannot register keydata %s with driver", scr->name);
		return "";
	}

	cp = findKeyword(img, line, "type");
	if(cp && !strnicmp(cp, "ext", 3))
		return "extensions not supported for xml app server binding";

	if(cp && !stricmp(cp, "agent"))		
		return "acd agents not support for xml app server binding";

	line->scr.method = (Method)&Methods::scrKeydata;
	line = (Line *)memcpy(img->alloc(sizeof(Line)), line, sizeof(Line));

	cp = driver->registerScript(ip, line);
	if(cp && *cp)
		return cp;
	return NULL;
}

const char *Checks::chkAssign(Line *line, ScriptImage *img)
{
	const char *cp = getMember(line);
	Name *scr = img->getCurrent();
	unsigned idx = 0, spid;
	char tag[65];
	BayonneDriver *driver;
	BayonneSession *s;
	const char *ext = strrchr(scr->filename, '.');

	if(scr->first && !stricmp(scr->first->cmd, "_keydata_"))
		return "cannot add assignment to this entry";

	if(ext && !stricmp(ext, ".conf"))
		return "assign not used in config";

	if(hasKeywords(line))
		return "keywords not used for assign";

	if(!line->argc)
		return "missing entries to assign";

	if(scr->access != ScriptInterp::scrPUBLIC)
		return "cannot assign non-public script";	

	if(!stricmp(line->cmd, "incoming"))
		goto dnis;

	if(!stricmp(line->cmd, "caller"))
		goto cid;

	if(!stricmp(line->cmd, "incoming.timeslot"))
		goto timeslot;

	if(!stricmp(line->cmd, "incoming.span"))
		goto span;

	if(!strnicmp(line->cmd, "incoming.", 9))
		return "invalid member for incoming"; 

	if(cp)
	{
		if(!stricmp(cp, "timeslot"))
			goto timeslot;

		if(!stricmp(cp, "span"))
			goto span;

		if(!stricmp(cp, "dnis"))
			goto dnis;

		if(!stricmp(cp, "did"))
			goto dnis;

		if(!stricmp(cp, "cid"))
			goto cid;

		driver = BayonneDriver::get(cp);
		if(!driver)
		{
			slog.warn("cannot assign %s with %s", scr->name, cp);
			return "";
		}
		return driver->assignScript(img, line);
	}

timeslot:
	while(NULL != (cp = getOption(line, &idx)))
	{
		if(!stricmp(cp, "default"))
		{
			slog.debug("assigning unassigned timeslots to %s", scr->name);
			img->setPointer("ts.default", scr);
			continue;
		}
		s = getSid(cp);
		if(s && s->getInterface() == IF_INET)
			s = NULL;
		if(!s)
		{
			slog.error("server cannot assign %s; invalid timeslot", cp);
			continue;
		}
		slog.debug("assigning timeslot %s to %s", cp, scr->name);
		snprintf(tag, sizeof(tag), "ts.%d", s->getSlot());
		img->setPointer(tag, scr);
	}
	return "";

span:
	while(NULL != (cp = getOption(line, &idx)))
	{
		spid = atoi(cp);
		slog.debug("assigning span %s to %s", cp, scr->name);
		snprintf(tag, sizeof(tag), "span.%d", spid);
		img->setPointer(tag, scr);
	}
	return "";

cid:
	while(NULL != (cp = getOption(line, &idx)))
	{
		slog.debug("assigning caller number %s to %s", cp, scr->name);
		snprintf(tag, sizeof(tag), "cid.%s", cp);
		img->setPointer(tag, scr);
	}
	return "";

dnis:
        while(NULL != (cp = getOption(line, &idx)))
        {
		if(!stricmp(cp, "default"))
		{
			cp = "default";
			slog.debug("assigning inbound default %s", scr->name);
		}
		else
	                slog.debug("assigning dialed number %s to %s", cp, scr->name);
                snprintf(tag, sizeof(tag), "did.%s", cp);
                img->setPointer(tag, scr);
        }
        return "";
}	

const char *Checks::chkSelect(Line *line, ScriptImage *img)
{
	const char *cp = getMember(line), *sn = "";
	Name *scr = img->getCurrent();
	timeslot_t ts;
	unsigned idx = 0, spid;
	interface_t iface;
	BayonneDriver *driver;
	BayonneSession *session;
	char *sp;
	const char *ext = strrchr(scr->filename, '.');
	const char *fn = strrchr(scr->filename, '/');
	char buf[65];
	char sbuf[96];

	if(fn)
		++fn;
	else
		fn = scr->filename;

	if(hasKeywords(line))
		return "keywords not used for select";

	if(!line->argc)
		return "missing entries to select";

	if(scr->access != ScriptInterp::scrPUBLIC)
		return "cannot select from non-public script";	
	
	snprintf(sbuf, sizeof(sbuf), "select.%s", scr->name);

	if(!stricmp(ext, ".conf"))
	{
		setString(buf, sizeof(buf), fn);
		sp = strchr(buf, '.');		
		if(sp)
			*sp = 0;
		driver = BayonneDriver::get(buf);
drivermode:
		sn = strrchr(scr->name, ':');
		if(sn)
			++sn;
		else
			sn = scr->name;

		if(!driver && stricmp(buf, "trunk"))
			return "cannot select in this config";

		if(!driver)
		{
			snprintf(sbuf, sizeof(sbuf), "select.%s", sn);
			goto trunking;
		}

		cp = getOption(line, &idx);
		if(!cp)
			return "default server missing from select";

		if(strchr(cp, '@'))
			return "default server cannot include user";

		snprintf(sbuf, sizeof(sbuf), "start-%s.%s", buf, sn);
		img->setPointer(sbuf, scr);
		img->addSelect(line);
		return "";
	}

trunking:
	img->setPointer(sbuf, scr);

	if(cp)
	{
		if(!stricmp(cp, "timeslot"))
			goto timeslot;

		if(!stricmp(cp, "span"))
			goto span;

		driver = BayonneDriver::get(cp);
		if(driver)
		{
			setString(buf, sizeof(buf), cp);
			goto drivermode;
		}

		return "unknown form of select";
	}

timeslot:
	while(NULL != (cp = getOption(line, &idx)))
	{
		ts = toTimeslot(cp);
		if(ts == NO_TIMESLOT)
		{
			slog.error("server cannot select %s; no such timeslot", cp);
			continue;
		}
		session = getSession(ts);
		if(!session)
		{
			slog.error("server cannot select %s; timeslot not allocated", cp);
			continue;
		}
		iface = session->getInterface();
		switch(iface)
		{
		case IF_INET:
			slog.error("server cannot select %s; non-physical port", cp);
			break;
		default:
			slog.debug("selecting timeslot %s for %s", cp, scr->name);
			img->addSelect(line);
		}
	}
	return "";

span:
	while(NULL != (cp = getOption(line, &idx)))
	{
		spid = atoi(cp);
		if(BayonneSpan::get(spid))
		{
			slog.debug("selecting span %s for %s", cp, scr->name);
			img->addSelect(line);
		}
		else
			slog.error("cannot select span %d; does not exist", spid);

	}
	return "";
}	

const char *Checks::chkKey(Line *line, ScriptImage *img)
{
        if(getMember(line))
                return "key doesnt use members";

        if(!useKeywords(line, "=index=value"))
                return "unknown keyword used for key";

        return chkAllVars(line, img);
}  

} // end namespace
