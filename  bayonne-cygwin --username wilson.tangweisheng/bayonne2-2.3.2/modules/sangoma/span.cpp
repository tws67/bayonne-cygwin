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

namespace sangomadriver {
using namespace ost;
using namespace std;

int Span::sspan_count = 1;

Span::Span(Driver *d, timeslot_t count) :
BayonneSpan(d, count), Thread(atoi(d->getLast("priority")))
{
	timeslot_t dchan = count;
	timeslot_t bchans = count - 1;
	sspan = sspan_count++;	

	if(sangoma_init_pri(&spri, sspan, dchan, bchans, d->pri_switch, d->pri_node, 0))
	{
		slog.error("sangoma span %d: failed to initialize", sspan);
		return;
	}

//	SANGOMA_MAP_PRI_EVENT(spri, SANGOMA_PRI_EVENT_ANY, pe_any);
	SANGOMA_MAP_PRI_EVENT(spri, SANGOMA_PRI_EVENT_RING, pe_ring);
	SANGOMA_MAP_PRI_EVENT(spri, SANGOMA_PRI_EVENT_HANGUP_REQ, pe_hangup);
//	SANGOMA_MAP_PRI_EVENT(spri, SANGOMA_PRI_EVENT_INFO_RECEIVED, pe_info);
	SANGOMA_MAP_PRI_EVENT(spri, SANGOMA_PRI_EVENT_RESTART, pe_restart);
}

void Span::run(void)
{
	sangoma_run_pri(&spri);
}

void Span::onRing(pri_event *pevent)
{
        Session *s;
        Event event;

        s = (Session *)Span::getTimeslot(pevent->hangup.channel - 1);
	if(!s)
		return;

	memset(&event, 0, sizeof(event));

	s->enterMutex();
	if(s->isOffhook())
	{
		event.id = STOP_DISCONNECT;
		s->leaveMutex();
		s->queEvent(&event);
		return;
	}
	memcpy(&s->call, pevent->ring.call, sizeof(q931_call));
	event.id = RING_ON;
	s->postEvent(&event);
	memset(&event, 0, sizeof(event));
	event.id = RING_OFF;
	s->leaveMutex();
	s->queEvent(&event);
}

void Span::onRestart(pri_event *pevent)
{
	Session *s;
	Event event;

        s = (Session *)Span::getTimeslot(pevent->hangup.channel - 1);
	if(!s)
		return;

	slog.error("sangoma(%d,%d): restarting", sspan, pevent->hangup.channel - 1);
	memset(&event, 0, sizeof(event));
	event.id = STOP_DISCONNECT;
	s->enterMutex();
	s->clearCall();
	s->leaveMutex();
}

void Span::onHangup(pri_event *pevent)
{
	Session *s;
	Event event;


        s = (Session *)Span::getTimeslot(pevent->hangup.channel - 1);

	if(!s)
		pri_hangup(spri.pri, pevent->hangup.call, pevent->hangup.cause);
	
	memset(&event, 0, sizeof(event));
	event.id = STOP_DISCONNECT;
	s->enterMutex();
	if(s->clearCall())
		pri_hangup(spri.pri, pevent->hangup.call, pevent->hangup.cause);
	s->leaveMutex();
	s->queEvent(&event);
}
	
} // namespace
