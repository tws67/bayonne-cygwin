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

ToneThread::ToneThread(Session *s, unsigned device) :
ScriptThread(s, s->driver->getAudioPriority()),
TimerPort()
{
	session = s;
	tone = s->audio.tone;
	devid = device;
	dev = NULL;
	buffer = NULL;
	resetflag = false;
}

ToneThread::~ToneThread()
{
	bool reset = resetflag;
	resetflag = true;
	terminate();
	if(buffer)
	{
		delete[] buffer;
		buffer = NULL;
	}

	if(dev && !reset)
	{
		session->setDevice(NULL);
		delete dev;
		dev = NULL;
	}
	else if(dev)
	{
		dev->flush();
		dev = NULL;
	}
}

void ToneThread::run(void)
{
	timeout_t timer;
	unsigned jitter = 2;
	timeout_t framing = 10;
	Linear buf;
	unsigned bufcount = framing * 8;

        dev = getDevice(devid);
        session->setDevice(dev);

        if(!hasDevice() && !dev)
        {
		slog.error("%s: audio %d device unsupported", session->logname, devid);
                exit("no-device");
        }

        if(!dev)
        {
		slog.error("%s: audio device %d unavailable", session->logname, devid);
                exit("no-device");
        }

	if(!dev->setAudio((Rate)rate8khz, false, framing))
        {
                slog.error("%s: audio encoding unsupported", session->logname);
                exit("no-encoding");
        }
	
	setTimer(0);
	Thread::yield();
	while(!resetflag)
	{
		buf = tone->getFrame();
		if(!buf)
			break;

		dev->putSamples(buf, bufcount);
                if(jitter)
                        --jitter;
                else
                        incTimer(framing);
                timer = getTimer();
                if(timer > 1 && timer != TIMEOUT_INF)
                        Thread::sleep(timer - 1);
        }
	if(resetflag)
		Thread::sync();

	resetflag = true;
        dev->flush();
        incTimer(framing * 3);
        timer = getTimer();
        if(timer > 1 && timer != TIMEOUT_INF)
                Thread::sleep(timer - 1);
        exit(NULL);
}

PlayThread::PlayThread(Session *s, unsigned device) :
ScriptThread(s, s->driver->getAudioPriority()),
TimerPort()
{
	session = s;
	audio = &s->audio;
	devid = device;
	dev = NULL;
	buffer = NULL;
	resetflag = false;
}

PlayThread::~PlayThread()
{
	bool reset = resetflag;

	resetflag = true;

	terminate();

	if(reset && dev)
	{
		session->setDevice(NULL);
		delete dev;
		dev = NULL;
	}
	else if(dev)
	{
		dev->flush();
		dev = NULL;
	}
	if(audio->isOpen() && session->state.audio.mode != modeReadAny)
		audio->getPosition(audio->var_position, 12);
	audio->cleanup();

	if(buffer)
	{
		delete[] buffer;
		buffer = NULL;
	}
}

void PlayThread::run(void)
{
	unsigned bufcount, pages;
	Info info;
	timeout_t timer;
	unsigned jitter = 2;

	dev = getDevice(devid);
	session->setDevice(dev);
	
	if(!hasDevice() && !dev)
	{
		slog.error("%s: audio %d device unsupported", session->logname, devid);
		exit("no-device");
	}

	if(!dev)
	{
		slog.error("%s: audio device %d unavailable", session->logname, devid);
		exit("no-device");
	}

	audio->play(session->state.audio.list, session->state.audio.mode);
	if(!audio->isOpen())
	{
		slog.error("%s: audio file access error", session->logname);
		exit("no-files");
	}

	if(!audio->isStreamable())
	{
		slog.error("%s: missing codec needed", session->logname);
		exit("no-codec");
	}
	
	audio->getInfo(&info);
	if(!dev->setAudio((Rate)info.rate, isStereo(info.encoding), info.framing))
	{
		slog.error("%s: audio encoding unsupported", session->logname);
		exit("no-encoding");
	}

	Thread::yield();

	bufcount = audio->getCount();
        if(isStereo(info.encoding))
                buffer = new Sample[bufcount * 2];
        else
                buffer = new Sample[bufcount];

	setTimer(0);
	while(!resetflag)
	{
                if(isStereo(info.encoding))
                        pages = audio->getStereo(buffer, 1);
                else
                        pages = audio->getMono(buffer, 1);

                if(!pages)
                        break;

                dev->putSamples(buffer, bufcount);
		if(jitter)
			--jitter;
		else
			incTimer(info.framing);
		timer = getTimer();
		if(timer > 1 && timer != TIMEOUT_INF)
			Thread::sleep(timer - 1);
	}
	if(resetflag)
		Thread::sync();

	resetflag = true;

	dev->flush();
	incTimer(info.framing * 3);
	timer = getTimer();
	if(timer > 1 && timer != TIMEOUT_INF)
		Thread::sleep(timer - 1);
	exit(NULL);
}

RecordThread::RecordThread(Session *s, unsigned device) :
ScriptThread(s, s->driver->getAudioPriority()),
TimerPort()
{
        session = s;
        audio = &s->audio;
        devid = device;
        dev = NULL;
        buffer = NULL;
	resetflag = false;

	if(s->state.audio.total)
		setTimer(s->state.audio.total);
}

RecordThread::~RecordThread()
{
	resetflag = true;

        terminate();

	if(dev && resetflag)
	{
		session->setDevice(NULL);
		delete dev;
		dev = NULL;
	}
        else if(dev)
        {
		dev->flush();
		dev = NULL; 
        }
        if(audio->isOpen())
                audio->getPosition(audio->var_position, 12);
        audio->cleanup();

        if(buffer)
        {
                delete[] buffer;
                buffer = NULL;
        }
}

void RecordThread::run(void)
{
        unsigned bufcount, pages;
        Info info;
	bool silent = true;
	bool compress = session->state.audio.compress;
	bool first = true;
	bool silence = false;
	Level level = session->state.audio.level;
	Level peak;
	Event event;

	dev = getDevice(devid, RECORD);
	session->setDevice(dev);

	if(session->state.audio.silence || compress)
		silence = true;

	if(!silence)
		silent = false;

        if(!hasDevice() && !dev)
        {
		slog.error("%s: audio %d device unsupported", session->logname, devid);
                exit("no-device");
        }

        if(!dev)
        {
		slog.error("%s: audio device %d unavailable", session->logname, devid);
                exit("no-device");
        }

        audio->record(session->state.audio.list[0], session->state.audio.mode, session->state.audio.note);
        if(!audio->isOpen())
        {
                slog.error("%s: audio file access error", session->logname);
                exit("no-files");
        }

        if(!audio->isStreamable())
        {
                slog.error("%s: missing codec needed", session->logname);
                exit("no-codec");
        }

	audio->getInfo(&info);
	if(!level)
		level = session->driver->getAudioLevel();

	if(!dev->setAudio((Rate)info.rate, isStereo(info.encoding), info.framing))
        {
                slog.error("%s: audio encoding unsupported", session->logname);
                exit("no-encoding");
        }

	Thread::yield();
	
	bufcount = audio->getCount();
	buffer = new Sample[bufcount];

	while(getTimer())
	{
		dev->getSamples(buffer, bufcount);

		if(silence)
		{
			peak = getPeak(info, buffer, bufcount);
			if(peak < level && !silent)
			{
				event.id = AUDIO_STOP;
				session->postEvent(&event);
				silent = true;
			}
			else if(peak >= level && silent)
			{
				first = false;
				event.id = AUDIO_START;
				session->postEvent(&event);
				silent = false;
			}
		}
		
		if(silent && first)
			goto skip;

		if(silent && compress)
			goto skip;

		pages = audio->putMono(buffer, 1);
		
		if(!pages)
			exit("file-error");

skip:
		Thread::yield();
	}
	resetflag = true;
	exit(NULL);
}	
	

} // namespace
