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

bool Methods::scrFeature(void)
{
        BayonneSession *r = getSid(var_recall);
        Event event;
	const char *encoding = getKeyword("encoding");
	timeout_t framing = getTimeoutKeyword("framing");
	const char *opt;
	char buf[96];

	dtmf_digits[digit_count] = 0;
	opt = getValue(dtmf_digits);

	if(!strcmp(var_pid, var_recall) && time_joined)
		exitCall("release");
        if(r)
        {
                memset(&event, 0, sizeof(event));
                event.id = DROP_RECALL;
                event.peer = this;
                r->queEvent(&event);
        }
	time_joined = 0;
	strcpy(var_recall, "none");

        if(!framing || framing == TIMEOUT_INF)
                framing = 0;

	if(encoding)
		if(setReconnect(encoding, framing))
			return false;

	if(!opt || !*opt)
	{
		advance();
		return true;
	}
	snprintf(buf, sizeof(buf), "feature::%s", opt);
	if(redirect(buf))
		return false;

	error("no-feature");
	return true;
}

bool Methods::scrRelease(void)
{
	BayonneSession *r = getSid(var_recall);
	Event event;

	if(r)
	{
		memset(&event, 0, sizeof(event));
		event.id = DROP_RECALL;
		event.peer = this;
		r->queEvent(&event);
		advance();
	}
	else
		error("no-refer");
	strcpy(var_recall, "none");
	return true;
}

bool Methods::scrKeydata(void)
{
	Line *line = getLine();
	const char *ext = getSymbol("session.identity");
	const char *did = getSymbol("session.dialed");
	const char *anon = getSymbol("session.anonymous");
	char buf[80];
	Name *scr = getName();
	ScriptImage *img = getImage();
	const char *cp = NULL;

	if(anon && !*anon)
		anon = NULL;

	if(did && !*did)
		did = NULL;

	if(ext && !*ext)
		ext = NULL;

	if(isAssociated() && ext && *ext)
	{
		redirect("connect::incoming");
		return false;
	}
	
	if(isAssociated())
	{
		redirect("connect::outgoing");
		return false;
	}

	snprintf(buf, sizeof(buf), "%s.connect", scr->name);
	cp = img->getLast(buf);
	scr = NULL;
	if(cp && strchr(cp, ':'))
		scr = getScript(cp);
	else if(cp)
	{
		snprintf(buf, sizeof(buf), "connect::%s", cp);
		scr = getScript(buf);
	}
	if(scr)
		goto branch;

	if(line->next)
		goto next;

	if(did && anon)
	{
		snprintf(buf, sizeof(buf), "dialed::%s", did);
		scr = getScript(buf);
	}

	if(anon && !scr)
		scr = getScript("connect::anonymous");

	if(ext && !scr)
		scr = getScript("connect::extension");
	
    if(scr)
    {
branch:
        clearStack();
        frame[stack].script = scr;
        frame[stack].line = frame[stack].first = scr->first;
        frame[stack].mask = getMask();
        return true;
    }

	redirect("connect::common");
	return true;

next:
	advance();
	return true;
}	

bool Methods::scrRedial(void)
{
	Name *scr = NULL;
	char buf[128];
	const char *opt = getValue(NULL);
	const char *cid = getSymbol("session.caller");

	if(!opt)
		opt = dtmf_digits;

	if(cid && !stricmp(cid, opt))
		scr = getScript("dialed::_self_");

	if(!scr)
	{
		snprintf(buf, sizeof(buf), "dialed::%s", opt);
    	scr = getScript(buf);
	}

    if(scr)
    {
        clearStack();
        frame[stack].script = scr;
        frame[stack].line = frame[stack].first = scr->first;
        frame[stack].mask = getMask();
        return true;
    }

	error("no-route");
	return true;
}

bool Methods::scrRecall(void)
{
	BayonneSession *r = getSid(var_recall);
	const char *cp = getMember();
	Event event;

	if(!r)
	{
		error("no-recall");
		return true;
	}

	if(recallReconnect())
		return false;

	strcpy(var_joined, var_recall);
	strcpy(var_recall, "none");
	state.timeout = 0;
	state.join.peer = r;
	state.join.dtmf = false;
	state.join.hangup = true;
	state.join.refer = true;
	state.peering = true;

	if(cp && !stricmp(cp, "norefer"))
		state.join.refer = false;

	if(cp && !stricmp(cp, "nohangup"))
		state.join.hangup = false;

	memset(&event, 0, sizeof(event));
	event.id = JOIN_RECALL;
	event.peer = this;
	r->queEvent(&event);

	setState(STATE_JOIN);
	return false;
}	

bool Methods::scrJoin(void)
{
	Event event;
	const char *cp = getMember();

	state.timeout = 0;
	state.join.peer = getSid(var_joined);
	state.join.dtmf = false;
	state.join.hangup = false;
	state.join.refer = true;
	state.peering = true;

	if(cp && !stricmp(cp, "norefer"))
		state.join.refer = false;

	if(!state.join.peer)
	{
		error("peer-missing");
		return true;
	}
	
	memset(&event, 0, sizeof(event));
	event.id = PEER_WAITING;
	event.peer = this;
	release();
	if(!state.join.peer->postEvent(&event))
	{
		error("peer-invalid");
		return true;
	}
	peer = state.join.peer;
	setState(STATE_JOIN);
	return false;
}

bool Methods::scrStart(void)
{
	const char *dial = getValue(NULL);
	const char *name = getValue(NULL);
	const char *caller = getKeyword("caller");
	const char *display = getKeyword("display");
	Symbol *sym = getKeysymbol("session", 16);
	BayonneSession *child;
	const char *cp, *id, *value;
	char buf[65];
	bool dg = false;
	const char *encoding = getKeyword("encoding");
	timeout_t framing = getTimeoutKeyword("framing");
	unsigned pos;
	Line *line = getLine();

	if(framing == TIMEOUT_INF)
		framing = 0;

	if(encoding)
		if(setReconnect(encoding, framing))
			return false;	

	if(!name)
	{
		dg = true;
		name = dial;
	}

	
	if(!dial || !name)
	{
		if(sym)
			ScriptInterp::commit(sym, "none");
		if(!scriptEvent("start:invalid"))
			error("start-incomplete");
		return true;
	}

	if(dg)
		dial = name;

	child = startDialing(dial, name, caller, display, this);

	if(!child)
	{
		if(sym)
			ScriptInterp::commit(sym, "none");
		if(!scriptEvent("start:failed"))
			error("start-failed");
		return true;
	}

	if(sym)
		ScriptInterp::commit(sym, child->getExternal("session.id"));

	while(NULL != (cp = getOption(NULL)))
	{
		sym = mapSymbol(cp, 0);
		if(!sym)
			continue;

		id = sym->id;
		if(!strchr(id, '.'))
		{
			snprintf(buf, sizeof(buf), "parent.%s", id);
			id = buf;
		}
		child->setConst(id, sym->data);
	}

	while(line->next && !stricmp(line->next->cmd, "param"))
	{
		skip();
		line = getLine();
		pos = 0;
		while(pos < line->argc)
		{
			id = line->args[pos++];
			if(!id)
				break;

			if(*id != '=')
				continue;

			++id;
			value = getContent(line->args[pos++]);
			if(!value || !*id)
				continue;
			snprintf(buf, sizeof(buf), "param.%s", id);
			child->setConst(buf, value);
		}
	}

	child->leave();
	advance();
	return true;
}

bool Methods::scrConnect(void)
{
	Line *line = getLine();
	const char *dial = getValue(NULL);
	const char *name = getValue(NULL);
	const char *caller = getKeyword("caller");
	const char *display = getKeyword("display");
	Symbol *sym = getKeysymbol("session", 16);
	BayonneSession *child;
	const char *cp, *id, *value;
	char buf[65];
	TelTone::tonekey_t *key = NULL;
	Audio::Level level = 26000;
	bool dg = false;
	const char *encoding = getKeyword("encoding");
	timeout_t framing = getTimeoutKeyword("framing");
	unsigned pos = 0;

	if(framing == TIMEOUT_INF)
		framing = 0;

	if(encoding)
		if(setReconnect(encoding, framing))
			return false;	

	if(!name)
	{
		dg = true;
		name = dial;
	}

	cp = getMember();
	if(cp && !stricmp(cp, "norefer"))
		state.tone.refer = false;
	else
		state.tone.refer = true;

	if(!stricmp(line->cmd, "norefer"))
		state.tone.refer = false;

	if(cp && !stricmp(cp, "nohangup"))
		state.tone.hangup = false;
	else
		state.tone.hangup = true;

	state.timeout = getTimeoutKeyword("timeout");
	state.tone.duration = getTimeoutKeyword("duration");
	state.tone.dtmf = true;
	state.peering = true;

	if(state.tone.duration && state.tone.duration != TIMEOUT_INF)
		state.tone.refer = false;

	if(!caller)
		caller = getSymbol("session.caller");

	if(!display)
		display = getSymbol("session.display");

	if(audio.tone)
	{
		delete audio.tone;
		audio.tone = NULL;
	}
	cp = getKeyword("tone");
	if(!cp)
		cp = "ring";
	if(cp && !stricmp(cp, "ringback"))
		cp = "ring";
	if(cp && stricmp(cp, "none"))
		key = TelTone::find(cp, Bayonne::server->getLast("location"));
	if(key)
		audio.tone = new TelTone(key, level, getToneFraming());
	else if(stricmp(cp, "none"))
		slog.debug("%s: connect; cannot find tone %s", logname, cp);
	
	if(!dial || !name)
	{
		if(sym)
			ScriptInterp::commit(sym, "none");
		if(!scriptEvent("start:invalid"))
			error("connect-incomplete");
		return true;
	}

	if(dg)
		dial = name;

	child = startDialing(dial, name, caller, display, this);

	if(!child)
	{
		if(sym)
			ScriptInterp::commit(sym, "none");
		if(!scriptEvent("dial:failed"))
			error("dial-failed");
		return true;
	}

	if(sym)
		ScriptInterp::commit(sym, child->getExternal("session.id"));

	state.tone.hangup = true;
	strcpy(state.tone.sessionid, child->getExternal("session.id"));
	strcpy(var_joined, child->getExternal("session.id"));

	while(NULL != (cp = getOption(NULL)))
	{
		sym = mapSymbol(cp, 0);
		if(!sym)
			continue;

		id = sym->id;
		if(!strchr(id, '.'))
		{
			snprintf(buf, sizeof(buf), "parent.%s", id);
			id = buf;
		}
		child->setConst(id, sym->data);
	}

	while(line->next && !stricmp(line->next->cmd, "param"))
	{
		skip();
		line = getLine();
		pos = 0;
		while(pos < line->argc)
		{
			id = line->args[pos++];
			if(*id != '=')
				continue;
			++id;
			value = getContent(line->args[pos++]);
			if(*id && value)
			{
				snprintf(buf, sizeof(buf), "param.%s", id);
				child->setConst(buf, value);
			}
		}
	}	

	child->startConnecting();
	child->leave();
	setState(STATE_CONNECT);
	return false;
}


bool Methods::scrStop(void)
{
	Event event;
	BayonneSession *s;
	const char *cp = getValue(NULL);

	if(!cp)
	{
		error("no-timeslot");
		return true;
	}

	s = getSid(cp);
	if(!s)
	{
		error("invalid-timeslot");
		return true;
	}

	memset(&event, 0, sizeof(event));
	event.id = CANCEL_CHILD;
	s->queEvent(&event);
	advance();
	return true;
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

bool Methods::scrDial(void)
{
        unsigned len = 0;
        const char *cp;
        
        state.join.answer_timer = state.timeout = getTimeoutKeyword("timeout");
        
        while(NULL != (cp = getValue(NULL)))
        {
                setString(state.join.digits + len, sizeof(state.join.digits) - len, cp);
                len = strlen(state.join.digits);
        }

        state.join.dial = state.join.digits;
        setState(STATE_DIAL);
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
