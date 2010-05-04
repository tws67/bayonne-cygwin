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

namespace h323driver {
using namespace ost;
using namespace std;

Session::Session(timeslot_t ts) :
BayonneSession(&Driver::h323, ts),
TimerPort()
{
#ifndef WIN32
        if(getppid() > 1)
                logevents = &cout;
#endif
	iface = IF_INET;
	bridge = BR_GATE;
	conn = NULL;
	update_pos = false;
	channelType = AUDIO_NONE;
	frame_first = frame_last = NULL;
}

Session::~Session()
{
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

void Session::attachConnection(Connection *connection)
{
        if(connection)
                conn = connection;
        callToken = conn->GetCallToken();
}

bool Session::lockConnection(void)
{
        conn = (Connection *)Driver::endpoint.FindConnectionWithLock(callToken);
        return (conn != NULL);
}

void Session::releaseConnection(void)
{
        if(conn)
                conn->Unlock();
        conn = NULL;
}

Connection *Session::attachConnection(unsigned ref)
{
        conn = new Connection(Driver::endpoint, ref, this);
        return conn;
}

bool Session::answerCall(void)
{
        if(lockConnection())
        {
                conn->AnsweringCall(H323Connection::AnswerCallNow);
                releaseConnection();
                return true;
        }
        return false;
}

void Session::dropCall(void)
{
        Driver::endpoint.ClearCall(callToken, H323Connection::EndedByLocalUser);

}

void Session::makeIdle(void)
{
	update_pos = false;
	BayonneSession::makeIdle();
	conn = NULL;
}

bool Session::enterRinging(Event *event)
{
	switch(event->id)
	{
	case ENTER_STATE:
		startTimer(driver->getRingTimer());
		return true;
	case CALL_OFFERED:
		callToken = (char *)event->data;
		memset(event, 0, sizeof(Event));
		event->id = CALL_OFFERED;
		if(!attachStart(event))
		{
			setState(STATE_RELEASE);
			return true;
		}
		setState(STATE_PICKUP);
		return true;
	case TIMER_EXPIRED:
	case STOP_DISCONNECT:
		setState(STATE_RELEASE);
		return true;
	}
	return false;	
}

bool Session::enterHangup(Event *event)
{
	switch(event->id)
	{
	case ENTER_STATE:
		update_pos = false;
		dropCall();
		return false;
	}
	return false;
}

bool Session::enterRelease(Event *event)
{
	switch(event->id)
	{
	case ENTER_STATE:
		dropCall();
		if(image)
			detach();

		startTimer(driver->getHangupTimer());
		return true;
	case TIMER_EXPIRED:
		setState(STATE_IDLE);
		return true;
	}
	return false;
}

bool Session::enterPickup(Event *event)
{
	switch(event->id)
	{
	case ENTER_STATE:
		offhook = answerCall();
		if(!offhook)
		{
			setState(STATE_RELEASE);
			return true;
		}
		if(!driver->getPickupTimer())
		{
			event->id = TIMER_EXPIRED;
			return false;
		}
		startTimer(driver->getPickupTimer());
		return true;
	case STOP_DISCONNECT:
		setState(STATE_RELEASE);
		return true;
	case CALL_ANSWERED:
		setRunning();
		return true;
	}
	return false;
}

bool Session::enterPlay(Event *event)
{
	if(event->id == ENTER_STATE)
	{
		if(state.audio.mode == Audio::modeReadAny)
			update_pos = false;
		else
			update_pos = true;
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
			update_pos = false;
			error("no-codec");
			slog.error("%s: missing codec", logname);
			setRunning();
			return true;
		}
		if(lockConnection())
		{
			conn->channel->attachTransmitter(&audio, audio.getCodec());
			releaseConnection();
		}
		else
		{
			error("play-failed");
			setRunning();
			return true;
		}
	}
	return false;
}

bool Session::enterRecord(Event *event)
{
	if(event->id == ENTER_STATE)
	{
		update_pos = true;
		audio.record(state.audio.list[0], state.audio.mode, state.audio.note);
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
		if(lockConnection())
		{
			conn->channel->attachReceiver(&audio, audio.getCodec(), state.audio.total);
			releaseConnection();
		}
		else
		{
			error("record-failed");
			setRunning();
			return true;
		}
	}
	return false;
}

bool Session::peerAudio(Encoded encoded)
{
	struct _frame *f;
	unsigned size = 20 * 16;
	enter();
	if(!peer)
	{
		leave();
		return false;
	}
	
	f = (struct _frame *)new char[sizeof(struct _frame) + size];
	f->next = NULL;
	if(frame_last)
	{
		frame_last->next = f;
		frame_last = f;
	}
	else
		frame_last = frame_first = f;

	memcpy(f->data, encoded, size);
	leave();
	return true;
}

void Session::clrAudio(void)
{
	struct _frame *frame, *next;
        if(!lockConnection())
                goto skip;

        if(!conn->channel)
        {
                releaseConnection();
                goto skip;
        }

	if(channelType != AUDIO_NONE)
	{
	        conn->channel->stop(channelType);
		channelType = AUDIO_NONE;
	}

        releaseConnection();

skip:
	if(audio.isOpen() && update_pos)
	{
                audio.getPosition(audio.var_position, 12);
                update_pos = false;
	}
	BayonneSession::clrAudio();

	frame = frame_first;
	while(frame)
	{
		next = frame->next;
		delete[] frame;
	}
	frame_first = frame_last = NULL;
}

} // namespace
