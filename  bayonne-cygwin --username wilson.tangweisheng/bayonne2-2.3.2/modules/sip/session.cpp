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

Session::Session(timeslot_t ts) :
BayonneSession(&Driver::sip, ts),
TimerPort()
{
#ifndef	WIN32
	if(getppid() > 1)
		logevents = &cout;
#endif
	iface = IF_INET;
	bridge = BR_GATE;
	cid = 0;
	did = 0;
	rtp = NULL;
	dtmf_sipinfo = Driver::sip.info_negotiate;
	dtmf_payload = Driver::sip.dtmf_negotiate;
	data_payload = Driver::sip.data_negotiate;
	dtmf_inband = Driver::sip.dtmf_inband;
	dtmf_sipinfo = false;

	rtp_flag = update_pos = reconnecting = noinvite = false;

	encoding_recall = NULL;
	peer_buffer = NULL;
	peer_codec = NULL;
	codec = NULL;
	session_timer = 0;
	sv = 0;
}

Session::~Session()
{
	stopRTP();
}

uint8 Session::sdpPayload(void)
{
	switch(info.encoding)
	{
	case Audio::mulawAudio:
		return 0;
	case Audio::alawAudio:
		return 8;
	case Audio::speexAudio:
	case Audio::speexVoice:
		return 97;
	case Audio::g721ADPCM:
		return 5;
	case Audio::g723_3bit:
		return 96;
	case Audio::g723_5bit:
		return 95;
	case Audio::g723_2bit:
		return 94;
	case Audio::gsmVoice:
		return 3;
	default:
		return 90;
	}
}

const char *Session::sdpNames(void)
{
	switch(info.encoding)
	{
	case Audio::mulawAudio:
		return "pcmu,mulaw";
	case Audio::alawAudio:
		return "pcma,alaw";
	case Audio::g721ADPCM:
		return "adpcm,a32,721,726-32";
	case Audio::g723_2bit:
		return "a16,726-16";
	case Audio::g723_3bit:
		return "a24,726-24";
	case Audio::g723_5bit:
		return "a40,726-40";
	case Audio::g729Audio:
		return "729";
	case Audio::pcm16Mono:
		return "linear,l16";
	case Audio::pcm16Stereo:
		return "stereo,l16-2ch";
	case Audio::gsmVoice:
		return "gsm";
	default:
		return "unknown";
	}
}

const char *Session::audioEncoding(void)
{
	switch(info.encoding)
	{
	case Audio::gsmVoice:
		return "gsm";
	case Audio::mulawAudio:
		return "pcmu";
	case Audio::alawAudio:
		return "pcma";
	case Audio::pcm16Mono:
		return "linear";
	case Audio::pcm16Stereo:
		return "stereo";
	case Audio::g721ADPCM:
		return "adpcm";
	case Audio::g723_2bit:
		return "a16";
	case Audio::g723_3bit:
		return "a24";
	case Audio::g723_5bit:
		return "a40";
	case Audio::g729Audio:
		return "g729";
	default:
		return "unknown";
	}
}

const char *Session::getAttributes(Audio::Encoding enc)
{
	switch(enc)
	{
	case g729Audio:
		return "annexb=no";
	default:
		return NULL;
	}
}

const char *Session::sdpEncoding(void)
{
	switch(info.encoding)
	{
	case Audio::gsmVoice:
		return "GSM/8000/1";
	case Audio::mulawAudio:
		return "PCMU/8000/1";
	case Audio::alawAudio:
		return "PCMA/8000/1";
	case Audio::pcm16Mono:
		return "L16/11025";
	case Audio::pcm16Stereo:
		return "L16-2CH/11025";
	case Audio::g721ADPCM:
		return "G726-32/8000/1";
	case Audio::g723_2bit:
		return "G726-16/8000/1";
	case Audio::g723_3bit:
		return "G726-24/8000/1";
	case Audio::g723_5bit:
		return "G726-40/8000/1";
	case Audio::g729Audio:
		return "G729/8000/1";	
	case Audio::speexVoice:
		return "SPEEX/8000/1";
	case Audio::speexAudio:
		return "SPEEX/16000/1";
	default:
		return NULL;
	}
}

void Session::setPeering(const char *enc)
{
	char map[65];

	if(!enc)
		return;

	if(!strnicmp(enc, "g.", 2))
		enc += 2;
	else if(!strnicmp(enc, "g7", 2))
		++enc;
	else if(*enc == '.')
		++enc;

	if(!stricmp(enc, "gsm"))
		enc = "a=rtpmap:3 GSM/8000/1";
	else if(!stricmp(enc, "ulaw") || !stricmp(enc, "mulaw") || !stricmp(enc, "pcmu"))
		enc = "a=rtpmap:0 PCMU/8000/1";
	else if(!stricmp(enc, "alaw") || !stricmp(enc, "pcma"))
		enc = "a=rtpmap:8 PCMU/8000/1";
	else if(!stricmp(enc, "adpcm") || !stricmp(enc, "721"))
		enc = "a=rtpmap:97 G726-32/8000/1";
	else if(!stricmp(enc, "stereo"))
		enc = "a=rtpmap:98 L16/11025/2";
	else if(!stricmp(enc, "726-16") || !stricmp(enc, "a16"))
		enc = "a=rtpmap:94 G726-16/8000/1";
	else if(!stricmp(enc, "726-32") || !stricmp(enc, "a32"))
		enc = "a=rtpmap:97 G726-32/8000/1";
	else if(!stricmp(enc, "726-24") || !stricmp(enc, "a24"))
		enc = "a=rtpmap:95 G726-24/8000/1";
	else if(!stricmp(enc, "726-40") || !stricmp(enc, "a40"))
		enc = "a=rtpmap:96 G726-40/8000/1";
	else if(!stricmp(enc, "729"))
		enc = "a=rtpmap:18 G729/8000/1";
	else
	{
		snprintf(map, sizeof(map), "a=rtpmap:%d %s",
			sdpPayload(), sdpEncoding());
		enc = map;
	}

	setConst("session.sdp_peering", enc);
}

void Session::redirect(const char *target, const char *encoding, unsigned char data, unsigned char dtmf, timeout_t framing)
{
	osip_message_t *msg = NULL;
	char sdp[512];
	char addr[65];
	char cbuf[65];
	char *ep;
	unsigned port;
	const char *invite = "";
	const char *cp;

	setString(addr, sizeof(addr), target);
	ep = strrchr(addr, ':');
	*ep = 0;
	port = atoi(++ep);

	slog.debug("%s: redirecting %s to %s:%d",
		logname, getSymbol("session.rtp_remote"), addr, port);

	eXosip_lock();
	if(sip_answered)
		eXosip_call_build_request(did, "INVITE", &msg);
	else
		eXosip_call_build_answer(tid, 200, &msg);
 
	if(!msg)
	{
		eXosip_unlock();
		return;
	}

	osip_message_set_supported(msg, "100rel,replaces");

	if(dtmf)
		snprintf(sdp, sizeof(sdp),
			"v=0\r\n"
			"o=bayonne 0 %ld IN IP4 %s\r\n"
			"s=reinvite\r\n"
			"c=IN IP4 %s\r\n"
			"t=0 0\r\n"
			"m=audio %d RTP/AVP %d %d\r\n"
			"a=rtpmap:%d %s\r\n"
			"a=rtpmap:%d telephone-event/8000\r\n"
			"a=fmtp:%d 0-15\r\n",
			++sv, addr, addr, port, data, dtmf,
			data, encoding, dtmf, dtmf);
	else
		snprintf(sdp, sizeof(sdp),
			"v=0\r\n"
			"o=bayonne 0 %ld IN IP4 %s\r\n"
			"s=reinvite\r\n"
			"c=IN IP4 %s\r\n"
			"t=0 0\r\n"
			"m=audio %d RTP/AVP %d\r\n"
			"a=rtpmap:%d %s\r\n",
			++sv, addr, addr, port, data, data, encoding);

	cp = getAttributes(Bayonne::getEncoding(encoding));
	if(cp)
	{
		snprintf(cbuf, sizeof(cbuf),
			"a=fmtp:%d %s\r\n",
			data, cp);
		addString(sdp, sizeof(sdp), cbuf);
	}
	if(framing)
	{
		snprintf(cbuf, sizeof(cbuf),
			"a=ptime:%ld\r\n", framing);
		addString(sdp, sizeof(sdp), cbuf);
	}
	
	if(invite)
		addString(sdp, sizeof(sdp), invite);
	
	osip_message_set_body(msg, sdp, strlen(sdp));
	if(sip_answered)
	{
	        osip_message_set_content_type(msg, "application/sdp");
		eXosip_call_send_request(did, msg);
	}
	else
	{
		osip_message_set_require(msg, "100rel");
                osip_message_set_header(msg, "RSeq", "1");
                osip_message_set_body(msg, sdp, strlen(sdp));
                osip_message_set_content_type(msg, "application/sdp");
		eXosip_call_send_answer(tid, 200, msg);
	}
	eXosip_unlock();
	reinvite = true;
}	

void Session::reconnect(void)
{
	const char *target = getSymbol("session.rtp_local");
	
	if(!target || !reinvite)
		return;

	redirect(target, sdpEncoding(), data_payload, dtmf_payload, info.framing);
	reinvite = false;
}

unsigned char Session::peerEvents(BayonneSession *s)
{
	const char *el = getSymbol("session.sdp_events");
	const char *er = s->getSymbol("session.sdp_events");
	unsigned char pl, pr, pe;

	if(!el || !er)
		return 255;

	pe = peerEncoding(s);
	if(!pe)
		return 255;

	pl = atoi(el + 9);
	pr = atoi(er + 9);
	if(pl == pr && pl != pe)
		return pl;

	if(pe != 101)
		return 101;
	else
		return 102;
}

unsigned char Session::peerEncoding(BayonneSession *s)
{
	const char *evl = getSymbol("session.sdp_events");
	const char *evr = s->getSymbol("session.sdp_events");
	const char *cl = getSymbol("session.sdp_peering");
	const char *cr = s->getSymbol("session.sdp_peering");
	char *el;
	unsigned char pl, pr;

	char buf[65], rbuf[65];

	if(evl && !evr)
		return 255;

	if(evr && !evl)
		return 255;

	if(!cl || !cr)
		return 255;

	if(evl && evr && !isPeering())
		return 255;

	if(evl && evr && !s->isPeering())
		return 255;

	pl = atoi(cl + 9);
	pr = atoi(cr + 9);

	cl = strchr(cl, ' ');
	++cl;

	cr = strchr(cr, ' ');
	++cr;

	setString(buf, sizeof(buf), cl);
	el = strchr(buf, '/');
	if(el)
	{
		el = strchr(++el, '/');
		if(el && !stricmp(el, "/1"))
			*el = 0;
	}
	
	setString(rbuf, sizeof(rbuf), cr);
	el = strchr(rbuf, '/');
	if(el)
	{
		el = strchr(++el, '/');
		if(el && !stricmp(el, "/1"))
			*el = 0;
	}

	if(stricmp(buf, rbuf))
		return 255;

	if(pl == pr)
		return pl;

	return 98;
}

void Session::setEncoding(const char *encoding, timeout_t framing)
{
	if(codec)
		AudioCodec::endCodec(codec);

	memset(&info, 0, sizeof(info));

	info.encoding = Bayonne::getEncoding(encoding);

	switch(info.encoding)
	{
	case Audio::pcm16Stereo:
	case Audio::pcm16Mono:
		info.rate = (Audio::Rate)11025;
		if(!framing)
			framing = 20;
		break;
	case Audio::speexAudio:
		info.rate = (Audio::Rate)16000;
		if(!framing)
			framing = 20;
		break;
	default:
		info.rate = Audio::rate8khz;
		if(!framing)
			framing = 20;
	}

	info.setFraming(framing);

	codec = AudioCodec::getCodec(info);
}

void Session::sendDTMFInfo(char digit, unsigned duration)
{
	osip_message_t *message = NULL;
	char dtmf_body[1000];

        snprintf(dtmf_body, sizeof(dtmf_body) - 1,
                "Signal=%c\r\nDuration=%d\r\n", digit, duration);  

	eXosip_lock();
	if(eXosip_call_build_info(did, &message) != 0)
	{
		eXosip_unlock();
		return;
	}

	osip_message_set_body(message, dtmf_body, strlen(dtmf_body));
	osip_message_set_content_type(message, "application/dtmf-relay");
	eXosip_call_send_request(did, message);
	eXosip_unlock();
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

tpport_t Session::getLocalPort(void)
{
	Driver *d = (Driver *)(driver);

	return d->rtp_port + (4 * timeslot);
}	

void Session::startRTP(void)
{
	if(rtp && rtp_flag)
	{
		slog.error("%s: rtp already started", logname);
		return;
	}

	if(!rtp)
		rtp = new RTPStream(this);
	else
		rtp->newSession();

	if(!rtp->addDestination(remote_address, remote_port))
		slog.error("%s: destination not available");

	rtp->start();
	rtp_flag = true;
}		

void Session::suspendRTP(void)
{
	if(!rtp_flag || !rtp)
		return;

	rtp_flag = false;

	Thread::yield();
	rtp->forgetDestination(remote_address, remote_port);
}

void Session::stopRTP(void)
{
	if(!rtp_flag || !rtp)
		return;
	rtp_flag = false;

	Thread::yield();

	if(Driver::sip.starting == START_ACTIVE)
	{
		delete rtp;
		rtp = NULL;
	}
	else
	{
		Thread::yield();
		rtp->forgetDestination(remote_address, remote_port);
	}
}

void Session::makeIdle(void)
{
	update_pos = false;
	session_timer = 0;
	reconnecting = false;
	noinvite = false;
	sv = 0;

	sip_answered = true;
	encoding_recall = NULL;

	if(offhook)
		sipRelease();
	BayonneSession::makeIdle();
	dtmf_inband = Driver::sip.dtmf_inband;
	dtmf_payload = Driver::sip.dtmf_negotiate;
	dtmf_sipinfo = Driver::sip.info_negotiate;
	data_payload = Driver::sip.data_negotiate;

	memcpy(&info, &Driver::sip.info, sizeof(info));
	if(codec)
		AudioCodec::endCodec(codec);
	codec = AudioCodec::getCodec(info);

	if(Driver::sip.starting == START_IMMEDIATE && !rtp)
	{
		rtp = new RTPStream(this);
		rtp->start();
	}
}

bool Session::enterJoin(Event *event)
{
	Level level = 26000;
	Rate rate = (Rate)info.rate;
	unsigned f1, f2;
	timeout_t framing = getToneFraming(), duration;
	struct dtmf2833 dtmfevent;
	const char *target;
	const char *encoding;
	unsigned char data_peering, dtmf_peering;

	switch(event->id)
	{
	case CALL_HOLD:
		if(!holding)
			return false;
		encoding_recall = audioEncoding();
		framing_recall = info.framing;
		return false;
	case EXIT_PARTING:
		if(reinvite && !isDisconnecting())
			reconnect();
		return false;
	case ENTER_STATE:
		reinvite = false;
		if(!state.join.peer)
		{
			state.peering = false;
			return false;
		}
		data_peering = peerEncoding(state.join.peer);
		if(data_peering == 255)
			return false;
		dtmf_peering = peerEvents(state.join.peer);
		if(dtmf_peering == 255)
			dtmf_peering = 0;
		encoding = state.join.peer->getSymbol("session.sdp_peering");
		target = state.join.peer->getSymbol("session.rtp_remote");
		if(encoding)
			encoding = strchr(encoding, ' ');
		if(encoding && target)
			++encoding;
		else
		{
			state.peering = false;
			return false;
		}
		redirect(target, encoding, data_peering, dtmf_peering);
		return false;
	case AUDIO_SYNC:
		rtp->syncAudio();
		return true;
	case AUDIO_IDLE:
		rtp->setTone(NULL);
		return true;
	case DTMF_GENTONE:
		if(dtmf_sipinfo)
		{
			sendDTMFInfo(getChar(event->dtmf.digit), event->dtmf.duration);
			return true;
		}
		if(dtmf_payload)
		{
			memset(&dtmfevent, 0, sizeof(dtmfevent));
			dtmfevent.event = event->dtmf.digit;
			rtp->set2833(&dtmfevent, event->dtmf.duration);
			return true;
		}
		rtp->setTone(NULL);
		if(audio.tone)
		{
			delete audio.tone;
			audio.tone = NULL;
		}

		f1 = f2 = 0;
		duration = event->dtmf.duration;
		duration = (duration + (framing - 1)) / framing;
		duration *= framing;
		switch(event->dtmf.digit)
		{
		case 0:
			f1 = 941; f2 = 1336; break;
		case 1:
			f1 = 697; f2 = 1209; break;
		case 2:
			f1 = 697; f2 = 1336; break;
		case 3:
			f1 = 697; f2 = 1447; break;
		case 4:
			f1 = 770; f2 = 1209; break;
		case 5:
			f1 = 770; f2 = 1336; break;
		case 6:
			f1 = 770; f2 = 1477; break;
		case 7:
			f1 = 852; f2 = 1209; break;
		case 8:
			f1 = 852; f2 = 1336; break;
		case 9:
			f1 = 852; f2 = 1477; break;
		case 10:
			f1 = 941; f2 = 1209; break;
		case 11:
			f1 = 941; f2 = 1477; break;
		}
		if(f1)
			audio.tone = new AudioTone(f1, f2, level, level, duration, rate);

		if(audio.tone)
			rtp->setTone(audio.tone);
		return true;
	default:
		return false;
	}
}

bool Session::enterRefer(Event *event)
{
	osip_message_t *msg = NULL;

	switch(event->id)
	{
	case ENTER_STATE:
		startTimer(3500);
		return false;
	case TIMER_EXPIRED:
		eXosip_lock();
		eXosip_call_build_answer(tid, 100, &msg);
		if(msg)
			eXosip_call_send_answer(tid, 100, msg);
		eXosip_unlock();
		startTimer(3500);
		return true;
	case DROP_RECALL:
		eXosip_lock();
		eXosip_call_build_answer(tid, 200, &msg);
		if(msg)
			eXosip_call_send_answer(tid, 200, msg);
		eXosip_unlock();
		return false;
	case DROP_REFER:
	case JOIN_RECALL:
		eXosip_lock();
		eXosip_call_build_answer(tid, 503, &msg);
		if(msg)
			eXosip_call_send_answer(tid, 503, msg);
		eXosip_unlock();
	default:
		return false;
	}
}

bool Session::enterSeize(Event *event)
{
	char buf[128];
	char cref[64];
	char nbuf[16];
	const char *cp, *to, *from = NULL;
	char *sp = NULL;
	osip_message_t *invite = NULL;
	Registry *reg = NULL;
	ScriptImage *img = getImage();
	char rbuf[16];
	char cbuf[65];
	char sdp[512];
	const char *rtpmap = NULL;
	const char *route = NULL;
        const char *lp = driver->getLast("localip");
	BayonneSession *parent = Bayonne::getSid(var_pid);
	const char *caller = NULL;
	Name *scr;
	unsigned idx;
	bool offline = false;
	unsigned short instance = 0;
	const char *proxy;
	char *ep;

	switch(event->id)
	{
	case ENTER_STATE:
		route = Driver::sip.getLast("outbound");
		cp = getSymbol("session.dialed");
		if(cp && !strchr(cp, '@') && !route)
		{
			if(!strnicmp(cp, "sip:", 4))
				cp += 4;

			snprintf(cbuf, sizeof(cbuf), "sip::%s", cp);
			scr = img->getScript(cbuf);
			if(scr && scr->select)
				state.join.select = scr->select;
			else
				goto single;

			state.join.index = 0;
			while(state.join.select)
			{
				idx = state.join.index++;
				cp = state.join.select->args[idx];
				if(idx < state.join.select->argc)
					cp = state.join.select->args[idx];
				else
					cp = NULL;
				if(!cp)
				{
					state.join.select =
						state.join.select->next;
					state.join.index = 0;
					continue;
				}
				snprintf(buf, sizeof(buf), "uri.%s", cp);
				reg = (Registry *)img->getPointer(buf);
				if(!reg)
					continue;
				if(strnicmp(reg->type, "ext", 3))
					goto single;
				if(!reg->isActive())
				{
					offline = true;
					continue;
				}
				goto dialer;
			}
			if(offline)
			{
				event->id = DIAL_OFFLINE;
				return false;
			}
		}
single:
		cp = getSymbol("session.dialed");
		if(!cp)
		{
			event->id = DIAL_FAILED;
			return false;
		}
//forward:

		if(!strchr(cp, '@') && !route)
		{
			if(!strnicmp(cp, "sip:", 4))
				cp += 4;

			snprintf(buf, sizeof(buf), "uri.%s", cp);
			reg = (Registry *)img->getPointer(buf);
			if(reg)
			{
				if(!reg->isActive())
				{
					event->id = DIAL_OFFLINE;
					return false;
				}
				if(!stricmp(reg->type, "anon"))
				{
					event->id = DIAL_INVALID;
					return false;
				}
dialer:
				switch(reg->traffic->presence)
				{
				case SIPTraffic::P_BUSY:
					event->id = DIAL_BUSY;
					return false;
				case SIPTraffic::P_AWAY:
					event->id = DIAL_AWAY;
					return false;
				case SIPTraffic::P_DND:
					event->id = DIAL_DND;
					return false;
				default:
					break;
				}
				instance = ++reg->traffic->active_calls;
				if(reg->call_limit && instance > reg->call_limit)
				{
					--reg->traffic->active_calls;
					event->id = DIAL_BUSY;
					return false;
				}
				snprintf(buf, sizeof(buf), "sip:%s@%s",
					reg->userid, reg->proxy);
				if(!route)
					route = reg->proxy;
				ep = strrchr(buf, ':');
				if(ep && !stricmp(sp, ":5060"))
					*ep = 0;
				setConst("session.uri_remote", buf);
				goto dialing;
			}
			else
			{
				event->id = DIAL_FAILED;
				return false;
			}
		}
		if(!strchr(cp, '@'))
			snprintf(buf, sizeof(buf), "sip:%s@%s", cp, route);
		else 
		{
			if(!strnicmp(cp, "sip:", 4))
				cp += 4;
			snprintf(buf, sizeof(buf), "sip:%s", cp);	
			sp = strrchr(buf, ':');
		}
		if(sp && stricmp(sp, ":5060"))
			sp = NULL;
		setConst("session.uri_remote", buf);
		cp = getSymbol("session.uri_remote");
		cp = strchr(cp, '@');
		if(cp)
		{
			snprintf(buf, sizeof(buf), "sip.%s", ++cp);
			reg = (Registry *)img->getPointer(buf);
		}
dialing:
		dtmf_sipinfo = Driver::sip.info_negotiate;
		proxy = getSymbol("proxyauth");
		if(reg)
		{
			if(reg->dtmf && !stricmp(reg->dtmf, "info"))
				dtmf_sipinfo = true;
			else if(reg->dtmf && !stricmp(reg->dtmf, "sipinfo"))
				dtmf_sipinfo = true;
			else if(reg->dtmf)
				dtmf_sipinfo = false;

			if(!route)
				route = reg->proxy;

			if(!instance)
				instance = ++reg->traffic->active_calls;
			++reg->traffic->call_attempts.oCount;
			++reg->traffic->call_complete.oCount;
			if(!strnicmp(reg->type, "ext", 3))
				snprintf(rbuf, sizeof(rbuf), 
					"uri.%s", reg->localid);
			else
				snprintf(rbuf, sizeof(rbuf), 
					"sipreg.%d", reg->regid);
			snprintf(nbuf, sizeof(nbuf), "%ld", reg->traffic->sequence);
			setConst("session.sequence", nbuf);
			setConst("session.registry", rbuf);
			setConst("session.uri_server", reg->proxy);
			setConst("session.uri_local", reg->contact);
			if(proxy && *proxy)
			{
				snprintf(buf, sizeof(buf), "sip:%s", proxy);
				ep = strchr(buf, ':');
				if(ep)
					*ep = 0;
				cp = strchr(reg->uri, '@');
				if(cp)
					addString(buf, sizeof(buf), cp);
				setConst("session.uri_caller", buf);
				caller = NULL;
			}
			else
			{
				caller = reg->caller;
            	setConst("session.uri_caller", reg->uri);
			}

                        if(reg->address)
                        {
                                InetAddress host(reg->address);
                                snprintf(cbuf, sizeof(cbuf), "%s",
                                        inet_ntoa(host.getAddress()));
                                setConst("session.ip_public", cbuf);
                        }
                        else
                                setConst("session.ip_public", lp);

			if(!strnicmp(reg->type, "ext", 3))
			{
				snprintf(buf, sizeof(buf), "sip:%s", reg->localid);
				setConst("session.authorized", buf);
				setConst("session.identity", buf);
				setConst("session.group", reg->group);
			}
			else if(reg->service)
				setConst("session.service", reg->service);

			snprintf(nbuf, sizeof(nbuf), "%d", instance);
			setConst("session.instance", nbuf);
		}
		else
		{
			if(proxy && *proxy)
			{
				setString(buf, sizeof(buf), "sip:");
				addString(buf, sizeof(buf), proxy);
				ep = strchr(buf, ':');
				if(ep)
					*ep = 0;
				addString(buf, sizeof(buf), "@");
				addString(buf, sizeof(buf), 
					Driver::sip.getLast("interface"));
				setConst("session.uri_caller", buf);
			}
			snprintf(buf, sizeof(buf), "sip:anon@%s",
					Driver::sip.getLast("interface"));
			setConst("session.uri_local", buf);
			setConst("session.uri_caller", buf);
                        setConst("session.ip_local", lp);
                        setConst("session.ip_public", lp);
			setConst("session.service", "anonymous");
		}
                snprintf(nbuf, sizeof(nbuf), "%d", instance);
                setConst("session.instance", nbuf);
					
		to = getSymbol("session.uri_remote");
		from = getSymbol("session.uri_caller");

		if(!caller)
		{
			caller = getSymbol("session.display");
			if(!caller)
				caller = getSymbol("session.caller");

			if(caller && !*caller)
				caller = "unknown";
		}

		if(!route)
		{
			route = strchr(to, '@');
			if(route)
				++route;
		}

		if(!strnicmp(route, "sip:", 4))
			route += 4;

		snprintf(sdp, sizeof(sdp), "\"%s\" <%s>", caller, from);
		snprintf(buf, sizeof(buf), "<sip:%s;lr>", route);
		slog.debug("invite %s from %s using %s", to, from, route);
		eXosip_lock();
		if(eXosip_call_build_initial_invite(&invite, 
			(char *)to, (char *)sdp, buf, "Bayonne Call"))
		{
			eXosip_unlock();
failure:
			slog.error("sip: invite invalid");
			event->id = DIAL_FAILED;
			return false;
		}
		osip_message_set_supported(invite, "100rel,replaces");
		eXosip_unlock();
		cp = Driver::sip.getLast("localip");
		data_payload = Driver::sip.data_negotiate;
		dtmf_payload = Driver::sip.dtmf_negotiate;

		if(parent && !parent->peerLinear())
			setEncoding(parent->audioEncoding(), parent->audioFraming());
		else if(reg && reg->encoding)
			setEncoding(reg->encoding, reg->framing);

		switch(info.encoding)
		{
		case pcm16Stereo:
			data_payload = 98;
			rtpmap = "a=rtpmap:98 L16/11025/2";
			break;
		case pcm16Mono:
			data_payload = 97;
			rtpmap = "a=rtpmap:97 L16/8000/1";
			break;
		case pcm8Mono:
			data_payload = 96;
			rtpmap = "a=rtpmap:96 L8/8000/1";
			break;
		case mulawAudio:
			data_payload = 0;
			rtpmap = "a=rtpmap:0 PCMU/8000/1";
			break;
		case alawAudio:
			data_payload = 8;
			rtpmap = "a=rtpmap:8 PCMA/8000/1";
			break;
		case g721ADPCM:
			data_payload = 5;
			rtpmap = "a=rtpmap:5 G726_32/8000/1";
			break;
		case gsmVoice:
			data_payload = 3;
			rtpmap = "a=rtpmap:3 GSM/8000/1";
			break;
		case speexVoice:
			data_payload = 97;
			rtpmap = "a=rtpmap:97 SPEEX/8000/1";
		default:
			break;
		}			
		if(dtmf_payload)
		{
			snprintf(sdp, sizeof(sdp),
				"a=rtpmap:%d telephone-event/8000",
				dtmf_payload);
			setConst("session.sdp_events", sdp);
                        snprintf(sdp, sizeof(sdp),
                                "v=0\r\n"
                                "o=bayonne 0 0 IN IP4 %s\r\n"
                                "s=call\r\n"
                                "c=IN IP4 %s\r\n"
                                "t=0 0\r\n"
                                "m=audio %d RTP/AVP %d %d\r\n"
                                "%s\r\n"
				"a=rtpmap:%d telephone-event/8000\r\n"
				"a=fmtp:%d 0-15\r\n",
				cp, cp, 
				getLocalPort(), data_payload, dtmf_payload,
				rtpmap, dtmf_payload, dtmf_payload);
		}
		else
			snprintf(sdp, sizeof(sdp),
				"v=0\r\n"
				"o=bayonne 0 0 IN IP4 %s\r\n"
				"s=call\r\n"
				"c=IN IP4 %s\r\n"
				"t=0 0\r\n"
				"m=audio %d RTP/AVP %d\r\n"
				"%s\r\n",
				cp, cp, 
				getLocalPort(), data_payload, 
				rtpmap);

		cp = getAttributes(info.encoding);
		if(cp)
		{
			snprintf(cbuf, sizeof(cbuf),
				"a=fmtp:%d %s\r\n",
				data_payload, cp);
			addString(sdp, sizeof(sdp), cbuf);
		}
		if(info.framing)
		{
			snprintf(cbuf, sizeof(cbuf),
				"a=ptime:%ld\r\n", info.framing);
			addString(sdp, sizeof(sdp), cbuf);
		}

		setConst("session.sdp_encoding", rtpmap);
		if(reg && reg->peering)
			setPeering(reg->peering);
		if(reg && reg->reconnect)
			setConst("session.sdp_reconnect", reg->reconnect);

		if(dtmf_sipinfo)
			setConst("session.dtmfmode", "info");
		else if(dtmf_payload)
			setConst("session.dtmfmode", "2833");
		else
			setConst("session.dtmfmode", "none");

		eXosip_lock();
		osip_message_set_body(invite, sdp, strlen(sdp));
		osip_message_set_content_type(invite, "application/sdp");
		
		offhook = true;
		setSid();
		snprintf(cref, sizeof(cref), "%s@%s",
			var_sid, inet_ntoa(local_address.getAddress()));
		setConst("session.callref", cref);
		cid = eXosip_call_send_initial_invite(invite);
		if(cid > 0)
			eXosip_call_set_reference(cid, cref);
		eXosip_unlock();
		if(cid <= 0)
			goto failure;
		startTimer(atol(Driver::sip.getLast("invite")));
		sip_answered = true;
		return true;
	case CALL_CONNECTED:
		startRTP();
		setConnecting();
		return true;
	case CALL_ANSWERED:
		setState(STATE_PICKUP);
		return true;
	case CALL_RINGING:
		stopTimer();
		return true;
	case STOP_SCRIPT:
	case TIMER_EXPIRED:
	case CANCEL_CHILD:
		sipHangup();
		offhook = false;
	default:
		return false;
	}
}

bool Session::enterHangup(Event *event)
{
	ScriptImage *img = getImage();
	Registry *reg;
	const char *cp;

	switch(event->id)
	{
	case ENTER_STATE:
		// if never answered, we are rejecting the invite...
		if(!sip_answered)
		{
			eXosip_lock();
			eXosip_call_send_answer(tid, 488, NULL);
			eXosip_unlock();
		}
		cp = getSymbol("session.registry");
		if(!cp)
			break;
		reg = (Registry *)img->getPointer(cp);
		if(reg)
			--reg->traffic->active_calls;
		break;
	case TIMER_EXPIRED:
		if(!rtp_flag)
			break;
		sipHangup();
		if(rtp && !rtp_flag)
		{
			setTimer(100);
			Thread::yield();
			return true;
		}
	default:
		break;
	}

	update_pos = false;
	return false;
}

bool Session::enterRecord(Event *event)
{
	switch(event->id)
	{
	case ENTER_STATE:
		audio.record(state.audio.list[0], state.audio.mode, state.audio.note);
		update_pos = true;
		if(!audio.isOpen())
		{
			slog.error("%s: audio file access error", logname);
                        error("no-files");
                        setRunning();
			return true;
		}
		rtp->setSink(&audio, state.audio.total);
		return false;
	default:
		return false;
	}
}

bool Session::enterPlay(Event *event)
{
	switch(event->id)
	{
	case AUDIO_IDLE:
		if(!Driver::sip.audio_timer)
			return false;
		startTimer(Driver::sip.audio_timer);
		return true;
	case ENTER_STATE:
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
		rtp->setSource(&audio);		
		return false;
	default:
		return false;
	}
}

bool Session::enterAnswer(Event *event)
{
	switch(event->id)
	{
	case ENTER_STATE:
		if(!sip_answered)
		{
			reinvite = true;
			startTimer(1000);
			reconnect();
		}
		return false;
	case TIMER_EXPIRED:
		if(!sip_answered)
		{
			sip_answered = true;
			scriptEvent("call:failed");
			error("no-answer");
			setRunning();
			return true;
		}
		return false;
	case CALL_ACCEPTED:
		startTimer(50);
		sip_answered = true;
		return true;
	default:
		return false;
	}
}

bool Session::enterPickup(Event *event)
{
	timeout_t accept;
	const char *cp;
	Registry *reg;
	ScriptImage *img = getImage();

	if(event->id == ENTER_STATE)
	{
		offhook = true;
		startRTP();
		startTimer(driver->getPickupTimer());
		return true;
	}
	else if(event->id == CALL_ACCEPTED)
	{
		cp = getSymbol("session.registry");
		if(cp)
			reg = (Registry *)img->getPointer(cp);
		else
			reg = NULL;
		if(reg)
			accept = reg->accept;
		else
			accept = 0;
		if(accept < Driver::sip.accept_timer)
			accept = Driver::sip.accept_timer;
		if(!accept)
		{
			event->id = TIMER_EXPIRED;
			return false;
		}
		startTimer(accept);
		return true;
	}

	return false;
}

bool Session::enterReconnect(Event *event)
{
	const char *sdp;
	const char *enc;
	
	if(!sip_answered)
		switch(event->id)
		{
		case ENTER_RECONNECT:
			startTimer(1000);
			reinvite = true;
			reconnect();
			return true;
		case ENTER_STATE:
			return true;
		case CALL_ACCEPTED:
			startTimer(50);
			return true;
		case DTMF_KEYDOWN:
		case DTMF_KEYUP:
		case LINE_DISCONNECT:
			return true;
		case TIMER_EXPIRED:
			sip_answered = true;
			reconnecting = false;
			setRunning();
			return true;
		default:
			return false;
		}					

	switch(event->id)
	{
	case RECALL_RECONNECT:
		if(!encoding_recall)
			return false;

		event->reconnect.framing = framing_recall;
		event->reconnect.encoding = encoding_recall;		
	case ENTER_RECONNECT:
		if(noinvite)
			return false;
		memcpy(&info_saved, &info, sizeof(info));
		dtmf_saved = dtmf_payload;
		data_saved = data_payload;
		sdp = getSymbol("session.sdp_reconnect");
		if(!sdp || !*sdp)
		{
			reconnecting = false;
			return false;
		}

		enc = event->reconnect.encoding;
		if(!enc)
			enc = audioEncoding();

		if(*enc == '.')
			++enc;
		else if(!stricmp(enc, "mulaw"))
			++enc;
		else if(!strnicmp(enc, "g.7", 3))
			enc += 2;
		else if(!strnicmp(enc, "g7", 2))
			++enc;


		if(event->reconnect.framing && event->reconnect.framing != info.framing)
			goto test;

		// no need for reconnect if we already are
		if(stristr(sdpNames(), enc))
		{
			reconnecting = false;
			return false;
		}

test:
		if(!stricmp(enc, "adpcm") || !stricmp(enc, "721"))
			if(stristr(sdp, "a32"))
				goto recon;

		if(!stricmp(enc, "adcpm") || !stricmp(enc, "a32"))
			if(stristr(sdp, "721"))
				goto recon;

		if(!stricmp(enc, "pcmu"))
			if(stristr(sdp, "ulaw"))
				goto recon;

		if(!stricmp(enc, "pcma"))
			if(strstr(sdp, "alaw"))
				goto recon;

		if(!stristr(sdp, enc))
		{
			reconnecting = false;
			return false;
		}

recon:
		if(reconnecting)
		{
			reconnecting = false;
			return false;
		}
		state.peering = false;
		reinvite = true;
		suspendRTP();
		setEncoding(event->reconnect.encoding, event->reconnect.framing);
		data_payload = sdpPayload();
		state.timeout = 0;
		encoding_recall = NULL;
		return true;
	case DIAL_FAILED:
		memcpy(&info, &info_saved, sizeof(info));
		dtmf_payload = dtmf_saved;
		data_payload = data_saved;
		noinvite = true;
		startRTP();
		return true;
	case CALL_ANSWERED:
		reconnecting = true;
		startRTP();
		return true;
	case STREAM_ACTIVE:
		setRunning();
		return true;
	case STREAM_PASSIVE:
		reconnect();
		return true;
	case EXIT_RECONNECT:
		if(isDisconnecting() && offhook)
			sipHangup();
		else if(!rtp_flag)
			startRTP();
		return true;
	default:
		return false;
	}
}


bool Session::enterTone(Event *event)
{
	if(event->id == ENTER_STATE && audio.tone)
		rtp->setTone(audio.tone);

	return false;
}

bool Session::enterXfer(Event *event)
{
	osip_message_t *refer;
	int rtn;

	switch(event->id)
	{
	case DIAL_FAILED:
		if(reconnecting)
		{
			noinvite = true;
			goto refer;
		}
		event->id = DIAL_INVALID;
	case DIAL_BUSY:
	case DIAL_INVALID:
	case ERROR_STATE:
		reconnect();
		return false;
	case ENTER_STATE:
		if(noinvite)
			goto refer;
		reconnecting = true;
		redirect("0.0.0.0:0", sdpEncoding(), data_payload, dtmf_payload);		
		startTimer(250);
		return true;
	case CALL_ANSWERED:
		reconnecting = false;
		stopTimer();
	case TIMER_EXPIRED:
refer:
		reconnecting = false;
		eXosip_lock();
		eXosip_call_build_refer(did, (char *)state.url.ref, &refer);
		rtn = eXosip_call_send_request(did, refer);
		eXosip_unlock();
		if(!rtn)
			return true;

		event->errmsg = "transfer-invalid";
		event->id = ERROR_STATE;
		return false;
	case STOP_DISCONNECT:
		setState(STATE_HANGUP);
		return true;
	case DTMF_KEYDOWN:
	case DTMF_KEYUP:
		return true;
	default:
		return false;
	}	
}

void Session::clrAudio(void)
{
	if(rtp)
	{
		rtp->setSource(NULL);
		rtp->setTone(NULL);
	}

	if(audio.isOpen() && update_pos)
	{
		audio.getPosition(audio.var_position, 12);
		update_pos = false;
	}
	audio.cleanup();
}

void Session::sipHangup(void)
{
	slog.debug("%s: terminating call; cid=%d, did=%d", logname, cid, did);
	eXosip_lock();
	if(cid || did)
		eXosip_call_terminate(cid, did);
	eXosip_unlock();
	stopRTP();
	offhook = false;
}

void Session::sipRelease(void)
{
	if(!rtp)
		return;

        eXosip_lock();
        eXosip_call_terminate(cid, did);
        eXosip_unlock();
	rtp_flag = false;
	delete rtp;
	rtp = NULL;	
        offhook = false;
}

timeout_t Session::getToneFraming(void)
{
	return info.framing;
}

const char *Session::audioExtension(void)
{
	return Audio::getExtension(info.encoding);
}

timeout_t Session::audioFraming(void)
{
	return info.framing;
}

const char *Session::checkAudio(bool live)
{
	audio.libext = ".au";

	switch(info.encoding)
	{
	case g721ADPCM:
		audio.libext = ".a32";
		break;
	case alawAudio:
		audio.libext = ".al";
		break;
	case gsmVoice:
		if(!audio.extension)
			audio.extension = ".gsm";
		audio.libext = ".gsm";
	default:
		break;
	}

	if(!audio.extension)
		audio.extension = ".au";

	if(audio.encoding == unknownEncoding)
		audio.encoding = info.encoding;
	
	if(!live)
	{
		if(!audio.framing)
			audio.framing = 10;
		return NULL;
	}

	audio.framing = info.framing;
	if(audio.encoding != info.encoding)
		return "unsupported audio format";

	return NULL;
}
	
bool Session::peerAudio(Encoded encoded)
{
	enter();
	if(!peer || !rtp || rtp->source || !isJoined())
	{
		leave();
		return false;
	}

	rtp->peerAudio(encoded, peer->peerLinear());
	leave();
	return true;
}
	
		
} // namespace
