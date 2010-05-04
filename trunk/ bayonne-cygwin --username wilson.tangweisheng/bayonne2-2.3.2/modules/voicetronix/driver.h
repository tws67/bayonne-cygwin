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

#ifdef	__linux__
#define	LINUX
#endif

#include <vpbapi.h>

#define CID_BUF_SZ 8000*4  // DR - up to 10 seconds of CID buffer

#ifdef  VPB_RING_STATION_ONCE
#define vpb_ring_station_sync(a, b) vpb_ring_station_async(a, b, 0)
#endif

namespace vpbdriver {
using namespace ost;
using namespace std;

class Driver : public BayonneDriver, public Thread
{
private:
	friend class PlayThread;
	friend class JoinThread;

	bool exit_reorder, exit_dialtone;
	float gain;

public:
	static Driver voicetronix;	// plugin activation

	Driver();

	void startDriver(void);
	void stopDriver(void);

	void run(void);
	void initial(void);
};

class Callerid : public ScriptThread
{
protected:
	int handle;
	Audio::Sample cidbuf[CID_BUF_SZ];
	char cidnbr[VPB_MAX_STR];
	int cidlen;

public:
	Callerid(ScriptInterp *interp, int handle);

	void run(void);
};

class JoinThread;

class Session : public BayonneSession
{
	friend class JoinThread;

protected:
	int handle;
	void *timer;
	bool inTimer;
        float inpgain, outgain;
	JoinThread *join;

	bool peerAudio(Audio::Encoded buffer);

	friend class ToneThread;
	friend class PlayThread;
	friend class RecordThread;

public:
	Session(timeslot_t ts, int dev);
	~Session();

	timeout_t audioFraming(void);
	const char *audioEncoding(void);

	// core timer virtuals all port session objects need to define
        timeout_t getRemaining(void);
        void startTimer(timeout_t timer);
        void stopTimer(void);

	const char *checkAudio(bool live);

	// we convert generic msgport destined events into voicetronix
	// queue events and push them back to the driver!  This is
	// particularly true for thread death notification...
	void queEvent(Event *event);

	inline int getHandle(void)
		{return handle;};

	void setOffhook(bool flag);

	bool enterRinging(Event *event);
	bool enterTone(Event *event);
	bool enterPlay(Event *event);
	bool enterRecord(Event *event);
	bool enterDial(Event *event);
	bool enterJoin(Event *event);
};

class ToneThread : public ScriptThread, public Audio, public Bayonne
{
protected:
	AudioTone *tone;
	Session *session;
	int handle;
	volatile bool reset;

	void run(void);

public:
	ToneThread(Session *interp);
	~ToneThread();
};

class JoinThread : public ScriptThread, public Audio, public Bayonne
{
protected:
	BayonneAudio *audio;
	Session *session;
	int handle;
	volatile bool reset;
	Encoded buffer;
	unsigned bufcount, bufsize;
	float gain;

	void run(void);
public:
	void peerAudio(Encoded encoded);

	JoinThread(Session *session);
	~JoinThread();
};

class RecordThread : public ScriptThread, public Audio, public Bayonne
{
protected:
	BayonneAudio *audio;
	Session *session;
	int handle;
	volatile bool reset;
	Linear lbuffer;
	Encoded buffer;
	unsigned bufcount, bufsize;
	timeout_t duration;
	
	void recordDirect(void);
	void recordConvert(void);

	void run(void);

public:
	RecordThread(Session *interp);
	~RecordThread();
};

class PlayThread : public ScriptThread, public Audio, public Bayonne
{
protected:
	BayonneAudio *audio;
	Session *session;
	int handle;
	volatile bool reset;
	Linear lbuffer;
	Encoded buffer;
	unsigned bufcount, bufsize;
	float gain;

	void playDirect(void);
	void playConvert(void);

	void run(void);

public:
	PlayThread(Session *interp);
	~PlayThread();
};

} // end namespace
