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
// As a special exception, you may use this file as part of a free software
// library without restriction.  Specifically, if other files instantiate
// templates or use macros or inline functions from this file, or you compile
// this file and link it with other files to produce an executable, this
// file does not by itself cause the resulting executable to be covered by
// the GNU General Public License.  This exception does not however
// invalidate any other reasons why the executable file might be covered by
// the GNU General Public License.
//
// This exception applies only to the code released under the name GNU
// ccScript.  If you copy code from other releases into a copy of GNU
// ccScript, as the General Public License permits, the exception does
// not apply to the code that you add in this way.  To avoid misleading
// anyone as to the status of such modified files, you must delete
// this exception notice from them.
//
// If you write modifications of your own for GNU ccScript, it is your choice
// whether to permit this exception to apply to your modifications.
// If you do not wish that, delete this exception notice.
//

#ifndef	CCXX_BAYONNE_H_
#define	CCXX_BAYONNE_H_

#ifndef	CCXX_SCRIPT3_H_
#include <cc++/script3.h>
#endif

#ifndef	CCXX_CCAUDIO2_H_
#include <cc++/audio2.h>
#endif

#ifndef	CCXX_SOCKET_H_
#include <cc++/socket.h>
#endif

#define	BAYONNE_RELEASE	1		// release check sequence
#define	NO_TIMESLOT	0xffff
#define	MAX_DTMF	32
#define	MAX_LIST	256
#define	MAX_LIBINPUT	256
#define	MAX_PATHNAME	256
#define	MIN_AUDIOFEED	(120 * 8)	// 120 millisecond starting
#define	MAX_AUDIOFEED	(600 * 8)	// 600 millisecond buffer
#define	RPC_MAX_PARAMS	96

#ifdef	WIN32
#define	PFD_INVALID	INVALID_HANDLE_VALUE
#else
#define	PFD_INVALID	-1
#endif

#if SCRIPT_RIPPLE_LEVEL < 2
#error "ccscript 3 0.8.0 or later required"
#endif

#ifdef	DEBUG
#define	SLOG_DEBUG(x, ...)	slog.debug(x, __VA_ARGS__)
#else
#define	SLOG_DEBUG(x, ...)
#endif

namespace ost {

class __EXPORT BayonneMsgport;
class __EXPORT BayonneDriver;
class __EXPORT BayonneSession;
class __EXPORT BayonneSpan;
class __EXPORT BayonneService;
class __EXPORT BayonneTranslator;
class __EXPORT BayonneRPC;
class __EXPORT ScriptEngine;

/**
 * Streaming buffer for audio, to be used in bgm audio sources.
 *
 * @author David Sugar <dyfet@gnutelephony.org>
 * @short audio buffering
 */
class __EXPORT StreamingBuffer : public Audio
{
private:
	static StreamingBuffer *first;
	StreamingBuffer *next;
	const char *id;
	Rate rate;

protected:
	unsigned long position, count;
	Linear data;

	void cleanup();

	StreamingBuffer(const char *id, timeout_t size = 600, Rate rate = rate8khz);
	virtual ~StreamingBuffer();

    virtual Linear putBuffer(timeout_t duration);
	virtual void clearBuffer(timeout_t duration);

public:
	/**
	 * Find a streaming feed by identifier.
	 *
	 * @return pointer to feed or NULL.
	 * @param identifer to search for.
	 * @param sample rate to use.
	 */
	static StreamingBuffer *get(const char *id, Rate rate);

	/**
	 * Check if streaming source is active.	
	 *
	 * @return true if active.
	 */
	virtual bool isActive(void);

	/**
	 * Get position marker we use in audio consumer.
	 *
	 * @param framing size we will use.
	 * @return consumer position.
	 */
	virtual unsigned long getPosition(timeout_t framing);

	/**
	 * Used by consumer to get a linear buffer of audio data.
	 *
	 * @return data buffer of samples.
	 * @param consumer position.
	 * @param timeout of frame for updating position.
	 */
	virtual Linear getBuffer(unsigned long *mark, timeout_t duration);
};
	

/**
 * Bayonnne specific static keydata class.  This is used for configuration
 * keys which cannot be reloaded at runtime.
 *
 * @author David Sugar <dyfet@gnutelephony.org>
 * @short Static Keydata
 */
class __EXPORT StaticKeydata : public Keydata
{
public:
	StaticKeydata(const char *path, Keydata::Define *defkeys = NULL, const char *homepath = NULL);

	inline const char *getString(const char *id)
		{return getLast(id);};

	inline long getValue(const char *id)
		{return getLong(id);};

	inline bool getBoolean(const char *id)
		{return getBool(id);};
};

/**
 * Bayonne specific dynamic keydata class.  This class is used for
 * keydata items which can be reloaded from the config file during
 * runtime.  The normal Bayonne "reload" operatio will be used for
 * this purpose.
 *
 * @author David Sugar <dyfet@gnutelephony.org>
 * @short Dynamically reloadable key data class.
 */
class __EXPORT DynamicKeydata : public ThreadLock
{
private:
	friend class __EXPORT BayonneConfig;
	friend class __EXPORT ReconfigKeydata;
	static DynamicKeydata *firstConfig;
	DynamicKeydata *nextConfig;
	const char *keypath;
	const char *homepath;
	Keydata *keys;
	Keydata::Define *defkeys;
	void loadConfig(void);

protected:
	virtual void updateConfig(Keydata *keydata);

public:
	DynamicKeydata(const char *keypath, Keydata::Define *def = NULL, const char *homepath = NULL);

	const char *getString(const char *key, char *buf, size_t size);
	long getValue(const char *key);
	bool isKey(const char *key);
	bool getBoolean(const char *key);

	static void reload(void);
};

/**
 * Bayonne specific reloaded keydata class.  This class is used for
 * keydata items which can be reloaded from the config file during
 * runtime while using keydata base for core compatibility and defaults.
 *
 * @author David Sugar <dyfet@gnutelephony.org>
 * @short Dynamically reloadable key data class.
 */
class __EXPORT ReconfigKeydata : public StaticKeydata, protected DynamicKeydata
{
protected:
	// use only in updateConfig method...
	const char *updatedString(const char *id);
	long updatedValue(const char *id);
	timeout_t updatedSecTimer(const char *id);
	timeout_t updatedMsecTimer(const char *id);
	bool updatedBoolean(const char *id);

public:
	inline const char *getInitial(const char *id)
		{return StaticKeydata::getString(id);};
	inline void setInitial(const char *id, const char *val)
		{StaticKeydata::setValue(id, val);};

	ReconfigKeydata(const char *keypath, Keydata::Define *def = NULL);

	const char *getString(const char *key, char *buf, size_t size);
	timeout_t getSecTimer(const char *key);
	timeout_t getMsecTimer(const char *key);
	long getValue(const char *key);
	bool isKey(const char *key);
	bool getBoolean(const char *key);
};
	  
/**
 * Generic Bayonne master class to reference various useful data types
 * and core static members used for locating resources found in libbayonne.
 *
 * @author David Sugar <dyfet@gnutelephony.org>
 * @short Master bayonne library class.
 */
class __EXPORT Bayonne : public Script
{
private:
	static SOCKET trap_so4;
	static unsigned trap_count4;
	static struct sockaddr_in trap_addr4[8];

#ifdef	CCXX_IPV6
	static SOCKET trap_so6;
	static unsigned trap_count6;
	static struct sockaddr_in6 trap_addr6[8];
#endif

public:
	static char dtmf_keymap[256];

	static void snmptrap(unsigned id, const char *descr = NULL);

#ifdef	WIN32
	typedef	WORD	timeslot_t;
	typedef	DWORD	rpcint_t;
#else
	typedef uint16_t timeslot_t;
	typedef	int32_t	rpcint_t;
#endif
	typedef	rpcint_t rpcbool_t;

protected:
	static BayonneSession **timeslots;
	static ScriptImage **localimages;
	static char *status;
	static ScriptCommand *server;
	static unsigned ts_trk;
	static unsigned ts_ext;
	static timeslot_t ts_limit;
	static timeslot_t ts_count;
	static timeslot_t ts_used;
	static std::ostream *logging;
	static const char *path_prompts;
	static const char *path_tmpfs;
	static const char *path_tmp;
	static unsigned idle_count;
	static unsigned idle_limit;
	static bool shutdown_flag;
	static char sla[64];
	static time_t start_time;
	static time_t reload_time;

public:
	static timeout_t step_timer;
	static timeout_t reset_timer;
	static timeout_t exec_timer;
	static unsigned compile_count;
	static volatile bool image_loaded;

	static BayonneTranslator *init_translator;
	static const char *init_voicelib;
	static const char *trap_community;
	static AtomicCounter libexec_count;

	/**
	 * Telephony endpoint interface identifiers.
	 */ 
	typedef enum
	{
		IF_PSTN,
		IF_SPAN,
		IF_ISDN,
		IF_SS7,
		IF_INET,
		IF_NONE,
		IF_POTS=IF_PSTN
	}	interface_t;

	/**
	 * Type of call session being processed.
	 */
	typedef enum
	{
		NONE,		/* idle */
		INCOMING,	/* inbound call */
		OUTGOING,	/* outbound call */
		PICKUP,		/* station pickup */
		FORWARDED,	/* inbound forwarded call */
		RECALL,		/* inbound hold recall */
		DIRECT,		/* node/endpoint direct */
		RINGING,	/* ringing incoming join */
		VIRTUAL		/* virtual channel driver */
	}	calltype_t;

	/**
	 * Type of bridge used for joining ports.
	 */
	typedef	enum
	{
		BR_TDM,
		BR_INET,
		BR_SOFT,
		BR_GATE,
		BR_NONE
	}	bridge_t;

	/**
	 * Call processing states offered in core library.  This list
	 * must be ordered to match the entries in the state table
	 * (statetab).
	 */ 
	typedef enum
	{
		STATE_INITIAL = 0,
		STATE_IDLE,
		STATE_RESET,
		STATE_RELEASE,
		STATE_BUSY,
		STATE_DOWN,
		STATE_RING,
		STATE_PICKUP,
		STATE_SEIZE,
		STATE_ANSWER,
		STATE_STEP,
		STATE_EXEC,
		STATE_THREAD,
		STATE_CLEAR,
		STATE_INKEY,
		STATE_INPUT,
		STATE_READ,
		STATE_COLLECT,
		STATE_DIAL,
		STATE_XFER,
		STATE_REFER,
		STATE_HOLD,
		STATE_RECALL,
		STATE_TONE,
		STATE_DTMF,
		STATE_PLAY,
		STATE_RECORD,
		STATE_JOIN,
		STATE_WAIT,
		STATE_CALLING,
		STATE_CONNECT,
		STATE_RECONNECT,
		STATE_HUNTING,
		STATE_SLEEP,
		STATE_START,
		STATE_HANGUP,
		STATE_LIBRESET,
		STATE_WAITKEY,
		STATE_LIBWAIT,
		STATE_IRESET,
		STATE_FINAL,

		STATE_SUSPEND = STATE_DOWN,
		STATE_STANDBY = STATE_DOWN,
		STATE_LIBEXEC = STATE_EXEC,
		STATE_RINGING = STATE_RING,
		STATE_RUNNING = STATE_STEP,
		STATE_THREADING = STATE_THREAD
	} state_t;

	/**
	 * Signaled interpreter events.  These can be masked and accessed
	 * through ^xxx handlers in the scripting language.
	 */
	typedef enum
	{
		SIGNAL_EXIT = 0,
		SIGNAL_ERROR,
		SIGNAL_TIMEOUT,
		SIGNAL_DTMF,
	
		SIGNAL_0,
		SIGNAL_1,
		SIGNAL_2,
		SIGNAL_3,
		SIGNAL_4,
		SIGNAL_5,
		SIGNAL_6,
		SIGNAL_7,
		SIGNAL_8,
		SIGNAL_9,
		SIGNAL_STAR,
		SIGNAL_POUND,
		SIGNAL_A,
		SIGNAL_OVERRIDE = SIGNAL_A,
		SIGNAL_B,
		SIGNAL_FLASH = SIGNAL_B,
		SIGNAL_C,
		SIGNAL_IMMEDIATE = SIGNAL_C,
		SIGNAL_D,
		SIGNAL_PRIORITY = SIGNAL_D,

		SIGNAL_RING,
		SIGNAL_TONE,
		SIGNAL_EVENT,
		SIGNAL_WINK,

		SIGNAL_CHILD,
		SIGNAL_FAIL,
		SIGNAL_PICKUP,
		SIGNAL_PART,

		SIGNAL_INVALID,
		SIGNAL_PARENT,
		SIGNAL_WAIT,

		SIGNAL_HANGUP = SIGNAL_EXIT
	} signal_t;

	/**
	 * Primary event identifiers.  These are the events that can
	 * be passed into the Bayonne state machine.  They are broken
	 * into categories.
	 */
	typedef enum 
	{
		// msgport management events, server control...

		MSGPORT_WAKEUP = 0,	// wakeup a waiting message port
		MSGPORT_SHUTDOWN,	// notify msgport of server shutdown
		MSGPORT_LOGGING,	// enable driver event logging
		MSGPORT_REGISTER,	// update registrations

		// primary event identifiers

		ENTER_STATE = 100,	// newly entered state handler
		EXIT_STATE,
		EXIT_THREAD,		// thread death notify
		EXIT_TIMER,		// timer notified exit
		EXIT_PARTING,		// exit a join...
		NULL_EVENT,		// generic null event pusher
		ERROR_STATE,		// generic enter with error
		ENTER_HUNTING,
		EXIT_HUNTING,
		ENTER_RECONNECT,
		EXIT_RECONNECT,
		RECALL_RECONNECT,
		EXIT_SCRIPT,
		STEP_SCRIPT,

		// primary session control events

		START_DIRECT = 200,	// start a script, directed
		START_INCOMING,		// start an incoming script
		START_OUTGOING,		// start selected script, outbound
		START_RECALL,		// start a recalled call
		START_FORWARDED,	// start a forwarded call
		START_RINGING,		// start a ringing call
		START_HUNTING,		// start a hunting call
		START_REFER,		// start a refer session
		STOP_SCRIPT,
		STOP_DISCONNECT,
		STOP_PARENT,
		CANCEL_CHILD,
		DETACH_CHILD,
		CHILD_RUNNING,
		CHILD_FAILED,
		CHILD_INVALID,
		CHILD_EXPIRED,
		CHILD_BUSY,
		CHILD_FAX,
		CHILD_DND,
		CHILD_AWAY,
		CHILD_NOCODEC,
		CHILD_OFFLINE,

		START_SCRIPT = START_INCOMING,
		START_SELECTED = START_OUTGOING,
		START_TRANSFER = START_REFER,

		// libexec specific events

		ENTER_LIBEXEC = 300,
		EXIT_LIBEXEC,
		HEAD_LIBEXEC,
		ARGS_LIBEXEC,
		GOT_LIBEXEC,
		READ_LIBEXEC,
		DROP_LIBEXEC,
		STAT_LIBEXEC,
		PROMPT_LIBEXEC,
		CLEAR_LIBEXEC,
		WAIT_LIBEXEC,
		RECORD_LIBEXEC,
		REPLAY_LIBEXEC,
		RESTART_LIBEXEC,
		TONE_LIBEXEC,
		XFER_LIBEXEC,
		POST_LIBEXEC,
		ERROR_LIBEXEC,

		// primary driver events

		TIMER_EXPIRED = 400,	// trunk timer expired
		LINE_WINK,
		LINE_PICKUP,
		LINE_HANGUP,
		LINE_DISCONNECT,
		LINE_ON_HOOK,
		LINE_OFF_HOOK,
		RING_ON,
		RING_OFF,
		RING_STOP,
		LINE_CALLER_ID,
		RINGING_DID,
		DEVICE_BLOCKED,
		DEVICE_UNBLOCKED,
		DEVICE_OPEN,
		DEVICE_CLOSE,
		DSP_READY,
		RING_SYNC,

		// primary call processing events

		CALL_DETECT = 500,
		CALL_CONNECTED,
		CALL_RELEASED,
		CALL_ACCEPTED,
		CALL_ANSWERED,
		CALL_HOLD,
		CALL_HOLDING=CALL_HOLD,
		CALL_NOHOLD,
		CALL_DIGITS,
		CALL_OFFERED,
		CALL_ANI,
		CALL_ACTIVE,
		CALL_NOACTIVE,
		CALL_BILLING,
		CALL_RESTART,
		CALL_SETSTATE,
		CALL_FAILURE,
		CALL_ALERTING,
		CALL_INFO,
		CALL_BUSY,
		CALL_DIVERT,
		CALL_FACILITY,
		CALL_FRAME,
		CALL_NOTIFY,
		CALL_NSI,
		CALL_RINGING,
		CALL_DISCONNECT,
		CALL_CLEARED,
		CALL_PROCEEDING,
		RESTART_FAILED,
		RELEASE_FAILED,

		// some timeslot specific control and dial events

		START_RING = 600,
		STOP_RING,
		CLEAR_TIMESLOT,	// garbage collect
		START_FLASH,
		STOP_FLASH,
		DIAL_CONNECT,
		DIAL_TIMEOUT,
		DIAL_FAILED,
		DIAL_INVALID,
		DIAL_BUSY,
		DIAL_FAX,
		DIAL_PAM,
		DIAL_DND,
		DIAL_AWAY,
		DIAL_OFFLINE,
		DIAL_NOCODEC,

		DIAL_MACHINE = DIAL_PAM,

		// basic audio stuff

		AUDIO_IDLE = 700,
		AUDIO_ACTIVE,
		AUDIO_EXPIRED,
		INPUT_PENDING,
		OUTPUT_PENDING,
		AUDIO_BUFFER,
		TONE_IDLE,
		DTMF_KEYDOWN,
		DTMF_KEYSYNC,	/* timing sync event */
		DTMF_KEYUP,
		TONE_START,
		TONE_STOP,
		AUDIO_START,
		AUDIO_STOP,
		DTMF_GENDOWN,	/* peer */
		DTMF_GENUP,	/* peer */
		AUDIO_SYNC,	/* peer */
		AUDIO_RECONNECT,	/* re-invite event... */
		AUDIO_DISCONNECT,	/* audio holding re-invite */
		PEER_RECONNECT,		/* peer re-invite notice */
		PEER_DISCONNECT,	/* peer holding notice */
		PEER_REFER,		/* referred by peer */
		DTMF_GENTONE = DTMF_GENUP,

		// make modes and special timeslot stuff

		MAKE_TEST = 800,
		MAKE_BUSY,
		MAKE_IDLE,
		MAKE_DOWN,
		MAKE_UP,
		MAKE_EXPIRED,
		ENABLE_LOGGING,
		DISABLE_LOGGING,
		PART_EXPIRED,
		PART_EXITING,
		PART_DISCONNECT,
		JOIN_PEER,
		PEER_WAITING,
		RELOCATE_REQUEST,	// queued event
		RELOCATE_ACCEPT,	// hunting reply
		RELOCATE_REJECT,	// a kind of disconnect
		START_RELOCATE,		// start timeslot for relocation
		STREAM_ACTIVE,
		STREAM_PASSIVE,
		JOIN_RECALL,
		DROP_RECALL,
		DROP_REFER,

		// aliasing

		ENTER_RESUME = MAKE_UP,
		ENTER_SUSPEND = MAKE_DOWN, 

		// master control events

		SYSTEM_DOWN = 900,	

		// driver specific events and anomalies

		DRIVER_SPECIFIC = 1000	// oddball events

	} event_t;

	typedef	enum
	{
		RESULT_SUCCESS = 0,
		RESULT_TIMEOUT,
		RESULT_INVALID,
		RESULT_PENDING,
		RESULT_COMPLETE,
		RESULT_FAILED,
		RESULT_BADPATH = 254,
		RESULT_OFFLINE = 255
	}	result_t;

	typedef	struct
	{
		Line line;
		char text[MAX_LIBINPUT];
		const char *list[MAX_LIBINPUT / 2];
	}	libaudio_t;

        typedef struct
        {
                const char *remote;
                const char *userid;
                const char *type;
                const char *status;
		unsigned short active_calls, call_limit;
		unsigned long attempts_iCount, attempts_oCount;
		unsigned long complete_iCount, complete_oCount;
		time_t updated;
        }	regauth_t;

	/**
	 * The event data structure includes the event identifier and
	 * any paramaters.  Additional information is attached both to
	 * assist in debugging, and to track which timeslot a given
	 * event is being issued against when queued through a master
	 * msgport.
	 */
	typedef struct
	{
		event_t id;
		timeslot_t timeslot;
		uint16 seq;

		union
		{
			// used to invoke attach.  The image refcount
			// is assumed to already be inc'd!  If no
			// scr is passed, then uses default or search
			// vars passed in sym, value pairs
			struct
			{
				ScriptImage *img;
				Script::Name *scr;
				BayonneSession *parent;
				const char *dialing;
			}	start;

			struct
			{
				ScriptImage *img;
				Script::Name *scr;
				BayonneSession *parent;
				Line *select;
			}	hunt;

			struct
			{
				BayonneSession *current;
				BayonneSession *replace;
			}	relocate;

			struct
			{
				const char *tid;
#ifdef	WIN32
				HANDLE	pfd;
#else
				const char *fname;
#endif
				int pid, result;
			}	libexec;

			struct
			{
				const char *tid;
				const char *errmsg;
			}	liberror;

			struct
			{
				timeout_t duration;
				int digit;
			}	dtmf;

			struct
			{
				const char *err;
				const char *msg;
			}	cpa;

			struct
			{
				const char *name;
				bool exit;
			}	tone;

			struct
			{
				std::ostream *output;
				const char *logstate;
			}	debug;

			struct
			{
				const char *encoding;
				timeout_t framing;
			}	reconnect;

			const char *dialing;
			const char *name;
			const char *errmsg;
			BayonneSession *pid;
			BayonneSession *peer;
			BayonneSession *child;
			void *data;
		};

	}	Event;

	/**
	 * This is an internal ring class for synchronized ringing.
	 */
	class __EXPORT Ring
	{
	private:
		static Mutex locker;
		static Ring *free;
		Ring *next;

		Ring() {};
	public:
		BayonneDriver *driver;
		const char *ring_id;
		unsigned count;
		BayonneSession *session;
		Script::Name *script;

		static Ring *attach(BayonneDriver *d, const char *id, Ring *list);
		static void detach(Ring *list);
		static Ring *find(Ring *list, BayonneSession *s);
		static void start(Ring *list, BayonneSession *s);
	};

	/**
	 * This is a class used for collecting statistics for call 
	 * traffic measurement, such as might be used by MRTG.
	 */
	class __EXPORT Traffic
	{
	private:
		static unsigned long stamp;

	public:
		Traffic();

		static inline unsigned long getStamp(void)
			{return stamp;};

		volatile unsigned long iCount, oCount;
	};

	/**
	 * A rpc method handler.
	 */
	typedef void (*rpcmethod_t)(BayonneRPC *rpc);

	/**
	 * This is a structure used to initialize XMLRPC method tables.
	 */
	typedef struct
	{
		const char *name;
		rpcmethod_t method;
		const char *help;
		const char *signature;
	}	RPCDefine;

	/**
	 * This is a little class used to associate XMLRPC call method
	 * tables with our server.
	 */
	class __EXPORT RPCNode
	{
	private:
		friend class __EXPORT BayonneRPC;
		static RPCNode *first;
		static unsigned count;
		RPCNode *next;
		RPCDefine *methods;
		const char *prefix;

	public:
		static inline RPCNode *getFirst(void)
			{return first;};

		inline RPCNode *getNext(void)
			{return next;};

		inline const char *getPrefix(void)
			{return prefix;};

		inline RPCDefine *getMethods(void)
			{return methods;};

		static inline unsigned getCount(void)
			{return count;};

		RPCNode(const char *prefix, RPCDefine *dispatch);
	};

	/**
	 * The current state handler in effect for a given channel
	 * to receive events.  This is done by a direct method pointer
	 * for fast processing.
	 */	
	typedef bool (BayonneSession::*Handler)(Event *event);

	/**
	 * A list of each state and a description.  This is used so that
	 * named states can be presented when debugging posted events.
	 */
	typedef struct
	{
		const char *name;
		Handler handler;
		char flag;
	}	statetab;

	/**
	 * The primary state data structure.   This holds data that is setup
	 * by the interpreter and which must remain persistent for
	 * the execution of a given state.  This is composed of some
	 * common elements which exist in a session for all states, and
	 * a union of state specific data elements, all packed together.
	 */
	typedef struct
	{
		Handler handler, logstate;
		const char *name;
		timeout_t timeout;
		bool peering;
		Name *menu;
		unsigned stack;
		Line *lib;
#ifdef	WIN32
		HANDLE pfd;
#else
		int pfd;
#endif
		result_t result;
		int pid;
		libaudio_t *libaudio;
		bool refer;

		union
		{
			struct
			{
				unsigned count;
				timeout_t interval;
			}	wait;

			struct
			{
				Audio::Mode mode;
				Audio::Level level;
				timeout_t total, silence, intersilence;
				long lastnum;
				bool exitkey;
				bool compress;
				bool trigger;
				const char *pos;
				const char *exit;
				const char *menu;
				const char *note;
				union {
					const char *list[MAX_LIST];
					struct {
						const char *pathv[4];
						char path1[128];
						char path2[128];
						char meta[MAX_LIST * 2];
					};
				};
			} 	audio;
				
			struct
			{
				timeout_t interdigit;
				timeout_t lastdigit;
				const char *var;
				const char *exit;
				const char *format;
				const char *ignore;
				const char *route;
				unsigned count, size, required;
			}	input;

			struct
			{
				const char *var;
				const char *menu;
			}	inkey;

			struct
			{
				const char *sequence;
				bool flashing;
				bool dialing, exiting, hangup, dtmf, refer;
				char *syncdigit;
				timeout_t synctimer;
				timeout_t duration;
				char digits[64];
				char sessionid[16];
			}	tone;

			struct
			{
				timeout_t on, off, interdigit;
				unsigned pos;
				bool flashing;
				bool dialing;
				unsigned char digits[64];
			}	pulse;	

			struct
			{
				const char *dial;
				const char *exit;
				bool dtmf, drop, hangup, refer;
				BayonneSession *peer;
				timeout_t answer_timer, hunt_timer;
				Line *select;
				unsigned index;
				char digits[64];
			}	join;

			struct
			{
				const char *ref;
				char buf[MAX_LIST * sizeof(char *)];
			}	url;
		};
		
	}	State;
	
	/**
	 * Table of states ordered by id.
	 */
	static statetab states[];

	/**
	 * A mutex to serialize any direct console I/O operations.  Sometimes
	 * used to serialize other kinds of time insensitive requests.
	 */
	static Mutex serialize;

	/**
	 * A mutex to serialize reload requests.
	 */
	static ThreadLock reloading;

	/**
	 * master traffic counters for call attempts and call completions.
	 */
	static Traffic total_call_attempts;
	static Traffic total_call_complete;
	static volatile unsigned short total_active_calls;

	/**
	 * Allocates the maximum number of timeslots the server will use as
	 * a whole and attaches a given server to the library.
	 *
	 * @param timeslots to allocate.
	 * @param pointer to server shell.
	 */	
	static void allocate(timeslot_t timeslots, ScriptCommand *pointer = NULL, timeslot_t overdraft = 0);

	static const char *getRegistryId(const char *id);

	static BayonneDriver *getDriverTag(const char *id);

	static Audio::Encoding getEncoding(const char *cp);

	/**
	 * Allocate local script engine sessions, if needed.
	 */
	static void allocateLocal(void);

	/**
	 * Add config file entry.
	 */
	static void addConfig(const char *cfgfile);

	/**
	 * Compute md5 hashes...
	 *
	 * @param md5 output string
	 * @param string to hash
	 */
	void md5_hash(char *out, const char *source);

	/**
	 * Wait for live flag...
	 */
	static void waitLoaded(void);
	
	/**
	 * Get server uptime.
	 *
	 * @return uptime in seconds.
	 */
	static unsigned long uptime(void);

	/**
	 * Request active scripts to be recompiled from the library.
	 *
	 * @return script image that was created, or NULL.
	 */
	static ScriptCompiler *reload(void);

	/**
	 * Used to down the server from the library.
	 */
	static void down(void);

	/**
	 * Sets server service level from the library.
	 *
	 * @param service level or NULL to clear.
	 * @return true if set.
	 */
	static bool service(const char *service);

	/**
	 * Get service level.
	 *
	 * @param return service level
	 */
	static const char *getRunLevel(void);

	/**
	 * Returns a session pointer for a server timeslot.  Each
	 * server timeslot can map to a single session object.
	 *
	 * @param timeslot number in server.
	 * @return session object or NULL if timeslot empty/invalid.
	 */
	static BayonneSession *getSession(timeslot_t timeslot);

	/**
	 * Returns a local image pointer for a server timeslot.
	 *
	 * @param timeslot number in server.
	 * @return pointer to image pointer or NULL if empty/invalid.
	 */
	static ScriptImage **getLocalImage(timeslot_t timeslot);

	/**
	 * Start a dialing session.  WARNING: this function leaves
	 * the channel locked so it can be examined by the returning
	 * task.
	 *
	 * @param dialing string or uri.
	 * @param script to run or start.
	 * @param caller id for this call.
	 * @param display name for this call.
	 * @param parent to join to.
	 */
	static BayonneSession *startDialing(const char *dial, 
		const char *name, const char *caller, const char *display, 
		BayonneSession *parent = NULL, const char *manager = NULL,
		const char *secret = NULL);

	/**
	 * Returns a session pointer for a string identifier.  This can
	 * be for a transaction id, a call id, or other unique identifiers
	 * which can be mapped into a single timeslot.
	 *
	 * @param id session identifier string.
	 * @return session object or NULL if not found.
	 */
	static BayonneSession *getSid(const char *id);

	/**
	 * Returns a server timeslot number for a string identifier.
	 *
	 * @return timeslot number or invalid value.
	 * @param id for a session.
	 * @see #getSid
	 */
	static timeslot_t toTimeslot(const char *id);

	/**
	 * Return total library timeslots used (highest used).
	 *
	 * @return highest server timeslot in use.
	 */
	static inline timeslot_t getTimeslotsUsed(void)
		{return Bayonne::ts_used;};

	/**
	 * Return total timeslots allocated for the server.
	 *
	 * @return total number of timeslots, max + 1.
	 */
	static inline timeslot_t getTimeslotCount(void)
		{return Bayonne::ts_count;};	

	/**
	 * Return remaining timeslots available to allocate driver
	 * ports into.
	 *
	 * @return remaining timeslots.
	 */
	static inline timeslot_t getAvailTimeslots(void)
		{return Bayonne::ts_count - Bayonne::ts_used;};

	/**
	 * Map a state name into a handler.  Used for logging requests.  Uses
	 * the statetab.
	 *
	 * @param name of state to lookup.
	 * @return handler method for state if found.
	 */
	static Handler getState(const char *name);

	/**
	 * Convert a dtmf character into a 0-15 number reference.
	 *
	 * @param dtmf digit as ascii
	 * @return dtmf digit number.
	 */
	static int getDigit(char dtmf);

	/**
	 * Convert a dtmf digit number into it's ascii code.
	 *
	 * @param dtmf digit number.
	 * @return dtmf character code.
	 */
	static char getChar(int dtmf);

	/**
	 * A function to support pattern matching and templates for digit
	 * strings.  This is used for digit \@xxx:... entries and the route
	 * command.
	 *
	 * @param digits to use.
	 * @param match digit pattern to match against.
	 * @param partial accept match if true.
	 * @return true if digits match to pattern.
	 */
	static bool matchDigits(const char *digits, const char *match, bool partial = false);

	/**
	 * Use the current compiled script image; mark as in use.
	 *
	 * @return current script image to pass to new calls.
	 */
	static ScriptImage *useImage(void);

	/**
	 * Release a script image in use.  If no active calls are using it
	 * and it's no longer the top active image, purge from memory.
	 *
	 * @param image to compiled script from useImage.
	 */
	static void endImage(ScriptImage *image);

	/**
	 * Load a plugin module.
	 *
	 * @param path id of plugin.
	 * @return true if successful.
	 */
	static bool loadPlugin(const char *path);

        /**
         * Load a monitoring/management module.
         *
         * @param path id of plugin.
         * @return true if successful.
         */
        static bool loadMonitor(const char *path);  

	/**
	 * Load a bgm/audio processing module for continues audio.
	 *
	 * @param path id of plugin.
	 * @return true if successful.
	 */
	static bool loadAudio(const char *path);

	static void errlog(const char *level, const char *fmt, ...); 

	static bool getUserdata(void);

	static void addTrap4(const char *addr);
#ifdef	CCXX_IPV6
	static void addTrap6(const char *addr);
#endif
};

/**
 * This class is used to bind services that are to be published with zeroconf, such
 * as by the avahi module.
 *
 * @author David Sugar <dyfet@gnutelephony.org>
 * @short Bayonne zerconf binding class.
 */
class __EXPORT BayonneZeroconf
{
protected:
	static BayonneZeroconf *zeroconf_first;
	BayonneZeroconf *zeroconf_next;
	const char *zeroconf_type;
	tpport_t zeroconf_port;

public:
	typedef enum 
	{
		ZEROCONF_IPANY,
		ZEROCONF_IPV6,
		ZEROCONF_IPV4
	}	zeroconf_family_t;

protected:
	zeroconf_family_t zeroconf_family;

	BayonneZeroconf(const char *type, zeroconf_family_t family = ZEROCONF_IPANY);

public:
	/**
	 * Get the first zeroconf binding, used by zeroconf plugins.
	 *
	 * @return first zeroconf binding.
	 */
	static inline BayonneZeroconf *getFirst(void)
		{return zeroconf_first;};

	/**
	 * Get the next zeroconf binding to iterate an object list.
	 *
	 * @return next zerconf binding.
	 */
	inline BayonneZeroconf *getNext(void)
		{return zeroconf_next;};

	/**
	 * Get the binding protocol description, usually "_svc._proto".
	 *
	 * @return binding description.
	 */
	inline const char *getType(void)
		{return zeroconf_type;};

	/**
	 * Get the binding service port number.  If 0, then disabled.
	 *
	 * @return 0 or port number.
	 */
	inline tpport_t getPort(void)
		{return zeroconf_port;};

	inline zeroconf_family_t getFamily(void)
		{return zeroconf_family;};
};

/**
 * A bayonne config class, used for special purposes, especially
 * during script compiles.
 *
 * @author David Sugar <dyfet@gnutelephony.org>
 * @short Bayonne config cache for compiler.
 */
class __EXPORT BayonneConfig : public DynamicKeydata
{
private:
	static BayonneConfig *first;

	BayonneConfig *next;
	const char *id;

public:
	BayonneConfig(const char *id, Keydata::Define *def, const char *path);
	BayonneConfig(const char *id, const char *path);
	void setEnv(const char *id);
	static BayonneConfig *get(const char *id);
	static void rebuild(ScriptImage *img);
};

/**
 * A core class to support language translation services in Bayonne
 * phrasebook.
 *
 * @author David Sugar <dyfet@gnutelephony.org>
 * @short Phrasebook translation base class.
 */
class __EXPORT BayonneTranslator : public Bayonne
{
protected:
	static BayonneTranslator *first;
	BayonneTranslator *next;
	const char *id;

	static const char *getToken(BayonneSession *s, Line *l, unsigned *idx);
	static unsigned addItem(BayonneSession *s, unsigned count, const char *text);
	static const char *getLast(BayonneSession *s, unsigned count);

public:
	/**
	 * Create a translator instance for a specific language identifier.
	 * Generally iso names will be sued.  Sometimes iso sub-identifiers 
	 * may be used for nation specific versions of phrasebook.
	 *
	 * @param iso name of language/locale.
	 */
	BayonneTranslator(const char *iso);

	virtual ~BayonneTranslator();

	/**
	 * Find a translator for a given name/location.
	 *
	 * @param iso language/locale name of translator to find.
	 * @return derived translator class for translator.
	 */
	static BayonneTranslator *get(const char *name);

	/**
	 * Get first translator.
	 * @return pointer to first translator.
	 */
	static inline BayonneTranslator *getFirst(void)
		{return first;};

	/**
	 * Get next translator.
	 *
	 * @return next translator.
	 */
	inline BayonneTranslator *getNext()
		{return next;};
	
	/**
	 * Load a named translator into memory for use.  This is used
	 * by the fifo/script language command.
	 *
	 * @param iso module name to load.
	 * @return true if successful.
	 */
	static BayonneTranslator *loadTranslator(const char *iso);

	/**
	 * Translate a simple set of digits to spoken speech.
	 *
	 * @param session to save list of prompts.
	 * @param count of current prompts used.
	 * @param string to be processed.
	 * @return new count of prompts used.
	 */
	virtual unsigned digits(BayonneSession *sessiob, unsigned count, const char *string);

        /**
         * Spell out the string as individual letters.
         *
         * @param session to save list of prompts.
         * @param count of current prompts used.
         * @param string to be processed.
         * @return new count of prompts used.
         */
	virtual unsigned spell(BayonneSession *session, unsigned count, const char *string);

        /**
         * Translate an ordinal number (xxnth) to prompts.
         *
         * @param session to save list of prompts.
         * @param count of current prompts used.
         * @param string to be processed.
         * @return new count of prompts used.
         */
	virtual unsigned sayorder(BayonneSession *session, unsigned count, const char *string);

        /**
         * Translate a number to spoken speech.
         *
         * @param session to save list of prompts.
         * @param count of current prompts used.
         * @param string to be processed.
         * @return new count of prompts used.
         */
	virtual unsigned number(BayonneSession *session, unsigned count, const char *string);

	/**
	 * Translate generic numbers to spoken speech.  This is the one
	 * used by the &number rule.
	 *
	 * @param session to save list of prompts.
	 * @param count of current prompts used.
	 * @param string to be processed.
	 * @return new count of prompts used.
	 */
	virtual unsigned saynumber(BayonneSession *session, unsigned count, const char *string);

        /**
         * Translate a counting number (integer) to spoken speech.
         *
         * @param session to save list of prompts.
         * @param count of current prompts used.
         * @param string to be processed.
         * @return new count of prompts used.
         */
	virtual unsigned saycount(BayonneSession *session, unsigned count, const char *string);

        /**
         * Translate a string for time into short hours.
         *
         * @param session to save list of prompts.
         * @param count of current prompts used.
         * @param string to be processed.
         * @return new count of prompts used.
         */
        virtual unsigned sayhour(BayonneSession *session, unsigned count, const char *string);

        /**
         * Translate a string for time into speech.
         *
         * @param session to save list of prompts.
         * @param count of current prompts used.
         * @param string to be processed.
         * @return new count of prompts used.
         */
	virtual unsigned saytime(BayonneSession *session, unsigned count, const char *string);

        /**
         * Translate a string with a date into the spoken weekday.
         *
         * @param session to save list of prompts.
         * @param count of current prompts used.
         * @param string to be processed.
         * @return new count of prompts used.
         */
	virtual unsigned weekday(BayonneSession *session, unsigned count, const char *string);

	/*
	 * Translate a string with a date into spoken date without year.
         *
         * @param session to save list of prompts.
         * @param count of current prompts used.
         * @param string to be processed.
         * @return new count of prompts used.
         */
        virtual unsigned sayday(BayonneSession *session, unsigned count, const char *string);

        /**
         * Translate a string with a date into a spoken date.
         *
         * @param session to save list of prompts.
         * @param count of current prompts used.
         * @param string to be processed.
         * @return new count of prompts used.
         */
	virtual unsigned saydate(BayonneSession *session, unsigned count, const char *string);

        /**
         * Translate a logical value and speak as yes/no.
         *
         * @param session to save list of prompts.
         * @param count of current prompts used.
         * @param string to be processed.
         * @return new count of prompts used.
         */
	virtual unsigned saybool(BayonneSession *session, unsigned count, const char *string);

        /**
         * Translate and speak a phone number.
         *
         * @param session to save list of prompts.
         * @param count of current prompts used.
         * @param string to be processed.
         * @return new count of prompts used.
         */
	virtual unsigned phone(BayonneSession *session, unsigned count, const char *string);

        /**
         * Translate and speak a phone extension.
         *
         * @param session to save list of prompts.
         * @param count of current prompts used.
         * @param string to be processed.
         * @return new count of prompts used.
         */
	virtual unsigned extension(BayonneSession *session, unsigned count, const char *string);

        /**
         * Translation dispatch, processes script and invokes other methods.
         *
         * @return error message or NULL if okay
	 * @param session to process a command from.
	 * @param line of compiled script to process.
         */
	virtual const char *speak(BayonneSession *session, Line *line = NULL);

	/**
	 * Get the id string.
	 *
	 * @return id string.
	 */
	inline const char *getId(void)
		{return id;};
};


/**
 * Offers core Bayonne audio processing in a self contained class.  The
 * BayonneAudio class is used with each session object.
 *
 * @short self contained Bayonne audio processing. 
 * @author David Sugar <dyfet@gnutelephony.org>
 */
class __EXPORT BayonneAudio : public AudioStream, public Bayonne
{
protected:
	char filename[MAX_PATHNAME];
	const char **list;
	char *getContinuation(void);

public:
	/**
	 * Current tone object to use for generation of audio tones,
	 * dtmf dialing sequences, etc.
	 */
	AudioTone *tone;

	/**
	 * Current language translator in effect for the current set of
	 * autio prompts.
	 */
	BayonneTranslator *translator;

	/**
	 * Alternate voicelib construct.
	 */
	char vlib[60];

	const char *extension, *voicelib, *libext, *prefixdir, *offset;
	Encoding encoding;
	timeout_t framing;
	char var_position[14];

	/**
	 * Initialize instance of audio.
	 */
	BayonneAudio();

	/**
	 * Convert a prompt identifier into a complete audio file pathname.
	 *
	 * @return pointer to fully qualified file path or NULL if invalid.
	 * @param name of prompt requested.
	 * @param write path required if true.
	 */
        const char *getFilename(const char *name, bool write = false);

	/**
	 * Clear open files and other data structures from previous
	 * audio processing operations.
	 */
	void cleanup(void);
	
	/**
	 * Open a sequence of audio prompts for playback.	
	 *
	 * @param list of prompts to open.
	 * @param mode for playback file processing of list.
	 */
	void play(const char **list, Mode mode = modeRead);

	/**
	 * Open an audio prompt for recording.
	 *
	 * @param name of prompt to open.
	 * @param mode whether to create or use pre-existing recording.
	 * @param annotation to save in file if supported by format used.
	 */
	void record(const char *name, Mode mode = modeCreate, const char *annotation = NULL);

	/**
	 * Check if a voice library is available.
	 *
	 * @return voice library id or NULL if invalid.
	 * @param iso name of library to request.
	 */
	const char *getVoicelib(const char *iso);

	/**
	 * Get audio codec used.
	 *
	 * @return audio codec.
	 */
	inline AudioCodec *getCodec(void)
		{return codec;};
};		

/**
 * Bayonne Msgports are used to queue and post session events which
 * normally have to be passed through another thread context.  This
 * can happen for a thread termination event, for example, since the
 * thread terminating must be joined from another thread.  Some drivers
 * use session specific msgports to process all channel events.
 *
 * @author David Sugar <dyfet@gnutelephony.org>
 * @short Msgport event queing and dispatch.
 */
class __EXPORT BayonneMsgport : public Thread, public Buffer, public Bayonne
{
public:
	/**
	 * Destroy a msgport.
	 */
	virtual ~BayonneMsgport();

	/**
	 * Create a message port and optionally bind it to a given driver.
	 *
	 * @param driver to bind msgport to.
	 */
	BayonneMsgport(BayonneDriver *driver);

	/**
	 * Request retiming.  This is used for msgports that are per
	 * session to get the session to be retimed after an event has
	 * been directly posted outside the msgport.
	 */
        void update(void); 

	/**
	 * Initialize msgport, determine which sessions it will perform
	 * timing for based on the driver it is bound to.
	 */
	void initial(void);

protected:
	BayonneDriver *msgdriver;
	Event *msglist;
	unsigned msgsize, msghead, msgtail;
	timeslot_t tsfirst, tscount;
	char msgname[16];

	/**
	 * Send shutdown event to the msgport.
	 */
	void shutdown(void);

	/**
	 * Determine sleep time to schedule for waiting, unless an
	 * update occurs to force rescheduling.
	 *
	 * @return shortest timeout based on session timers.
	 * @param event to pass when timeout occurs.
	 */
	virtual timeout_t getTimeout(Event *event);
	
	void run(void);

        size_t onWait(void *buf);
        size_t onPost(void *buf);
        size_t onPeek(void *buf);
};

/**
 * The principle driver node for a given collection of spans and sessions
 * of a given Bayonne driver family type.
 *
 * @author David Sugar <dyfet@gnutelephony.org>
 * @short Bayonne driver node class.
 */
class __EXPORT BayonneDriver : public Bayonne, public ReconfigKeydata, public Mutex
{
protected:
	friend class __EXPORT BayonneSession;
	friend class __EXPORT BayonneMsgport;
	static BayonneDriver *firstDriver;
	static BayonneDriver *lastDriver;
	static BayonneDriver *trunkDriver;
	static BayonneDriver *protoDriver;
	static Semaphore oink;	// the pig has been kicked!
	static bool protocols;

	BayonneSession *firstIdle, *lastIdle, *highIdle;
	BayonneMsgport *msgport;
	BayonneDriver *nextDriver;
	const char *name;
	timeslot_t timeslot, count;
	unsigned avail;
	unsigned span, spans;	
	bool running;
	static bool stopping;
	std::ostream *logevents;

	int audio_priority;
	size_t audio_stack;
	Audio::Level audio_level;

	timeout_t pickup_timer, hangup_timer, seize_timer, ring_timer, hunt_timer;
	timeout_t reset_timer, release_timer, flash_timer, interdigit_timer;
	unsigned answer_count;

	/**
	 * Virtual to notify driver that a server image reload is
	 * in progress.
	 */
	virtual void reloadDriver(void);

	/**
	 * Virtual to override method for activating the driver and
	 * creating all session and span objects associated with it.
	 */
	virtual void startDriver(void);

	/**
	 * Virtual to override method for clean shutdown of the driver.
	 */
	virtual void stopDriver(void);

	/**
	 * Relist idle drivers on high idle list, for drivers which do
	 * highwater marking allocation.
	 */
	void relistIdle(void);

public:
	/**
	 * Determine if user id and secret is authorized for this
	 * driver subsystem (registry).
	 *
	 * @param userid to check
	 * @param secret to check
	 * @return true if authorized
	 */
	virtual bool isAuthorized(const char *userid, const char *secret);

	virtual bool deregister(const char *id);
	virtual bool reregister(const char *id, const char *uri, const char *secret, timeout_t expires);

	Traffic call_attempts, call_complete;
	volatile unsigned short active_calls;

	/**
	 * Create a driver instance.
	 *
	 * @param pairs of default keyword entries for config.
	 * @param key name of config key.
	 * @param id string of driver.
	 * @param whether virtual driver of some sort or real.
	 */
	BayonneDriver(Keydata::Define *pairs, const char *key, const char *id, bool virt = false);

	/**
	 * Destroy driver instance.
	 */
	~BayonneDriver();

	/**
	 * Return flag for protocols active.
	 */
	static inline bool useProtocols(void)
		{return protocols;}

	/**
	 * Return is stopping flag.
	 */
	static bool isStopping(void)
		{return stopping;};

	/**
	 * Return primary trunk driver, if driver trunking...
	 */
	static inline BayonneDriver *getTrunking(void)
		{return trunkDriver;}

	/**
	 * Return the first loaded driver.
	 */
	static inline BayonneDriver *getPrimary(void)
		{return firstDriver;}

	/**
	 * Authorize a user and return associated driver.
	 *
	 * @param userid to authorize.
	 * @param secret to use.
	 * @return driver authorized under or NULL.
	 */
	static BayonneDriver *authorize(const char *userid, const char *secret);
	
	/**
	 * Get next driver...
	 */
	inline BayonneDriver *getNext(void)
		{return nextDriver;};

	static inline BayonneDriver *getRoot(void)
		{return firstDriver;};

	/**
	 * Return primary protocol driver...
	 */
	static inline BayonneDriver *getProtocol(void)
		{return protoDriver;}

	/**
	 * Get longest idle session to active for call processing.
	 *
	 * @return handle to longest idle session, if none idle, NULL.
	 */
	BayonneSession *getIdle(void);

	/**
	 * Suspend a driver.
	 *
	 * @return true if successful.
	 */
	virtual bool suspend(void);

	/**
	 * Resume a driver.
	 *
	 * @return true if successful.
	 */
	virtual bool resume(void);

	/**
	 * Re-register.
	 */
	virtual void reregister(void);

	/**
	 * Process driver protocol specific proxy registration requests.
	 *
	 * @return error message if invalid request, NULL if ok.
	 * @param image of script being compiled.
	 * @param line record of "register" command.
	 */
        virtual const char *registerScript(ScriptImage *image, Line *line);

	/**
	 * Process driver specific assign requests.
	 *
	 * @return error message if invalid request, NULL if ok.
	 * @param image of script being compiled.
	 * @param line record of "assign" command.
	 */
	virtual const char *assignScript(ScriptImage *image, Line *line);

	/**
	 * Find and return driver object from id name.
	 *
	 * @param id driver name.
	 * @return associated driver node.
	 */
	static BayonneDriver *get(const char *id);

	/**
	 * Load a bayonne driver into memory.
	 *
	 * @param id driver name to load.
	 * @return NULL or pointer to loaded driver.
	 */
	static BayonneDriver *loadDriver(const char *id);

	/**
	 * Load a bayonne trunking driver into memory, set protocols.
	 *
	 * @return NULL or pointer to loaded driver.
	 * @param id of trunking driver to load.
	 */
	static BayonneDriver *loadTrunking(const char *id);

	/**
	 * Load a protocol driver into memory, set timeslots.
	 *
	 * @return NULL or pointer to loaded protocol.
	 * @param id of protocol driver to load.
	 * @param timeslots of protocol.
	 */
	static BayonneDriver *loadProtocol(const char *id, unsigned timeslots = 0);

	/**
	 * Get list of driver names into string array.
	 *
	 * @param items array to save in.
	 * @param max count of elements available.
	 * @return number of drivers.
	 */
	static unsigned list(char **items, unsigned max);

	/**
	 * Start all loaded drivers.
	 */
	static void start(void);
	
	/**
	 * Stop all loaded drivers.
	 */
	static void stop(void);

	/**
	 * Notify all drivers about reload.
	 */
	static void reload(void);

	/**
	 * Add session to driver idle list for getIdle, usually during
	 * stateIdle.
	 *
	 * @param session being added.
	 */ 
	static void add(BayonneSession *session);

	/**
	 * Remove session from driver idle list if still present.  Usually
	 * when changing from idle to an active state.
	 *
	 * @param session being removed.
	 */
	static void del(BayonneSession *session);

	/**
	 * Get first server timeslot this driver uses.
	 *
	 * @return first server timeslot for driver.
	 */
	inline timeslot_t getFirst(void)
		{return timeslot;};

	/**
	 * Get the total number of timeslots this driver uses.
	 *
	 * @return total timeslots for driver.
	 */
	inline timeslot_t getCount(void)
		{return count;};

	/**
	 * Get the first span id used.
	 *
	 * @return span id.
	 */
	inline unsigned getSpanFirst(void)
		{return span;};

	/**
	 * Get the number of span objects used by driver.
	 *
	 * @return span count.
	 */
	inline unsigned getSpansUsed(void)
		{return spans;};

	/**
	 * Get the name of the driver.
	 *
	 * @return name of driver.
	 */
	inline const char *getName(void)
		{return name;};

	/**
	 * Get the reset timer for this driver when resetting a thread in
	 * the step state.
	 *
	 * @return reset timer in milliseconds.
	 */
	inline timeout_t getResetTimer(void)
		{return reset_timer;};

	/**
	 * Get the release timer when releasing a trunk.
 	 *
	 * @return release timer in milliseconds.
	 */
	inline timeout_t getReleaseTimer(void)
		{return release_timer;};

	/**
	 * Get the hangup timer for hang time before going idle.
	 *
	 * @return hangup timer in milliseconds.
	 */
	inline timeout_t getHangupTimer(void)
		{return hangup_timer;};

	/**
	 * Get the pickup timer to wait for channel pickup.
	 *
	 * @return pickup timer in milliseconds.
	 */
	inline timeout_t getPickupTimer(void)
		{return pickup_timer;};

	/**
	 * Get the sieze time to wait for dialtone on outbound call.
	 *
	 * @return seize timer in milliseconds.
	 */
	inline timeout_t getSeizeTimer(void)
		{return seize_timer;};

	/**
	 * Get the hunting timer.
	 *
	 * @return hunt timer in milliseconds.
	 */
	inline timeout_t getHuntTimer(void)
		{return hunt_timer;};

	/**
	 * Get the programmed flash timer to signal trunk flash.
	 *
	 * @return flash timer in milliseconds.
	 */
	inline timeout_t getFlashTimer(void)
		{return flash_timer;};

	/**
	 * Get default dtmf interdigit timer to use.
	 *
	 * @return interdigit timer in milliseconds.
	 */
	inline timeout_t getInterdigit(void)
		{return interdigit_timer;};

	/**
	 * Get the timer to wait for next ring before deciding a call
	 * has dissapeared.  Used when set to answer on nth ring.
	 *
	 * @return ring timer in milliseconds.
	 * #see getAnswerCount.
	 */
	inline timeout_t getRingTimer(void)
		{return ring_timer;};

	/**
	 * Get the number of rings to wait before answering.
	 *
	 * @return number of rings before answer.
	 */
	inline unsigned getAnswerCount(void)
		{return answer_count;};

	/**
	 * Get the nth span object associated with this driver.
	 *
	 * @param id of nth span to return.
	 * @return span object or NULL if past limit/no spans.
	 */
	BayonneSpan *getSpan(unsigned id);

	/**
	 * Get the session associated with the nth timeslot for this
	 * driver.
	 *
	 * @param id of nth timeslot of driver.
	 * @return session object.
	 */
	BayonneSession *getTimeslot(timeslot_t id);

	/**
	 * Return the message port bound with this driver.
	 *
	 * @return bound msgport for driver.
	 */
	inline BayonneMsgport *getMsgport(void)
		{return msgport;};

	/**
	 * Get the size of the stack for audio threads.
	 *
	 * @return stack size in bytes.
	 */
	inline size_t getAudioStack(void)
		{return audio_stack;};

	/**
	 * Get the thread priority to use for audio threads for this driver.
	 *
	 * @return thread priority.
	 */
	inline int getAudioPriority(void)
		{return audio_priority;};

	/**
	 * Get the audio level for silence detection.
	 *
	 * @return audio threashold for silence.
	 */
	inline Audio::Level getAudioLevel(void)
		{return audio_level;};

	/**
	 * Set driver logging.
	 *
	 * @param output stream to log driver.
	 */
	inline void setLogging(std::ostream *output)
		{logevents = output;};

	/**
	 * Determine if a span is available.
	 *
	 * @param span associated with driver to check.
	 * @return true if available ports.
	 */
	inline bool isSpanable(unsigned span);

	/**
	 * Deterime if a network destination is reachable through this
	 * driver, and convert dialing string into network reference.
	 *
	 * @param target network destination
	 * @param dial string
	 * @param output buffer
	 * @param size of output buffer
	 * @return true if reachable
	 */
	virtual bool getDestination(const char *target, const char *dial, char *output, size_t size);

	/**
	 * Get available timeslots.
	 *
	 * @return available slots.
	 */
	inline unsigned getAvail(void)
		{return avail;}

	/**
	 * See if a given potential dialed number is an external entry
	 * in our registrar.
	 *
	 * @return true if external.
	 * @param destination to test.
	 */
	virtual bool isExternal(const char *dest);

	/**
	 * See if a given potential dialed number is registered
	 *
	 * @return true if extern and registered.
	 * @param destination to test.
	 */
	virtual bool isRegistered(const char *dest);

	/**
	 * See if a given potential dialed number is available
	 *
	 * @return true if extern and available.
	 * @param destination to test.
	 */
	virtual bool isAvailable(const char *dest);

	/**
	 * See if a given selected server is currently considered
	 * reachable.  This could be used for failover.
	 *
	 * @return true if reachable.
	 * @param server to test.
	 */
	virtual bool isReachable(const char *proxy);

        /**
         * Fill registration data.
         *
         * @return number of records filled.
         * @param data array to fill.
         * @param number of entries available.
	 * @param optional id to match.
	 * @param optional flag if only extensions.
         */
        virtual unsigned getRegistration(regauth_t *data, unsigned count, const char *id = NULL);
};	

/**
 * A span is a collection of ports under a single control interface or
 * communication channel, such as a T1/E1/PRI/BRI span.
 *
 * @author David Sugar <dyfet@gnutelephony.org>
 * @short Span management object.
 */
class __EXPORT BayonneSpan : public Bayonne, public Keydata
{
protected:
	friend class __EXPORT BayonneSession;
	friend class __EXPORT BayonneDriver;

	static BayonneSpan *first;
	static BayonneSpan *last;
	static unsigned spans;
	static BayonneSpan **index;
	
	unsigned id;
	BayonneDriver *driver;
	BayonneSpan *next;
	timeslot_t timeslot, count, used;	// timeslots

public:
	Traffic call_attempts, call_complete;
	volatile unsigned short active_calls;

	/**
	 * Create a span for a specified number of timeslots.
	 *
	 * @param driver associated with span.
	 * @param timeslots this span covers.
	 */
	BayonneSpan(BayonneDriver *driver, timeslot_t timeslots);

	/**
	 * Get a span by a global span id.
	 *
	 * @param id of span.
	 * @return span object associated with id.
	 */
	static BayonneSpan *get(unsigned id);
	
	/**
	 * Get the session associated with the nth timeslot of the span.
	 *
	 * @param id of nth timeslot of span.
	 * @return associated session object.
	 */
	BayonneSession *getTimeslot(timeslot_t id);

	/**
	 * Allocate the total number of spans this server will support.
	 *
	 * @param total span count.
	 */
	static void allocate(unsigned total = 0);

	/**
	 * Return total spans in use.
	 *
	 * @return total spans in use.
	 */
	static inline unsigned getSpans(void)
		{return spans;};

	/**
	 * Get the first server timeslot of this span.
	 *
	 * @return first server timeslot.
	 */
	inline timeslot_t getFirst(void)
		{return timeslot;};

	/**
	 * Return total number of server timeslots in this span.
	 *
	 * @return server timeslot count.
	 */
	inline timeslot_t getCount(void)
		{return count;};

	/**
	 * Get the id associated with this span.
	 *
	 * @return global id of this span object.
	 */
	inline unsigned getId(void)
		{return id;};

	/**
	 * Get driver associated with this span.
	 *
	 * @return driver object for span.
	 */
	inline BayonneDriver *getDriver(void)
		{return driver;};

	/**
	 * Get number of call slots still available.
	 *
	 * @return count of call slots available.
	 */
	inline unsigned getAvail(void)
		{return count - used;}

	/**
	 * Suspend a span.
	 *
	 * @return true if successful.
	 */
	virtual bool suspend(void);

	/**
	 * Resume a suspended span.
	 *
	 * @return true if successful.
	 */
	virtual bool resume(void);

	/**
	 * Reset a span.
	 *
	 * @return true if successful.
	 */
	virtual bool reset(void);
};

/**
 * An intermediary binder class for Bayonne engine.
 *
 * @author David Sugar <dyfet@gnutelephony.org>
 * @short Binder class.
 */
class __EXPORT BayonneBinder : public ScriptBinder, public Bayonne
{
private:
	friend class __EXPORT BayonneSession;
	static BayonneBinder *binder;

	class Image : public ScriptImage
	{
	public:
		inline Image() : ScriptImage(NULL, NULL) {};
		unsigned gatherPrefix(const char *prefix, const char **list, unsigned max);
	};

protected:
	virtual const char *submit(const char **data);

	virtual ScriptCompiler *compiler(void);
	
	virtual unsigned destinations(Image *img, const char **array, unsigned max);

	virtual bool isDestination(Image *img, const char *name);

	BayonneSession *session(ScriptInterp *interp);

	bool scriptEvent(ScriptInterp *interp, const char *evt);

	bool digitEvent(ScriptInterp *interp, const char *evt);

	BayonneBinder(const char *id);

	virtual void makeCall(BayonneSession *child);

	virtual void dropCall(BayonneSession *child);

	virtual Name *getIncoming(ScriptImage *img, BayonneSession *s, Event *event);

public:
	static const char *submitRequest(const char **data);

	static ScriptCompiler *getCompiler(void);

	static unsigned gatherDestinations(ScriptImage *img, const char **index, unsigned max);

	static bool isDestination(const char *name);
};

/**
 * The primary session object representing a server timeslot and active
 * communication endpoint in Bayonne.
 *
 * @author David Sugar <dyfet@gnutelephony.org>
 * @short Session timeslot object.
 */
class __EXPORT BayonneSession : public ScriptInterp, public Bayonne
{
private:
	friend class __EXPORT ScriptEngine;
	friend class __EXPORT BayonneMsgport;
	friend class __EXPORT BayonneTranslator;
	friend class __EXPORT BayonneDriver;
	friend class __EXPORT Bayonne;

	BayonneSession() {};

	BayonneSession *nextIdle, *prevIdle;
	bool isAvail;

protected:
	static BayonneTranslator langNone;
	static ScriptSymbols *globalSyms;
	static Mutex globalLock;	

	std::ostream *logevents, *logtrace;
	Ring *ring;
	BayonneDriver *driver;
	BayonneMsgport *msgport;
	BayonneSession *peer;
	BayonneSpan *span;
	timeslot_t timeslot;
	uint8 seq;
	uint16 evseq;
	uint32 tseq;
	bool offhook, dtmf, answered, starting, holding, connecting, referring;
	time_t audiotimer, starttime;
	interface_t iface;
	bridge_t bridge;
	calltype_t type;
	event_t seizure;
	ScriptEngine *vm;

//	Name *getScript(const char *scr);

	/**
	 * Used to indicate commands which require dtmf handling to be
	 * enabled.
	 *
	 * @return true if dtmf was enabled.  If false, error processing
	 * occured for the interpreter.
	 */
	bool requiresDTMF(void);

	/**
	 * Enter a co-joined call session; used to notify other
	 * services.  Always performed by child node.
	 */
	void enterCall(void);

	/**
	 * Exit a co-joined call session; used to notify other
	 * services.  May happen when a child node exits, drops
	 * a recall, or is dropped by a parent.
	 *
	 * @param reason call is terminated.
	 */
	void exitCall(const char *reason);

	/**
	 * Enable dtmf detection for this channel.
	 *
	 * @return true if successful.
	 */
	virtual bool enableDTMF(void);

	/**
	 * Disable dtmf detection for this channel.
	 */
	virtual void disableDTMF(void);

	/**
	 * Check audio properties for file and channel audio processing
	 * based on the driver specific capabilities of this channel
	 * through it's virtual.
	 *
	 * @return error message if audio format unacceptable, NULL if ok.
	 * @param true if for live audio, false if for file only.
	 */
	virtual const char *checkAudio(bool live);

	/**
	 * virtual to filter incoming events.
	 *
	 * @param event being sent to channel.
	 * @return true if accepting event.
	 */
	virtual bool filterPosting(Event *event);

	virtual bool enterCommon(Event *event);
	virtual bool enterInitial(Event *event);
	virtual bool enterFinal(Event *event);
	virtual bool enterIdle(Event *event);
	virtual bool enterReset(Event *event);
	virtual bool enterRelease(Event *event);
	virtual bool enterRinging(Event *event);
	virtual bool enterPickup(Event *event);
	virtual bool enterAnswer(Event *event);
	virtual bool enterSeize(Event *event);
	virtual bool enterHunting(Event *event);
	virtual bool enterHangup(Event *event);
	virtual bool enterTone(Event *event);
	virtual bool enterReconnect(Event *event);
	virtual bool enterDTMF(Event *event);
	virtual bool enterPlay(Event *event);
	virtual bool enterRecord(Event *event);
	virtual bool enterJoin(Event *event);
	virtual bool enterWait(Event *event);
	virtual bool enterDial(Event *event);
	virtual bool enterBusy(Event *event);
	virtual bool enterStandby(Event *event);
	virtual bool enterXfer(Event *event);
	virtual bool enterRefer(Event *event);
	virtual bool enterHold(Event *event);
	virtual bool enterRecall(Event *event);

	/**
	 * Check dtmf handling and other nessisities for the interpreter
	 * after an event has changed interpreter state.
	 */
	void check(void);

	void renameRecord(void);
	bool stateInitial(Event *event);
	bool stateFinal(Event *event);
	bool stateIdle(Event *event);
	bool stateIdleReset(Event *event);
	bool stateReset(Event *event);
	bool stateRelease(Event *event);
	bool stateBusy(Event *event);
	bool stateStandby(Event *event);
	bool stateRinging(Event *event);
	bool statePickup(Event *event);
	bool stateAnswer(Event *event);
	bool stateSeize(Event *event);
	bool stateHunting(Event *event);
	bool stateRunning(Event *event);
	bool stateLibexec(Event *event);
	bool stateLibreset(Event *event);
	bool stateLibwait(Event *event);
	bool stateWaitkey(Event *event);
	bool stateThreading(Event *event);
	bool stateHangup(Event *event);
	bool stateCollect(Event *event);
	bool stateSleep(Event *event);
	bool stateStart(Event *event);
	bool stateClear(Event *event);
	bool stateInkey(Event *event);
	bool stateInput(Event *event);
	bool stateRead(Event *event);
	bool stateDial(Event *event);
	bool stateXfer(Event *event);
	bool stateRefer(Event *event);
	bool stateHold(Event *event);
	bool stateRecall(Event *event);
	bool stateTone(Event *event);
	bool stateDTMF(Event *event);
	bool statePlay(Event *event);
	bool stateRecord(Event *event);
	bool stateJoin(Event *event);
	bool stateWait(Event *event);
	bool stateConnect(Event *event);
	bool stateReconnect(Event *event);
	bool stateCalling(Event *event);

	/**
	 * Direct method to post an event to a channel.
	 *
	 * @return true if event is claimed by channel.
	 * @param event being posted.
	 */
	bool putEvent(Event *event);

	/**
	 * Write libexec...
	 *
	 * @param string to write.
	 */
	void libWrite(const char *string);

	void libClose(const char *string);

	bool isLibexec(const char *tsid);

	timeout_t getLibexecTimeout(void);

	const char *getWritepath(char *buf = NULL, size_t len = 0);

	void incIncomingAttempts(void);

	void incOutgoingAttempts(void);

	void incIncomingComplete(void);

	void incOutgoingComplete(void);

	void incActiveCalls(void);

	void decActiveCalls(void);

public:
	Traffic call_attempts, call_complete;

	inline bool isDTMF(void)
		{return dtmf;};

	inline bool isPeering(void)
		{return state.peering;};

	inline BayonneSpan *getSpan(void)
		{return span;};

	inline timeslot_t getTimeslot(void)
		{return timeslot;};

	inline ScriptEngine *getEngine(void)
		{return vm;};

	/**
	 * Process interpreter session symbols.
	 *
	 * @param option symbol being requested.
	 * @return NULL if not external, else value.
	 */
	const char *getExternal(const char *option);

	/**
	 * Add to an existing symbol.
	 *
	 * @param id of symbol.
	 * @param value to add.
	 * @return false if not exists.
	 */
	bool addSymbol(const char *id, const char *value);

    /**
     * Clear an existing symbol.
     *
     * @param id of symbol.
     * @return false if not exists.
     */
    bool clearSymbol(const char *id);

	/**
	 * Get event sequence id
	 *
	 * @return event sequence id.
	 */
	inline uint16 getEventSequence(void)
		{return evseq;};

	static const char *getGlobal(const char *id);
	static bool setGlobal(const char *id, const char *value);
	static bool sizeGlobal(const char *id, unsigned size);
	static bool addGlobal(const char *id, const char *value);
	static bool clearGlobal(const char *id);

protected:
	/**
	 * Return session id for interpreter session command.
	 *
	 * @param id of session request.
	 * @return ccengine object to return for the id.
	 */
	ScriptInterp *getInterp(const char *id);

	/**
	 * Return ccengine symbol page map.  Gives access to globals.
	 *
	 * @param id table of symbols.
	 * @return table map to use.
	 */
	ScriptSymbols *getSymbols(const char *id);

	/**
	 * Translator in effect for this session.
	 */
	BayonneTranslator *translator;

	/**
	 * Start a script from idle or ringing.  This may use the
	 * assign statements to find the script name if none is passed.
	 *
	 * @param event passed to kick off the script.
	 * @return script to run or NULL.
	 */
	Name *attachStart(Event *event);

	/**
	 * Used by ccengine.
	 */
	unsigned getId(void);

	/**
	 * Compute a uneque call session id for the current call on the
	 * current session object.  Also a great key for cdr records.
	 */
	void setSid(void);

	/**
	 * Set the session to a new state.
	 */
	void setState(state_t);

	/**
	 * Set the session to the running state, resume interpreter or
	 * libexec.
	 */
	void setRunning(void);

	/**
	 * Attempt to readjust encoding of active session if supported.
	 *
	 * @param encoding to try
	 * @param timeout to use
	 */
	bool setReconnect(const char *enc, timeout_t framing);


	/**
	 * Attempt to readjust encoding of active session for recall.
	 */
	bool recallReconnect(void);

	/**
	 * Set the session to the connecting (join) state or run state
	 * based on flags and circumstances from seize/pickup.
	 */
	void setConnecting(const char *evname = NULL);

	/**
	 * Set the result of a libexec initiated process and change
	 * state to libexec.  Return false if no libexec support, or
	 * if not currently libexecing...
	 *
	 * @return true if in libexec
	 * @param result code to set.
	 */
	bool setLibexec(result_t result);

        /**
         * Set the result of a libexec initiated process and execute a
	 * reset timer interval.  This is used to schedule reset hang
	 * time before resuming libexec. 
         *
         * @return true if in libexec
         * @param result code to set.
         */  
	bool setLibreset(result_t result);

public:
	/**
	 * Get the current language translator.
	 *
	 * @return translator.
	 */
	inline BayonneTranslator *getTranslator(void)
		{return translator;};

	/**
	 * Start connecting child...
	 */
	inline void startConnecting(void)
		{connecting = true;};

protected:
	/**
	 * Get the libaudio object.  Used by server libexec to prepare
	 * a libaudio/phrasebook session.
	 *
	 * @return libaudio object for this session.
	 */
	inline libaudio_t *getLibaudio(void)
		{return state.libaudio;};

	/**
	 * ccengine.
	 */
	void finalize(void);

	/**
	 * Exit processing for interpreter.
	 *
	 * @return true of exiting.
	 */
	bool exit(void);

	char var_date[12];
	char var_time[12];
	char var_duration[12];
	char var_callid[12];
	char var_tid[14];
	char var_sid[16];
	char var_pid[16];
	char var_recall[16];
	char var_joined[16];
	char var_rings[4];
	char var_timeslot[8];
	char var_spanid[8];
	char var_bankid[4];
	char var_spantsid[12];
	const char *voicelib;
	char *dtmf_digits;		// dtmf sym space;
	unsigned digit_count, ring_count;
	time_t time_joined;

	State state;

public:
	/**
	 * Create a new session
	 *
	 * @param driver to bind.
	 * @param timeslot to bind.
	 * @param span to bind, or NULL if no span associated.
	 */
	BayonneSession(BayonneDriver *driver, timeslot_t timeslot, BayonneSpan *span = NULL);

	/**
	 * Destroy a session.
	 */
	virtual ~BayonneSession();

	const char *getDigits(void);

	inline const char *defVoicelib(void)
		{return voicelib;}

	inline const char *getSessionId(void)
		{return var_sid;};

	inline const char *getSessionParent(void)
		{return var_pid;};

	inline const char *getSessionJoined(void)
		{return var_joined;};

	inline time_t getSessionStart(void)
		{return starttime;};

	/**
	 * Initial kickoff event.
	 */
	void initialevent(void);

	/**
	 * Initialine ccengine script environment.
	 */
	void initialize(void);

	/**
	 * Detach interpreter.
	 */
	void detach(void);

	/**
	 * Return time this call is joined or 0 if not child node.
	 */
	inline time_t getJoined(void)
		{return time_joined;};

	/**
	 * Return driver associated with this session.
	 *
	 * @return driver object.
	 */
	inline BayonneDriver *getDriver(void)
		{return driver;}

	/**
	 * Return time remaining until timer expires.  Commonly used for
	 * msgport scheduling.
	 *
	 * @return time remaining in milliseconds.
	 */
	virtual timeout_t getRemaining(void) = 0;

	/**
	 * Start a timer on the session.  Used extensivily in state
	 * handler code.
	 *
	 * @param timer to start for specified milliseconds.
	 */
	virtual void startTimer(timeout_t timer) = 0;

	/**
	 * Stop the timer for the session.  Used extensivily in state
	 * handler code.
	 */
	virtual void stopTimer(void) = 0;

	/**
	 * Set the port hook state.  Mostly for analog devices.
	 *
	 * @param state true to set offhook, false onhook.
	 */
	virtual void setOffhook(bool state);

	/**
	 * Handles driver specific stuff when going idle.
	 */
	virtual void makeIdle(void);

	/**
	 * Disconnect notify peer...
	 *
	 * @param reason event id to pass
	 */
	void part(event_t reason);

	/**
	 * Post an event to the session state engine.
	 *
	 * @return true if event claimed.
	 * @param event being posted.
	 */
	virtual bool postEvent(Event *event);


	bool matchLine(Line *line);

	/**
	 * queue an event through the msgport.
	 *
	 * @param event to queue.
	 */
        virtual void queEvent(Event *event);

	/**
	 * ccengine thread handling.
	 */
	virtual void startThread(void);

	/**
	 * ccengine thread handling.
	 */
	virtual void enterThread(ScriptThread *thr);

	/**
	 * ccengine thread handling.
	 */
	virtual void exitThread(const char *msg);

	/**
	 * Clear/cleanup audio processing for the session.
	 */
	virtual void clrAudio(void);

	/**
	 * Get frame rate used for creating tone generators.
	 */
	virtual timeout_t getToneFraming(void);

	/**
	 * Get driver default encoding.
	 */
	virtual const char *audioEncoding(void);

	/**
	 * Get driver default extension.
	 */
	virtual const char *audioExtension(void);
	
	/**
	 * Get driver default framing.
	 */
	virtual timeout_t audioFraming(void);

	/**
	 * Check script keywords for audio processing.
	 *
	 * @return NULL if ok, else error message.
	 * @param live true if for live, else for file processing.
	 */
	const char *getAudio(bool live = true);

	/**
	 * ccengine branch event notification.  Used for menudef
	 * processing.
	 */
	void branching(void);

	/**
	 * Return hook state.
	 *
	 * @return true if offhook.
	 */
	inline bool isOffhook(void)
		{return offhook;};

	/**
	 * Return interface type of this session.
	 *
	 * @return interface type.
	 */
	inline interface_t getInterface(void)
		{return iface;};

	/**
	 * Return bridge type for joins.
	 *
	 * @return bridge type.
	 */
	inline bridge_t getBridge(void)
		{return bridge;};

	/**
	 * Return call type on session.
	 *
	 * @return call type.
	 */
	inline calltype_t getType(void)
		{return type;};

	/**
	 * Return server timeslot this session uses.
	 *
	 * @return server timeslot.
	 */
	inline timeslot_t getSlot(void)
		{return timeslot;};

	/**
	 * Return if the session is currently idle.
	 *
	 * @return true if currently idle.
	 */
	inline bool isIdle(void)
		{return isAvail;};

	/**
	 * Return if currently referring.
	 *
	 * @return true if referring.
	 */
	bool isRefer(void);

	/**
	 * Return state of join.
	 *
	 * @return true if currently joined.
	 */
	bool isJoined(void);

	/**
	 * Return state of association with parent in call.
	 *
	 * @return true if currently associated.
	 */
	bool isAssociated(void);

	/**
 	 * Set new call association.  Used by ACD.
	 *
	 * @param session to associate with.
	 */
	void associate(BayonneSession *s);

	inline bool isHolding(void)
		{return holding;};

	/**
	 * Return state disconnecting or idle...
	 */
	bool isDisconnecting(void);

	/**
	 * Return parent answer timer, if joining.
	 *
	 * @return timeout for joining.
	 */
	timeout_t getJoinTimer(void);

	/**
	 * Signal notification to script.
	 *
	 * @param signal to send to script engine.
	 * @return true if signal claimed.
	 */
	bool signalScript(signal_t signal);

	/**
	 * Indicate whether session peers audio as linear frames.
	 *
	 * @return true if peering linear.
	 */
	virtual bool peerLinear(void);

	/**
	 * Post a peer audio frame into the driver.  The frame is assumed
	 * to either be in the format used for global peering, or, if
	 * the driver supports setPeering, perhaps in the session selected
	 * format.
	 *
	 * @return true if peer frame was posted.
	 * @param encoded audio frame to peer.
	 */
	virtual bool peerAudio(Audio::Encoded encoded);

	/**
	 * Set peer audio encoding to the encoding type and framing
	 * specified by peer on drivers which can switch encoding.  This
	 * can enable audio conversions to be bypassed.
	 *
	 * @return true if set.
	 * @param encoding format requested.
	 * @param framing timer to use.
	 */
	virtual bool setPeering(Audio::Encoding encoding, timeout_t framing);

	const char *getKeyString(const char *id);
	bool getKeyBool(const char *id);
	long getKeyValue(const char *id);
	timeout_t getSecTimeout(const char *id);
	timeout_t getMSecTimeout(const char *id);
	timeout_t getTimeoutValue(const char *opt = NULL);
	timeout_t getTimeoutKeyword(const char *kw);
	const char *getExitKeyword(const char *def);
	const char *getMenuKeyword(const char *def);

	unsigned getInputCount(const char *digits, unsigned max);

	/**
	 * Compute a new unique transaction id.  These are like pids and
	 * are often used to assure transaction coherence, such as in
	 * the libexec system.
	 *
	 * @return generated integer transaction identifier.
	 */
	uint32 newTid(void);

	/**
	 * Get the current transaction identifier string for the session.
	 *
	 * @return transaction identifier.
	 */
	inline const char *getTid(void)
		{return var_tid;};

	/**
	 * Throw a digit pattern matching event message to the interprer.
	 *
	 * @return true if throw caught.
	 * @param event message.
	 */
	bool digitEvent(const char *event);

	inline bool stringEvent(const char *evt)
		{return scriptEvent(evt);}

	/**
	 * Get the next pending digit in the DTMF input buffer.
	 *
	 * @return digit.
	 */
	char getDigit(void);

	BayonneAudio audio;
};

/**
 * Bayonne RPC arguments, may be passed through to binders from 
 * webservice sessions for extensions to soap & xmlrpc services.
 *
 * @short rpc arguments parsed
 * @author David Sugar <dyfet@gnutelephony.org>
 */
class __EXPORT BayonneRPC : public Bayonne
{
protected:
	BayonneRPC();
	virtual ~BayonneRPC();

	struct params
	{
		char *name[RPC_MAX_PARAMS];
		char *map[RPC_MAX_PARAMS];
		char *value[RPC_MAX_PARAMS];
		unsigned short param[RPC_MAX_PARAMS];
		unsigned count;
		short argc;
	}	params;

	bool parseCall(char *cp);

public:
	virtual void setComplete(BayonneSession *s);

	struct {
		char *buffer;
		size_t bufsize;
		size_t bufused;
		const char *agent_id;
		const char *protocol;
		bool authorized;		// transport authorized
		const char *userid;		// null if anonymous
		BayonneDriver *driver;		// authorizing driver
	}	transport;

	struct {
		unsigned code;
		const char *string;
	}	result;

        struct {
		const char *prefix;	// xmlrpc has prefix, soap NULL
                const char *method;
                const char *tranid;
                const char *action;
                const char *resuri;
        }	header;

	inline unsigned getCount(void)
		{return params.argc;};

	const char *getParamId(unsigned short param, unsigned short offset); 
	const char *getIndexed(unsigned short param, unsigned short offset = 0);
	const char *getNamed(unsigned short param, const char *member);
	const char *getMapped(const char *map, const char *member);

	friend size_t xmlwrite(char **buf, size_t *max, const char *fmt, ...);
	bool buildResponse(const char *fmt, ...);
	void sendSuccess(void);
	void sendFault(int code, const char *string);
	
	inline void transportFault(unsigned code, const char *string)
		{result.code = code; result.string = string;};

	bool invokeXMLRPC(void);
};	

size_t xmlwrite(char **buf, size_t *max, const char *fmt, ...);
	 
/**
 * Bayonne services are used for threaded modules which may be
 * installed at runtime.  These exist to integrate plugins with
 * server managed startup and shutdown.
 *
 * @short threaded server service.
 * @author David Sugar <dyfet@gnutelephony.org>
 */ 
class __EXPORT BayonneService : public Thread                
{          
private:            
	friend class __EXPORT BayonneSession;

        static BayonneService *first;
	static BayonneService *last;
        BayonneService *next;
        friend void startServices(void);        
        friend void stopServices(void);

protected:
        BayonneService(int pri, size_t stack);

        /**
         * Used for stop call interface.             
         */
        virtual void stopService(void);

        /**
         * Used for start call interface.
         */
        virtual void startService(void);

	/**
	 * Used at end of call.
	 */
	virtual void detachSession(BayonneSession *s);

	/**
	 * Used at running state.
	 */
	virtual void attachSession(BayonneSession *s);

	/**
	 * Used to notify when call is joined.
	 */
	virtual void enteringCall(BayonneSession *child);

	/**
	 * Used to notify when exiting join.
	 */
	virtual void exitingCall(BayonneSession *child);

public:
	/**
	 * Start all service threads.
	 */
	static void start(void);
	
	/**
	 * Stop all service threads.
	 */
	static void stop(void); 
};

/**
 * Offers interface bridge for embedded scripting engines through an
 * abstract interface.  This may be used to support calling of integrated
 * virtual machines, etc.
 *
 * @author David Sugar <dyfet@gnutelephony.org>
 * @short vm scripting interface bridge.
 */
class __EXPORT ScriptEngine
{
protected:
	friend class __EXPORT BayonneSession;

	/**
	 * Signal bridge about Bayonne signal event.
	 *
	 * @param bayonne signal event
	 * @return true if claimed by scripting engine
	 */
	virtual bool signalEngine(Bayonne::signal_t signal) = 0;

	/**
	 * Release bridge session because we moved.  If detached thread
	 * then should also NULL the vm pointer.
	 */
	virtual void releaseEngine(void) = 0;

	/**
	 * Disconnect bridge session for hangup/stopping of processing.  If
	 * detached thread, then should also NULL the vm pointer.
	 */
	virtual void disconnectEngine(void) = 0;

	/**
	 * Step notification, often used to signal completion handler so
	 * bridge can change state of Bayonne engine and then wait for
	 * completion by notification at running state again.  Hence
	 * we often use a Common C++ "Event" object for completion notify.
	 *
	 * @param true if to continue step counter also.
	 */
	virtual bool stepEngine(void) = 0;

	/**
	 * If not detached thread we may delete it ourselves.
	 */
	virtual ~ScriptEngine() = 0;

	/*
	 * Clears vm session.
	 */
	inline void clearEngine(BayonneSession *s)
		{s->vm = NULL;};

	/**
	 * Set vm session.
	 */
	inline void setEngine(BayonneSession *s)
		{s->vm = this;};
};


} // namespace

#endif
