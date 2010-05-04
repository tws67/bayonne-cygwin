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

namespace vpbdriver {
using namespace ost;
using namespace std;

Session::Session(timeslot_t ts, int dev) :
BayonneSession(&Driver::voicetronix, ts)
{
	VPB_DETECT reorder;
	const char *cp = driver->getLast("reorder");
	handle = dev;
        char cbuf[65];
        char *tok;
        unsigned f1, f2;

	vpb_timer_open(&timer, handle, ts, 0);
	inTimer = false;
	logevents = &cerr;
	iface = IF_PSTN;
	bridge = BR_SOFT;

        if(cp)
        {
                reorder.stran[0].type = reorder.stran[2].type = 
VPB_RISING;
                reorder.stran[1].type = VPB_FALLING;
		reorder.stran[0].tfire = reorder.stran[1].tfire =  reorder.stran[2].tfire = 0;
                reorder.stran[0].tmin = reorder.stran[0].tmax = 0;
                reorder.stran[1].tmin = reorder.stran[2].tmin = 220;
                reorder.stran[1].tmax = reorder.stran[2].tmax = 280;

                reorder.nstates = 3;
                reorder.tone_id = VPB_GRUNT;

                strcpy(cbuf, cp);
                cp = strtok_r(cbuf, " \t\r\n,;:", &tok);
                f1 = atoi(cp);
                cp = strtok_r(NULL, " \t\r\n,;:", &tok);
                if(cp)
                        f2 = atoi(cp);
                else
                        f2 = 0;

                reorder.glitch = 40;
                reorder.snr = 10;

                reorder.freq1 = f1;
                reorder.bandwidth1 = 100;
                reorder.minlevel1 = -20;
                if(f2)
                {
                        reorder.ntones = 2;
                        reorder.twist = 10;
                        reorder.freq2 = f2;
                        reorder.bandwidth2 = 100;
                        reorder.minlevel2 = -20;
                }
                else
                {
                        reorder.ntones = 1;
                        reorder.twist = 0;
                        reorder.freq2 = 0;
                        reorder.bandwidth2 = 0;
                        reorder.minlevel2 = 0;
                }
                vpb_settonedet(handle, &reorder);
        }

        vpb_play_get_gain(handle, &outgain);
        vpb_record_get_gain(handle, &inpgain);

	setOffhook(false);
}

Session::~Session()
{
	if(handle < 0)
		return;

	stopTimer();
	if(timer)
		vpb_timer_close(timer);

        vpb_sethook_sync(handle, VPB_ONHOOK);
        vpb_close(handle);

	timer = NULL;
	handle = -1;
}

timeout_t Session::audioFraming(void)
{
	return 20;
}

const char *Session::audioEncoding(void)
{
	return Audio::getName(Audio::mulawAudio);
}

bool Session::peerAudio(Audio::Encoded encoded)
{
	enter();
	if(!join || !peer)
	{
		leave();
		return false;
	}

	join->peerAudio(encoded);
	leave();
	return true;
}

void Session::stopTimer(void)
{
	if(inTimer)
                vpb_timer_stop(timer);

        inTimer = false;
}

void Session::startTimer(timeout_t timeout)
{

        if(inTimer)
                vpb_timer_stop(timer);

        vpb_timer_change_period(timer, timeout);
        vpb_timer_start(timer);
        inTimer = true;
}

timeout_t Session::getRemaining(void)
{
	if(inTimer)
		return 0;

	return TIMEOUT_INF;
}

void Session::queEvent(Event *event)
{
	VPB_EVENT e;

	switch(event->id)
	{
	case EXIT_LIBEXEC:
	case EXIT_THREAD:
		e.data = event->id;
		e.type = VPB_DIALEND;
		e.handle = handle;
		vpb_put_event(&e);
	default:
		break;
	};
}

void Session::setOffhook(bool flag)
{
	if(flag == offhook)
		return;

	if(flag)
		vpb_sethook_async(handle, VPB_OFFHOOK);
	else
		vpb_sethook_async(handle, VPB_ONHOOK);

	offhook = flag;	
}

bool Session::enterRecord(Event *event)
{
	if(event->id == ENTER_STATE)
	{
		thread = new RecordThread(this);
		thread->start();
	}
	return false;
}

bool Session::enterPlay(Event *event)
{
	if(event->id == ENTER_STATE)
	{
		thread = new PlayThread(this);
		thread->start();
	}
	return false;
}

bool Session::enterTone(Event *event)
{
	if(event->id == ENTER_STATE && audio.tone)
	{
		thread = new ToneThread(this);
		thread->start();
	}
	return false;
}

bool Session::enterJoin(Event *event)
{
	if(event->id == ENTER_STATE)
	{
		thread = new JoinThread(this);
		thread->start();
	}
	return false;
}

bool Session::enterDial(Event *event)
{
        VPB_CALL cpp;
        VPB_DETECT toned;

	switch(event->id)
	{
	case ENTER_STATE:
		disableDTMF();
		
		if(state.join.answer_timer && state.join.answer_timer != TIMEOUT_INF)
		{
			// reset dialtone detect
                        vpb_gettonedet(handle, VPB_DIAL, &toned);
                        vpb_settonedet(handle, &toned);		
			
                        vpb_get_call(handle, &cpp);
                        cpp.ringback_timeout = getMSecTimeout("noringback");
			cpp.inter_ringback_timeout = getMSecTimeout("cpringback");
			cpp.answer_timeout = state.join.answer_timer;
			vpb_set_call(handle, &cpp);
			vpb_call_async(handle, (char *)state.join.dial);
		}
		else
			vpb_dial_async(handle, (char *)state.join.dial);
		return true;
	default:
		return false;
	}
}

bool Session::enterRinging(Event *event)
{
	switch(event->id)
	{
	case RING_OFF:
		if(ring_count || driver->getAnswerCount() < 2)
			return false;

		thread = new Callerid(this, handle);
		thread->start();
	default:
		return false;
	}
}

const char *Session::checkAudio(bool live)
{
	audio.libext = ".au";

	if(audio.encoding == Audio::unknownEncoding)
		audio.encoding = Audio::mulawAudio;

	if(!live)
	{
		if(!audio.framing)
			audio.framing = 10;
		return NULL;
	}

	switch(audio.encoding)
	{
	case Audio::mulawAudio:
	case Audio::alawAudio:
	case Audio::okiADPCM:
	case Audio::g723_3bit:
		audio.framing = 20;
		return NULL;
	default:
		if(Audio::isLinear(audio.encoding))
		{
			audio.framing = 20;
			break;
		}
		if(AudioCodec::load(audio.encoding))
			break;
		return "unsupported audio encoding";
	};
	audio.framing = Audio::getFraming(audio.encoding, audio.framing);
	if(!audio.framing)
		audio.framing = 20;
	return NULL;
}

} // namespace
