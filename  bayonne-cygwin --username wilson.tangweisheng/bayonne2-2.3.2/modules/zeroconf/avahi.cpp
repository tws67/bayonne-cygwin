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

#include <engine.h>

#ifdef	ZEROCONF_AVAHI
#include <cc++/slog.h>
#include <cc++/socket.h>
#include <avahi-client/client.h>
#include <avahi-client/publish.h>
#include <avahi-common/alternative.h>
#include <avahi-common/simple-watch.h>
#include <avahi-common/malloc.h>
#include <avahi-common/error.h>
#include <avahi-common/timeval.h>

namespace moduleZeroconf {
using namespace ost;
using namespace std;

class Service : public BayonneService, public StaticKeydata
{
private:
	AvahiSimplePoll *poller;
	AvahiClient *client;
	AvahiEntryGroup *group;
	char *name;
	int error;
	void run(void);
	void stopService(void);

public:
	static Service zeroconf;
	Service();

	inline void setClient(AvahiClient *c)
		{client = c;};

	void setClient(AvahiClientState state);
	void setGroup(AvahiEntryGroupState state);
};

extern "C" {

static void client_callback(AvahiClient *c, AvahiClientState state, void *userdata)
{
	if(!c)
		return;

	Service::zeroconf.setClient(c);
	Service::zeroconf.setClient(state);
}

static void group_callback(AvahiEntryGroup *g, AvahiEntryGroupState state, void *userdata)
{
	Service::zeroconf.setGroup(state);
}

};

Service Service::zeroconf;

static Keydata::Define defkeys[] = {
    {"name", "bayonne"},
    {NULL, NULL}};

Service::Service() :
BayonneService(0, 0), 
StaticKeydata("/bayonne/module/zeroconf", defkeys)
{
	name = avahi_strdup(getString("name"));
	group = NULL;
	client = NULL;
}

void Service::setGroup(AvahiEntryGroupState state)
{
	char *newname;

	switch(state)
	{
	case AVAHI_ENTRY_GROUP_ESTABLISHED:
		slog.info("zeroconf: %s service(s) established", name);
		break;
	case AVAHI_ENTRY_GROUP_COLLISION:
		newname = avahi_alternative_service_name(name);
		slog.notice("zeroconf: service %s renamed %s", name, newname);
		avahi_free(name);
		name = newname;
		setClient(AVAHI_CLIENT_S_RUNNING);
	case AVAHI_ENTRY_GROUP_FAILURE:
		slog.error("zeroconf: service failure; error=%s",
			avahi_strerror(avahi_client_errno(client)));
		avahi_simple_poll_quit(poller);
	default:
		break;			
	}
}	

void Service::setClient(AvahiClientState state)
{
	BayonneZeroconf *node;
	int ret;
	unsigned count = 0;
	AvahiProtocol family = AVAHI_PROTO_UNSPEC;

	switch(state)
	{
	case AVAHI_CLIENT_S_RUNNING:
		goto add;
	case AVAHI_CLIENT_FAILURE:
failed:
		slog.error("zeroconf failure; error=%s",
			avahi_strerror(avahi_client_errno(client)));
		avahi_simple_poll_quit(poller);
		break;
	case AVAHI_CLIENT_S_COLLISION:
	case AVAHI_CLIENT_S_REGISTERING:
		if(group)
			avahi_entry_group_reset(group);
	default:
		break;
	}
	return;

add:
	if(!group)
		group = avahi_entry_group_new(client, group_callback, NULL);
	if(!group)
		goto failed;

	node = BayonneZeroconf::getFirst();
	while(node)
	{
		switch(node->getFamily())
		{
		case BayonneZeroconf::ZEROCONF_IPANY:
			family = AVAHI_PROTO_UNSPEC;
			break;
		case BayonneZeroconf::ZEROCONF_IPV4:
			family = AVAHI_PROTO_INET;
			break;
		case BayonneZeroconf::ZEROCONF_IPV6:
			family = AVAHI_PROTO_INET6;
			break;
		}
		if(node->getPort())
		{
			SLOG_DEBUG("zeroconf: adding service=%s, type=%s, port=%d",
				name, node->getType(), node->getPort());
			ret = avahi_entry_group_add_service(group,
				AVAHI_IF_UNSPEC, family,
				(AvahiPublishFlags)0, name, 
				node->getType(), NULL, NULL,
				node->getPort(), NULL);
		}
		else
			ret = 0;
		if(ret < 0)
			slog.error("zeroconf: %s failed; error=%s",
				node->getType(), avahi_strerror(ret));
		else if(node->getPort())
			++count;
		node = node->getNext();
	}	
	if(!count)
	{
		slog.error("zeroconf: no services registered");
		avahi_simple_poll_quit(poller);
		return;
	}
	ret = avahi_entry_group_commit(group);
	if(ret >= 0)
		return;

	slog.error("zeroconf service commit failure; error=%s",
		avahi_strerror(ret));
}

void Service::stopService(void)
{
	terminate();
	if(client)
		avahi_client_free(client);
	if(poller)
		avahi_simple_poll_free(poller);
	if(name)
		avahi_free(name);
	client = NULL;
	poller = NULL;
	name = NULL;
}

void Service::run(void)
{
	poller = avahi_simple_poll_new();
	if(!poller)
	{
		slog.debug("zeroconf service failed to start");
		Thread::sync();
	}

	client = avahi_client_new(avahi_simple_poll_get(poller), 
		(AvahiClientFlags)0, 
		client_callback, NULL, &error);

	slog.debug("zeroconf service started");
	Thread::setCancel(cancelImmediate);
	avahi_simple_poll_loop(poller);
	Thread::sync();
}

} // namespace 

#endif
