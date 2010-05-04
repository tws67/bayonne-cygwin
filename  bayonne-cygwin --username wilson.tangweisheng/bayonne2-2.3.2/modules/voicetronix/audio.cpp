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

JoinThread::JoinThread(Session *s) :
ScriptThread(s, s->driver->getAudioPriority())
{
	session = s;
	handle = s->getHandle();
	reset = true;
	buffer = NULL;

        vpb_play_get_gain(handle, &gain); 
        vpb_set_codec_reg(handle, 0x32, 197); 
}

JoinThread::~JoinThread()
{
	session->join = NULL;

	if(!reset) 
        {
                reset = true;
                vpb_play_terminate(handle);
		vpb_record_terminate(handle);
                Thread::yield();              
        }
        terminate();  
	if(buffer)
		delete[] buffer;
	buffer = NULL;
        vpb_play_set_gain(handle, gain); 
}

void JoinThread::peerAudio(Encoded encoded)
{
	if(!reset)
		vpb_play_buf_sync(handle, (char *)encoded, bufsize);
}

void JoinThread::run(void)
{
	BayonneSession *peer;
	BayonneSession *parent = Bayonne::getSid(session->var_pid);
	const char *errmsg = NULL;
	bool exiting;

	Audio::Encoding encoding = 
		Bayonne::getEncoding(Driver::voicetronix.getLast("encoding"));
	timeout_t framing = 
		atol(Driver::voicetronix.getLast("framing"));

	if(parent)
	{
		encoding = Bayonne::getEncoding(parent->audioEncoding());
		framing = parent->audioFraming();
	}
			
	switch(encoding)
	{
	case mulawAudio:
		vpb_play_buf_start(handle, VPB_MULAW);
		vpb_record_buf_start(handle, VPB_MULAW);
		bufcount = bufsize = framing * 8;
		break;
	case alawAudio:
                vpb_play_buf_start(handle, VPB_ALAW);
                vpb_record_buf_start(handle, VPB_ALAW); 
                bufcount = bufsize = framing * 8;
                break; 
	case pcm16Mono:
                vpb_play_buf_start(handle, VPB_LINEAR); 
                vpb_record_buf_start(handle, VPB_LINEAR);  
                bufsize = framing * 16;
		bufcount = framing * 8; 
		break;
	case okiADPCM:
		vpb_play_buf_start(handle, VPB_OKIADPCM);
		vpb_record_buf_start(handle, VPB_OKIADPCM);
		bufsize = framing * 4;
		bufcount = framing * 8;
	default:
		break;
	}

	reset = false;
	buffer = new unsigned char[bufsize];
        vpb_play_set_gain(handle, Driver::voicetronix.gain);  

	Thread::yield();
	session->join = this;

	while(!reset)
	{
		Thread::yield();
		if(vpb_record_buf_sync(handle, (char *)buffer, bufsize) != VPB_OK)                              
		{    
                        errmsg = "record-failed";
                        break;
                }  
		session->enter();
		peer = session->peer;
		session->leave();
		if(peer)
			peer->peerAudio(buffer);
	}

	exiting = reset;
	reset = true;

        Thread::yield();
        session->join = NULL;
                                
	vpb_play_buf_finish(handle);
	vpb_record_buf_finish(handle);
	if(exiting)
		Thread::sync();

	exit(errmsg);
}

ToneThread::ToneThread(Session *s) :
ScriptThread(s, s->driver->getAudioPriority())
{
	session = s;
	handle = s->getHandle();
	tone = s->audio.tone;
	reset = true;
}

ToneThread::~ToneThread()
{
	if(!reset)
	{
		reset = true;
		vpb_play_terminate(handle);
		Thread::yield();
	}
	terminate();
}

void ToneThread::run(void)
{
	Linear buf;
	Event event;
	bool exiting;

	vpb_play_buf_start(handle, VPB_LINEAR);
	reset = false;

	while(!reset)
	{
		buf = tone->getFrame();
		if(buf)
		{
			vpb_play_buf_sync(handle, (char *)buf, 320);
			Thread::yield();
			continue;
		}

		if(tone->isComplete())
			break;
		
		memset(&event, 0, sizeof(event));
		event.id = START_FLASH;
		session->postEvent(&event);
		Thread::sleep(session->driver->getFlashTimer());
		memset(&event, 0, sizeof(event));
		event.id = STOP_FLASH;
		session->postEvent(&event);
		Thread::sleep(1000 - session->driver->getFlashTimer());
	}
	exiting = reset;
	reset = true;
	vpb_play_buf_finish(handle);
	if(exiting)
		Thread::sync();
	exit(NULL);
}
	
PlayThread::PlayThread(Session *s) :
ScriptThread(s, s->driver->getAudioPriority())
{
	session = s;
	handle = s->getHandle();
	audio = &s->audio;
	reset = true;
	lbuffer = NULL;
	buffer = NULL;

	vpb_play_get_gain(handle, &gain);
}

PlayThread::~PlayThread()
{
	if(!reset)
	{
		reset = true;
		vpb_play_terminate(handle);
		Thread::yield();
	}
	terminate();
        if(audio->isOpen() && session->state.audio.mode != modeReadAny)
                audio->getPosition(audio->var_position, 12);
        audio->cleanup();
	if(lbuffer)
		delete[] lbuffer;
	if(buffer)
		delete[] buffer;
	lbuffer = NULL;
	buffer = NULL;

	vpb_play_set_gain(handle, gain);
}

void PlayThread::run(void)
{
	audio->play(session->state.audio.list, session->state.audio.mode);
        if(!audio->isOpen())   
        {         
                slog.error("%s: audio file access error", session->logname);
                exit("no-files");
        }

	switch(audio->getEncoding())
	{
	case mulawAudio:
		vpb_play_buf_start(handle, VPB_MULAW);
		bufsize = 160;
		bufcount = 160;
		playDirect();
	case alawAudio:
		vpb_play_buf_start(handle, VPB_ALAW);
		bufsize = 160;
		bufcount = 160;
		playDirect();
	case okiADPCM:
		vpb_play_buf_start(handle, VPB_OKIADPCM);
		bufsize = 80;
		bufcount = 160;
		playDirect();
	default:
		playConvert();
	}
}

void PlayThread::playDirect(void)
{
	bool exiting;
	const char *errmsg = NULL;

	reset = false;
	buffer = new unsigned char[bufsize];
	size_t count;

	vpb_play_set_gain(handle, Driver::voicetronix.gain);

	while(!reset)
	{
		Thread::yield();
		count = audio->getBuffer(buffer, bufsize);
		if(count < 0)
			errmsg = "read-failed";

		if(count < bufcount)
			break;

		if(vpb_play_buf_sync(handle, (char *)buffer, count) != VPB_OK)
		{
			errmsg = "play-failed";
			break;
		}		
	}

	exiting = reset;
	reset = true;
	vpb_play_buf_finish(handle);
	if(exiting)
		Thread::sync();

	exit(errmsg);
}

void PlayThread::playConvert(void)
{
	unsigned pages;
	const char *errmsg = NULL;
	bool exiting = false;

	if(!audio->isStreamable())
	{
	        slog.error("%s: missing codec needed", session->logname);
                exit("no-codec");
        }

	bufcount = audio->getCount();
	bufsize = bufcount * 2;
	lbuffer = new Sample[bufcount];

	vpb_play_buf_start(handle, VPB_LINEAR);
        reset = false;
	vpb_play_set_gain(handle, Driver::voicetronix.gain);

	while(!reset)
	{
		Thread::yield();
		pages = audio->getMono(lbuffer, 1);
		if(!pages)
			break;

		if(vpb_play_buf_sync(handle, (char *)lbuffer, bufsize) != VPB_OK)
		{
			errmsg = "play-failed";
			break;
		}

	}

	exiting = reset;
	reset = true;
	vpb_play_buf_finish(handle);
	if(exiting)
		Thread::sync();
	exit(errmsg);
}

RecordThread::RecordThread(Session *s) :
ScriptThread(s, s->driver->getAudioPriority())
{
	session = s;
	handle = s->getHandle();
	audio = &s->audio;
	reset = true;
	lbuffer = NULL;
	buffer = NULL;
	duration = session->state.audio.total;
}

RecordThread::~RecordThread()
{
	if(!reset)
	{
		reset = true;
		vpb_record_terminate(handle);
		Thread::yield();
	}
	terminate();
        if(audio->isOpen())
                audio->getPosition(audio->var_position, 12);
        audio->cleanup();
	if(buffer)
		delete[] buffer;
	if(lbuffer)
		delete[] lbuffer;
	buffer = NULL;
	lbuffer = NULL;
}

void RecordThread::run(void)
{
	audio->record(session->state.audio.list[0], session->state.audio.mode, session->state.audio.note);
        if(!audio->isOpen())
        {
                slog.error("%s: record file access error", session->logname);
                exit("no-files");
        }
	switch(audio->getEncoding())
	{
	case mulawAudio:
		if(session->state.audio.compress || session->state.audio.silence)
			goto convert;
                vpb_record_buf_start(handle, VPB_MULAW);
                bufsize = 160;
                bufcount = 160;
                recordDirect();
        case alawAudio: 
                if(session->state.audio.compress || session->state.audio.silence)
                        goto convert;
                vpb_record_buf_start(handle, VPB_ALAW);
                bufsize = 160;
                bufcount = 160;
                recordDirect();
        case okiADPCM:
                vpb_record_buf_start(handle, VPB_OKIADPCM);
                bufsize = 80;
                bufcount = 160;
                recordDirect();
	default:
convert:
		recordConvert();
	}
}

void RecordThread::recordConvert(void)
{
	Event event;
	bool exiting;
	const char *errmsg = NULL;
	unsigned long frames;
	bool silent = true;
        bool compress = session->state.audio.compress;
        bool first = true;
        bool silence = false;
        Level level = session->state.audio.level;
        Level peak;
	Info info;
	unsigned pages;

        if(!audio->isStreamable())
        {
                slog.error("%s: missing codec needed", session->logname);
                exit("no-codec");
        }

	audio->getInfo(&info);

        if(session->state.audio.silence || compress)
                silence = true;

        if(!silence)
                silent = false;

        bufcount = audio->getCount();
        bufsize = bufcount * 2;
	frames = duration / (bufcount / 8);
        lbuffer = new Sample[bufcount];

	vpb_record_buf_start(handle, VPB_LINEAR);
	reset = false;
	
	while(!reset && frames--)
	{
		Thread::yield();

		if(vpb_record_buf_sync(handle, (char *)lbuffer, bufsize) != VPB_OK)
		{
			errmsg = "record-failed";
			break;
		}

                if(silence)
                {
                        peak = getPeak(info, lbuffer, bufcount);
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
                        continue;

                if(silent && compress)
                        continue;

                pages = audio->putMono(lbuffer, 1);

                if(!pages)
                {
			errmsg = "write-error";
			break;
		}
	}

        exiting = reset;
        reset = true;
        vpb_play_buf_finish(handle);
        if(exiting)
                Thread::sync();
        exit(errmsg);
}

void RecordThread::recordDirect(void)
{
	Event event;
	bool exiting;
	const char *errmsg = NULL;
        unsigned long frames = duration / (bufcount / 8);

	reset = false;
	buffer = new unsigned char[bufsize];

	memset(&event, 0, sizeof(event));
	event.id = AUDIO_START;
	session->postEvent(&event);

	while(!reset && frames--)
	{
		if(vpb_record_buf_sync(handle, (char *)buffer, bufsize) != VPB_OK)
		{
			errmsg = "record-failed";
			break;
		}
		if(audio->putBuffer(buffer, bufsize) < (int)bufsize)
		{
			errmsg = "write-failed";
			break;
		}
		Thread::yield();
	}

	exiting = reset;
	reset = true;
	vpb_record_buf_finish(handle);
	if(exiting)
		Thread::sync();

	exit(errmsg);
}

} // namespace
