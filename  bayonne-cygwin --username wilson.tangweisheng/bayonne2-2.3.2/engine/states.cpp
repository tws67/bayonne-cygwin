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
#ifdef	HAVE_LIBEXEC
#include "libexec.h"
#endif

using namespace ost;
using namespace std;

bool BayonneSession::stateInitial(Event *event)
{
	if(enterInitial(event))
		return true;

	switch(event->id)
	{
	case MAKE_IDLE:
	case ENTER_STATE:
		setState(STATE_IDLE);
		return true;
	default:
		return false;
	}
}

bool BayonneSession::stateLibexec(Event *event)
{
	char buf[256];
	const char *cp;
	char evtmsg[65];

	switch(event->id)
	{
	case PROMPT_LIBEXEC:
	case REPLAY_LIBEXEC:
		setState(STATE_PLAY);
		return true;				
	case RECORD_LIBEXEC:
		setState(STATE_RECORD);
		return true;				
	case TONE_LIBEXEC:
		setState(STATE_TONE);
	case WAIT_LIBEXEC:
		setState(STATE_WAITKEY);
		return true;
	case READ_LIBEXEC:
		setState(STATE_INPUT);
		return true;
	case ENTER_STATE:
		if(!state.libaudio)
		{
			state.libaudio = new libaudio_t;
			state.libaudio->line.args = state.libaudio->list; 
		}
	case RESTART_LIBEXEC:
		if(state.lib && getLine() != state.lib)
		{
			newTid();
			setRunning();
			return true;
		}
		if(!state.lib)
		{
			setRunning();
			return true;
		}
		clrAudio();
		if(state.pid && state.pfd != PFD_INVALID)
		{
			libWrite("100 TRANSACTION\n");

			cp = audio.var_position;
			while(*cp && (ispunct(*cp) || *cp == '0'))
				++cp;

			if(*cp)
			{
				snprintf(buf, sizeof(buf), "POSITION: %s\n", audio.var_position);
				libWrite(buf);
			}
	
			if(state.result == RESULT_SUCCESS && *dtmf_digits)
			{
				snprintf(buf, sizeof(buf), "DIGITS: %s\n", dtmf_digits);
				libWrite(buf);
				*dtmf_digits = 0;
				digit_count = 0;
			}

			snprintf(buf, sizeof(buf), "RESULT: %d\n\n", state.result);
			libWrite(buf);
			state.result = RESULT_SUCCESS;
		}
		strcpy(audio.var_position, "00:00:00.000");
timer:
		state.timeout = getLibexecTimeout();
		if(state.timeout != TIMEOUT_INF)
			startTimer(state.timeout);
		return true;
	case ENTER_LIBEXEC:
		if(stricmp(var_tid, event->libexec.tid))
			return false;
		state.result = RESULT_SUCCESS;
		state.pid = event->libexec.pid;
#ifdef	WIN32
		state.pfd = event->libexec.pfd;
#else
		state.pfd = ::open(event->libexec.fname, O_RDWR);
		remove(event->libexec.fname);
#endif
		goto timer;
	case TIMER_EXPIRED:
		libClose("902 TIMEOUT\n\n");
		state.pid = 0;
		newTid();
	default:
		return enterCommon(event);
        case DROP_LIBEXEC:
		if(!state.pid)
			return false;
		if(stricmp(var_tid, event->libexec.tid))
			return false;
		newTid();
		if(!signalScript(SIGNAL_EXIT))
			error("libexec-hangup");
		return true;	
	case ERROR_LIBEXEC:
		if(!state.pid)
			return false;

		if(stricmp(var_tid, event->liberror.tid))
			return false;

		error(event->liberror.errmsg);
		setRunning();
		return true;
	
	case EXIT_LIBEXEC:
		if(!state.pid)
			return false;
		if(stricmp(var_tid, event->libexec.tid))
			return false;
		if(event->libexec.result)
		{
			snprintf(evtmsg, sizeof(evtmsg), "exit:%d", 
				event->libexec.result);
			if(!scriptEvent(evtmsg))
			{
				evtmsg[4] = '-';
				error(evtmsg);
			}
		}
		else
			advance();
		setRunning();
		return true;		
	}
}
		
bool BayonneSession::stateLibwait(Event *event)
{
	switch(event->id)
	{
	case TIMER_EXPIRED:
	case ENTER_STATE:
#ifdef	HAVE_LIBEXEC
		if((unsigned)(++libexec_count) <= state.wait.count || !state.wait.count)
		{
			if(!BayonneSysexec::create(this))
			{
				--libexec_count;
				error("libexec-failed");
				setRunning();
				return true;
			}
			state.lib = getLine();
			setState(STATE_LIBEXEC);
			return true;
		}
		--libexec_count;
		startTimer(state.wait.interval);
		return true;
#else
		error("libexec-unsupported");
		setRunning();
		return true;
#endif
	default:
		return enterCommon(event);
	}
}

bool BayonneSession::stateLibreset(Event *event)
{
	switch(event->id)
	{
	case ENTER_STATE:
		if(thread)
		{
			delete thread;
			thread = NULL;
		}
		startTimer(reset_timer);
		return true;
	case TIMER_EXPIRED:
		clrAudio();
		if(setLibexec(state.result))
			return true;
		setRunning();
		return true;
	default:
		return enterCommon(event);
	}
}

bool BayonneSession::stateIdleReset(Event *event)
{
	switch(event->id)
	{
	case ENTER_STATE:
		if(thread)
		{
			delete thread;
			thread = NULL;
		}
		startTimer(reset_timer);
		return true;
	case TIMER_EXPIRED:
		if(image)
			detach();
		setState(STATE_IDLE);
		return true;
	case MAKE_IDLE:
		return true;
	default:
		return false;
	}
}

bool BayonneSession::stateReset(Event *event)
{
	timeout_t timer;

	if(enterReset(event))
		return true;

	switch(event->id)
	{
	case ENTER_STATE:
		timer = driver->getResetTimer();
		if(thread)
		{
			delete thread;
			thread = NULL;
			if(timer < reset_timer)
				timer = reset_timer;
		}
		startTimer(timer);
		return true;
	case TIMER_EXPIRED:
	case CALL_RESTART:
restart:
		stopTimer();
		setState(STATE_IDLE);
		return true;
	case RESTART_FAILED:
		slog.error("%s: reset failed", logname);
		goto restart;
	case MAKE_IDLE:
		return true;
	default:
		return false;
	}
}

bool BayonneSession::stateRelease(Event *event)
{
	if(enterRelease(event))
		return true;

	switch(event->id)
	{
	case ENTER_STATE:
		startTimer(driver->getReleaseTimer());
		return true;
	case TIMER_EXPIRED:
		slog.error("%s: release timeout failure");
	case CALL_RELEASED:
release:
		stopTimer();
		setState(STATE_IDLE);
		return true;
	case RELEASE_FAILED:
		slog.error("%s: release failed");
		goto release;
	case MAKE_IDLE:
		return true;
	default:
		return false;
	}
}

bool BayonneSession::stateStart(Event *event)
{
	Event cancel;
	BayonneSession *s;

	if(enterTone(event))
		return true;

	switch(event->id)
	{
	case ENTER_STATE:
		startTimer(state.timeout);
		return true;
	case CHILD_INVALID:
		if(!scriptEvent("start:invalid"))
			error("start-invalid");
		setRunning();
		return true;
	case STOP_PARENT:
	case CHILD_FAILED:
		if(!scriptEvent("start:failed"))
			error("start-failed");
		setRunning();
		return true;
	case TIMER_EXPIRED:
		memset(&event, 0, sizeof(event));
		cancel.id = CANCEL_CHILD;
		cancel.pid = this;
		s = getSid(state.tone.sessionid);
		if(s)
			s->queEvent(&cancel);		
	case CHILD_EXPIRED:
		if(!scriptEvent("start:expired"))
			error("start-expired");
		setRunning();
		return true;
	case CHILD_BUSY:
		if(!scriptEvent("start:busy"))
			error("start-busy");
		return true;
	case CHILD_FAX:
		if(!scriptEvent("start:fax"))
			error("start-fax");
		setRunning();
		return true;
	case CHILD_RUNNING:
		advance();
		return true;
	default:
		return enterCommon(event);
	}
}

bool BayonneSession::stateWaitkey(Event *event)
{
	switch(event->id)
	{
	case DTMF_KEYUP:
		digit_count = 1;
		dtmf_digits[0] = getChar(event->dtmf.digit);
		dtmf_digits[1] = 0;
gotkey:
		if(setLibexec(RESULT_PENDING))
			return true;
		advance();
		setRunning();
		return true;
	case ENTER_STATE:
		if(*dtmf_digits)
			goto gotkey;
		if(state.timeout)
		{
			startTimer(state.timeout);
			return true;
		}
	case TIMER_EXPIRED:
		if(setLibexec(RESULT_TIMEOUT))
			return true;
		advance();
		setRunning();
		return true;
	default:
		return enterCommon(event);
	}
}

bool BayonneSession::stateSleep(Event *event)
{
	switch(event->id)
	{
	case ENTER_STATE:
		if(state.timeout)
		{
			startTimer(state.timeout);
			return true;
		}
	case TIMER_EXPIRED:
		advance();
		setRunning();
		return true;
	default:
		return enterCommon(event);
	}
}

bool BayonneSession::stateClear(Event *event)
{
	switch(event->id)
	{
	case ENTER_STATE:
	case DTMF_KEYUP:
		digit_count = 0;
		dtmf_digits[0] = 0;
		if(state.timeout)
		{
			startTimer(state.timeout);
			return true;
		}
		advance();
		setRunning();
		return true;
	case TIMER_EXPIRED:
		advance();
		setRunning();
		return true;
	default:
		return enterCommon(event);
	}
}	

bool BayonneSession::stateCollect(Event *event)
{
        unsigned count, copy, pos, size;
        char data[MAX_DTMF + 1];
        const char *cp;

	char evtmsg[MAX_DTMF + 32];
	char ch;

	switch(event->id)
	{
	case TIMER_EXPIRED:
		snprintf(evtmsg, sizeof(evtmsg), "%s:expired", state.input.route);
		if(!scriptEvent(evtmsg))
			if(!signalScript(SIGNAL_TIMEOUT))
				error("collect-expired");
		setRunning();
		return true;
	case DTMF_KEYUP:
		ch = getChar(event->dtmf.digit);
		if(state.input.ignore && strchr(state.input.ignore, ch))
			return true;
		if(digit_count < MAX_DTMF)
		{
			dtmf_digits[digit_count++] = ch;
			dtmf_digits[digit_count] = 0;
		}
	case ENTER_STATE:
		stopTimer();
		count = digit_count;
		if(digit_count)
		{
			snprintf(evtmsg, sizeof(evtmsg), "%s:%s",
				state.input.route, dtmf_digits);
			if(digitEvent(evtmsg))
				goto input;
		}			
		count = getInputCount(state.input.exit, state.input.count);
		if(!count)
		{
			if(digit_count)
				state.timeout = state.input.interdigit;

			startTimer(state.timeout);
			return true;
		}
		snprintf(evtmsg, sizeof(evtmsg), "%s:complete",
			state.input.route);
		if(!scriptEvent(evtmsg))
			advance();
input:
		size = count;	
                if(state.input.format)
                {
                        size = copy = 0;
                        cp = state.input.format;
                        while(size < MAX_DTMF && copy < count && cp[size])
                        {
                                if(cp[size] == '?')
                                        data[size] = dtmf_digits[copy++];
                                else
                                        data[size] = cp[size];
                                ++size;
                        }
                        while(size < MAX_DTMF && copy < count)
                                data[size++] = dtmf_digits[copy++];

			data[size] = 0;
                }
                else
		{
                        memcpy(data, dtmf_digits, count);
			data[count] = 0;
		}

                pos = 0;
                while(count + pos <= digit_count)
                {
                        dtmf_digits[pos] = dtmf_digits[pos + count];
                        ++pos;
                }
                digit_count = strlen(dtmf_digits); 
		if(state.input.var)
			setSymbol(state.input.var, data, state.input.size);
		setRunning();
		return true;
	default:
		return enterCommon(event);
	}
}

bool BayonneSession::stateInput(Event *event)
{
	unsigned count, copy, size, last;
	char data[MAX_DTMF + 1];
	const char *cp;
	signal_t trapkey;

	switch(event->id)
	{
	case TIMER_EXPIRED:
		count = getInputCount(state.input.exit, state.input.count);
		if(count >= state.input.required)
			goto input;
		dtmf_digits[0] = 0;
		digit_count = 0;
		if(setLibexec(RESULT_TIMEOUT))
			return true;
		setSymbol(state.input.var, "", state.input.size);
		if(scriptEvent("input:timeout"))
		{
			setRunning();
			return true;
		}
		break;
	case DTMF_KEYUP:
		if(digit_count < MAX_DTMF)
		{
			dtmf_digits[digit_count++] = getChar(event->dtmf.digit);
			dtmf_digits[digit_count] = 0;
		}
	case ENTER_STATE:
		stopTimer();
		if(state.input.exit)
		{
			last = state.input.count;
			count = getInputCount(state.input.exit, state.input.count + 1);
		}
		else
		{
			last = state.input.count - 1;
			count = getInputCount(state.input.exit, state.input.count);
		}

		if(!count)
		{
                        if(digit_count && digit_count == last)
                                state.timeout = state.input.lastdigit; 
			else if(digit_count)
				state.timeout = state.input.interdigit;

			startTimer(state.timeout);
			return true;
		}

		if(count > state.input.count && !strchr(state.input.exit, dtmf_digits[count - 1]))
		{
			dtmf_digits[0] = 0;
			digit_count = 0;

			if(setLibexec(RESULT_INVALID))
				return true;

			if(!scriptEvent("input:invalid"))
				if(!signalScript(SIGNAL_INVALID))
					error("invalid-input");
			setSymbol(state.input.var, "", state.input.size);
			setRunning();
			return true;
		}
input:
		if(setLibexec(RESULT_SUCCESS))
		{
			if(state.input.exit && strchr(state.input.exit, dtmf_digits[digit_count - 1]))
				dtmf_digits[--digit_count] = 0;
			return true;
		}

		size = count;
		if(state.input.format)
		{
			size = copy = 0;
			cp = state.input.format;
			while(size < MAX_DTMF && copy < count && cp[size])
			{
				if(cp[size] == '?')
					data[size] = dtmf_digits[copy++];
				else
					data[size] = cp[size];
				++size;
			}
			while(size < MAX_DTMF && copy < count)
				data[size++] = dtmf_digits[copy++];
			data[size] = 0;
		}
		else
		{
			memcpy(data, dtmf_digits, count);
			data[count] = 0;
		}


		dtmf_digits[0] = 0;
		digit_count = 0;
		data[size--] = 0;
		trapkey = (signal_t)0;
		if(state.input.exit && strchr(state.input.exit, data[size]))
		{
			trapkey = (signal_t)(Bayonne::getDigit(data[size]) + SIGNAL_0);
			data[size] = 0;
		}
		setSymbol(state.input.var, data, state.input.size);
		if(trapkey)
		{
			snprintf(data, sizeof(data), "exitkey:%s", server->getTrapName(trapkey));
			if(scriptEvent(data))
				goto finish;
			if(signalScript(trapkey))
				goto finish;
		}
		advance();
		goto finish;
	default:
		break;
	}

	return enterCommon(event);

finish:
	setRunning();
	return true;
}

bool BayonneSession::stateRead(Event *event)
{
	unsigned count, copy, pos, size;
	char data[MAX_DTMF + 1];
	const char *cp;
	signal_t trapkey;

	switch(event->id)
	{
	case TIMER_EXPIRED:
		if(scriptEvent("read:timeout"))
		{
			setRunning();
			return true;
		}
		break;
	case DTMF_KEYUP:
		if(digit_count < MAX_DTMF)
		{
			dtmf_digits[digit_count++] = getChar(event->dtmf.digit);
			dtmf_digits[digit_count] = 0;
		}
	case ENTER_STATE:
		stopTimer();
		count = getInputCount(state.input.exit, state.input.count);

		if(!count)
		{
                        if(digit_count && digit_count == state.input.count - 1)
                                state.timeout = state.input.lastdigit;   
			else if(digit_count)
				state.timeout = state.input.interdigit;

			startTimer(state.timeout);
			return true;
		}

		size = count;
		if(state.input.format)
		{
			size = copy = 0;
			cp = state.input.format;
			while(size < MAX_DTMF && copy < count && cp[size])
			{
				if(cp[size] == '?')
					data[size] = dtmf_digits[copy++];
				else
					data[size] = cp[size];
				++size;
			}
			while(size < MAX_DTMF && copy < count)
				data[size++] = dtmf_digits[copy++];
		}
		else
			memcpy(data, dtmf_digits, count);

		pos = 0;
		while(count + pos <= digit_count)
		{
			dtmf_digits[pos] = dtmf_digits[pos + count];
			++pos;
		}
		digit_count = strlen(dtmf_digits);

		data[size--] = 0;
		trapkey = (signal_t)0;
		if(state.input.exit && strchr(state.input.exit, data[size]))
		{
			trapkey = (signal_t)(Bayonne::getDigit(data[size]) + SIGNAL_0);
			data[size] = 0;
		}
		setSymbol(state.input.var, data, state.input.size);
		if(trapkey)
		{
			snprintf(data, sizeof(data), "exitkey:%s", server->getTrapName(trapkey));
			if(scriptEvent(data))
				goto finish;
			if(signalScript(trapkey))
				goto finish;
		}
		advance();
		goto finish;
	default:
		break;
	}

	return enterCommon(event);

finish:
	setRunning();
	return true;
}

bool BayonneSession::stateInkey(Event *event)
{
	char key;
	char evt[32];
	char dig[2];

	switch(event->id)
	{
	case LINE_WINK:
		if(state.inkey.var)
			setSymbol(state.inkey.var, "f", 1);
		if(scriptEvent("menukey:flash"))
			goto finish;
		if(enterCommon(event))
			goto finish;
		advance();
		goto finish;
        case DTMF_KEYUP:    
                dtmf_digits[digit_count++] = getChar(event->dtmf.digit);
                dtmf_digits[digit_count] = 0;	
	case ENTER_STATE:
		key = getDigit();
		if(!key && state.timeout)
		{
			startTimer(state.timeout);
			return true;
		}

		dig[0] = key;
		dig[1] = 0;

		if(state.inkey.var)
			setSymbol(state.inkey.var, dig, 1);

		if(!state.inkey.menu)
		{
			advance();
			goto finish;
		}

		if(!key)
		{
			if(scriptEvent("menukey:none"))
				goto finish;
			if(signalScript(SIGNAL_FAIL))
				goto finish;
			error("menukey-none");
			goto finish;
		}

		if(!strchr(state.inkey.menu, key))
		{
			if(scriptEvent("menukey:invalid"))
				goto finish;
			if(signalScript(SIGNAL_INVALID))
				goto finish;
			error("menukey-invalid");
			goto finish;
		}

		if(key == '*')
			strcpy(evt, "menukey:star");
		else if(key == '#')
			strcpy(evt, "menukey:pound");
		else
			snprintf(evt, sizeof(evt), "menukey:%c", key);

		if(signalScript((signal_t)(Bayonne::getDigit(key) + SIGNAL_0)))				 
			goto finish;
		
		if(scriptEvent(evt))
			goto finish;

		error("menukey-unknown");
		goto finish;
		
	case TIMER_EXPIRED:
		if(state.inkey.var)
			setSymbol(state.inkey.var, "", 1);
		if(state.inkey.menu)
			if(scriptEvent("menukey:timeout"))
				goto finish;
	default:
		return enterCommon(event);
	}

finish:
	setRunning();
	return true;
}

bool BayonneSession::stateTone(Event *event)
{
	if(enterTone(event))
		return true;

	switch(event->id)
	{
	case ENTER_STATE:
		state.tone.flashing = false;
		if(state.timeout && state.timeout != TIMEOUT_INF)
			startTimer(state.timeout);
		return true;
	case AUDIO_IDLE:
	case TIMER_EXPIRED:
		if(state.tone.exiting)
		{
			setState(STATE_HANGUP);
			return true;
		}
		if(setLibreset(RESULT_COMPLETE))
			return true;

		advance();
		setRunning();
		return true;
	case LINE_HANGUP:
		if(state.tone.flashing)
			return false;
	default:
		return enterCommon(event);
	case START_FLASH:
		state.tone.flashing = true;
		setOffhook(false);
		return enterCommon(event);
	case STOP_FLASH:
		setOffhook(true);
		state.tone.flashing = false;
		return enterCommon(event);
	case TONE_START:
		return false;
	}
}

bool BayonneSession::stateDTMF(Event *event)
{
	return stateTone(event);
}

bool BayonneSession::stateReconnect(Event *event)
{
	bool rtn = false;
	Event ex;

	if(enterReconnect(event))
		return true;

	switch(event->id)
	{
	case ENTER_RECONNECT:
	case RECALL_RECONNECT:
		return false;
	case ENTER_STATE:
		if(state.timeout && state.timeout != TIMEOUT_INF)
			startTimer(state.timeout);
		return true;
	case TIMER_EXPIRED:
		setRunning();
		goto finish;
	default:
		rtn = enterCommon(event);
		if(state.handler != &BayonneSession::stateReconnect)
		{
finish:
			memset(&ex, 0, sizeof(event));
			ex.id = EXIT_RECONNECT;
			enterReconnect(&ex);
		}
		return rtn;
	}
}

bool BayonneSession::stateCalling(Event *event)
{
	bool dtmf, refer, hangup;
	Ring *r;

	if(event->id == TIMER_EXPIRED)
	{
		if(state.timeout)
			event->id = RING_SYNC;
	}

	if(enterTone(event))
		return true;

	switch(event->id)
	{
	case TIMER_EXPIRED:
		Ring::detach(ring);
		ring = NULL;
		if(!scriptEvent("call:expired"))
			error("call-expired");
		setRunning();
		return true;
	case CHILD_DND:
	case CHILD_AWAY:
	case CHILD_NOCODEC:
	case CHILD_INVALID:
	case CHILD_OFFLINE:
		r =Ring::find(ring, event->child);
		if(!r)
			return false;
		r->session = NULL;
		r->driver = NULL;
		return true;
	case CHILD_BUSY:
	case CHILD_FAILED:
		r = Ring::find(ring, event->child);
		if(!r)		
			return false;
		r->session = NULL;
		return true;
	case CHILD_FAX:
		r = Ring::find(ring, event->child);
		r->session = NULL;
		if(!scriptEvent("call:fax"))
			error("call-fax");
		setRunning();
		return true;
	case PEER_WAITING:
		r = Ring::find(ring, event->peer);
		if(!r)
			return false;
		r->session = NULL;
		dtmf = state.tone.dtmf;
		hangup = state.tone.hangup;
		refer = state.tone.refer;
		state.timeout = state.tone.duration;
		state.join.dtmf = dtmf;
		state.join.hangup = hangup;
		state.join.refer = refer;
		state.join.dial = NULL;
		state.join.exit = NULL;
		state.join.peer = event->peer;
		Ring::detach(ring);
		clrAudio();
		setState(STATE_JOIN);
		return true;
	case RING_SYNC:
	case ENTER_STATE:
		Ring::start(ring, this);
		if(state.timeout < 4000)
			state.timeout = 0;
		else
			state.timeout -= 4000;
		startTimer(4000);
		return true;
	default:
		return enterCommon(event);
	}
}


bool BayonneSession::stateConnect(Event *event)
{
	Event cancel;
	BayonneSession *s;
	bool dtmf, rtn, hangup, refer;

        if(enterTone(event))
                return true;

        switch(event->id)
        {
        case ENTER_STATE:
		if(state.timeout && state.timeout != TIMEOUT_INF)
	                startTimer(state.timeout);
                return true;
failed:
        case STOP_PARENT:
        case CHILD_FAILED:
                if(!scriptEvent("dial:failed"))
                        error("dial-failed");
                setRunning();
                return true;
	case CHILD_OFFLINE:
		if(!scriptEvent("dial:offline"))
			if(!scriptEvent("dial:failed"))
				error("dial-offline");
		setRunning();
		return true;
	case CHILD_INVALID:
		if(!scriptEvent("dial:invalid"))
			error("dial-invalid");
		setRunning();
		return true;
        case TIMER_EXPIRED:
                memset(&event, 0, sizeof(event));
                cancel.id = CANCEL_CHILD;
                cancel.pid = this;
                s = getSid(state.tone.sessionid);
                if(s)
                        s->queEvent(&cancel);
        case CHILD_EXPIRED:
                if(!scriptEvent("dial:noanswer"))
                        error("dial-noanswer");
                setRunning();
                return true;
        case CHILD_BUSY:
                if(!scriptEvent("dial:busy"))
                        error("dial-busy");
		setRunning();
                return true;
	case CHILD_NOCODEC:
		if(!scriptEvent("dial:nocodec"))
			if(!scriptEvent("dial:failed"))
				error("dial-nocodec");
		setRunning();
		return true;
	case CHILD_AWAY:
		if(!scriptEvent("dial:away"))
			if(!scriptEvent("dial:noanswer"))
				error("dial-away");
		setRunning();
		return true;	
	case CHILD_DND:
		if(!scriptEvent("dial:dnd"))
			if(!scriptEvent("dial:busy"))
				error("dial-dnd");
		setRunning();
		return true;
	case CHILD_FAX:
		if(!scriptEvent("dial:fax"))
			error("dial-fax");
		return true;
        case PEER_WAITING:
		s = getSid(state.tone.sessionid);
		dtmf = state.tone.dtmf;
		hangup = state.tone.hangup;
		refer = state.tone.refer;
		state.timeout = state.tone.duration;
		state.join.dtmf = dtmf;
		state.join.hangup = hangup;
		state.join.refer = refer;
		state.join.dial = NULL;
		state.join.exit = NULL;
		state.join.peer = s;
		if(!s)
			goto failed;
		clrAudio();
		setState(STATE_JOIN);
                return true;
	case STOP_SCRIPT:
	case CALL_FAILURE:
	case CALL_DISCONNECT:		
	case STOP_DISCONNECT:
                memset(&cancel, 0, sizeof(cancel));
                cancel.id = STOP_DISCONNECT;
                s = getSid(state.tone.sessionid);
                if(s)
                        s->queEvent(&cancel);
		return enterCommon(event);
	default:
		s = getSid(state.tone.sessionid);
		rtn = enterCommon(event);
		if(state.handler != &BayonneSession::stateConnect)
		{
			memset(&cancel, 0, sizeof(cancel));
			cancel.id = STOP_DISCONNECT;
			s->queEvent(&cancel);
		}
		return rtn;
        }
}

bool BayonneSession::statePlay(Event *event)
{
	char evtmsg[65];
	char key;

        if(enterPlay(event))
                return true;

	switch(event->id)
	{
	case ENTER_STATE:
		if(dtmf_digits)
			digit_count = strlen(dtmf_digits);
		else
			digit_count = 0;

		if(digit_count && state.audio.exitkey)
		{
			if(setLibreset(RESULT_PENDING))
				return true;
			advance();
			setRunning();
		}
		return true;
	case DTMF_KEYUP:
                key = getChar(event->dtmf.digit);
                if(state.audio.exit)
                        if(strchr(state.audio.exit, key))
                        {
                                snprintf(evtmsg, sizeof(evtmsg), "exitkey:%c", key);
                                if(scriptEvent(evtmsg))
                                {
                                        setRunning();
                                        return true;
                                }
                        }
                        else
                                goto exitkey;

                if(state.audio.menu)
                 	if(strchr(state.audio.menu, key))
                 	{
                        	snprintf(evtmsg, sizeof(evtmsg), "playkey:%c", key);
                                if(scriptEvent(evtmsg))
                                {
                                        setRunning();
                                        return true;
                                }
                        }

                if(state.audio.exitkey)
                {
exitkey:
                        *dtmf_digits = key;
                        dtmf_digits[1] = 0;
                        digit_count = 1;
			if(setLibreset(RESULT_PENDING))
				return true;
                        advance();
                        setRunning();
                        return true;
                }
	default:
		return enterCommon(event);
	case AUDIO_IDLE:
	case TIMER_EXPIRED:
		if(setLibreset(RESULT_COMPLETE))
			return true;

		advance();
		setRunning();
		return true;
	}
}

void BayonneSession::renameRecord(void)
{
	const char *path = audio.getFilename(state.audio.list[0], true);

	if(!path || !state.audio.list[1])
		return;

	rename(path, state.audio.list[1]);
	state.audio.list[1] = NULL;
}

bool BayonneSession::stateRecord(Event *event)
{
	char evtmsg[65];
	char key;
	time_t now;
	const char *path;
	char *buffer = (char *)&state.audio.list[2];

        switch(event->id)
        {
        case ENTER_STATE:
                time(&audiotimer);
		if(!state.audio.list[1])
			break;
		path = audio.getFilename(state.audio.list[1], true);
		if(path)
		{
			setString(buffer, 256, path);
			state.audio.list[1] = buffer;
		}
		else
			state.audio.list[1] = NULL;	
                break;
        case STOP_DISCONNECT:
        case STOP_SCRIPT:
        case CALL_DISCONNECT:
        case CALL_FAILURE:
	case START_REFER:
                time(&now);
                if(now - audiotimer > 3 || state.audio.total < 30000)
		{
			renameRecord();
                        break;
		}
		path = audio.getFilename(state.audio.list[0], true);
                remove(path);
		state.audio.list[1] = 0;
	default:
		break;
	case AUDIO_IDLE:
		renameRecord();
        }

        if(enterRecord(event))
                return true;

	switch(event->id)
	{
	case AUDIO_EXPIRED:
		event->id = TIMER_EXPIRED;
		break;
	case ENTER_STATE:
                digit_count = 0;
		*dtmf_digits = 0;
		state.audio.trigger = false;

		if(state.audio.exit || state.audio.exitkey || state.audio.menu)
			dtmf = enableDTMF();

		if(state.audio.silence)
			startTimer(state.audio.silence);

		return true;
	case AUDIO_START:
		state.audio.trigger = true;
		stopTimer();
		return true;
	case AUDIO_STOP:
		if(!state.audio.trigger)
		{
			stopTimer();
			if(state.audio.silence)
				startTimer(state.audio.silence);	
			return true;
		}
		if(state.audio.intersilence)
			startTimer(state.audio.intersilence);
		else if(state.audio.silence)
			startTimer(state.audio.silence);
		return true;
	case DTMF_KEYUP:
		key = getChar(event->dtmf.digit);
		if(state.audio.exit && strchr(state.audio.exit, key))
		{
			renameRecord();
			snprintf(evtmsg, sizeof(evtmsg), "exitkey:%c", key);
			if(scriptEvent(evtmsg))
			{
				renameRecord();
				setRunning();
				return true;
			}
			goto exitkey;
		}

		if(state.audio.menu && strchr(state.audio.menu, key))
		{
			snprintf(evtmsg, sizeof(evtmsg), "recordkey:%c", key);
			if(scriptEvent(evtmsg))
			{
				setRunning();
				return true;
			}
		}

		if(state.audio.exitkey)
		{
exitkey:
			*dtmf_digits = key;
			dtmf_digits[1] = 0;
			digit_count = 1;
			renameRecord();
			advance();
			setRunning();
			return true;
		}
	default:
		break;
	case AUDIO_IDLE:
		if(setLibreset(RESULT_COMPLETE))
			return true;

		advance();
		setRunning();
		return true;
	}

        return enterCommon(event);
}

bool BayonneSession::stateJoin(Event *event)
{
	Event dtmf_event, hold_event;
	const char *msg;
	const char *err;
        event_t pid = PART_DISCONNECT;
        bool rtn;  
	BayonneSession *p;

	if(enterJoin(event))
		return true;

	switch(event->id)
	{
	case CHILD_RUNNING:
		return true;
	case AUDIO_IDLE:
		clrAudio();
		return true;
    case ENTER_STATE:
		if(!strcmp(var_recall, var_pid) && time_joined)
			exitCall("release");
		p = getSid(var_recall);
		if(p && p != state.join.peer)
		{
			memset(&hold_event, 0, sizeof(hold_event));
			hold_event.peer = this;
			hold_event.id = DROP_RECALL;
			p->queEvent(&hold_event);
		}
                if(state.timeout && state.timeout != TIMEOUT_INF)
                        startTimer(state.timeout);
		strcpy(var_recall, "none");
                peer = state.join.peer;
		if(state.join.dtmf && !dtmf)
			dtmf = enableDTMF();
                setSymbol("script.error", "none");
		if(!strcpy(var_pid, peer->var_sid))
		{
			if(!time_joined)
			{
				time(&time_joined);
				enterCall();
			}
		}
		else
			time_joined = 0;
                return true; 
	case PEER_REFER:
		if(!peer || peer != event->peer)
			return false;

		if(holding)
		{
			
			memset(&hold_event, 0, sizeof(hold_event));
			hold_event.id = JOIN_RECALL;
			hold_event.peer = this;
			event->peer->queEvent(&hold_event);
			return true;
		}

		strcpy(var_recall, var_joined);
		setString(dtmf_digits, MAX_DTMF, event->dialing);
		if(!scriptEvent("call:transfer"))
			if(!redirect("connect::transfer"))
				error("call-transfer");
		setRunning();
		referring = false;
		return false;
			
	case START_REFER:
		if(!holding || !state.join.refer)
			return false;
		if(!peer)
			return false;
		peer->referring = true;
		if(peer->holding)
		{
			peer->referring = false;
			return false;
		}
		memset(&hold_event, 0, sizeof(hold_event));
		setString(state.join.digits, sizeof(state.join.digits), event->dialing);
		state.join.peer = peer;
		peer = NULL;
		setState(STATE_REFER);
		return true;
	case CALL_HOLD:
		if(holding)
			return false;
		holding = true;
	case AUDIO_DISCONNECT:
		if(!peer)
			return true;
		memcpy(&hold_event, event, sizeof(hold_event));
		hold_event.id = PEER_DISCONNECT;
		peer->queEvent(&hold_event);
		return true;
	case CALL_NOHOLD:
		holding = false;
        case AUDIO_RECONNECT:
		if(!peer)
			return true;
                memcpy(&hold_event, event, sizeof(hold_event));
                hold_event.id = PEER_RECONNECT;
                peer->queEvent(&hold_event);
                return true;
	case DTMF_KEYDOWN:
		if(!state.join.dtmf)
			return true;
		if(!peer)
			return true;
		memcpy(&dtmf_event, event, sizeof(dtmf_event));
		dtmf_event.id = DTMF_GENDOWN;
		peer->queEvent(&dtmf_event);
		return true;
	case DTMF_KEYUP:
		if(!state.join.dtmf)
		{
			pid = PART_EXITING;
			break;
		}
		if(!peer)
			break;
		memcpy(&dtmf_event, event, sizeof(dtmf_event));
		dtmf_event.id = DTMF_GENUP;
		peer->queEvent(&dtmf_event);
		break;
	case TIMER_EXPIRED:
		pid = PART_EXPIRED;
	default:
		break;
	case PART_EXITING:
		msg = "part:exiting";
		err = "part-exiting";
		goto part;	
	case PART_EXPIRED:
		msg = "part:expired";
		err = "part-expired";
		goto part;
	case PART_DISCONNECT:
		msg = "part:disconnect";
		err = "part-disconnect";
		goto part;
	}

	rtn = enterCommon(event);
	if(!isJoined())
	{
		part(pid);
		event->id = EXIT_PARTING;
		enterJoin(event);
	}
	return rtn;

part:
	if(scriptEvent(msg))
		state.join.hangup = false;
	else if(signalScript(SIGNAL_PART))
		state.join.hangup = false;
	else if(!state.join.hangup)
		error(err);

	if(!state.join.hangup)
		setRunning();

	event->id = EXIT_PARTING;
	enterJoin(event);
	if(state.join.hangup)
		setState(STATE_HANGUP);
	return true;
}
		
bool BayonneSession::stateWait(Event *event)
{
	Event pe;

        if(enterWait(event))
                return true;

	switch(event->id)
	{
	case ENTER_STATE:
		if(state.timeout && state.timeout != TIMEOUT_INF)
			startTimer(state.timeout);

		if(!state.join.peer)
			return true;
		memset(&pe, 0, sizeof(pe));
		pe.id = PEER_WAITING;
		pe.peer = this;
		state.join.peer->queEvent(&pe);
		return true;
	case JOIN_PEER:
		if(state.join.peer && event->peer != state.join.peer)
			return false;
		state.join.peer = event->peer;
		state.timeout = 0;
		setState(STATE_JOIN);
		return true;
	default:
	        return enterCommon(event);
	}
}

bool BayonneSession::stateHold(Event *event)
{
	if(enterHold(event))
		return true;

	switch(event->id)
	{
	case ENTER_STATE:
		error("hold-failed");
		setRunning();
		return true;
	default:
		return enterCommon(event);
	}
}

bool BayonneSession::stateRecall(Event *event)
{
        if(enterRecall(event))
                return true;

        switch(event->id)
        {
        case ENTER_STATE:
                error("recall-failed");    
                setRunning();
                return true;
	default:
	        return enterCommon(event); 
	}
}

bool BayonneSession::stateRefer(Event *event)
{
	Event pe;

	if(enterRefer(event))
		return true;

	switch(event->id)
	{
	case ENTER_STATE:
		memset(&pe, 0, sizeof(pe));
		pe.id = PEER_REFER;
		state.join.peer->queEvent(&pe);
		return true;
	case JOIN_RECALL:
		if(event->peer != state.join.peer)
			return false;
		strcpy(var_joined, event->peer->var_sid);
		state.timeout = 0;
		setState(STATE_JOIN);
		return true;
	case DROP_REFER:
		if(event->peer != state.join.peer)
			return false;
		if(scriptEvent("part:disconnect"))
			state.join.hangup = false;
		else if(signalScript(SIGNAL_PART))
			state.join.hangup = false;
		else if(!state.join.hangup)
			error("part-disconnect");
		if(state.join.hangup)
			setState(STATE_HANGUP);
		else
			setRunning();
		return true;
	case DROP_RECALL:
		if(event->peer != state.join.peer)
			return false;
		if(time_joined)
		{
			exitCall("transfer");
			time_joined = 0;
		}
		if(scriptEvent("part:transfer"))
			state.join.hangup = false;
		else if(signalScript(SIGNAL_PART))
			state.join.hangup = false;
		else if(!state.join.hangup)
			error("part-transfer");
		if(state.join.hangup)
			setState(STATE_HANGUP);
		else
			setRunning();
		return true;
	default:
		return enterCommon(event);
	}
}

bool BayonneSession::stateXfer(Event *event)
{
	if(enterXfer(event))
		return true;

	switch(event->id)
	{
        case DIAL_BUSY:
                if(!scriptEvent("transfer:busy"))
                        error("transfer-busy");
                setRunning();
                return true;
        case DIAL_INVALID:
                if(!scriptEvent("transfer:invalid"))
                        error("transfer-invalid");
                setRunning();
                return true;
        case DIAL_FAILED:
                if(!scriptEvent("transfer:failed"))
                        error("transfer-failed");
                setRunning();
                return true;
	case ENTER_STATE:
		event->errmsg = "feature-unsupported";
	case ERROR_STATE:
		error(event->errmsg);
		if(setLibexec(RESULT_FAILED))
			return true;
		setRunning();
		return true;
	case TIMER_EXPIRED:
		setState(STATE_HANGUP);
		return true;
	default:
		return enterCommon(event);
	}
}

bool BayonneSession::stateDial(Event *event)
{
        if(enterDial(event))
                return true;

	switch(event->id)
	{
	case ENTER_STATE:
		if(!scriptEvent("missing:dial"))
			error("dial-unsupported");
		setRunning();
		return true;			
	case DIAL_CONNECT:
		setSymbol("script.error", "none");
		advance();
		setRunning();
		return true;
	case TIMER_EXPIRED:
	case DIAL_TIMEOUT:
		if(!scriptEvent("dial:noanswer"))
			error("dial-timeout");
		setRunning();
		return true;		
	case DIAL_INVALID:
		if(!scriptEvent("dial:invalid"))
			error("dial-invalid");
		setRunning();
		return true;
	case DIAL_FAILED:
		if(!scriptEvent("dial:failed"))
			error("dial-failed");
		setRunning();
		return true;
	case DIAL_BUSY:
		if(!scriptEvent("dial:busy"))
			error("dial-busy");
		return true;
	case TONE_START:
		return true;
	default:	
	        return enterCommon(event);
	}
}

bool BayonneSession::stateBusy(Event *event)
{
	if(enterBusy(event))
		return true;

        return enterCommon(event);
}

bool BayonneSession::stateStandby(Event *event)
{
	if(enterStandby(event))
		return true;

	if(event->id == MAKE_UP)
	{
		setState(STATE_IDLE);
		return true;
	}
	return false;
}

bool BayonneSession::stateFinal(Event *event)
{
	if(enterFinal(event))
		return true;

	if(event->id == ENTER_STATE)
		makeIdle();

	return true;
}

bool BayonneSession::stateRinging(Event *event)
{
	if(enterRinging(event))
		return true;

	switch(event->id)
	{
	case ENTER_STATE:
		startTimer(driver->getRingTimer());
		return true;
	case TIMER_EXPIRED:
		slog.notice("%s: call dissapeared", logname);
		setState(STATE_IDLE);
		return true;
	case RING_ON:
		stopTimer();
		return true;
	case RING_OFF:
		stopTimer();
		startTimer(driver->getRingTimer());
		snprintf(var_rings, sizeof(var_rings), "%d", ++ring_count);
		if(ring_count < driver->getAnswerCount())
			return true;
		if(attachStart(event))
		{
			scriptEvent("incoming:ringing");
			setState(STATE_PICKUP);
		}
		return true;
	default:
		return enterCommon(event);		
	}
}

bool BayonneSession::stateIdle(Event *event)
{
	if(enterIdle(event))
		return true;

	switch(event->id)
	{
	case ENTER_STATE:
		makeIdle();
		BayonneDriver::add(this);
		return true;
	case TONE_START:
	case TONE_STOP:
	case LINE_HANGUP:
	case LINE_WINK:
	case LINE_DISCONNECT:
	case STOP_DISCONNECT:
	case CALL_DISCONNECT:
	case CANCEL_CHILD:
	case STOP_SCRIPT:
	case EXIT_THREAD:
	case MAKE_IDLE:
	case CALL_HOLD:
	case CALL_NOHOLD:
		return true;
	case RING_ON:
		type = INCOMING;
		setState(STATE_RINGING);
		return true;
	case LINE_PICKUP:
		offhook = true;
		type = PICKUP;
		goto attach;
	case START_RECALL:
		type = RECALL;
		goto attach;
	case START_FORWARDED:
		type = FORWARDED;
		goto attach;
	case START_DIRECT:
		type = DIRECT;
		goto attach;
	case START_RINGING:
		type = RINGING;
		switch(iface)
		{
		case IF_INET:
			if(!event->start.dialing)
			{
				purge();
				BayonneDriver::add(this);
				return false;
			}
			goto seize;
			break;
		case IF_NONE:
			if(event->start.dialing)
			{
				purge();
				BayonneDriver::add(this);
				return false;
			}
			goto seize;
		default:
			purge();
			BayonneDriver::add(this);
			type = NONE;
			return false;
		}
	case START_HUNTING:
		type = RINGING;
		if(attachStart(event))
			setState(STATE_HUNTING);
		else
		{
			type = NONE;
			slog.error("%s: hunt failed", logname);
			purge();
		}
		return true;			
	case START_INCOMING:
		type = INCOMING;

attach:
		if(attachStart(event))
			setState(STATE_PICKUP);
		else
		{
			type = NONE;
			slog.error("%s: start failed", logname);
			purge();
		}
		return true;
	case START_OUTGOING:
		type = OUTGOING;
		switch(iface)
		{
		case IF_INET:
			if(!event->start.dialing)
			{
				purge();
				BayonneDriver::add(this);
				return false;
			}
			break;
		case IF_NONE:
			if(event->start.dialing)
			{
				purge();
				BayonneDriver::add(this);
				return false;
			}
		default:
			break;
		}
seize:
		if(attachStart(event))
		{
			setState(STATE_SEIZE);
			return true;
		}
		slog.error("%s: start failed", logname);
		purge();
		BayonneDriver::add(this);
		type = NONE;
	case AUDIO_DISCONNECT:
	case AUDIO_RECONNECT:
		return false;
	default:
		return enterCommon(event);
	}
}

bool BayonneSession::stateAnswer(Event *event)
{
	if(enterAnswer(event))
		return true;

	switch(event->id)
	{
	case ENTER_STATE:
		startTimer(50);
		return true;
	case TIMER_EXPIRED:
		advance();
		setRunning();
		return true;
	case LINE_DISCONNECT:
		return true;
	case AUDIO_DISCONNECT:
	case AUDIO_RECONNECT:
	case DTMF_KEYDOWN:
	case DTMF_KEYUP:
		return false;
	default:
		return enterCommon(event);
	}
}

bool BayonneSession::statePickup(Event *event)
{
	if(event->id == ENTER_STATE && !offhook)
		incIncomingAttempts();

	if(enterPickup(event))
		return true;

	switch(event->id)
	{
	case ENTER_STATE:
		if(image && offhook)
			goto running;
		setOffhook(true);
		startTimer(driver->getPickupTimer());
		return true;
	case LINE_OFF_HOOK:
	case TIMER_EXPIRED:
		goto running;
	case LINE_DISCONNECT:
		return true;
	case AUDIO_DISCONNECT:
	case AUDIO_RECONNECT:
		return false;
	default:
		return enterCommon(event);
	}

running:
	switch(type)
	{
	case INCOMING:
		break;
	case OUTGOING:
	case RINGING:
		setConnecting();
		return true;
	case PICKUP:
		scriptEvent("call:pickup");
		break;
	case FORWARDED:
		scriptEvent("call:forward");
		break;
	case RECALL:
		scriptEvent("call:recall");
		break;
	default:
		break;
	}
	setRunning();
	return true;
}		

bool BayonneSession::stateHunting(Event *event)
{
	const char *cp;
	unsigned idx;

	if(event->id == ENTER_STATE)
	{
		type = RINGING;
		state.join.index = 0;
		incOutgoingAttempts();
		state.join.hunt_timer = driver->getHuntTimer();
	}

	if(event->id != ENTER_HUNTING)
		if(enterHunting(event))
			return true;

	switch(event->id)
	{
	case ENTER_STATE:
		setOffhook(true);
		event->id = ENTER_HUNTING;
		return true;
	case EXIT_HUNTING:
		state.join.drop = true;
		startTimer(driver->getHangupTimer());
		return true;
	case DIAL_CONNECT:
		setConst("session.dialed", state.join.dial);
		seizure = CHILD_RUNNING;
		setConnecting();
		return true;
	case LINE_DISCONNECT:
	case TONE_START:
		return true;
	case DIAL_FAX:
		setConst("session.dialed", state.join.dial);
		seizure = CHILD_FAX;
		setConnecting("hunt:faxout");
		return true;
	case DIAL_PAM:
		setConst("session.dialed", state.join.dial);
		seizure = CHILD_RUNNING;
		setConnecting("call:machine");
		return true;		
	case TIMER_EXPIRED:
		if(state.join.drop)
		{
			event->id = ENTER_HUNTING;
			state.join.drop = false;
			return true;
		}			
	case CALL_FAILURE:
	case DIAL_FAILED:
	case DIAL_BUSY:
	case DIAL_INVALID:
		event->id = EXIT_HUNTING;
		return true;
	case ENTER_HUNTING:
hunting:
		state.join.drop = false;
		while(state.join.select)
		{
			idx = state.join.index++;
			cp = state.join.select->args[idx];
			if(cp)
			{
				++state.join.index;
				break;
			}
			state.join.select = state.join.select->next;
			state.join.index = 0;
		}		
		if(state.join.dial && state.join.select)
		{
			if(enterHunting(event))
			{
				startTimer(state.join.hunt_timer);
				return true;
			}
			goto hunting;
		}
		seizure = CHILD_FAILED;
		if(scriptEvent("call:hunting"))
		{
			setRunning();
			return true;
		}
		setState(STATE_HANGUP);
		return true;		
	default:
		return enterCommon(event);
	}
}

bool BayonneSession::stateSeize(Event *event)
{
	char evtname[96];

	if(event->id == ENTER_STATE && !offhook)
		incOutgoingAttempts();

	if(enterSeize(event))
		return true;

	switch(event->id)
	{
	case ENTER_STATE:
		state.join.dial = NULL;

		if(image && offhook)
		{
			setConnecting();
			return true;
		}
		setOffhook(true);
		if(driver->getSeizeTimer())
			startTimer(driver->getSeizeTimer());
		else
			setConnecting();
		return true;
	case TONE_START:
		if(state.join.dial)
			return true;

		snprintf(evtname, sizeof(evtname), "call:%s", event->tone.name);
  		if(scriptEvent(evtname))
		{
			setRunning();
			return true;
		}
		if(!stricmp(event->tone.name, "dialtone"))
		{
			state.join.dial = getSymbol("session.dialed");
			if(state.join.dial)
			{
				event->id = ENTER_STATE;
				if(enterDial(event))
					return true;
				goto failed;
			}
			setConnecting();
			return true;
		}
		if(event->tone.exit)
		{
			slog.notice("%s: %s; exiting", logname, event->tone.name);
			setState(STATE_HANGUP);
		}
		return true;
	case DIAL_FAX:
		seizure = CHILD_FAX;
		setConnecting("call:faxout");
		return true;
	case DIAL_PAM:
		seizure = CHILD_RUNNING;
		setConnecting("call:machine");
		return true;
	case DIAL_INVALID:
		seizure = CHILD_INVALID;
		if(scriptEvent("call:invalid"))
		{
			setRunning();
			return true;
		}
		setSymbol("script.error", "session-invalid");
		setState(STATE_HANGUP);
		return true;
	case DIAL_DND:
		seizure = CHILD_DND;
                if(scriptEvent("call:dnd") || scriptEvent("call:busy"))
                {
                        setRunning();
                        return true;
                }
		setSymbol("script.error", "session-dnd");
                setState(STATE_HANGUP);
                return true;		
	case DIAL_BUSY:
		seizure = CHILD_BUSY;
		if(scriptEvent("call:busy"))
		{
			setRunning();
			return true;
		}
		setSymbol("script.error", "session-busy");
		setState(STATE_HANGUP);
		return true;
	case DIAL_CONNECT:
		seizure = CHILD_RUNNING;
		setConnecting();
		return true;
	case DIAL_AWAY:
		seizure = CHILD_AWAY;
		if(scriptEvent("call:away") || scriptEvent("call:noanswer"))
		{
			setRunning();
			return true;
		}
		setSymbol("script.error", "session-away");
		setState(STATE_HANGUP);
		return true;
	case DIAL_TIMEOUT:
noanswer:
		seizure = CHILD_EXPIRED;
		if(scriptEvent("call:noanswer"))
		{
			setRunning();
			return true;
		}
		setSymbol("script.error", "session-noanswer");
		setState(STATE_HANGUP);
		return true;
	case TIMER_EXPIRED:
		if(state.join.dial)
			goto noanswer;

	case DIAL_NOCODEC:
		seizure = CHILD_NOCODEC;
		goto failure;
	case DIAL_OFFLINE:
		seizure = CHILD_OFFLINE;
		goto failure;
	case CALL_FAILURE:
	case DIAL_FAILED:
failed:
		seizure = CHILD_FAILED;
failure:
		if(scriptEvent("call:failed"))
		{
			setRunning();
			return true;
		}
		slog.notice("%s: no dialtone; exiting", logname);
		setSymbol("script.error", "session-failed");
		setState(STATE_HANGUP);
		return true;
	case LINE_DISCONNECT:
		return true;
	case AUDIO_DISCONNECT:
	case AUDIO_RECONNECT:
	default:
		return enterCommon(event);
	}
}		

bool BayonneSession::stateRunning(Event *event)
{
	BayonneSession *parent;

	switch(event->id)
	{
	case ENTER_STATE:
		if(ring)
		{
			Ring::detach(ring);
			ring = NULL;
		}
		check();
		if(!starting)
		{
			event->id = seizure;
			event->child = this;
			parent = getSid(var_pid);
			if(parent)
				parent->queEvent(event);
			starting = true;
			event->id = ENTER_STATE;
			if(seizure != CHILD_RUNNING)
				strcpy(var_pid, "none");
			setConst("session.callref", var_sid);
			switch(type)
			{
			case RINGING:
			case OUTGOING:
				incOutgoingComplete();
			case VIRTUAL:
				break;
			default:
				incIncomingComplete();
			}			
		}
		if(state.pid)
			newTid();
		if(thread)
		{
			delete thread;
			thread = NULL;
			startTimer(reset_timer);
			return true;
		}
	case TIMER_EXPIRED:
		if(holding)
			return true;
		clrAudio();
		if(vm)
		{
			if(vm->stepEngine())
					startTimer(step_timer);
			return true;
		}
		step();
		check();
		if(state.handler == &BayonneSession::stateRunning)
			startTimer(step_timer);
		else
			setSymbol("script.error", "none");
		return true;
	case STOP_DISCONNECT:
	case STOP_SCRIPT:
	case CALL_DISCONNECT:
	case LINE_DISCONNECT:
	case LINE_WINK:
		if(holding)
			startTimer(step_timer);
		return enterCommon(event);
	case EXIT_SCRIPT:
		startTimer(step_timer);
		return enterCommon(event);
	case CALL_NOHOLD:
		if(!holding)
			return false;
		startTimer(step_timer);
		holding = false;
		return true;
	default:
		return enterCommon(event);
	}
}

bool BayonneSession::stateThreading(Event *event)
{
        switch(event->id)
        {
	case ENTER_STATE:
		if(!thread)
		{
			error("thread-missing");
			setRunning();
			return true;
		}
		thread->start();
		startTimer(thread->getTimeout());
		return true;
	case TIMER_EXPIRED:	// use common handler...
		slog.warn("%s: thread expired", logname);
	default:
	        return enterCommon(event);
	}
}

bool BayonneSession::stateHangup(Event *event) 
{
	timeout_t timer;
	Event pe;
	BayonneSession *parent = NULL;

	if(event->id == ENTER_STATE)
	{
		if(vm)
			vm->disconnectEngine();
		decActiveCalls();
		holding = false;
	}

        if(enterHangup(event))
                return true; 

        switch(event->id)
        {
	case CANCEL_CHILD:
	case TONE_START:
	case TONE_STOP:
	case LINE_PICKUP:
	case LINE_HANGUP:
	case LINE_WINK:
	case LINE_DISCONNECT:
	case STOP_DISCONNECT:
	case STOP_SCRIPT:
	case EXIT_THREAD:
	case CALL_HOLD:
	case CALL_NOHOLD:
	case MAKE_IDLE:		// ignore since already going idle
		return true;

	case AUDIO_DISCONNECT:
	case AUDIO_RECONNECT:
		return false;

	case LINE_ON_HOOK:
	case TIMER_EXPIRED:
		if(vm)
		{
			startTimer(100);
			return true;
		}
		clrAudio();
		setState(STATE_IDLE);
		return true;
	case ENTER_STATE:
		exiting = true;	// definately exiting...
		newTid();

		if(ring)
		{
			Ring::detach(ring);
			ring = NULL;
		}

		if(!starting)
			parent = getSid(var_pid);

		starting = true;
		if(parent)
		{
			memset(&pe, 0, sizeof(pe));
			pe.id = seizure;
			pe.child = this;
			parent->queEvent(&pe);
		}			

		timer = driver->getHangupTimer();
		if(!offhook)
			timer = 0;

		if(thread)
		{
			if(timer < reset_timer)
				timer = reset_timer;
			delete thread;
			thread = NULL;
		}

		if(image)
			detach();

		if(!timer)
		{
			clrAudio();
			setState(STATE_IDLE);
			return true;
		}
		setOffhook(false);
		startTimer(timer);
		return true;
        default:
	        return enterCommon(event);
	}
}

bool BayonneSession::filterPosting(Event *event)
{
	BayonneSession *pid;

	switch(event->id)
	{
	case CALL_HOLD:
	case CALL_NOHOLD:
		if(referring)
			return false;
		return true;
	case PART_DISCONNECT:
	case PART_EXPIRED:
	case PART_EXITING:
		if(!peer)
			return false;
		return true;
	case DTMF_KEYUP:
	case DTMF_KEYDOWN:
		if(!dtmf || !dtmf_digits || holding)
			return false;
		digit_count = strlen(dtmf_digits);
		return true;
	case STOP_PARENT:
	case CHILD_FAILED:
	case CHILD_INVALID:
	case CHILD_BUSY:
	case CHILD_EXPIRED:
	case CHILD_FAX:
		if(!image || holding)
			return false;
		return true;
	case EXIT_THREAD:
		if(!thread)
			return false;
		return true;
	case TIMER_EXPIRED:
		if(getRemaining() == TIMEOUT_INF)
			return false;
		stopTimer();
		return true;
	case ENABLE_LOGGING:
		if(logevents && logevents != &cout)
			return false;
		logevents = event->debug.output;
		state.logstate = getState(event->debug.logstate);
		return true;
	case DISABLE_LOGGING:
		if(!event->debug.output || logevents == event->debug.output || logevents == &cout)
		{
			logevents = NULL;
			state.logstate = NULL;
			return true;
		}
		return false;
	case LINE_HANGUP:
		if(type == PICKUP)
			offhook = false;
		return true;
	case CANCEL_CHILD:
	case DETACH_CHILD:
		Thread::yield();
		if(!stricmp(var_pid, "none"))
			return false;
		pid = getSid(var_pid);
		if(pid && pid != event->pid)
			return false;
		strcpy(var_pid, "none");
	default:
		return true;	
	}
}

bool BayonneSession::enterCommon(Event *event)
{
	result_t result;
	char evtmsg[65];

	switch(event->id)
	{
	case EXIT_SCRIPT:
		if(vm)
		{
			delete vm;
			vm = NULL;
		}
		return true;
	case ENABLE_LOGGING:
	case DISABLE_LOGGING:
		return true;

	case START_REFER:
		return false;

	case SYSTEM_DOWN:
		return true;

	case CALL_HOLD:
		if(offhook && !holding)
			holding = true;
		else
			return false;
		return true;
	case CALL_NOHOLD:
		if(holding)
			holding = false;
		else
			return false;
		return true;

	case MAKE_DOWN:
		makeIdle();
		setState(STATE_DOWN);
		return true;
	case MAKE_IDLE:
		if(offhook)
		{
			setState(STATE_HANGUP);
			return true;
		}
		if(thread)
		{
			setState(STATE_IRESET);
			return true;
		}
		if(image)
			detach();
		setState(STATE_IDLE);
		return true;
	case ENTER_STATE:
		stopTimer();
		return true;
	case CANCEL_CHILD:
		if(signalScript(SIGNAL_PARENT))
		{
			setRunning();
			return true;
		}
	case STOP_SCRIPT:
		if(image)
		{
			holding = false;
			setState(STATE_HANGUP);
			return true;
		}
		return false;
	case POST_LIBEXEC:
                if(scriptEvent(event->name))
                {
                        setRunning();
                        return true;
                }
                return false;  
	case PEER_WAITING:
		if(scriptEvent("start:waiting"))
		{
			setRunning();
			return true;
		}
		if(signalScript(SIGNAL_WAIT))
		{
			setRunning();
			return true;
		}
		return false;

child:
	case CHILD_FAILED:
		if(scriptEvent("child:failed"))
			goto childerr;
	case STOP_PARENT:
		if(scriptEvent("child:exiting"))
			setRunning();
		else if(signalScript(SIGNAL_CHILD))
			setRunning();
		return true;
	case RING_ON:
		if(offhook || answered)
			return true;
		snprintf(var_rings, sizeof(var_rings), "%d", ++ring_count);
		return true;
	case RING_OFF:
		if(offhook || answered)
			return false;
		if(!image)
			return false;
		snprintf(evtmsg, sizeof(evtmsg), "ring:%d", ring_count);		
		if(scriptEvent(evtmsg))
		{
			setRunning();
			return true;
		}
		return false;
	case RING_STOP:
		if(offhook || answered)
			return false;
		if(!image)
			return false;
		if(scriptEvent("ring:stop"))
		{
			setRunning();
			return true;
		}
		goto stop;
	case LINE_HANGUP:
		offhook = false;
		goto stop;
	case TONE_START:
		if(!image)
			return true;

		if(!offhook)
			return false;

		holding = false;
		snprintf(evtmsg, sizeof(evtmsg), "tone:%s", event->tone.name);
		if(scriptEvent(evtmsg))
		{
			setRunning();
			return true;
		}
		if(event->tone.exit)
			goto stop;
		return false;
	case LINE_DISCONNECT:
		holding = false;
		if(!offhook || !image)
			return true;

		if(scriptEvent("line:disconnect"))
		{
			setRunning();
			return true;
		}
	case STOP_DISCONNECT:
	case CALL_DISCONNECT:
stop:
		holding = false;
		if(!image)
			return false;

		if(exiting)
			return false;

		if(!signalScript(SIGNAL_HANGUP))
			return false;

		exiting = true;
		setRunning();
		return true;
	case LINE_WINK:
		if(!image)
			return false;

		holding = false;
		if(!scriptEvent("line:wink"))
			if(!signalScript(SIGNAL_WINK))
				return false;

		setRunning();
		return true;
	case LINE_PICKUP:
		if(!image)
			return true;

		holding = false;

		if(!scriptEvent("line:pickup"))
			if(!signalScript(SIGNAL_PICKUP))
				return false;

		setRunning();
		return true;			
	case EXIT_THREAD:
		if(!thread)
			return true;

		if(!image)
		{
			delete thread;
			thread = NULL;
			return true;
		}

		if(event->errmsg)
			result = RESULT_FAILED;
		else
			result = RESULT_COMPLETE;

		if(setLibreset(result))
			return true;

		if(!updated)
		{
			if(event->errmsg)
				error(event->errmsg);
			else
				advance();
		}

		setRunning();
		return true;
	case CHILD_RUNNING:
		if(!image)
			return false;
		if(scriptEvent("child:started"))
			setRunning();
		return true;
	case CHILD_INVALID:
		if(!image)
			return false;
		if(scriptEvent("child:invalid"))
			goto childerr;
		goto child;
        case CHILD_EXPIRED:
                if(!image)
                        return false;
                if(scriptEvent("child:expired"))
		{
childerr:
                        setRunning();   
			return true;
		}
		goto child;
        case CHILD_FAX:
                if(!image)
                        return false;
                if(scriptEvent("child:fax"))
                        goto childerr;
                goto child;
        case CHILD_BUSY:
                if(!image)
                        return false;
                if(scriptEvent("child:busy"))
			goto childerr;
                goto child;
	case TIMER_EXPIRED:
		if(!image)
			return false;

		if(thread)
		{
			if(setLibreset(RESULT_TIMEOUT))
				return true;
		}

		if(!signalScript(SIGNAL_TIMEOUT))
			error("timer-expired");
		setRunning();
		return true;
	case DTMF_KEYUP:
		if(!image)
			return false;

		if(digit_count < MAX_DTMF && dtmf_digits)
		{
			dtmf_digits[digit_count++] = getChar(event->dtmf.digit);
			dtmf_digits[digit_count] = 0;
		}
		
		if(!state.menu)
			if(signalScript((signal_t)(event->dtmf.digit + SIGNAL_0)))
			{
				setRunning();
				return true;
			}

		return false;
	case AUDIO_DISCONNECT:
	case AUDIO_RECONNECT:
		return true;
	default:
		return false;
	}
}	

bool BayonneSession::enterInitial(Event *event)
{
	return false;
}

bool BayonneSession::enterFinal(Event *event)
{
	return false;
}

bool BayonneSession::enterIdle(Event *event)
{
	return false;
}

bool BayonneSession::enterRinging(Event *event)
{
	return false;
}

bool BayonneSession::enterSeize(Event *event)
{
	return false;
}

bool BayonneSession::enterHunting(Event *event)
{
	return false;
}

bool BayonneSession::enterHangup(Event *event)
{
	return false;
}

bool BayonneSession::enterAnswer(Event *event)
{
	return false;
}

bool BayonneSession::enterPickup(Event *event)
{
	return false;
}
 
bool BayonneSession::enterReconnect(Event *event)
{
	return false;
}

bool BayonneSession::enterTone(Event *event)
{
        return false;
}

bool BayonneSession::enterDTMF(Event *event)
{
	return enterTone(event);
}

bool BayonneSession::enterPlay(Event *event)
{
        return false;
}

bool BayonneSession::enterRecord(Event *event)
{
        return false;
}

bool BayonneSession::enterJoin(Event *event)
{
        return false;
}

bool BayonneSession::enterWait(Event *event)
{
        return false;
}

bool BayonneSession::enterDial(Event *event)
{
        return false;
}

bool BayonneSession::enterXfer(Event *event)
{
	return false;
}

bool BayonneSession::enterRefer(Event *event)
{
	return false;
}

bool BayonneSession::enterHold(Event *event)
{
	return false;
}

bool BayonneSession::enterRecall(Event *event)
{
	return false;
}

bool BayonneSession::enterBusy(Event *event)
{
        return false;
}

bool BayonneSession::enterStandby(Event *event)
{
        return false;
}

bool BayonneSession::enterReset(Event *event)
{
	return false;
}

bool BayonneSession::enterRelease(Event *event)
{
	return false;
}

