// Copyright (C) 2006 David Sugar, Tycho Softworks.
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

#include <private.h>
#include <bayonne.h>
#include <cc++/slog.h>
#include <cc++/socket.h>

#ifdef  WIN32
#define WEBSERVER_FILES "C:\\Program Files\\GNU Telephony\\Bayonne Webserver"
#else
#define WEBSERVER_FILES SHARED_FILES "/webserver"
#endif

namespace moduleWebservice {
using namespace ost;
using namespace std;

#define	XML_SESSION_INVALID	602
#define	XML_ARGUMENT_MISSING	601
#define	XML_COMMAND_FAILED	600

#define	MAX_XML_REQUEST		2048
#define	MAX_XML_REPLY		16384

class EventStream : public Bayonne, public Socket
{
private:
	friend class Service;
	static Mutex lock;
	static EventStream *first;
	static EventStream *last;
	static EventStream *free;

	class EventStream *next, *prev;

	char authorized[96];

	void release(void);

public:
	EventStream();
	static EventStream *create(int so, const char *auth = NULL);
	bool sendEvent(const char *msg);
	void sendReply(const char *msg);
	bool isConnected(void);
};

class Service : public BayonneService, public BayonneZeroconf, public StaticKeydata, public Bayonne
{
private:
	void run(void);
	void stopService(void);

	void enteringCall(BayonneSession *s);
	void exitingCall(BayonneSession *s);
	void attachSession(BayonneSession *s);
	void detachSession(BayonneSession *s);
	void joinedSession(BayonneSession *s);
	void breakSession(BayonneSession *s);
	void postEvent(const char *buf, BayonneSession *s);

public:
	static bool active;
	static Service webservice;
	static DynamicKeydata admin, rpc;
	Service();
};

class Session : public BayonneRPC, public Socket, public Thread
{
protected:
	char buffer[MAX_XML_REQUEST];
	char request[128];
	char auth[128];
	const char *req_command;
	const char *req_path;
	char *req_query;
	char *req_auth;
	char *req_agent;
	regauth_t regs[1024];

	void syncExit(void);
	void run(void);
	void sendText(const char *text);
	void sendAuth(const char *text, const char *ctype = NULL, size_t size = 0);
	void sendError(const char *text, const char *ctype = NULL, size_t size = 0);
	void sendHeader(const char *ctype = "text/html", unsigned long clen = 0);
	void rpcFault(int code, const char *text);
	void rpcReply(const char *string = NULL);	
	void textDocument(const char *ctype, const char *file);
	void binDocument(const char *ctype, const char *file);
	void postDocument(const char *file, unsigned long size);
		
	unsigned long htmlAccess(const char *fname);
	void htmlInsert(const char *section, const char *title);
	void htmlHeader(const char *title, bool refresh);
	void htmlFooter(void);
	void htmlStatus(void);
	void htmlSessions(void);
	void htmlCalls(void);
	void htmlControl(void);

	void setComplete(BayonneSession *s);

	char *urlDecode(char *source);
	char *b64Decode(char *source);

public:
	Session(SOCKET so);
	virtual ~Session();
};
	
class TCP
{
private:
        static TCP *first;
        static fd_set selectors;
        static fd_set input;
        static SOCKET hiwater;
        static SOCKET last;
        static TCP *index[sizeof(fd_set) * 8];
        TCP *next;

protected:
        TCP(SOCKET so);

        virtual ~TCP() {};
        virtual void disconnect(void) = 0;

public:
        virtual SOCKET getAccept(void) = 0;

        static TCP *getSelect(void);
        static void endSockets(void);
        static tpport_t getPort(void);
        static unsigned getBacklog(void);
        static unsigned getSegment(void);
        static size_t getInputBuffer(void);
	static size_t getOutputBuffer(void);

#ifdef  CCXX_IPV6
        static IPV6Address getV6Address(const char *iface);
#endif
        static IPV4Address getV4Address(const char *iface);

};

class TCPV4 : public TCPSocket, public TCP
{
public:
        TCPV4(const char *iface);

        void disconnect(void);

        SOCKET getAccept(void);
};

#ifdef  CCXX_IPV6
class TCPV6 : public TCPV6Socket, public TCP
{
public:
        TCPV6(const char *iface);

        void disconnect(void);

        SOCKET getAccept(void);
};
#endif

} // namespace

	
