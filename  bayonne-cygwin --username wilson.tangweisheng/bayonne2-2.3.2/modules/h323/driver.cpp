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
//
// As a special exception to the GNU General Public License, permission is
// granted for additional uses of the text contained in its release
// of Bayonne as noted here.
//
// This exception is that permission is hereby granted to link Bayonne 2
// with the OpenH323 and Pwlib libraries, and distribute the combination, 
// without applying the requirements of the GNU GPL to the OpenH323
// and pwd libraries as long as you do follow the requirements of the 
// GNU GPL for all the rest of the software thus combined.
//
// This exception does not however invalidate any other reasons why
// the resulting executable file might be covered by the GNU General
// public license or invalidate the licensing requirements of any
// other component or library.

#include "driver.h"

#ifdef  WIN32
#define CONFIG_FILES    "C:/Program Files/GNU Telephony/Bayonne Config"
#else
#include <private.h>
#endif 

namespace h323driver {
using namespace ost;
using namespace std;

static Keydata::Define driver[] = {
	{"type", "proto"},
	{"proto", "h323"},
	{"stack", "0"},
	{"events", "128"},
	{"priority", "0"},
        {"first", "3128"},
        {"count", "32"},
        {"inc", "4"},
        {"interface", "*"},
        {"port","1720"},
        {"usegk","0"},
        {"gatewayprefixes",""},
        {"username","Bayonne"},
        {"uimode","rfc2833"},
        {"faststart","1"},
        {"h245tunneling","1"},
        {"h245insetup","1"},
        {"forwarding","1"},
        {"inbanddtmf","1"},
        {"rtpmin","0"},
        {"rtpmax","0"},
        {"tcpmin","0"},
        {"tcpmax","0"},
        {"udpmin","0"},
        {"udpmax","0"},
	{"hangup", "120"},	// call release timer
	{"pickup", "4000"},	// call pickup, wait for start...
	{"offer", "4000"},	// call offer timer
        {NULL, NULL}};

Driver Driver::h323;
Endpoint Driver::endpoint;

Driver::Driver() :
BayonneDriver(driver, "/bayonne/driver/h323", "h323", false)
{
	const char *cp;
	cp = getLast("hangup");
	if(cp)
		hangup_timer = atoi(cp);

	cp = getLast("pickup");
	if(cp)
		pickup_timer = atoi(cp);

	cp = getLast("offer");
	if(cp)
		ring_timer = atoi(cp);

	addConfig("h323.conf");	
}

void Driver::startDriver(void)
{
	timeslot = ts_used;
	msgport = new BayonneMsgport(this);
	unsigned max_count = ts_limit;
        PIPSocket::Address interfaceAddress(getLast("interface"));
        WORD port = atoi(getLast("port"));
	unsigned retries = 0;
	const char *cp = getLast("timeslots");

	if(cp)
		max_count = atoi(cp);

        if (!endpoint.Initialise())
        {
		slog.error() << "h323: the endpoint failed to initialise" << endl;
                return;
        }

	listener = NULL;
	while(retries < 3 && !listener)
	{
        	listener = new H323ListenerTCP(endpoint, interfaceAddress, port);
        	if(!endpoint.StartListener(listener))
        	{
                	delete listener;
                	listener = NULL;
			port += 10;
			++retries;
        	}
	}

	if(retries >= 3)
	{
		slog.error() << "h323 cannot bind listener" << endl;
		return;
	}

	slog.info("h323 listening on port %d", port);

	while(ts_used < ts_limit && max_count--)
		new Session(ts_used);

	count = ts_used - timeslot;
	if(count)
	{
		msgport->start();
		BayonneDriver::startDriver();
	}	
}

void Driver::stopDriver(void)
{
	if(!running)
		return;

        if(listener)
        {
                endpoint.ClearAllCalls(H323Connection::EndedByLocalUser, true);
//              endpoint.RemoveListener(listener);
//              delete listener;
                listener = NULL;
        }


}

bool Driver::getBool(const char *name)
{
        const char *cp = getLast(name);
        if(atoi(cp) == 1 || !stricmp(cp, "on"))
                return true;
	if(!stricmp(cp, "yes") || !stricmp(cp, "true"))
		return true;
        return false;
}

} // namespace
