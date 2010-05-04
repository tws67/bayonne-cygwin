// Copyright (C) 2005 Open Source Telecom Corporation.
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
#ifndef	WIN32
#include "private.h"
#endif

namespace moduleWebservice {
using namespace ost;
using namespace std;

unsigned long Session::htmlAccess(const char *fname)
{
	char path[256];
	struct stat ino;
	const char *prefix = Service::webservice.getString("datafiles");

	snprintf(path, sizeof(path), "%s/%s", prefix, fname);

	if(stat(path, &ino))
		return 0;

	return ino.st_size;
}

void Session::htmlInsert(const char *path, const char *title)
{
	char cbuf[256];
	FILE *fp;
	const char *cp;
	char *dp = buffer;
	const char *prefix = Service::webservice.getString("datafiles");
        const char *sl = sla;
        unsigned long upt = uptime();
        char dur[65];

        if(upt < 3600)
                snprintf(dur, sizeof(dur), "%ld minute(s)", upt / 60);
        else if(upt < (36000l))
                snprintf(dur, sizeof(dur), "%ld hour(s), %ld minute(s)",
                        upt / 3600, (upt / 60) % 60);
        else if(upt < (360000l))
                snprintf(dur, sizeof(dur), "%ld hours", upt / 3600);
        else
                snprintf(dur, sizeof(dur), "%ld days", upt / (3600l * 24l));

        if(!sl || !*sl)
                sl = "up";

	snprintf(cbuf, sizeof(cbuf), "%s/%s", prefix, path);
	fp = fopen(cbuf, "r");

	if(!fp)
		return;

	while(NULL != fgets(cbuf, sizeof(cbuf), fp))
	{
		if(feof(fp))
			break;

		cp = cbuf;
		while(*cp)
		{
			switch(*cp)
			{
			case '\n':
				*(dp++) = '\r';
				*(dp++) = '\n';
			case '\r':
				++cp;
				break;
			case '%':
				*dp = 0;
				if(!strncmp(cp, "%title%", 7) && title)
				{
					cp += 7;
					strcpy(dp, title);
				}	
				else if(!strncmp(cp, "%version%", 9))
				{
					cp += 9;
					strcpy(dp, VERSION);
				}
				else if(!strncmp(cp, "%node%", 6))
				{
					cp += 6;
					strcpy(dp, server->getLast("servername"));
 				}
				else if(!strncmp(cp, "%timeslots%", 11))
 				{
					cp += 11;
					snprintf(dp, 10, "%d", ts_used);
				}
				else if(!strncmp(cp, "%level%", 7))
				{
					cp += 7;
					strcpy(dp, sl);
				}
				else if(!strncmp(cp, "%uptime%", 8))
				{
					cp += 8;
					strcpy(dp, dur);
				}  
				if(*dp)
				{
					dp += strlen(dp);
					break;
				} 
			default:
				*(dp++) = *(cp++);
			}
		}
	}
	fclose(fp);
	*dp = 0;
	if(dp > buffer)
		sendText(buffer);
}

void Session::htmlFooter(void)
{
	htmlInsert("footer.part", NULL);
        sendText("</BODY></HTML>\r\n");
        sendText("\r\n");
	syncExit();
}

void Session::htmlHeader(const char *title, bool refresh)
{
	sendHeader();
	snprintf(buffer, sizeof(buffer), "<HTML><HEAD><TITLE>%s</TITLE>\r\n", title);
	sendText(buffer);
	if(refresh)
	{
		snprintf(buffer, sizeof(buffer),
			"<META HTTP-EQUIV=Refresh CONTENT=\"%s\"/>\r\n",
			Service::webservice.getString("refresh"));
		sendText(buffer);
	}
	sendText("</HEAD>\r\n");
	htmlInsert("header.part", title);
//	snprintf(buffer, sizeof(buffer), "<BODY><H1>%s</H1><P>\r\n", title); 
//	sendText(buffer);
}

void Session::htmlCalls(void)
{
	BayonneSession *s;
	timeslot_t index = 0;
	char status[16];
	time_t now;
	long diff;
	const char *from, *to;

	htmlHeader("Bayonne Calls", true);
	sendText("<TABLE border=\"1\" summary=\"Active Calls\">\r\n");
	sendText("<CAPTION><EM>Active Calls</EM></CAPTION>\r\n");
	sendText("<TR><TH>Parent<TH>Child<TH>Status<TH>From<TH>To\r\n");

	time(&now);
	while(index < ts_used)
	{
		s = getSession(index++);
		if(!s)
			continue;

		s->enter();
		if(!s->isAssociated())
		{
			s->leave();
			continue;
		}

		if(!s->getJoined())
			setString(status, sizeof(status), "ringing");
		else
		{
			diff = (long)(now - s->getJoined());
			snprintf(status, sizeof(status), "%02ld:%02ld:%02ld",
				diff / 3600l, (diff / 60) % 60, diff % 60);
		}
		from = s->getSymbol("session.associated");
		if(!from || !*from)
			from = s->getSymbol("session.caller");
		to = s->getSymbol("session.identity");
		if(!to)
			to = s->getSymbol("session.dialed");
		snprintf(buffer, sizeof(buffer),
			"<TR><TD>%s<TD>%s<TD>%s<TD>%s<TD>%s\r\n",
			s->getExternal("session.pid"),
			s->getExternal("session.id"),
			status, from, to);

		sendText(buffer);
		s->leave();
	}
	sendText("</TABLE>\r\n");
	htmlFooter();
}

void Session::htmlSessions(void)
{
	BayonneSession *s;
	timeslot_t index = 0;

	htmlHeader("Bayonne Calls", true);
	sendText("<TABLE border=\"1\" summary=\"Active Calls\">\r\n");
	sendText("<CAPTION><EM>Active Calls</EM></CAPTION>\r\n");
	sendText("<TR><TH>Session<TH>Parent<TH>Type<TH>Time<TH>Duration<TH>From<TH>Caller<TH>Line\r\n");

	while(index < ts_used)
	{
		s = getSession(index++);
		if(!s)
			continue;

		s->enter();
		if(s->isIdle())
		{
			s->leave();
			continue;
		}
		snprintf(buffer, sizeof(buffer),
			"<TR><TD>%s<TD>%s<TD>%s<TD>%s<TD>%s<TD>%s<TD>%s<TD>%s\r\n",
			s->getExternal("session.id"),
			s->getExternal("session.pid"),
			s->getExternal("session.type"),
			s->getExternal("session.time"),
			s->getExternal("session.duration"),
			s->getSymbol("session.caller"),
			s->getSymbol("session.display"),
			s->getExternal("session.line"));
		sendText(buffer);
		s->leave();
	}
	sendText("</TABLE>\r\n");
	htmlFooter();
}


void Session::htmlControl(void)
{
	const char *cmd = NULL;
	bool valid = true;
	bool success = false;
	const char *sid = NULL;
	char *tok;
	const char *qtarget = NULL;
	const char *qscript = NULL;
	const char *qcaller = NULL;
	const char *qdisplay = NULL;
	
	const char *qsid = NULL;
	const char *errmsg = NULL;
	BayonneSession *s;
	Event event;

	if(req_query)
		req_query = strtok_r(req_query, "&\r\n", &tok);

	while(req_query)
	{
		if(!strnicmp(req_query, "command=", 8)) 
			cmd = urlDecode(req_query + 8);
		else if(!strnicmp(req_query, "session=", 8))
			qsid = urlDecode(req_query + 8);
		else if(!strnicmp(req_query, "target=", 7))
			qtarget = urlDecode(req_query + 7);
		else if(!strnicmp(req_query, "script=", 7))
			qscript = urlDecode(req_query + 7);
		else if(!strnicmp(req_query, "caller=", 7))
			qcaller = urlDecode(req_query + 7);
		else if(!strnicmp(req_query, "display=", 8))
			qdisplay = urlDecode(req_query + 8); 
		
		req_query = strtok_r(NULL, "&\r\n", &tok);
	}

	htmlHeader("Bayonne Control", false);

	if(!cmd)
	{
		sendText("Request: missing<BR>\r\n");
		htmlFooter();
	}

	if(!stricmp(cmd, "start"))
	{
		if(!qscript)
		{
			errmsg = "script name missing";
			goto results;
		}
		if(!qtarget)
		{
			errmsg = "destination missing";
			goto results;
		}
		s = Bayonne::startDialing(qtarget, qscript, qcaller, qdisplay, NULL, "webservice");
		if(s)
		{
			sid = s->getExternal("session.id");
			success = true;
			s->leave();
		}
		goto results;
	}			

	if(!stricmp(cmd, "stop"))
	{
		if(!qsid)		
		{
			errmsg = "session id missing";
			goto results;
		}
		s = getSid(qsid);
		if(!s)
		{
			errmsg = "session id invalid";
			goto results;
		}
		memset(&event, 0, sizeof(event));
		event.id = STOP_SCRIPT;
		if(s->postEvent(&event))
			success = true;
		else
			errmsg = "stop ignored";
		goto results;				
	}
	if(!stricmp(cmd, "hold"))
	{
		if(!qsid)		
		{
			errmsg = "session id missing";
			goto results;
		}
		s = getSid(qsid);
		if(!s)
		{
			errmsg = "session id invalid";
			goto results;
		}
		memset(&event, 0, sizeof(event));
		event.id = CALL_HOLD;
		if(s->postEvent(&event))
			success = true;
		else
			errmsg = "hold ignored";
		goto results;				
	}
	else if(!stricmp(cmd, "resume"))
	{
		if(!qsid)		
		{
			errmsg = "session id missing";
			goto results;
		}
		s = getSid(qsid);
		if(!s)
		{
			errmsg = "session id invalid";
			goto results;
		}
		memset(&event, 0, sizeof(event));
		event.id = CALL_NOHOLD;
		if(s->postEvent(&event))
			success = true;
		else
			errmsg = "resume ignored";
		goto results;				
	}
	else if(!stricmp(cmd, "reload"))
	{
		Bayonne::reload();
		success = true;
	}
	else if(!stricmp(cmd, "shutdown"))
	{
		Service::active = false;
		Bayonne::down();
		success = true;
	}
	else
		valid = false;

results:
	snprintf(buffer, sizeof(buffer), "Request: %s<BR>\r\n", cmd);
	sendText(buffer);

	if(!valid)
	{
		sendText("Result: command invalid<BR>\r\n");
		htmlFooter();
	}

	if(success)
		sendText("Result: success<BR>\r\n");
	else if(errmsg)
	{
		snprintf(buffer, sizeof(buffer),
			"Result: %s<BR>\r\n", errmsg);
		sendText(buffer);
	}
	else
		sendText("Result: failed<BR>\r\n");

	if(sid)
	{
		snprintf(buffer, sizeof(buffer), "Session: %s<BR>\r\n", sid);
		sendText(buffer);
	}
	htmlFooter();
}

void Session::htmlStatus(void)
{	
	BayonneDriver *d = BayonneDriver::getPrimary();
	unsigned count;
	regauth_t *r;
	char dbuf[33];
	char abuf[33];
	time_t tb;

	time(&tb);
	htmlHeader("Bayonne Status", true);

	sendText("<TABLE border=\"1\" summary=\"Installed Drivers\">\r\n");
	sendText("<CAPTION><EM>Drivers loaded into server</EM></CAPTION>\r\n");
	sendText("<TR><TH rowspan=\"2\">Name"
		"<TH colspan=\"3\">Timeslots"
		"<TH colspan=\"2\">Spans"
		"<TH colspan=\"2\">Total"
		"<TH colspan=\"2\">Completed\r\n");
	sendText("<TR><TH>first<TH>count<TH>active<TH>first<TH>count"
		"<TH>incoming<TH>outgoing<TH>incoming<TH>outgoing\r\n");
	while(d)
	{
		snprintf(buffer, sizeof(buffer), 
			"<TR><TD>%s<TD>%d<TD>%d<TD>%d<TD>%d<TD>%d"
			"<TD>%lu<TD>%lu<TD>%lu<TD>%lu\r\n",
			d->getName(), (int)d->getFirst(), (int)d->getCount(),
			(int)d->active_calls,
 			(int)d->getSpanFirst(), (int)d->getSpansUsed(),
			d->call_attempts.iCount, d->call_attempts.oCount,
			d->call_complete.iCount, d->call_complete.oCount);

		sendText(buffer);
		d = d->getNext();
	}	
	sendText("</TABLE><BR>\r\n");

	d = BayonneDriver::getPrimary();
	sendText("<TABLE border=\"1\" summary=\"Server Registrations\">\r\n");
	sendText("<CAPTION><EM>Server Registrations</EM></CAPTION>\r\n");
	sendText("<TR><TH>Remote<TH>User<TH>Type<TH>Status"
		"<TH>Refresh<TH>Incoming<TH>Outgoing<TH>active\r\n");
	while(d)
	{
		reloading.readLock();
		count = d->getRegistration(regs, 1024);
		r = regs;
		while(count--)
		{
			if(r->updated)
			{
				r->updated = (unsigned long)tb - r->updated;
				snprintf(dbuf, sizeof(dbuf), "%02ld:%02ld:%02ld",
					r->updated / 3600,
					(r->updated / 60) % 60,
					r->updated % 60);
			}
			else
				strcpy(dbuf, "-");

			if(r->call_limit)
				snprintf(abuf, sizeof(abuf), "%u/%u", 
					(int)r->active_calls, (int)r->call_limit);
			else
				snprintf(abuf, sizeof(abuf), "%u/-",
					(int)r->active_calls);

			snprintf(buffer, sizeof(buffer),
				"<TR><TD>%s<TD>%s<TD>%s<TD>%s"
				"<TD>%s<TD>%lu<TD>%lu<TD>%s\r\n",
				r->remote, r->userid, r->type, r->status,
				dbuf, 
				r->attempts_iCount,
				r->attempts_oCount,
				abuf);
			sendText(buffer);
			++r;
		} 
		reloading.unlock();
		d = d->getNext();
	}
	sendText("</TABLE><BR>\r\n");

        snprintf(buffer, sizeof(buffer),
                "Completed incoming calls: %lu<BR>\r\n",
                Bayonne::total_call_complete.iCount);
        sendText(buffer);
        snprintf(buffer, sizeof(buffer),
                "Completed outgoing calls: %lu<BR>\r\n",
                Bayonne::total_call_complete.oCount);
	sendText(buffer);

	snprintf(buffer, sizeof(buffer), 
		"Total incoming calls: %lu<BR>\r\n", 
		Bayonne::total_call_attempts.iCount);
	sendText(buffer);
	snprintf(buffer, sizeof(buffer), 
		"Total outgoing calls: %lu<BR>\r\n", 
		Bayonne::total_call_attempts.oCount);
	sendText(buffer);

	snprintf(buffer, sizeof(buffer),
		"Total active calls: %u<BR>\r\n",
		(int)Bayonne::total_active_calls);
	sendText(buffer);

	snprintf(buffer, sizeof(buffer), "<BR>Activity: %s<BR>\r\n", Bayonne::status);
	sendText(buffer);

	htmlFooter();	
}

} // namespace 
