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

#include "module.h"

namespace binder {
using namespace ost;
using namespace std;


bool Methods::scrKeydata(void)
{
	Line *line = getLine();
	char buf[80];
	Name *scr = getName();
	ScriptImage *img = getImage();
	const char *cp = NULL;

	if(isAssociated())
	{
		redirect("connect::outgoing");
		return false;
	}

	snprintf(buf, sizeof(buf), "%s.connect", scr->name);
	cp = img->getLast(buf);
	if(cp)
	{
		redirect(cp);
		return false;
	}


	if(line->next)
	{
		advance();
		return true;
	}

	redirect("connect::incoming");
	return false;
}	

bool Methods::scrStart(void)
{
	ScriptImage **ip = getLocalImage(timeslot);
	Name *dest;
	const char *id = getValue("_main_");

	if(!stricmp(id, "_main_"))
		id = "1";
	
	if(!ip || !*ip)
	{
		error("xml-unallocated");
		return true;
	}
	dest = (*ip)->getScript(id);
	if(!dest)
	{
		error("xml-unstartable");
		return true;
	}

	while(frame[stack].base)
		pull();

	push();
	frame[stack].base = stack;
	frame[stack].caseflag = false;
	frame[stack].script = dest;
	frame[stack].line = frame[stack].first = dest->first;
	frame[stack].index = 0;
	frame[stack].mask = dest->mask;
	updated = true;
	return true;
}

bool Methods::scrParse(void)
{
	const char *cp = getValue(NULL);
	ScriptImage **ip = getLocalImage(timeslot);

	if(!cp)
	{
		error("no-path");
		return true;
	}

	if(!ip)
	{
		error("no-xml");
		return true;
	}

	new ParseThread(dynamic_cast<ScriptInterp*>(this), cp, ip);
	return false;
}
	
bool Methods::scrEndinput(void)
{
	frame[stack].mask &= ~0x08;
	state.menu = NULL;
	advance();
	return false;
}

bool Methods::scrEndform(void)
{
	if(stack < 1 || stack == frame[stack].base)
	{
		error("stack-underflow");
		return true;
	}

	if(!frame[stack].line->argc)
		goto exit;

	if(conditional())
		goto exit;

	frame[stack].line = frame[stack - 1].line;
	frame[stack].tranflag = true;

	if(!state.menu)
		state.menu = getName();

	goto clear;
	
exit:

	frame[stack - 1] = frame[stack];
	--stack;

	if(stack < state.stack)
		state.menu = NULL;
		
	frame[stack].tranflag = false;

clear:
	*dtmf_digits = 0;
	advance();
	return false;
}

bool Methods::scrForm(void)
{
	const char *cp, *value;
	Line *line = getLine();
	unsigned idx = 0;

	if(!push())
	{
		error("stack-overflow");
		return true;
	}

	if(!state.menu)
	{
		state.menu = getName();
		state.stack = stack;
	}

	while(idx < line->argc)
	{
		cp = line->args[idx++];
		if(*cp != '=')
			continue;

                value = getContent(line->args[idx++]);   
		if(!value)
			continue;

		setSymbol(++cp, value);
	}

	frame[stack].tranflag = true;

	*dtmf_digits = 0;

	if(!requiresDTMF())
		return true;

	advance();
	return false;
}

bool Methods::scrKey(void)
{
        const char *cp;
        const char *val = getKeyword("value");
        const char *ind = getKeyword("index");
        Symbol *sym;

        while(NULL != (cp = getOption(NULL)))
        {
                sym = mapSymbol(cp, PersistProperty::getSize());
                if(!sym)
                        continue;
                PersistProperty::refresh(sym, ind, val);
        }
        advance();       
        return true;  
} 

} // end namespace
