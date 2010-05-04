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
#include <cc++/process.h>

#ifdef	WIN32
#include <conio.h>
#else
#include "private.h"
#include <termios.h>
#include <fcntl.h>
#endif

namespace scdriver {
using namespace ost;
using namespace std;

class Driver;
class Session;

#ifdef	HAVE_FOX

// this is a class for the optional GUI...
class Desktop : public Thread, public Bayonne, public ScriptBinder
{
public:
	Desktop(timeslot_t ts);
	void run(void);
};
#endif

// this is just a quick seperate class to fetch and post keyboard input
// to a msgport.  Maybe someday this will be a thread for a full 
// simulation gui
class Keyboard : public Thread, public Bayonne
{
private:
	timeslot_t ts;

#ifndef	WIN32
	termios t;	// saved termios to restore on exiting
#endif

public:
	Keyboard(timeslot_t ts);
	~Keyboard();

	void run(void);
};

// all drivers need a "driver"
class Driver : public BayonneDriver
{
public:
	static Driver soundcard;	// plugin activation
	unsigned device;		// ccAudio device index
	bool active;
	Keyboard *keyboard;		// keyboard/console interface
#ifdef	HAVE_FOX
	Desktop *desktop;
#endif

	Driver();

	void startDriver(void);
	void stopDriver(void);

};

// in this driver we really only have one instance of session since we
// only use one timeslot, but the coding style is more reflective of
// drivers with multiport
class Session : public BayonneSession, public TimerPort
{
protected:
	friend class PlayThread;
	friend class RecordThread;
	friend class ToneThread;

	unsigned dev;			// ccAudio device copied from driver
	AudioDevice *ad;		// staged for delete in clraudio

public:
	Session(timeslot_t ts, unsigned sc);
	~Session();

	// core timer virtuals all port session objects need to define
	timeout_t getRemaining(void);
	timeout_t getToneFraming(void);
	void startTimer(timeout_t timer);
	void stopTimer(void);

	bool enterIdle(Event *event);
	bool enterPlay(Event *event);
	bool enterRecord(Event *event);
	bool enterTone(Event *event);

	void makeIdle(void);
	void clrAudio(void);

	inline void setDevice(AudioDevice *d)
		{ad = d;};
};

class ToneThread : public ScriptThread, public Audio, public TimerPort, public Bayonne
{
protected:
	AudioTone *tone;
	Session *session;
	Mode mode;
	unsigned devid;
	AudioDevice *dev;
	Linear buffer;
	bool resetflag;

	void run(void);

public:
	ToneThread(Session *interp, unsigned dev);
	~ToneThread();
};

class PlayThread : public ScriptThread, public Audio, public TimerPort, public Bayonne
{
protected:
	BayonneAudio *audio;
	Session *session;
	Mode mode;
	unsigned devid;
	AudioDevice *dev;
	Linear buffer;
	bool resetflag;

	void run(void);

public:
	PlayThread(Session *interp, unsigned dev);
	~PlayThread();
};

class RecordThread : public ScriptThread, public Audio, public TimerPort, public Bayonne
{
protected:
	BayonneAudio *audio;
	Session *session;
        unsigned devid;                   
	AudioDevice *dev;
	Linear buffer;
	bool resetflag;

        void run(void);

public:
        RecordThread(Session *interp, unsigned dev);
        ~RecordThread();
};

} // namespace
