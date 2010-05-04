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
#define strdup  ost::strdup
#else
#include <private.h>
#endif

namespace capidriver {
using namespace ost;
using namespace std;

static Keydata::Define driver[] = {
	{"stack", "0"},
	{"events", "64"},
	{"priority", "0"},
	{"buffersize", "128"},
        {NULL, NULL}};

Driver Driver::capi;

Driver::Driver() :
BayonneDriver(driver, "/bayonne/driver/capi", "capi20", false),
Thread(atoi(getLast("priority")), atoi(getLast("stack")) * 1024)
{
}

void Driver::startDriver(void)
{
	msgport = new BayonneMsgport(this);
	timeslot = ts_used;
	int err;
	struct capi_profile spanprofile;
	unsigned port;
	BayonneSpan *s;

	appl_id = 0;
	msg_id = 1;
	unsigned span = 1;

	err = capi20_isinstalled();
	if(err != CapiNoError)
	{
		slog.error("capi20: driver not installed; reason=%s",
			strerror(err));
		return;
	}

        capi20_get_profile(0, (CAPI_MESSAGE)&cprofile);
	if(!cprofile.ncontroller)
	{
		slog.error("capi20: no controllers found");
		return;
	}

	while(span <= cprofile.ncontroller)
	{
		capi20_get_profile(span, (CAPI_MESSAGE)&spanprofile);		
		slog.debug("capi20: span %d has %d ports", span, spanprofile.nbchannel);
		port = 0;
		s = new BayonneSpan(this, spanprofile.nbchannel);
		while(port < spanprofile.nbchannel)
		{
			// create session
			++port;
		}
		++span;
	}	

	count = ts_used - timeslot;
	if(count)
	{
		err = capi20_register(count, 7, atoi(getLast("buffersize")), &appl_id);
		if(err != CapiNoError)
		{
			slog.error("capi20: could not register; reason=%s", strerror(err));
			return;
		}
		msgport->start();
		Thread::start();
		BayonneDriver::startDriver();
	}
}

void Driver::stopDriver(void)
{
	if(running)
	{
		terminate();
		BayonneDriver::stopDriver();
	}
}

void Driver::run(void)
{
        unsigned err;
        _cmsg cmsg;

	setCancel(cancelImmediate);
	for(;;)
	{
	        err = capi_get_cmsg(&cmsg, appl_id);
	}
}

} // namespace
