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

namespace sangomadriver {
using namespace ost;
using namespace std;

Session::Session(Span *sp, timeslot_t ts) :
BayonneSession(&Driver::sangoma, ts, sp)
{
#ifndef WIN32
        if(getppid() > 1)
                logevents = &cout;
#endif
	iface = IF_ISDN;
	bridge = BR_SOFT;
	spri = &sp->spri;
	streamer = NULL;
}

Session::~Session()
{
	if(streamer)
	{
		delete streamer;
		streamer = NULL;
	}
}

bool Session::peerLinear(void)
{
	return true;
}

timeout_t Session::getRemaining(void)
{
        return TimerPort::getTimer();
}

void Session::startTimer(timeout_t timer)
{
        TimerPort::setTimer(timer);
        msgport->update();
}

void Session::stopTimer(void)
{
        TimerPort::endTimer();
        msgport->update();
}

bool Session::clearCall(void)
{
	bool rtn = offhook;

	if(rtn && streamer)
	{
		delete streamer;
		streamer = NULL;
	}

	offhook = false;
	return rtn;
}

void Session::setOffhook(bool flag)
{
	if(flag == offhook)
		return;

	if(!flag && streamer)
	{
		delete streamer;
		streamer = NULL;
	}

	if(flag)
		pri_answer(spri->pri, &call, timeslot - span->getFirst() + 1, 1);
	else
		pri_hangup(spri->pri, &call, -1);

	if(flag && !streamer)
	{
		update_pos = false;
		streamer = new AudioStreamer((Span *)span, this);
		streamer->start();
	}

	offhook = flag;
}	

void Session::makeIdle(void)
{
	update_pos = false;
	if(offhook)
		setOffhook(false);
	BayonneSession::makeIdle();
}

void Session::clrAudio(void)
{
	if(streamer)
	{
		streamer->enter();
		streamer->setSource(NULL, NULL);
		streamer->setTone(NULL);
		streamer->setSink(NULL, NULL);	
		streamer->leave();
	}

	if(audio.isOpen() && update_pos)
	{
		audio.getPosition(audio.var_position, 12);
		update_pos = false;
	}
	audio.cleanup();
}

bool Session::enterTone(Event *event)
{
        if(event->id == ENTER_STATE && audio.tone && streamer)
                streamer->setTone(audio.tone);

        return false;
}

bool Session::enterRecord(Event *event)
{
	if(event->id != ENTER_STATE)
		return false;

	if(!streamer)
	{
		error("no-audio");
		setRunning();
		return true;
	}

	audio.record(state.audio.list[0], state.audio.mode, state.audio.note);
	update_pos = true;
        if(!audio.isOpen())
        {
                slog.error("%s: audio file access error", logname);
                error("no-files");
                setRunning();
                return true;
        }
        if(!audio.isStreamable())
        {
                update_pos = false;
                error("no-codec");
                slog.error("%s: missing codec", logname);
                setRunning();
                return true;
        }
	streamer->setSink(&audio, audio.getCodec());
	return false;
}	

bool Session::enterPlay(Event *event)
{
	if(event->id != ENTER_STATE)
		return false;

	if(state.audio.mode == Audio::modeReadAny)
		update_pos = false;
	else
		update_pos = true;

	if(!streamer)
	{
		error("no-audio");
		setRunning();
		return true;
	}

	audio.play(state.audio.list, state.audio.mode);
	if(!audio.isOpen())
	{
		slog.error("%s: audio file access error", logname);
		error("no-files");
		setRunning();
		return true;
	}
	if(!audio.isStreamable())
	{
		error("no-codec");
		update_pos = false;
		slog.error("%s: missing codec", logname);
		setRunning();
		return true;
	}
	streamer->setSource(&audio, audio.getCodec());
	return false;
}

} // namespace
