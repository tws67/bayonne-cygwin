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

AudioStreamer::AudioStreamer(Span *span, Session *s) :
Thread(), AudioBase(&Driver::sangoma.info), Mutex()
{
	session = s;
	channel = s->getSlot() - span->getFirst() + 1;
	spri = &span->spri;
	source = sink = NULL;
	tone = NULL;
	dtmf = new DTMFDetect();
	ending = false;
}

AudioStreamer::~AudioStreamer()
{
	terminate();
	::close(so);
	if(dtmf)
	{
		delete dtmf;
		dtmf = NULL;
	}
}

void AudioStreamer::start(void)
{
	so = sangoma_create_socket_intr(spri->span, channel);
	sangoma_tdm_set_usr_period(so, &tdm_api, 20);	// 20 millisec for now
	sangoma_tdm_set_codec(so, &tdm_api, WP_SLINEAR);
	sangoma_tdm_flush_bufs(so, &tdm_api);
	mtu = sangoma_tdm_get_usr_mtu_mru(so, &tdm_api);
	Thread::start();
}

void AudioStreamer::setTone(AudioTone *t)
{
	enter();
	tone = t;
	if(t)
		ending = false;
	leave();
}

void AudioStreamer::setSource(AudioBase *get, AudioCodec *i)
{
	Audio::Info ifa;

	enter();
	source = get;
	if(get)
		ending = false;
	inCodec = i;
	if(i)
	{
		ifa = i->getInfo();
		isize = toBytes(ifa, mtu / 2);
	}
	leave();
}

void AudioStreamer::setSink(AudioBase *put, AudioCodec *o)
{
	Audio::Info ofa;

	enter();
	sink = put;
	if(put)
		ending = false;
	outCodec = o;
	if(o)
	{
		ofa = o->getInfo();
		osize = toBytes(ofa, mtu / 2);
	}
	leave();
}

void AudioStreamer::peerAudio(Encoded encoded)
{
}

void AudioStreamer::run(void)
{
	fd_set readfds;
	int inlen, outlen;
	Encoded tbuf;
	Event event;
	bool digits = false;
	char dbuf[65];
	int dig;

	for(;;)
	{
		Thread::yield();

		if(digits)
		{
                        dtmf->getResult(dbuf, 64);
                        dig = -1;
                        if(dbuf[0])
                                dig = Bayonne::getDigit(dbuf[0]);
                        if(dig > -1)
                        {
                                memset(&event, 0, sizeof(event));
                                event.id = DTMF_KEYUP;
                                event.dtmf.digit = dig;
                                event.dtmf.duration = 60;
                                session->queEvent(&event);
                        }
                        digits = false;
                }	

		FD_ZERO(&readfds);
		FD_SET(so, &readfds);

		if(!select(so + 1, &readfds, NULL, NULL, NULL))
			Thread::sync();

		inlen = sangoma_readmsg_socket(so, &hdrframe, sizeof(hdrframe), inframe, SANGOMA_MAX_BYTES, 0);
		if(dtmf && inlen > 0)
			digits = dtmf->putSamples((Linear)inframe, mtu / 2);

		enter();
		if(sink && outCodec)
		{
			outCodec->encode((Linear)inframe, inencoded, mtu / 2);
			sink->putBuffer(inencoded, osize);
		}
		else if(sink)
			sink->putBuffer(inframe, inlen);
		leave();
		
		memset(outframe, 0, mtu);
		outlen = 0;
		tbuf = NULL;

		enter();
		if(source && inCodec)
		{
			if(source->getBuffer(outencoded, isize) >= isize)
			{
				inCodec->decode((Linear)outframe, outencoded, mtu / 2); 
				outlen = mtu;
			}
			else
				outlen = 0;
		}
		else if(source)
			outlen = source->getBuffer(outframe, mtu);
		else if(tone)
			tbuf = (Encoded)tone->getFrame();
		leave();

		if(tbuf)
			sangoma_sendmsg_socket(so, &hdrframe, sizeof(hdrframe), tbuf, mtu, 0);
		else
			sangoma_sendmsg_socket(so, &hdrframe, sizeof(hdrframe), outframe, mtu, 0);

		if(ending)
			continue;

		if((tone && !tbuf) || (source && outlen < mtu))
		{
			ending = true;
			memset(&event, 0, sizeof(event));
			event.id = AUDIO_IDLE;
			session->queEvent(&event);
		}
	}
}	

ssize_t AudioStreamer::getBuffer(Encoded data, size_t len)
{
	return 0;
}

ssize_t AudioStreamer::putBuffer(Encoded data, size_t len)
{
	return 0;
}

} // namespace
