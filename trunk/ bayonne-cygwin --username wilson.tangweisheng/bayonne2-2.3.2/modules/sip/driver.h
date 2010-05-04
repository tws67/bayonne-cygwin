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
#include <ccrtp/rtp.h>
#undef	HAVE_CONFIG_H
#include <eXosip2/eXosip.h>

#define BAD_REQ                 400
#define UNAUTHORIZED    401
#define FORBIDDEN               403
#define NOT_FOUND               404
#define NOT_ALLOWED             405
#define NOT_ACCEPTABLE  406
#define AUTH_REQUIRED   407
#define REQ_TIMEOUT             408
#define UNSUP_MEDIA_TYPE        415
#define TEMP_UNAVAILABLE 480
#define ADDR_INCOMPLETE 484
#define BUSY_HERE               486
#define REQ_TERMINATED  487
#define NOT_ACCEPTABLE_HERE 488 // Not Acceptable Here
// 5XX errors
#define SERVICE_UNAVAILABLE     503
// 6XX errors
#define BUSY_EVERYWHERE 600
#define DECLINE                 603

namespace sipdriver {
using namespace ost;
using namespace std;

typedef	enum
{
	START_IMMEDIATE,
	START_DELAYED,
	START_ACTIVE
}	starting_mode_t;

struct dtmf2833
{
#if __BYTE_ORDER == __BIG_ENDIAN
        unsigned event : 8;
        unsigned char ebit : 1;
        unsigned char rbit : 1;
        unsigned vol : 6;
        uint32 duration : 16;
#else
        unsigned event : 8;
        unsigned vol : 6;
        unsigned char rbit : 1;
        unsigned char ebit : 1;
        uint32 duration : 16;
#endif
};

class Session;

class Image : public ScriptImage
{
private:
	Image() : ScriptImage(NULL, NULL) {};

public:
	const char *dupString(const char *str);
};

class SIPTraffic : public TimerPort
{
private:
	friend class Driver;

	static SIPTraffic *first;
	SIPTraffic *next;

public:
	Bayonne::Traffic call_attempts, call_complete;
	unsigned refcount;
	volatile short active_calls;
	time_t updated;
	bool active;
	unsigned long sequence;

	enum
	{
		P_AVAILABLE = 0,
		P_BUSY,
		P_AWAY,
		P_DND,

		P_ONLINE = P_AVAILABLE
	} presence;
	
	char address[80];
	char secret[16];
};	

class Registry : public ScriptRegistry
{
public:
	Registry *next;
	SIPTraffic *traffic;
	const char *uri, *contact, *proxy;
	const char *iface;
//	const char *route;
	const char *address;
	const char *hostid;
	const char *portid;
	const char *userid, *localid, *service;
	const char *secret;
	const char *group;
	const char *type;
	const char *realm;
	const char *dtmf;
	const char *encoding;
	const char *peering;
	const char *answer;
	const char *reconnect;
	const char *caller;
	const char *hold, *transfer, *update;
	timeout_t framing, accept;
	int regid;
	unsigned short call_limit;

	bool insecure_publish;


	bool isActive(void);
	void add(ScriptImage *img);

	inline unsigned long getSequence(void)
		{return traffic->sequence;};
};
	
class SIPThread : public Thread
{
private:
	friend class Driver;

	void run(void);

	static SIPThread *first;
	SIPThread *next;

	SIPThread();
};

class Driver : public BayonneDriver, public BayonneZeroconf, public Audio, public Thread, public Assoc
{
protected:
	friend class Session;
	friend class RTPStream;
	friend class SIPThread;

	InetAddress sip_addr, rtp_addr;
	tpport_t rtp_port, sip_port;
	uint8 dtmf_negotiate, data_negotiate;
	bool info_negotiate;
	Info info;
	bool registry;
	bool exiting;
	bool dtmf_inband;
	bool data_filler;
	bool send_immediate;
	Level silence;
	unsigned jitter;
	timeout_t accept_timer;
	timeout_t audio_timer;
	starting_mode_t starting;
	static unsigned instance;

	void *getMemory(size_t size);

	void updateConfig(Keydata *keys);

public:
	static Driver sip;		// plugin activation

	Driver();

	inline tpport_t getPort(void)
		{return sip_port;};

	void startDriver(void);
	void stopDriver(void);
	void reloadDriver(void);

	SIPTraffic *getTraffic(const char *id);

	void requestOptions(eXosip_event_t *sevent);
	void requestAuth(eXosip_event_t *sevent);
	Registry *getAnonymous(ScriptImage *img, const char *hid, const char *uid);
	bool isLocalName(const char *host);
	bool getRegistry(eXosip_event_t *sevent, Registry **reg, ScriptImage *img);
	void publishRequest(eXosip_event_t *sevent, const char *target);
	void publishUpdate(eXosip_event_t *sevent, Registry *reg);
	void registerRequest(eXosip_event_t *sevent);
	const char *registerScript(ScriptImage *img, Line *line);
	const char *assignScript(ScriptImage *img, Line *line);
	void updateExpires(Registry *reg, timeout_t expires);

	Session *getCall(int callid);

	bool deregister(const char *id);
	bool reregister(const char *id, const char *uri, const char *secret, timeout_t expires);

	void setAuthentication(Registry *reg, osip_message_t *response);
	bool getAuthentication(Session *s, osip_message_t *response);
	bool isExternal(const char *ext);
	bool isReachable(const char *proxy);
	bool isRegistered(const char *ext);
	bool isAvailable(const char *ext);
	bool isAuthorized(const char *userid, const char *secret);

	void run(void);

	inline Level getSilence(void)
		{return silence;};

	unsigned getRegistration(regauth_t *data, unsigned count, const char *id);
};

class RTPStream : public SymmetricRTPSession, public AudioBase, private TimerPort, private Mutex, public Bayonne
{
private:
	friend class Session;

protected:
	Encoded pbuffer;
	Encoded buffer;
	Linear lbuffer;
	Linear silent_frame;
	Encoded silent_encoded;
	DTMFDetect *dtmf;
        Session *session;
        size_t rtpBufferSize;
        bool dropInbound;
	bool ending;
	unsigned long fcount, jitter, jsend, dtmfcount;

	struct dtmf2833 dtmfpacket;	
	unsigned lastevt;
	unsigned tonecount;
	event_t stopid;

        AudioBase *source, *sink;
	AudioTone *tone;

public:
        uint32  iBytes;
        uint32  oBytes;
        uint32  oTimestamp, pTimestamp, interval;

	RTPStream(Session *session);
	~RTPStream();

	void newSession(void);
	void endSession(void);

        void start(void);

	void put2833(struct dtmf2833 *data);
        ssize_t putBuffer(Encoded data, size_t len);
        ssize_t getBuffer(Encoded data, size_t len);

	void runActive(void);
        void run(void);
	void peerAudio(Encoded encoded, bool linear);
	void postAudio(Encoded encoded);
	void syncAudio(void);

        void setSource(AudioBase *get, timeout_t max = 0);
        void setSink(AudioBase *put, timeout_t max = 0);
	void setTone(AudioTone *tone, timeout_t max = 0);
	void set2833(struct dtmf2833 *data, timeout_t duration);

	inline bool isEnding(void)
		{return ending;};

    	bool onRTPPacketRecv(IncomingRTPPkt &pkt);
};

// in this driver we really only have one instance of session since we
// only use one timeslot, but the coding style is more reflective of
// drivers with multiport
class Session : public BayonneSession, public TimerPort, public Audio
{
protected:
	friend class Driver;
	friend class RTPStream;

	Info info;
	AudioCodec *codec;
	InetHostAddress local_address, remote_address;
	tpport_t remote_port;
	RTPStream *rtp;
	bool update_pos, rtp_flag;
	Linear peer_buffer;
	AudioCodec *peer_codec;
	uint8 dtmf_payload, data_payload;
	uint8 dtmf_saved, data_saved;
	Info info_saved;
	bool dtmf_sipinfo, sip_answered;
	bool reinvite, reconnecting, noinvite;
	const char *encoding_recall;
	timeout_t framing_recall;
	volatile bool dtmf_inband;
	volatile time_t session_timer;

	int cid, did, tid;
	unsigned long sv;

	bool peerAudio(Encoded encoded);

	void sendDTMFInfo(char digit, unsigned timeout = 160);

	void setDTMFMode(const char *mode);

	void redirect(const char *target, const char *encoding, unsigned char data, unsigned char dtmf, timeout_t framing = 0);
	void reconnect(void);

public:
	Session(timeslot_t ts);
	~Session();

	timeout_t audioFraming(void);
	const char *audioEncoding(void);
	const char *audioExtension(void);
	const char *checkAudio(bool live);

        uint8 sdpPayload(void);
	static const char *getAttributes(Audio::Encoding enc);
        const char *sdpEncoding(void);
	const char *sdpNames(void);
	void setPeering(const char *reg);

	// core timer virtuals all port session objects need to define
	timeout_t getRemaining(void);
	void startTimer(timeout_t timer);
	void stopTimer(void);

	void startRTP(void);
	
	void stopRTP(void);

	void suspendRTP(void);

	void makeIdle(void);

	void clrAudio(void);

	bool enterJoin(Event *event);

	bool enterRefer(Event *event);

	bool enterSeize(Event *event);

	bool enterRunning(Event *event);

	bool enterHangup(Event *event);

	bool enterAnswer(Event *event);

	bool enterPickup(Event *event);

	bool enterPlay(Event *event);

	bool enterRecord(Event *event);

	bool enterTone(Event *event);

	bool enterReconnect(Event *event);

	bool enterXfer(Event *event);

	timeout_t getToneFraming(void);

	tpport_t getLocalPort(void);

	void sipHangup(void);

	void sipRelease(void);

	inline tpport_t getRemotePort(void)
		{return remote_port;};

	inline InetHostAddress getLocalAddress(void)
		{return local_address;};

	inline InetHostAddress getRemoteAddress(void)
		{return remote_address;};

	void setEncoding(const char *encoding, timeout_t framing);

	unsigned char peerEncoding(BayonneSession *s);
	unsigned char peerEvents(BayonneSession *s);

	void requestHold(void);
	void requestJoin(void);
};

}

