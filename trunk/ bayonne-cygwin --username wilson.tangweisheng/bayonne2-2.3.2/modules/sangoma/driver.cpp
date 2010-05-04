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

namespace sangomadriver {
using namespace ost;
using namespace std;

static Keydata::Define driver[] = {
	{"type", "peer"},
	{"stack", "0"},
	{"events", "64"},
	{"priority", "0"},
	{"spans", "1"},
	{"channels", "24"},
	{"switch", "dms100"},
	{"mode", "cpe"},
	{NULL, NULL}};

Driver Driver::sangoma;

Driver::Driver() :
BayonneDriver(driver, "/bayonne/driver/sangoma", "sangoma", false)
{
        const char *cp;
	spancount = 0;

	pri_switch = SANGOMA_PRI_SWITCH_UNKNOWN;
	cp = getLast("switch");
	if(cp && !stricmp(cp, "NI2"))
		pri_switch = SANGOMA_PRI_SWITCH_NI2;
	else if(cp && !stricmp(cp, "DMS100"))
		pri_switch = SANGOMA_PRI_SWITCH_DMS100;
	else if(cp && !stricmp(cp, "LUCENT"))
		pri_switch = SANGOMA_PRI_SWITCH_LUCENT5E;
	else if(cp && !stricmp(cp, "4ESS"))
		pri_switch = SANGOMA_PRI_SWITCH_ATT4ESS;
	else if(cp && !stricmp(cp, "EUROISDN") && atoi(getLast("channels")) > 25)
		pri_switch = SANGOMA_PRI_SWITCH_EUROISDN_E1;
	else if(cp && !stricmp(cp, "EUROISDN"))
		pri_switch = SANGOMA_PRI_SWITCH_EUROISDN_T1;
	else if(cp && !stricmp(cp, "NI1"))
		pri_switch = SANGOMA_PRI_SWITCH_NI1;
	else if(cp && !stricmp(cp, "QSIG"))
		pri_switch = SANGOMA_PRI_SWITCH_QSIG;

	pri_node = SANGOMA_PRI_CPE;
	cp = getLast("node");
	if(cp && !stricmp(cp, "network"))
		pri_node = SANGOMA_PRI_NETWORK;

	info.framing = 20;
	info.encoding = Audio::pcm16Mono;
/*	cp = getLast("encoding");
	if(cp && !stricmp(cp, "alaw"))
		info.encoding = Audio::alawAudio;
	else if(cp && !stricmp(cp, "ulaw"))
		info.encoding = Audio::mulawAudio;
	else if(cp && !stricmp(cp, "mulaw"))
		info.encoding = Audio::mulawAudio;
*/
	info.set();
}

Span *Driver::find(sangoma_pri *spri, const char *id)
{
	unsigned span;

	for(span = 0; span < spancount; ++span)
	{
		if(&spans[span]->spri == spri)
			return spans[span];
	}

	slog.error("sangoma: unable to match pri event %s with span", id);
	return NULL;
}

void Driver::startDriver(void)
{
	unsigned scount = atoi(getLast("spans"));
	unsigned pcount = atoi(getLast("channels"));
	unsigned used;
	unsigned span;

	timeslot = ts_used;
	msgport = new BayonneMsgport(this);

	while(scount--)
	{
		spans[spancount] = new Span(this, pcount);
		used = pcount;
		while(used--)
			new Session(spans[spancount], ts_used);
	}

	count = ts_used - timeslot;

	msgport->start();

	for(span = 0; span < spancount; ++span)
		spans[span]->start();

	BayonneDriver::startDriver();
}

void Driver::stopDriver(void)
{
	unsigned span;

	if(spancount)
		for(span = 0; span < spancount; ++span)
			delete spans[span];

	if(msgport)
	{
		delete msgport;
		msgport = NULL;
	}

	BayonneDriver::stopDriver();
}

extern "C" {

int pe_hangup(struct sangoma_pri *spri, sangoma_pri_event_t et, pri_event *pevent)
{
	sangomadriver::Span *sp;
	
	sp = sangomadriver::Driver::sangoma.find(spri, "hangup");
	if(!sp)
		return 0;

	sp->onHangup(pevent);
	return 0;
}

int pe_ring(struct sangoma_pri *spri, sangoma_pri_event_t et, pri_event *pevent)
{
	sangomadriver::Span *sp;

        sp = sangomadriver::Driver::sangoma.find(spri, "hangup");
        if(!sp)
                return -1;

	sp->onRing(pevent);
}

int pe_restart(struct sangoma_pri *spri, sangoma_pri_event_t et, pri_event *pevent)
{
        sangomadriver::Span *sp;

        sp = sangomadriver::Driver::sangoma.find(spri, "hangup");
        if(!sp)
                return -1;

        sp->onRestart(pevent);
}
	
	
}} // namespace
