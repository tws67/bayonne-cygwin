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

EventStream *EventStream::free = NULL;
EventStream *EventStream::first = NULL;
EventStream *EventStream::last = NULL;
Mutex EventStream::lock;

EventStream::EventStream() : Socket()
{
	next = prev = NULL;
};

EventStream *EventStream::create(SOCKET s, const char *auth)
{
	EventStream *event;
	char buffer[256];

	lock.enter();
	event = free;
	if(!event)
		event = new EventStream();
	else
		free = event->next;

	if(!first)
	{
		first = last = event;
		event->prev = event->next = NULL;
	}
	else
	{
		event->prev = last;
		last->next = event;
		event->next = NULL;
		last = event;
	}

	event->flags.thrown = false;
	event->flags.broadcast = false;
	event->flags.route = true;
	event->flags.loopback = false;
	event->flags.multicast = false;
	event->flags.linger = false;
	event->so = s;
	event->state = CONNECTED;
	event->setKeepAlive(true);

	if(auth)
		setString(event->authorized, sizeof(event->authorized), auth);
	else
		event->authorized[0] = 0;

	snprintf(buffer, sizeof(buffer),
		"<?xml version=\"1.0\" ?>\r\n"
		"<events>\r\n");
	::send(event->so, buffer, strlen(buffer), MSG_NOSIGNAL);	
	SLOG_DEBUG("webservice/%d: streaming events", event->so);

	lock.leave();
	return event;
}

bool EventStream::isConnected(void)
{
	char buf;

	for(;;)
	{
		if(isPending(pendingError, 0))
			return false;

		if(!isPending(pendingInput, 0))
			return true;

		if(::recv(so, &buf, 1, MSG_DONTWAIT) < 1)
			return false;
	}
	return true;
}	

bool EventStream::sendEvent(const char *msg)
{
	int len = strlen(msg);

	if(::send(so, msg, len, MSG_NOSIGNAL) < len)
		return false;

	return true;
}	

void EventStream::sendReply(const char *msg)
{
	static char buffer[256];	// always inside lock
	int len = strlen(msg);

	snprintf(buffer, sizeof(buffer), 
		"HTTP/1.1 200 OK\r\n"
		"Server: Bayonne/%s\r\n"
		"Connection: close\r\n"
		"Pragma: no-cache\r\n"
		"Cache-Control: no-cache\r\n"
		"Content-Type: text/xml\r\n"
		"Content-Length: %d\r\n"
		"\r\n", VERSION, len);

	::send(so, buffer, strlen(buffer), MSG_NOSIGNAL);
	::send(so, msg, len, MSG_NOSIGNAL);
	release();
}

void EventStream::release(void)
{
	lock.enter();

	if(prev)
		prev->next = next;
	else if(first == this)
		first = next;

	if(next)
		next->prev = prev;
	else if(last == this)
		last = prev;

	next = free;
	free = this;

	endSocket();
	lock.leave();
}
	
} // namespace 
