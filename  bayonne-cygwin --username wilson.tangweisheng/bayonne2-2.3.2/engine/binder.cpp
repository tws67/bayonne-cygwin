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

BayonneBinder *BayonneBinder::binder = NULL;

unsigned BayonneBinder::Image::gatherPrefix(const char *prefix, const char **list, unsigned max)
{
	unsigned count = 0;
	unsigned key = 0;
	Name *scr;
	unsigned len = strlen(prefix);

	while(count < max && key < SCRIPT_INDEX_SIZE)
	{
		scr = index[key];
		while(scr && count < max)
		{
			if(!strnicmp(scr->name, prefix, len))
				list[count++] = scr->name + len;
			scr = scr->next;
		}
		++key;
	}
	return count;
}

BayonneBinder::BayonneBinder(const char *id) :
ScriptBinder(id)
{
	if(!binder)
		binder = this;
}

bool BayonneBinder::isDestination(const char *dest)
{
	bool rtn = false;
	Image *img = (Image *)useImage();
	if(binder && img)
		rtn = binder->isDestination(img, dest);
	if(img)
		endImage(img);
	return rtn;
}

unsigned BayonneBinder::gatherDestinations(ScriptImage *img, const char **list, unsigned max)
{
	if(binder)
		return binder->destinations((Image *)img, list, max);
	return 0;
}

Script::Name *BayonneBinder::getIncoming(ScriptImage *img, BayonneSession *s, Event *event)
{
	BayonneSpan *span = s->getSpan();
	Name *scr;
	char buf[65];

	if(sla[0])
	{
		scr = img->getScript(sla);
		if(scr)
			return scr;
	}

	if(s->getInterface() == IF_INET)	
		return NULL;

	if(span)
	{
		snprintf(buf, sizeof(buf), "timeslot::span%d", span->getId());
		scr = img->getScript(buf);
		if(scr)
			return scr;
	}

	snprintf(buf, sizeof(buf), "timeslot::%d", s->getTimeslot());
	return img->getScript(buf);
}

BayonneSession *BayonneBinder::session(ScriptInterp *s)
{
	return (BayonneSession *)(s);
}

const char *BayonneBinder::submit(const char **args)
{
	return NULL;
}

ScriptCompiler *BayonneBinder::compiler(void)
{
	ScriptCompiler *img = new ScriptCompiler(server, "/bayonne/server/config");
	if(getUserdata())
		img->loadPrefix("config", "~bayonne/config");

	return img;
}

bool BayonneBinder::scriptEvent(ScriptInterp *interp, const char *evt)
{
	return (session(interp))->stringEvent(evt);
}

bool BayonneBinder::digitEvent(ScriptInterp *interp, const char *evt)
{
	return (session(interp))->digitEvent(evt);
}

ScriptCompiler *BayonneBinder::getCompiler(void)
{
	return binder->compiler();
}

const char *BayonneBinder::submitRequest(const char **args)
{
	return binder->submit(args);
}

void BayonneBinder::makeCall(BayonneSession *s)
{
}

void BayonneBinder::dropCall(BayonneSession *s)
{
}

unsigned BayonneBinder::destinations(Image *img, const char **list, unsigned max)
{
	return img->gatherPrefix("dialed::", list, max);
}

bool BayonneBinder::isDestination(Image *img, const char *check)
{
	char buf[80];
	snprintf(buf, sizeof(buf), "dialed::%s", check);
	if(img->getScript(buf))
		return true;

	return false;
}

