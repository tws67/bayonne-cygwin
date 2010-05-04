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

namespace scdriver {
using namespace ost;
using namespace std;

static Keydata::Define driver[] = {
	{"type", "peer"},
        {"device", "0"},
	{"stack", "0"},
	{"events", "8"},
	{"priority", "0"},
	{"level", "200"},
        {NULL, NULL}};

Driver Driver::soundcard;

Driver::Driver() :
BayonneDriver(driver, "/bayonne/driver/soundcard", "soundcard", false)
{
	const char *cp;

	device = atoi(getLast("device"));
	active = Audio::hasDevice(device);
	keyboard = NULL;

#ifdef	HAVE_FOX
	desktop = NULL;
#endif

	cp = getLast("level");
	if(cp)
		audio_level = atoi(cp);

	if(!active)
		slog.warn("soundcard/*: device %d unavailable", device);

	seize_timer = 0;	// no seizure
}

void Driver::startDriver(void)
{
	bool kb = true;
	msgport = new BayonneMsgport(this);

#ifdef	HAVE_FOX
	const char *d = Process::getEnv("DISPLAY");
	if(d && *d)
	{
		desktop = new Desktop(getFirst());
//		kb = false;
	}
#endif

#ifdef  WIN32
	if(kb)
	        keyboard = new Keyboard(ts_used);
#else
        if(getppid() > 1 && kb)
                keyboard = new Keyboard(ts_used);
#endif   

	timeslot = ts_used;
	new Session(ts_used, device);
	count = ts_used - timeslot;

	msgport->start();
	if(keyboard)
		keyboard->start();
#ifdef	HAVE_FOX
	if(desktop)
		desktop->start();
#endif
	BayonneDriver::startDriver();
}

void Driver::stopDriver(void)
{
	if(keyboard)
	{
		delete keyboard;
		keyboard = NULL;
	}
#ifdef	HAVE_FOX
	if(desktop)
	{
		delete desktop;
		desktop = NULL;
	}
#endif

	BayonneDriver::stopDriver();
}

}
