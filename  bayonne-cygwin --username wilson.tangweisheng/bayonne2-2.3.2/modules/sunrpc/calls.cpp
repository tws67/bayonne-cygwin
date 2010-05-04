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

#include <bayonne.h>
#include "bayonne_rpc.h"

using namespace ost;
using namespace std;

class Interface : public Bayonne
{
public:
	static inline const char *getLast(const char *id)
		{return server->getLast(id);};

	static inline int getCount(void)
		{return ts_count;};

	static inline int getUsed(void)
		{return ts_used;};

	static bayonne_result *startSelect(bayonne_start *start);

	static bayonne_result *startDriver(BayonneDriver *driver, bayonne_start *start);
};

static Interface bayonne;

bayonne_result *Interface::startSelect(bayonne_start *start)
{
	static bayonne_result result;
	BayonneSpan *span;
	BayonneSession *session;
	ScriptImage *img = useImage();
	Name *scr = img->getScript(start->start_script);
	Event event;
	Line *sel;
	unsigned idx = 0, count;
	const char *cp;
	timeslot_t pos;
	static char rid[BAYONNE_SESSION_SZ];

	result.result_code = BAYONNE_SUCCESS;
	result.result_id = rid;
	strcpy(rid, "none");

	if(!scr || !scr->select || scr->access != scrPUBLIC)
	{
		endImage(img);
		if(Bayonne::start_driver && scr)
			return startDriver(start_driver, start);

		result.result_code = BAYONNE_INVALID_SCRIPT;
		return &result;
	}
	
	while(sel)
	{
                idx = 0;
                cp = strchr(cp, '.');
                if(cp && !stricmp(cp, ".span"))
                     while(NULL != (cp = sel->args[idx++]))
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
                else while(NULL != (cp = sel->args[idx++]))
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
	result.result_code = BAYONNE_BUSY;
	endImage(img);
	return &result;

start:
        memset(&event, 0, sizeof(event));
        event.id = START_OUTGOING;
        event.start.img = img; 
        event.start.scr = scr;
        event.start.dialing = start->start_number;

	if(!*event.start.dialing)
		event.start.dialing = NULL;	

        if(!start->start_caller || !*start->start_caller)
        {
                start->start_display = "bayonne";
                start->start_caller = "none";
        }

        if(!start->start_display || !*start->start_display)
                start->start_display = start->start_caller;

        session->setConst("session.caller", start->start_caller);
        session->setConst("session.display", start->start_display);

        if(!session->postEvent(&event))
        {
		result.result_code = BAYONNE_FAILURE;
                session->leave();
                endImage(img);
                return &result;
        }
	strcpy(rid, session->getExternal("session.id"));	
	session->leave();
	return &result;
}

bayonne_result *Interface::startDriver(BayonneDriver *d, bayonne_start *start)
{
	static bayonne_result result;
	BayonneSession *session;
	ScriptImage *img = useImage();
	Name *scr = img->getScript(start->start_script);
	Event event;
	static char rid[BAYONNE_SESSION_SZ];

	result.result_code = BAYONNE_SUCCESS;
	result.result_id = rid;
	strcpy(rid, "none");

	if(!d)
		result.result_code = BAYONNE_INVALID_DRIVER;
	else if(!scr || scr->access != scrPUBLIC)
		result.result_code = BAYONNE_INVALID_SCRIPT;

	if(!d || !scr || scr->access != scrPUBLIC)
	{
		endImage(img);
		return &result;
	}

	if(!start->start_caller || !*start->start_caller)
	{
		start->start_display = "bayonne";
		start->start_caller = "none";
	}

	if(!start->start_display || !*start->start_display)
		start->start_display = start->start_caller;

	session = d->getIdle();
	if(!session)
	{
		result.result_code = BAYONNE_BUSY;
		endImage(img);
		return &result;
	}

	memset(&event, 0, sizeof(event));
	event.id = START_OUTGOING;
	event.start.img = img;
	event.start.scr = scr;
	event.start.dialing = start->start_number;

	if(!*event.start.dialing)
		event.start.dialing = NULL;

	session->enter();
	session->setConst("session.caller", start->start_caller);
	session->setConst("session.display", start->start_display);

	if(!session->postEvent(&event))
	{
		result.result_code = BAYONNE_FAILURE;
		session->leave();
		endImage(img);
		return &result;
	}

	strcpy(rid, session->getExternal("session.id"));
	session->leave();
	return &result;
}

bayonne_status *bayonne_status_2_svc(void *, struct svc_req *req)
{
	static char node_server[BAYONNE_NODE_SERVER_SZ];
	static char node_version[BAYONNE_NODE_VERSION_SZ];
	static bayonne_status status;

	memset(&status, 0, sizeof(status));
	snprintf(node_server, sizeof(node_server), "%s",
		bayonne.getLast("servername"));
        snprintf(node_version, sizeof(node_version), "%s",
                bayonne.getLast("serverversion"));
	status.node_uptime = Bayonne::uptime();
	status.node_server = node_server;
	status.node_version = node_version;
	status.node_count = bayonne.getCount();
	status.node_active = bayonne.getUsed();
	return &status;
}

bayonne_error *bayonne_shutdown_2_svc(void *, struct svc_req *req)
{
	static bayonne_error result = BAYONNE_SUCCESS;

	Bayonne::down();
	return &result;
}

bayonne_error *bayonne_reload_2_svc(void *, struct svc_req *req)
{
        static bayonne_error result = BAYONNE_SUCCESS;

        Bayonne::reload();
        return &result;
}

bayonne_result *bayonne_start_2_svc(bayonne_start *s, struct svc_req *req)
{
	return Interface::startSelect(s);
}

bayonne_error *bayonne_stop_2_svc(bayonne_session *s, struct svc_req *req)
{
        static bayonne_error result = BAYONNE_SUCCESS;
	Bayonne::Event event;
	BayonneSession *session = Bayonne::getSid(s->session_id);

	if(!session)
	{
		result = BAYONNE_INVALID_SESSION;
		return &result;
	}

	memset(&event, 0, sizeof(event));
	event.id = Bayonne::STOP_SCRIPT;
	if(!session->postEvent(&event))
		result = BAYONNE_FAILURE;

	return &result;
}
	

