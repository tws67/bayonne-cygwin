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
#include <cc++/process.h>

#ifdef  WIN32
#define CONFIG_FILES    "C:/Program Files/GNU Telephony/Bayonne Config"
#define strdup  ost::strdup
#else
#include <private.h>
#endif

namespace vpbdriver {
using namespace ost;
using namespace std;

static Keydata::Define driver[] = {
	{"type", "peer"},
	{"stack", "0"},
	{"events", "64"},
	{"priority", "0"},
	{"answer", "2"},
	{"noringback", "8000"},
	{"cpringback", "4000"},
	{"encoding", "mulaw"},
	{"framing", "20"},
        {NULL, NULL}};

Driver Driver::voicetronix;

Driver::Driver() :
BayonneDriver(driver, "/bayonne/driver/voicetronix", "voicetronix", false),
Thread(atoi(getLast("priority")), atoi(getLast("stack")) * 1024)
{
	const char *cp;
	cp = getLast("pickup");
	if(cp)
		pickup_timer = atol(cp);

	cp = getLast("flash");
	if(cp)
		flash_timer = atol(cp);

	cp = getLast("hangup");
	if(cp)
		hangup_timer = atol(cp);

	cp = getLast("seize");
	if(cp)
		seize_timer = atol(cp);

	cp = getLast("ring");
	if(cp)
		ring_timer = atol(cp);

	cp = getLast("answer");
	if(cp)
		answer_count = atoi(cp);

	cp = getLast("exitreorder");
	if(cp && (tolower(*cp) == 'n' || tolower(*cp) == 'f'))
		exit_reorder = false;
	else
		exit_reorder = true;

	cp = getLast("exitdialtone");
	if(cp && (tolower(*cp) == 'y' || tolower(*cp) == 't'))
		exit_dialtone = true;
	else
		exit_dialtone = false;

	gain = 20.0;
	cp = getLast("gain");
	if(cp)
		gain = atof(cp);
}

void Driver::startDriver(void)
{
	unsigned cards, ports, port;
	int h = vpb_open(1, 1);
	unsigned card = 0;
#ifndef	WIN32
	int major, minor, patch;
#endif

#ifdef	LINUX
	VPB_CARD_INFO	cardinfo;
#endif

	if(h < 0)
	{
		slog.critical("voicetronix: failed startup");
		return;
	}

	cards = vpb_get_num_cards();
#ifndef	WIN32
	vpb_get_driver_version(&major, &minor, &patch); 
	slog.info("voicetronix: detected %d card(s), driver=%d.%d.%d", 
		cards, major, minor, patch);
#endif

	timeslot = ts_used;

	while(card < cards)
	{
		ports = vpb_get_ports_per_card();
		if(ports > getAvailTimeslots())
			break;

#ifdef	LINUX
		vpb_get_card_info(card, &cardinfo);
		slog.debug("voicetronix: ports=%d, model=%s, date=%s, rev=%s, sn=%s",
			ports, cardinfo.model, cardinfo.date,
			cardinfo.rev, cardinfo.sn);
#endif

		new Session(ts_used, h);
		port = 1;
		while(port < ports)
		{
			h = vpb_open(card + 1, port + 1);
			if(h < 0)
				slog.error("voicetronix/%d: failed open");

			new Session(ts_used, h);
			++port;
		}
		if(++card < cards)
			h = vpb_open(card + 1, 1);
	}
	count = ts_used - timeslot;
	Thread::start();
	BayonneDriver::startDriver();
}

void Driver::stopDriver(void)
{
	if(running)
	{
		terminate();
		BayonneDriver::stopDriver();
	}
}

void Driver::initial(void)
{
	timeslot_t ts = getFirst(), tsc = getCount();
	BayonneSession *session;

	while(tsc--)
	{
		session = getSession(ts++);
		if(session)
			session->initialevent();
	}
}

void Driver::run(void)
{
	Event event;
	VPB_EVENT evt;
	timeslot_t ts;
	Session *session = NULL;
	char msg[VPB_MAX_STR];

#ifndef	WIN32
	if(getppid() > 1)
		logevents = &cerr;
#endif

	oink.post();
	waitLoaded();

	for(;;)
	{
		if(vpb_get_event_sync(&evt, 0) != VPB_OK)
		{
			Thread::yield();
			continue;
		}

		for(ts = timeslot; ts < count; ++ts)
		{
			session = (Session *)getSession(ts);
			if(!session)
				continue;

			if(session->getHandle() == evt.handle)
				break;
		}

		if(logevents)
		{
			vpb_translate_event(&evt, msg);
			serialize.enter();
			*logevents << "voicetronix: " << msg << flush;			
			serialize.leave();
		}

		if(!session)
			continue;

		memset(&event, 0, sizeof(event));
		event.timeslot = ts;
	
		switch(evt.type)
		{
		case VPB_RING:
			if(session->isOffhook())
			{
				event.id = STOP_DISCONNECT;
				break;
			}
			event.id = RING_ON;
			session->postEvent(&event);
			event.id = RING_OFF;
			break;
		case VPB_DIALEND:
			event.id = (event_t)evt.data;
			if(!event.id)
				event.id = DIAL_CONNECT;
			break;
		case VPB_DTMF:
			event.id = DTMF_KEYUP;
			event.dtmf.digit = getDigit(evt.data);
			event.dtmf.duration = 40;
			break;
		case VPB_DROP:
			event.id = LINE_DISCONNECT;
			break;
		case VPB_TIMEREXP:
			event.id = TIMER_EXPIRED;
			break;
		case VPB_VOXON:
			event.id = AUDIO_START;
			break;
		case VPB_VOXOFF:
			event.id = AUDIO_STOP;
			break;
		case VPB_PLAYEND:
		case VPB_RECORDEND:
			event.id = AUDIO_IDLE;
			break;
		case VPB_STATION_OFFHOOK:
			event.id = LINE_PICKUP;
			break;
		case VPB_STATION_ONHOOK:
			event.id = LINE_HANGUP;
			break;
		case VPB_STATION_FLASH:
			event.id = LINE_WINK;
			break;
		case VPB_CALLEND:
			switch(evt.data)
			{
			default:
				event.id = DIAL_CONNECT;
				break;
			case VPB_CALL_NO_DIAL_TONE:
				event.id = DIAL_FAILED;
				break;
			case VPB_CALL_NO_RING_BACK:
				event.id = DIAL_FAILED;
				break;
			case VPB_CALL_BUSY:
				event.id = DIAL_BUSY;
				break;
			case VPB_CALL_NO_ANSWER:
				event.id = DIAL_TIMEOUT;
				break;
			case VPB_CALL_DISCONNECTED:
				event.id = LINE_DISCONNECT;
				break;
			}
			break;
		case VPB_TONEDETECT:
			event.id = TONE_START;
			event.tone.exit = false;
			switch(evt.data)
			{
			case VPB_GRUNT + 1:
				event.tone.exit = exit_reorder;
				event.tone.name = "reorder";
				break;
			case VPB_GRUNT + 2:
				event.tone.name = "user1";
				break;
			case VPB_GRUNT + 3:
				event.tone.name = "user2";
				break;
			case VPB_GRUNT + 4:
				event.tone.name = "user3";
				break;
			case VPB_GRUNT + 5:
				event.tone.name = "user4";
				break;
			case VPB_GRUNT:
				event.tone.name = "grunt";
				break;
			case VPB_DIAL:
				event.tone.exit = exit_dialtone;
				event.tone.name = "dialtone";
				break;
			case VPB_RINGBACK:
				event.tone.name = "ringback";
				break;
			case VPB_BUSY:
				event.tone.name = "busytone";
				break;
			}
			break;
		}
		if(event.id)
			session->postEvent(&event);
	}
}

} // end namespace
