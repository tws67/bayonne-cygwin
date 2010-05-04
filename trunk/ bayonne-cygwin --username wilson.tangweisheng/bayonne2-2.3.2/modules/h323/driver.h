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
//
// As a special exception to the GNU General Public License, permission is
// granted for additional uses of the text contained in its release
// of Bayonne as noted here.
//
// This exception is that permission is hereby granted to link Bayonne 2
// with the OpenH323 and Pwlib libraries, and distribute the combination, 
// without applying the requirements of the GNU GPL to the OpenH323
// and pwd libraries as long as you do follow the requirements of the 
// GNU GPL for all the rest of the software thus combined.
//
// This exception does not however invalidate any other reasons why
// the resulting executable file might be covered by the GNU General
// public license or invalidate the licensing requirements of any
// other component or library.

#include "private.h"
#include "bayonne.h"
#include <cc++/slog.h>

#if __BYTE_ORDER == __LITTLE_ENDIAN
  #define PBYTE_ORDER PLITTLE_ENDIAN
#else
  #define PBYTE_ORDER PBIG_ENDIAN
#endif

#define P_USE_PRAGRMA
#define PHAS_TEMPLATES
#define P_PLATFORM_HAS_THREADS
#define P_PTHREADS 1
#define P_HAS_SEMAPHORES 1
#define NDEBUG

#include <ptlib.h>        
#include <h323.h>
#include <h323pdu.h>               

#include <ptclib/pwavfile.h>
#include <ptclib/delaychan.h> 

#define NDEBUG  

#define AUDIO_TRANSMIT  0
#define AUDIO_RECEIVE   1
#define AUDIO_BOTH      2
#define AUDIO_NONE      3

namespace h323driver {
using namespace ost;
using namespace std;

class Driver;
class Session;

struct _frame
{
	struct _frame *next;
	Audio::Sample data[CCXX_EMPTY];
};

class Gatekeeper : public H323Gatekeeper, public Bayonne
{
        PCLASSINFO(Gatekeeper, H323Gatekeeper);
private:
        static bool SendTPKT(PTCPSocket *sender, const PBYTEArray &buf);
        static bool ForwardMesg(PTCPSocket *receiver, PTCPSocket *sender);
	static void DoForwarding(PTCPSocket *incomingTCP, PTCPSocket *outgoingTCP);

protected:
        virtual void StopDetecting();
        bool SendInfo(int state);

        class DetectIncomingCallThread : public PThread
        {
                PCLASSINFO(DetectIncomingCallThread, PThread);
        public:
                DetectIncomingCallThread(Gatekeeper *gk)
        : PThread(1000, NoAutoDeleteThread), gatekeeper(gk) { Resume(); };
                void Main() { gatekeeper->DetectIncomingCall(); };
        private:
                Gatekeeper *gatekeeper;
        };

        bool isMakeRequestCalled;
        DetectIncomingCallThread *detectorThread;
        PTCPSocket *incomingTCP, *outgoingTCP;
        PMutex threadMutex, socketMutex;
        PIPSocket::Address gkip;
        WORD gkport;
        bool isDetecting;

public:
        Gatekeeper(H323EndPoint &ep, H323Transport *trans);
        ~Gatekeeper();

	virtual BOOL OnReceiveRegistrationConfirm(const H225_RegistrationConfirm &rcf);
	virtual BOOL OnReceiveUnregistrationRequest(const H225_UnregistrationRequest &);
	virtual void OnSendRegistrationRequest(H225_RegistrationRequest &);
	virtual void OnSendUnregistrationRequest(H225_UnregistrationRequest &);

        virtual BOOL MakeRequest(Request &);
        virtual void DetectIncomingCall();
};

class Endpoint : public H323EndPoint, public Bayonne
{
	PCLASSINFO(Endpoint, H323EndPoint);
protected:
	void SetCapabilities(void);

	unsigned sessions;
	H323ListenerTCP *listener;
	H323Gatekeeper *gatekeeper;

public:
	Endpoint();
	~Endpoint();

	BOOL Initialise();

	virtual H323Connection *CreateConnection(unsigned callRef, void *userData, H323Transport *transport, H323SignalPDU *setupPDU);
	virtual void OnConnectionCleared(H323Connection & connection, const PString & token);
	virtual void OnConnectionEstablished(H323Connection &connection, const PString &token);
 
	virtual void SetEndpointTypeInfo(H225_EndpointType &info) const;
	virtual void SetVendorIdentifierInfo(H225_VendorIdentifier &info) const;
	virtual void SetH221NonStandardInfo(H225_H221NonStandard &info) const;
	virtual H323Gatekeeper *CreateGatekeeper(H323Transport *);

	virtual BOOL OnConnectionForwarded(H323Connection &connection, const PString & forwardParty, const H323SignalPDU &);

	bool MakeCall(const PString & dest, PString & token, unsigned int *callRef, H323Capability *cap, char *callerId, void *userData);

	const char *callToken;
};

class Channel : public PChannel, public Mutex, public Bayonne, public Audio
{
	PCLASSINFO(Channel, PChannel);

protected:
	Session *session;
	Encoded tbuf, rbuf;
	unsigned tsize, rsize;
	AudioCodec *tcodec, *rcodec;
	AudioBase *transmit, *receive;
	bool stopTransmit, stopReceive;
	BOOL closed;
	PAdaptiveDelay readDelay, writeDelay;
	unsigned long rcount;

public:
	Channel(Session *parent);
	~Channel();

	DTMFDetect *dtmf;

	BOOL Read(void *buf, PINDEX len);
	BOOL Write(const void *buf, PINDEX len);
	BOOL Close();
	BOOL IsOpen();

	BOOL hasTransmit(void);
	BOOL hasReceive(void);

	void attachTransmitter(AudioBase *s, AudioCodec *c);
	void attachReceiver(AudioBase *s, AudioCodec *c, timeout_t duration);
	void stop(int channelType);
};

class Connection : public H323Connection, public Bayonne
{
	PCLASSINFO(Connection, H323Connection);
	friend class Session;
	friend class Endpoint;
	friend class Driver;

protected:
	Endpoint *endpoint;
	H323AudioCodec *outCodec, *inCodec;
	Channel *channel;
	Session *session;
	bool isForwarding;
	bool transmitUp, receiveUp;

public:
	Connection(Endpoint &ep, unsigned ref, Session *s);
	~Connection();

	virtual BOOL OnIncomingCall(const H323SignalPDU &, H323SignalPDU &);
	virtual H323Connection::AnswerCallResponse OnAnswerCall(const PString &, const H323SignalPDU &, H323SignalPDU &);
	virtual void OnEstablished();
	virtual void OnCleared();
	virtual BOOL OpenAudioChannel(BOOL isEncoding, unsigned bufferSize, H323AudioCodec &codec);
	virtual BOOL OnStartLogicalChannel(H323Channel &);
	virtual BOOL OnSendSignalSetup(H323SignalPDU &);
	virtual void OnUserInputString(const PString &);
	virtual void OnUserInputTone(char tone, unsigned duration, unsigned channel, unsigned rtpts);
	virtual void CleanUpOnCallEnd();

	void muteTransmit(BOOL mute);
	void muteReceive(BOOL mute);
	void setForward(void);
};

class Driver : public BayonneDriver
{
        //PWLib entry point
        class PWLibProcess : public PProcess
        {
                PCLASSINFO(PWLibProcess, PProcess)

        PWLibProcess() : PProcess("GNU", "Bayonne", 2, 0, ReleaseCode, 0)
                {
                };

                public:
                        void Main() { }

        } pwlibProcess;

protected:
        H323ListenerTCP *listener;

public:
	static Driver h323;		// plugin activation
	static Endpoint endpoint;

	Driver();

	void startDriver(void);
	void stopDriver(void);

        bool getBool(const char *name);
};

	
// in this driver we really only have one instance of session since we
// only use one timeslot, but the coding style is more reflective of
// drivers with multiport
class Session : public BayonneSession, public TimerPort, public Audio
{
private:
	friend class Endpoint;
	friend class Channel;

	Connection *conn;
	PString callToken;
	bool update_pos;
	int channelType;
	
	struct _frame *frame_first, *frame_last;	

public:
	Session(timeslot_t ts);
	~Session();

	bool peerLinear(void);

        void attachConnection(Connection *connection);
        Connection *attachConnection(unsigned ref);

	// core timer virtuals all port session objects need to define
	timeout_t getRemaining(void);
	void startTimer(timeout_t timer);
	void stopTimer(void);

	bool lockConnection(void);
	void releaseConnection(void);
	bool answerCall(void);
	void dropCall(void);
	void makeIdle(void);
	void clrAudio(void);

	bool enterPickup(Event *event);
	bool enterRinging(Event *event);
	bool enterRelease(Event *event);
	bool enterHangup(Event *event);
	bool enterPlay(Event *event);
	bool enterRecord(Event *event);

	bool peerAudio(Audio::Encoded encoded);
};

} // end namespace

