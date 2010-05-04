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

namespace sipdriver {
using namespace ost;
using namespace std;

RTPStream::RTPStream(Session *s) :
SymmetricRTPSession(s->getLocalAddress(), s->getLocalPort()), 
AudioBase(&s->info), 
TimerPort(), Mutex()
{
        session = s;
	dtmf = NULL;
	silent_frame = NULL;
	silent_encoded = NULL;
	pbuffer = NULL;
	buffer = NULL;
	lbuffer = NULL;

	newSession();
}

void RTPStream::newSession(void)
{
	endSession();

	setTimer(0);
	memcpy(&info, &session->info, sizeof(info));

        dropInbound = false;
        oTimestamp = 0;
	pTimestamp = 0;
	source = NULL;
	sink = NULL;
	tone = NULL;
	lastevt = 256;
	dtmfcount = 0;
	stopid = MSGPORT_WAKEUP;
	dtmf = NULL;
	fcount = 0;
	silent_frame = new Sample[info.framecount];
	memset(silent_frame, 0, info.framecount * 2);
	silent_encoded = new unsigned char[info.framesize];
        if(info.encoding == Audio::pcm16Mono)
	{
		info.order = __BIG_ENDIAN;
                memset(silent_encoded, 0, info.framesize);
	}
        else if(session->codec)
                session->codec->encode(silent_frame, silent_encoded, info.framecount);

	pbuffer = new unsigned char[info.framesize];
	buffer = new unsigned char[info.framesize];
	lbuffer = new Sample[info.framecount];
}

RTPStream::~RTPStream()
{
	terminate();
	endSession();
	endSocket();
}

void RTPStream::endSession(void)
{
	setSource(NULL);
	setSink(NULL);
	setTone(NULL);

	if(silent_frame)
		delete[] silent_frame;
	if(silent_encoded)
		delete[] silent_encoded;
	if(lbuffer)
		delete[] lbuffer;
	if(buffer)
		delete[] buffer;
	if(pbuffer)
		delete[] pbuffer;
	silent_frame = NULL;
	silent_encoded = NULL;
	lbuffer = NULL;
	buffer = NULL;
	pbuffer = NULL;
	if(dtmf)
	{
		delete dtmf;
		dtmf = NULL;
	}
	endQueue();
}

void RTPStream::start(void)
{
	DynamicPayloadFormat pf(session->data_payload, info.rate);
	
	setPayloadFormat(pf);	
	setMaxSendSegmentSize(1024);

	pTimestamp = 0;
	source = NULL;
	sink = NULL;
	tonecount = 0;

        SymmetricRTPSession::startRunning();
}

void RTPStream::set2833(struct dtmf2833 *data, timeout_t duration)
{
	enter();
	jsend = Driver::sip.jitter;
	oTimestamp = 0;
	dtmfcount = (duration + (info.framing - 1)) / info.framing;
	memcpy(&dtmfpacket, data, sizeof(dtmfpacket));
	dtmfpacket.ebit = 0;
	dtmfpacket.rbit = 0;
	dtmfpacket.vol = 12;
	dtmfpacket.duration = 0;
	leave();
}	

void RTPStream::setTone(AudioTone *t, timeout_t max)
{
	enter();
	jsend = Driver::sip.jitter;
	tone = t;
	fcount = max / info.framing;
	oTimestamp = 0;
	leave();
}

void RTPStream::setSource(AudioBase *get, timeout_t max)
{
        enter();
	jsend = Driver::sip.jitter + 1;
	ending = false;
        source = get;
	fcount = max / info.framing;
	oTimestamp = 0;
        leave();
}

void RTPStream::setSink(AudioBase *put, timeout_t max)
{
        enter();
        sink = put;
	fcount = max / info.framing;
        leave();
}

bool RTPStream::onRTPPacketRecv(IncomingRTPPkt &pkt)
{
	Event event;
	struct dtmf2833 *evt;
	unsigned pid = session->dtmf_payload;
	AudioCodec *codec;
	Level level;

        // this is where we test for 2833!!!

	if(!pid || pid != pkt.getPayloadType())
		goto check2;

	session->dtmf_inband = false;
	evt = (struct dtmf2833 *)pkt.getPayload();

	if(evt->event != lastevt)
		goto post;

	if(tonecount)
		goto checkbit;

	if(evt->ebit)
		goto checkbit;

post:
	lastevt = evt->event;

	memset(&event, 0, sizeof(event));
	switch(evt->event)
	{
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
        case 11:
        case 12:
        case 13:
        case 14:
        case 15:
		event.id = DTMF_KEYUP;
		event.dtmf.digit = evt->event;
		event.dtmf.duration = 60;
		break;
	case 16:
		event.id = LINE_PICKUP;
		break;
	case 64:
		event.id = LINE_OFF_HOOK;
		break;
	case 65:
		event.id = LINE_ON_HOOK;
		break;
	case 66:
	case 67:
	case 68:
	case 69:
		event.id = TONE_START;
		event.tone.name = "dialtone";
		break;
	case 70:
	case 71:
		event.id = TONE_START;
		event.tone.name = "ringback";
		break;
	case 72:
		event.id = TONE_START;
		event.tone.name = "busytone";
		break;
	case 73:
		event.id = TONE_START;
		event.tone.name = "reorder";
		break;
	case 74:
		event.id = TONE_START;
		event.tone.name = "intercept";
		break;
	case 76:
		event.id = CALL_HOLD;
		break;
	case 78:
	case 79:
		event.id = TONE_START;
		event.tone.name = "waiting";
		break;
	case 160:
	case 161:
		event.id = LINE_WINK;
		break;
	default:
		return false;
	}				

	session->queEvent(&event);

checkbit:
	if(evt->ebit && tonecount && stopid != MSGPORT_WAKEUP)
	{
		memset(&event, 0, sizeof(event));
		event.id = stopid;
		stopid = MSGPORT_WAKEUP;
		session->queEvent(&event);
		tonecount = 0;
	}
	else if(evt->ebit)
		tonecount = 0;
	else
		tonecount = 3;
	return false;

check2:
	lastevt = 256;

	if(pkt.getPayloadType() != session->data_payload)
		return false;

	if(sink || session->peer)
		goto silence;

        if(session->dtmf_inband)
		goto silence;

	return false;

silence:
	level = Driver::sip.silence;
	codec = session->codec;
	if(!level || !codec)
		return true;

	// bug...
	if(session->info.encoding == gsmVoice)
		return true;

	if(codec->isSilent(level, (void *)pkt.getRawPacket(), session->info.framecount))
		return false;

	return true;
}

void RTPStream::postAudio(Encoded encoded)
{
	register BayonneSession *peer;

	peer = session->peer;
	if(!peer || !session->isJoined())
		return;

	if(peer->peerLinear())
	{
		session->codec->decode(lbuffer, encoded, info.framecount);
		peer->peerAudio((Encoded)lbuffer);
	}
	else
		peer->peerAudio(encoded);
}

void RTPStream::syncAudio(void)
{
	oTimestamp = 0;
}

void RTPStream::peerAudio(Encoded encoded, bool linear)
{
	jsend = Driver::sip.jitter;

	if(tone || dtmfcount)
		return;

	if(!encoded)
	{
		oTimestamp = 0;
		return;
	}

	if(linear && session->codec)
	{
		session->codec->encode((Linear)encoded, pbuffer, info.framecount);
		putNative(pbuffer, info.framesize);
	}
	else
		putNative(encoded, info.framesize);
}

void RTPStream::run(void)
{
	for(;;)
	{
		if(session->rtp_flag)
			runActive();
		else
			Thread::sleep(50);
	}
}

void RTPStream::runActive(void)
{
        size_t r, n;
        size_t size = info.framesize;
        timeout_t timer;
	bool first = true;
	Event event;
	unsigned count;
	AudioCodec *codec = session->codec;
	bool dtmfdigits = false;
	char dbuf[65];
	int dig;
	Linear tbuf;
	bool silent;
	DynamicPayloadFormat pf_data(session->data_payload, info.rate);
	DynamicPayloadFormat pf_dtmf(session->dtmf_payload, info.rate);
	unsigned short pps = (unsigned short)(1000 / info.framing);
	unsigned short dtmfoffset = (unsigned short)(info.rate / pps);
	BayonneSession *joined;
	Registry *reg = NULL;
	unsigned long seq = 0;
//	unsigned ecount = 0;
	time_t now;
	const char *cp;

	cp = session->getSymbol("session.registry");
	if(cp)
		reg = (Registry *)((session->getImage())->getPointer(cp));

	cp = session->getSymbol("session.sequence");
	if(cp)
		seq = atol(cp);

	if(reg)
		slog.debug("%s: going active; associated with %s",
			session->getLogname(), reg->userid);
	else
		slog.debug("%s: going active", session->getLogname());

	memset(&event, 0, sizeof(event));
	event.id = STREAM_ACTIVE;
	session->queEvent(&event);

	jitter = Driver::sip.jitter;
	jsend = Driver::sip.jitter;

	if(session->dtmf_inband)
		dtmf = new DTMFDetect();

        setTimer(0);

	interval = info.framing;
	interval *= (getCurrentRTPClockRate() / 1000);

	setCancel(cancelDeferred);
        for(;;)
        {
		if(jsend && Driver::sip.send_immediate)
			jsend = 0;			

		if(!session->rtp_flag)
		{
			slog.debug("%s: going passive", session->getLogname());
			memset(&event, 0, sizeof(event));
			event.id = STREAM_PASSIVE;
			session->queEvent(&event);
			return;
		}

		silent = true;
		if(dtmfdigits)
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
			dtmfdigits = false;
		}

                incTimer(info.framing);
		if(reg)
		{
			if(!reg->isActive() || reg->traffic->sequence != seq)
			{
				slog.debug("%s: registration %s lost",
					session->getLogname(), reg->userid);

				memset(&event, 0, sizeof(event));
				event.id = STOP_DISCONNECT;
				session->queEvent(&event);
				reg = NULL;
			}
		}

		if(session->session_timer)
		{
			time(&now);
			if(session->session_timer < now)
			{
				slog.debug("%s: sessione expired",
					session->getLogname());
				memset(&event, 0, sizeof(event));
				event.id = STOP_DISCONNECT;
				session->queEvent(&event);
				session->session_timer = 0;
			}
		}

                enter();

		// inband detector disabled if dtmf supported elsewhere

		if(dtmf && !session->dtmf_inband)
		{
			delete dtmf;
			dtmf = NULL;
			dtmfdigits = false;
		}

		if(dtmfcount)
		{
			--dtmfcount;
			if(!dtmfcount)
				dtmfpacket.ebit = 1;
			setPayloadFormat(pf_dtmf);
			put2833(&dtmfpacket);
			pps = ntohs(dtmfpacket.duration) + dtmfoffset;
			dtmfpacket.duration = htons(pps);
			if(!dtmfcount)
				setPayloadFormat(pf_data);
			if(jsend)
			{
				--jsend;
				leave();
				dispatchDataPacket();
				continue;
			}
		}
		else if(tone)
		{
			tbuf = tone->getFrame();
			if(!tbuf || !codec)
			{
				fcount = 0;
				tone = NULL;
				memset(&event, 0, sizeof(event));
				event.id = AUDIO_IDLE;
				session->queEvent(&event);
				goto done;
			}
			if(codec)
			{
				codec->encode(tbuf, buffer, info.framecount);
				putNative(buffer, size);
				silent = false;
			}				
			if(jsend)
			{
				--jsend;
				leave();
				dispatchDataPacket();
				continue;
			}
			
		}
                else if(source && !session->holding)
                {
                        n = source->getBuffer(buffer, size);
			if(n == size)
			{
				if(ending)
				{
					memset(&event, 0, sizeof(event));
					event.id = AUDIO_ACTIVE;
					session->queEvent(&event);
					ending = false;
				}
				silent = false;
	                        putNative(buffer, size);
			}

			else if(!ending)
			{
				fcount = 0;
				memset(&event, 0, sizeof(event));
				event.id = AUDIO_IDLE;
				session->queEvent(&event);
				ending = true;
			}
			if(jsend && !silent)
			{
				--jsend;
				leave();
				dispatchDataPacket();
				continue;
			}
                }
		else if(!session->peer)
			jsend = Driver::sip.jitter;

done:
		leave();
		if(silent && Driver::sip.data_filler)
		{
			putNative(silent_encoded, info.framesize);
			silent = false;
		}

		r = dispatchDataPacket();

		if(tonecount == 1 && stopid != MSGPORT_WAKEUP)
		{
			memset(&event, 0, sizeof(event));
			event.id = stopid;
			stopid = MSGPORT_WAKEUP;
			session->postEvent(&event);
		}			

		if(tonecount)
			--tonecount;

		if(first)
		{
			while(isPendingData(0))
			{
//				ecount = 1;
        	                r = takeInDataPacket();
                	        if(r < 0)
                        	        break;

				Thread::yield();
				jitter = Driver::sip.jitter;
				if(session->isJoined())
					jitter = 1;

				if(first)
				{
					memset(&event, 0, sizeof(event));
					event.id = AUDIO_START;
					session->queEvent(&event);
					count = 6;
					while(dtmf && count-- && session->dtmf)
						dtmf->putSamples(silent_frame, info.framecount);
				}

				first = false;

	                        enter();
        	                n = getNative(buffer, r);
				if(dtmf && codec && n == size && session->dtmf)
				{
					codec->decode(lbuffer, buffer, info.framecount);
					Thread::yield();
					dtmfdigits = dtmf->putSamples(lbuffer, info.framecount) > 0;					
				}
                	        if(sink)
                        	        sink->putBuffer(buffer, n);
				else
					postAudio(buffer);
                        	leave();
				Thread::yield();
	                }
		}
		else if(!jitter)
		{
			if(isPendingData(0))
			{
//				ecount = 1;
				r = takeInDataPacket();
				if(r < 0)
					break;

				Thread::yield();
				enter();
				n = getNative(buffer, r);
				if(dtmf && codec && n == size && session->dtmf)
				{
					codec->decode(lbuffer, buffer, info.framecount);
					Thread::yield();
					dtmfdigits = dtmf->putSamples(lbuffer, info.framecount) > 0;
				}
				if(sink)
					sink->putBuffer(buffer, n);
				else 
					postAudio(buffer);
				leave();
				Thread::yield();
			}
//			else if(ecount)
//				--ecount;
			else
			{
				memset(&event, 0, sizeof(event));
				event.id = AUDIO_STOP;
				session->queEvent(&event);
				joined = session->peer;
				memset(&event, 0, sizeof(event));
				event.id = AUDIO_SYNC;
				if(joined && session->isJoined())
					joined->queEvent(&event);
				first = true;
			}
		}
		else if(!first)
			--jitter;

                timer = getTimer();

		if(timer > info.framing + 5)
			timer = info.framing + 5;
		if(timer > 5)
                        Thread::sleep(timer - 5);
		else
			Thread::yield();

		enter();
		if(fcount > 1)
			--fcount;
		else if(fcount == 1)
		{
			fcount = 0;
			source = NULL;
			sink = NULL;
			tone = NULL;
			memset(&event, 0, sizeof(event));
			event.id = AUDIO_IDLE;
			session->queEvent(&event);
		}
		leave();		
        }
}

ssize_t RTPStream::putBuffer(Encoded data, size_t len)
{
        if(!oTimestamp)
	{
                oTimestamp = (getCurrentTimestamp() / interval) * interval - interval;
		if(pTimestamp && oTimestamp < pTimestamp)
			oTimestamp = pTimestamp;
		else if(pTimestamp && oTimestamp >= pTimestamp + interval && Driver::sip.data_filler)
		{
			unsigned add = ((oTimestamp - pTimestamp) / interval);
			if(add > 10)
				add = 1;
			while(add--)
			{
				oTimestamp += interval;
				if(Driver::sip.send_immediate)
					sendImmediate(oTimestamp, silent_encoded, info.framesize);
				else
					putData(oTimestamp, silent_encoded, info.framesize);
			}
			slog.debug("%s: rtp underfill; packets added", session->logname);
		}
	}
	// printf("DRIFT %ld\n", (long)oTimestamp - (long)getCurrentTimestamp());
        oTimestamp += interval;
	pTimestamp = oTimestamp;
	if(Driver::sip.send_immediate)
		sendImmediate(oTimestamp, (const unsigned char *)data, len);
	else
	        putData(oTimestamp, (const unsigned char *)data, len);
        return (ssize_t)len;
}

void RTPStream::put2833(struct dtmf2833 *data)
{
        if(!oTimestamp)
	{
                oTimestamp = getCurrentTimestamp();
		if(oTimestamp < pTimestamp)
			oTimestamp = pTimestamp;
	}
        oTimestamp += interval;
	pTimestamp = oTimestamp;
	if(Driver::sip.send_immediate)
		sendImmediate(oTimestamp, (const unsigned char *)data, 
			sizeof(struct dtmf2833));
	else
	        putData(oTimestamp, (const unsigned char *)data, 
			sizeof(struct dtmf2833));
}

ssize_t RTPStream::getBuffer(Encoded data, size_t len)
{
        const AppDataUnit *adu;
        size_t n;

        adu = getData(getFirstTimestamp());
        if(!adu)
                return 0;
        n = adu->getSize();
        if(n <= 0)
        {
                delete adu;
                return 0;
        }

        ::memcpy(data, adu->getData(), n);
        delete adu;
        return (ssize_t)n;
}

} // namespace
