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

Session::Session(timeslot_t ts, unsigned sc) :
BayonneSession(&Driver::soundcard, ts),
TimerPort()
{
	ad = NULL;
	dev = sc;
#ifndef	WIN32
	if(getppid() > 1)
		logevents = &cout;
#else
	logevents = &cout;
#endif
#ifdef	COMMON_DEADLOCK_DEBUG
	nameMutex(logname);
#endif
}

Session::~Session()
{
}

void Session::clrAudio(void)
{
	BayonneSession::clrAudio();
	if(ad)
	{
		delete ad;
		ad = NULL;
	}
}

timeout_t Session::getRemaining(void)
{
	return TimerPort::getTimer();
}

timeout_t Session::getToneFraming(void)
{
	return 10;
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

bool Session::enterIdle(Event *event)
{
	if(event->id == NULL_EVENT)
		event->id = START_SCRIPT;

	return false;
}

bool Session::enterTone(Event *event)
{
	if(event->id == ENTER_STATE && audio.tone)
	{
		thread = new ToneThread(this, Driver::soundcard.device);
		thread->start();
	}
	return false;
}

bool Session::enterPlay(Event *event)
{
	if(event->id == ENTER_STATE)
	{
		thread = new PlayThread(this, Driver::soundcard.device);
		thread->start();
	}
	return false;
}

bool Session::enterRecord(Event *event)
{
        if(event->id == ENTER_STATE)
        {
                thread = new RecordThread(this, Driver::soundcard.device);
                thread->start();
        }
        return false;
}

void Session::makeIdle(void)
{
	if(thread)
	{
		delete thread;
		thread = NULL;
	}
	BayonneSession::makeIdle();
}

} // namespace
