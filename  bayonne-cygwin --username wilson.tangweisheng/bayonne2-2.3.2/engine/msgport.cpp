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

BayonneMsgport::BayonneMsgport(BayonneDriver *driver) :
Thread(atoi(driver->getLast("priority")), atoi(driver->getLast("stack")) * 1024),
Buffer(atoi(driver->getLast("events")))
{
	unsigned size = atoi(driver->getLast("events"));

	msgdriver = driver;
	msglist = new Event[size];
	msgsize = size;
	msghead = msgtail = 0;

	setString(msgname, sizeof(msgname), driver->getName());
}

BayonneMsgport::~BayonneMsgport()
{
	shutdown();
}

void BayonneMsgport::shutdown(void)
{
	Event down;

	if(msglist)
	{
		SLOG_DEBUG("%s msgport stopping", msgname);
	
		down.timeslot = NO_TIMESLOT;
		down.id = MSGPORT_SHUTDOWN;
		post(&down, 0);

		terminate();
	
		delete[] msglist;
		msglist = NULL;
	}
}

void BayonneMsgport::update(void)
{
	Event event;

	event.timeslot = NO_TIMESLOT;
	event.id = MSGPORT_WAKEUP;

	// if already pending, no need to update.
	if(msghead != msgtail)
		return;

	if(!post(&event, 10))
		slog.warn("%s: event buffer overflow", msgname);
}

void BayonneMsgport::initial(void)
{
	timeslot_t timeslot, count;
	BayonneSession *session;

	if(!msgdriver)
		return;

	timeslot = tsfirst = msgdriver->getFirst();
	count = tscount = msgdriver->getCount();
	
	while(count--)
	{
		session = getSession(timeslot++);
		if(!session)
			continue;
		session->initialevent();
	}

	msgdriver->oink.post();	// kick the pig
}
		
timeout_t BayonneMsgport::getTimeout(Event *event)
{
	timeout_t timeout = TIMEOUT_INF, remaining;
	timeslot_t timeslot, count, selected = NO_TIMESLOT;
	BayonneSession *session;
	
	if(!tscount)
	{
		event->id = MSGPORT_WAKEUP;
		event->timeslot = NO_TIMESLOT;
		return TIMEOUT_INF;
	}

	timeslot = tsfirst;
	count = tscount;
	while(count--)
	{
		session = getSession(timeslot);
		if(!session)
		{
			++timeslot;
			continue;
		}
		session->enter();
		remaining = session->getRemaining();
		session->leave();

		if(remaining && remaining < timeout)
		{
			selected = timeslot;
			timeout = remaining;
		}	
		else if(!remaining)
		{
			event->id = TIMER_EXPIRED;
			event->timeslot = timeslot;
			session->putEvent(event);
		}
		++timeslot;
	}

	event->id = TIMER_EXPIRED;
	event->timeslot = selected;
	return timeout;	
}

void BayonneMsgport::run(void)
{
	BayonneSession *session;
	Event event;
	timeout_t timeout;

	SLOG_DEBUG("%s msgport starting", msgname);

	for(;;)
	{
		Thread::yield();

		timeout = getTimeout(&event);

		if(timeout)
		{
			if(timeout == TIMEOUT_INF)
				wait(&event);
			else
				wait(&event, timeout);
		}

		switch(event.id)
		{
		case MSGPORT_SHUTDOWN:
			Thread::sync();
		default:
			break;
		}

		session = getSession(event.timeslot);
		if(session)
			session->putEvent(&event);
	}
}

size_t BayonneMsgport::onWait(void *buf)
{
	Event *ep = (Event *)buf;
	
	memcpy(ep, &msglist[msghead++], sizeof(Event));
	if(msghead >= msgsize)
		msghead = 0;

	return sizeof(Event);
}

size_t BayonneMsgport::onPost(void *buf)
{
        Event *ep = (Event *)buf;

	memcpy(&msglist[msgtail++], ep, sizeof(Event));
        if(msgtail >= msgsize)
                msgtail = 0;

        return sizeof(Event);
}

size_t BayonneMsgport::onPeek(void *buf)
{
        Event *ep = (Event *)buf;
	if(msghead == msgtail)
		return 0;

	memcpy(ep, &msglist[msghead], sizeof(Event));
	return sizeof(Event);
}


	
