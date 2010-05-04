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

#include "bayonne.h"
#include <cc++/slog.h>

#ifndef	__LINUX__
#ifdef	__linux__
#define	__LINUX__
#endif
#endif

extern "C" {

#include <libsangoma.h>

//#ifdef	HAVE_PRI_Q931_H
#include <libpri.h>
#include <pri_q931.h>
//#endif

#define	private	privproc

#include <sangoma_pri.h>

#undef private

}

namespace sangomadriver {
using namespace ost;
using namespace std;

extern "C" {
	int pe_info(struct sangoma_pri *spri, sangoma_pri_event_t pt, pri_event *pevent);
	int pe_hangup(struct sangoma_pri *spri, sangoma_pri_event_t pt, pri_event *event);
	int pe_ring(struct sangoma_pri *spri, sangoma_pri_event_t pt, pri_event *pevent);
	int pe_restart(struct sangoma_pri *spri, sangoma_pri_event_t pt, pri_event *pevent);
	int pe_any(struct sangoma_pri *spri, sangoma_pri_event_t pt, pri_event *pevent);
};

class Driver;
class AudioStreamer;

class Span : public BayonneSpan, public Thread
{
private:
	static int sspan_count;

public:
	int sspan;

	struct sangoma_pri spri;

	Span(Driver *d, timeslot_t timeslots);

	void run(void);

	void onHangup(pri_event *pevent);

	void onRing(pri_event *pevent);

	void onRestart(pri_event *pevent);
};

class Driver : public BayonneDriver
{
public:
	friend class Span;

	Audio::Info info;
	Span *spans[64];
	unsigned spancount;
	sangoma_pri_switch_t pri_switch;
	sangoma_pri_node_t pri_node;

	static Driver sangoma;	// plugin activation

	Driver();

	void startDriver(void);
	void stopDriver(void);

	void run(void);
	void initial(void);

	Span *find(sangoma_pri *spri, const char *id);
};

class Session : public BayonneSession, public TimerPort, public Audio
{
public:
        Session(Span *sp, timeslot_t ts);
        ~Session();

	q931_call call;
	struct sangoma_pri *spri;
	AudioStreamer *streamer;
	bool update_pos;

        // core timer virtuals all port session objects need to define
        timeout_t getRemaining(void);
        void startTimer(timeout_t timer);
        void stopTimer(void);

	void setOffhook(bool flag);
	bool clearCall(void);
	void makeIdle(void);
	void clrAudio(void);
	bool enterPlay(Event *event);
	bool enterRecord(Event *event);
	bool enterTone(Event *event);
	bool peerLinear(void);
};

#define	SANGOMA_MAX_BYTES	1000

class AudioStreamer : public Thread, public AudioBase, public Mutex, public Bayonne
{
private:
	int so;
	unsigned char inframe[SANGOMA_MAX_BYTES];
	unsigned char outframe[SANGOMA_MAX_BYTES];
	unsigned char inencoded[SANGOMA_MAX_BYTES];
	unsigned char outencoded[SANGOMA_MAX_BYTES];
	sangoma_api_hdr_t hdrframe;
	int mtu;
	wanpipe_tdm_api_t tdm_api;
	DTMFDetect *dtmf;
	Session *session;
	unsigned channel;
	struct sangoma_pri *spri;

	AudioCodec *inCodec, *outCodec;
	AudioBase *source, *sink;
	AudioTone *tone;
	bool ending;
	unsigned isize, osize;

public:
	AudioStreamer(Span *span, Session *s);
	~AudioStreamer();

	void start(void);

	ssize_t putBuffer(Encoded data, size_t len);
	ssize_t getBuffer(Encoded data, size_t len);

        void run(void);
        void peerAudio(Encoded encoded);

        void setSource(AudioBase *get, AudioCodec *inpc);
        void setSink(AudioBase *put, AudioCodec *outc);
        void setTone(AudioTone *tone);
};

} // end namespace
