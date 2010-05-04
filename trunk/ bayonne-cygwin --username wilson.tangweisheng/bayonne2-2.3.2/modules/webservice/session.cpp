// Copyright (C) 2005 Open Source Telecom Corporation.
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

#include "module.h"
#ifndef	WIN32
#include "private.h"
#endif
#include <cerrno>

namespace moduleWebservice {
using namespace ost;
using namespace std;

Session::Session(SOCKET so) :
BayonneRPC(), Socket(so), Thread(0)
{
	transport.buffer = NULL;
	setError(false);
	SLOG_DEBUG("webservice/%d: starting session", so);

	sendBuffer(TCP::getOutputBuffer());
	receiveBuffer(TCP::getInputBuffer());
}

Session::~Session()
{
	if(so != INVALID_SOCKET)
	{
		SLOG_DEBUG("webservice/%d: ending session", so);
		endSocket();
	}
	terminate();

	if(transport.buffer)
		delete[] transport.buffer;

	transport.buffer = NULL;
}

char *Session::b64Decode(char *src)
{
	static const unsigned char alphabet[65] =
        	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

	char *ret = src;
	unsigned char *pdst = (unsigned char *)src;

	char decoder[256];
	int i, bits, c;

        for (i = 0; i < 256; ++i)
                decoder[i] = 64;
        for (i = 0; i < 64 ; ++i)
                decoder[alphabet[i]] = i;

	bits = 1;

	while(*src && !isspace(*src))
	{
                c = (unsigned char)(*(src++));
                if (c == '=')
                {
                        if(bits & 0x40000)
                        {
                                *(pdst++) = (bits >> 10);
                                *(pdst++) = (bits >> 2) & 0xff;
                                break;
                        }
                        if (bits & 0x1000)
                                *(pdst++) = (bits >> 4);
                        break;
                }
                if (decoder[c] == 64)
                        continue;
                bits = (bits << 6) + decoder[c];
                if (bits & 0x1000000)
                {
                        *(pdst++) = (bits >> 16);
                        *(pdst++) = (bits >> 8) & 0xff;
                        *(pdst++) = (bits & 0xff);
                        bits = 1;
                }
	}
	*pdst = 0;
	return ret;
}

char *Session::urlDecode(char *source)
{
	char *ret = source;
	char *dest = source;
        char hex[3];

        while(*source)
        {
                switch(*source)
                {
                case '+':
                        *(dest++) = ' ';
                        break;
                case '%':
                        if(source[1])
                        {
                                hex[0] = source[1];
                                ++source;
                                if(source[1])
                                {
                                        hex[1] = source[1];
                                        ++source;
                                }
                                else
                                        hex[1] = 0;
                        }
                        else
                                hex[0] = hex[1] = 0;
                        hex[2] = 0;
                        *(dest++) = (char)strtol(hex, NULL, 16);
                        break;
                default:
                        *(dest++) = *source;
                }
                ++source;
        }
        *dest = 0;
        return ret;
}

void Session::syncExit(void)
{
	if(so != INVALID_SOCKET)
	{
		SLOG_DEBUG("webservice/%d: ending session", so);
		endSocket();
	}
	Thread::exit();
}

#ifndef	VERSION
#define	VERSION "w32"
#endif

void Session::sendError(const char *text, const char *cttype, size_t size)
{
	char buffer[256];

	sendText(text);
	sendText("Server: Bayonne/" VERSION "\r\n");
	sendText("Pragma: no-cache\r\n");
	sendText("Cache-Control: no-cache\r\n");
	sendText("Connection: close\r\n");
	if(cttype)
	{
		snprintf(buffer, sizeof(buffer), 
			"Content-Type: %s\r\n", cttype);
		sendText(buffer);
	}
	if(size)
	{
		snprintf(buffer, sizeof(buffer),
			"Content-Length: %ld\r\n", (long)size);
		sendText(buffer);
	}
	sendText("\r\n");
}

void Session::sendAuth(const char *text, const char *cttype, size_t size)
{
	char buffer[256];
	const char *realm = Service::webservice.getString("realm");
	sendText(text);
	sendText("Server: Bayonne/" VERSION "\r\n");
	sendText("Pragma: no-cache\r\n");
	sendText("Cache-Control: no-cache\r\n");
	sendText("Connection: close\r\n");
	snprintf(buffer, sizeof(buffer),
		"WWW-Authenticate: Basic realm=\"%s\"\r\n", realm);
	sendText(buffer);
	if(cttype)
	{
		snprintf(buffer, sizeof(buffer), "Content-Type: %s\r\n", cttype);
		sendText(buffer);
	}
	if(size)
	{
		snprintf(buffer, sizeof(buffer), "Content-Length: %ld\r\n", (long)size);
		sendText(buffer);
	}
	sendText("\r\n");
}

#ifndef	ECONNRESET
#define	ECONNRESET -99
#endif

void Session::sendText(const char *text)
{
	int flag = 0;
#ifdef	MSG_NOSIGNAL
	flag |= MSG_NOSIGNAL;
#endif
	unsigned len = strlen(text);
	if(::send(so, text, len, flag) < (int)len)
	{
		if((errno == EPIPE) || (errno == ECONNRESET))
			syncExit();
	}
}

void Session::sendHeader(const char *cttype, unsigned long clen)
{
	const char *typeinfo = strchr(cttype, '/');
	char buf[96];
	sendText("HTTP/1.1 200 OK\r\n");
	sendText("Connection: close\r\n");
	snprintf(buf, sizeof(buf), "Server: Bayonne/%s\r\n", VERSION);
	sendText(buf);
	snprintf(buf, sizeof(buf), "Content-Type: %s\r\n", cttype);
	sendText(buf);	

	if(!typeinfo)
		typeinfo = cttype;
	else
		++typeinfo;

	if(!stricmp(typeinfo, "xml") || !stricmp(typeinfo, "html"))
	{
		sendText("Pragma: no-cache\r\n");
		sendText("Cache-Control: no-cache\r\n");
	}

	if(clen)
	{
		snprintf(buf, sizeof(buf), "Content-Length: %ld\r\n", clen);
		sendText(buf);
	}
	sendText("\r\n");
}

void Session::run(void)
{
	const char *secret, *passwd;
	char *userid, *driver;
	const char *cp;
	char *tok;
	const char *ext;
	char agent[65];
	char secret_buf[65];
	unsigned long xmlsize = 0;
	bool xmltext = false;
	BayonneDriver *d;
	unsigned pos = 0;

	if(!isPending(pendingInput, 10000))
		goto timeout;

	if(::recv(so, agent, 8, MSG_PEEK) < 8)
	{
failed:
        slog.notice("webservice/%d: connection invalid", so);		
		syncExit();
	}	

	while(pos < 8)
	{
		if(agent[pos] != 0x03 && agent[pos] != 0x01)
		{
			++pos;
			continue;
		}
		goto failed;
	}

	if(readLine(request, sizeof(request), 10000) < 1)
	{
timeout:
		slog.notice("webservice/%d: request timeout", so);
		sendError("HTTP/1.1 408 Request Timeout\r\n");
		syncExit();
	}

	req_agent = "unknown";
	req_command = strtok_r(request, " \t\r\n", &tok);
	req_path = strtok_r(NULL, " \t\r\n", &tok);
	req_query = NULL;
	req_auth = NULL;
	if(req_path)
		tok = strchr(req_path, '?');
	else
		tok = NULL;
	if(tok)
	{
		*tok = 0;
		req_query = ++tok;
	}

	for(;;)
	{
		if(readLine(buffer, sizeof(buffer), 10000) < 1)
			goto timeout;

		if(!strnicmp(buffer, "Content-Type:", 13))
		{
			cp = buffer + 13;
			while(isspace(*cp))
				++cp;

			if(!strnicmp(cp, "text/xml", 8))
				xmltext = true;
		}			

		if(!strnicmp(buffer, "Content-Length:", 15))
		{
			cp = buffer + 15;
			while(isspace(*cp))
				++cp;

			xmlsize = atol(cp);
		} 

		if(!strnicmp(buffer, "User-Agent:", 11))
		{
			cp = buffer + 11;
			while(isspace(*cp))
				++cp;
			tok = strrchr(cp, '\r');
			if(!tok)
				tok = strrchr(cp, '\n');
			if(tok)
				*tok = 0;
			setString(agent, sizeof(agent), cp);
			req_agent = agent;
		} 

		if(!strnicmp(buffer, "Authorization: Basic", 20))
		{
			cp = buffer + 20;
			while(isspace(*cp))
				++cp;
			setString(auth, sizeof(auth), cp);
			b64Decode(auth);
			req_auth = auth;
		} 

		cp = buffer;
		while(isspace(*cp))
			++cp;
		if(!*cp)
			break;
	}

	if(!req_path || (stricmp(req_command, "get") && stricmp(req_command, "post")))
	{
		slog.notice("webservice/%d: invalid request", so);
		sendError("HTTP/1.1 400 Bad Request\r\n");
		syncExit();
	}
	if(*req_path == '/')
		++req_path;


	userid = NULL;
	passwd = NULL;
	if(req_auth)
	{
		userid = req_auth;
		tok = strrchr(req_auth, ':');
		if(tok)
		{
			*(tok++) = 0;
			passwd = tok;
		}
	}

	if(strstr(req_path, ".."))
		rpcFault(403, "Path Invalid");

	ext = strrchr(req_path, '.');
	if(!ext)
		ext = "";
	if(!stricmp(ext, ".html") && Service::webservice.getBoolean("authenticate"))
	{
		if(!req_auth || !userid || !passwd)
			rpcFault(401, "Authorization Required");
		secret = Service::webservice.admin.getString(userid, secret_buf, sizeof(secret_buf));
		if(!secret || strcmp(secret, passwd))
			rpcFault(401, "Authorization Invalid");
	}

	if(strchr(req_path, '/'))
	{
		if(!req_auth || !userid || !passwd)
			rpcFault(401, "Authorization Required");

		secret = Service::webservice.admin.getString(userid, secret_buf, sizeof(secret_buf));
		if(secret && !strcmp(secret, passwd))
			goto allow;
	
		rpcFault(403, "Access Forbidden");
	}

allow:
	if(!stricmp(req_command, "post") && *ext && strchr(req_path, '/'))
	{
		if(!xmlsize)
			rpcFault(411, "No Size Specified");			
		postDocument(req_path, xmlsize);
	}
	if(!stricmp(req_command, "get") && *ext)
	{
		if(!stricmp(ext, ".dtd"))
			textDocument(req_path, "application/xml-dtd");
		if(!stricmp(ext, ".css"))
			textDocument(req_path, "text/css");
		if(!stricmp(ext, ".txt") || !stricmp(ext, ".text"))
			textDocument(req_path, "text/plain");
		if(!stricmp(ext, ".au") || !stricmp(ext, ".snd"))
			binDocument(req_path, "audio/basic");
		if(!stricmp(ext, ".wav") || !stricmp(ext, ".wave"))
			binDocument(req_path, "audio/x-wav");
		if(!stricmp(ext, ".gsm"))
			binDocument(req_path, "audio/x-gsm");
		if(!stricmp(ext, ".jpg") || !stricmp(ext, ".jpeg"))
			binDocument(req_path, "image/jpeg");
		if(!stricmp(ext, ".ico") || !stricmp(ext, ".icon"))
			binDocument(req_path, "image/x-icon");
		if(!stricmp(ext, ".mpg") || !stricmp(ext, ".mpeg"))
			binDocument(req_path, "video/mpeg");
		if(!stricmp(ext, ".ogg"))
			binDocument(req_path, "application/ogg");
		if(!stricmp(ext, ".gif"))
			binDocument(req_path, "image/gif");
		if(!stricmp(ext, ".png"))
			binDocument(req_path, "image/png");
		if(!stricmp(ext, ".js"))
			textDocument(req_path, "application/x-javascript");
		if(!stricmp(ext, ".jar"))
			binDocument(req_path, "application/java-archive");
		if(!stricmp(ext, ".class"))
			binDocument(req_path, "application/java-vm");
		if(!stricmp(ext, ".pdf"))
			binDocument(req_path, "application/pdf");
		if(!stricmp(ext, ".ps"))
			binDocument(req_path, "application/postscript");
		if(!stricmp(ext, ".xhtml"))
			textDocument(req_path, "application/xhtml+xml");
	}

	// client generated uuid based events access
	if(!strnicmp(req_path, "events-uuid-", 12)) 
	{
		userid = (char *)req_path + 12;
		goto events;
	}

	if(!stricmp(req_path, "events"))
	{
		if(!userid && Service::webservice.getBoolean("authenticate"))
			rpcFault(401, "Authorization Required");

		if(!userid)
			goto events;

		secret = Service::webservice.rpc.getString(userid, secret_buf, sizeof(secret_buf));
		if(secret && passwd)
			if(!strcmp(secret, passwd))
			{
				userid = NULL;
				goto events;
			}

		if(strchr(userid, '/'))
		{
			driver = userid;
			userid = strchr(userid, '/');
            *(userid++) = 0;
            d = BayonneDriver::get(driver);
			if(d && !d->isAuthorized(userid, passwd))
            	d = NULL;
        }
        else if(strchr(userid, ':'))
        {
            driver = userid;
            userid = strchr(userid, ':');
            *(userid++) = 0;
            d = BayonneDriver::get(driver);
            if(d && !d->isAuthorized(userid, passwd))
                d = NULL;
        }
		else
			d = BayonneDriver::authorize(userid, passwd);

		if(!d)
			rpcFault(401, "Authorization Invalid");

events:
		sendHeader("text/xml");
		EventStream::create(so, userid);
		so = INVALID_SOCKET;
		state = Socket::INITIAL;
		syncExit();
	}

	if(!stricmp(req_command, "post"))
	{
		transport.authorized = false;
		transport.driver = NULL;
		transport.userid = userid;
		transport.protocol = "http";
		transport.agent_id = req_agent;

		if(userid)
		{
			secret = Service::webservice.rpc.getString(userid, secret_buf, sizeof(secret_buf));
			if(secret && passwd)
				if(!strcmp(secret, passwd))
					transport.authorized = true;

			if(strchr(userid, '/'))
			{
				driver = userid;
				userid = strchr(userid, '/');
				*(userid++) = 0;
				transport.driver = BayonneDriver::get(driver);
				if(transport.driver && 
					!transport.driver->isAuthorized(userid, passwd))
					transport.driver = NULL;
			}
			else if(strchr(userid, ':'))
			{
				driver = userid;
				userid = strchr(userid, ':');
				*(userid++) = 0;
				transport.driver = BayonneDriver::get(driver);
				if(transport.driver && !transport.driver->isAuthorized(userid, passwd))
					transport.driver = NULL;
			}
			else
				transport.driver = BayonneDriver::authorize(userid, passwd);
			if(!transport.driver && !transport.authorized)
				rpcFault(401, "Authorization Invalid");
		}
		else
			rpcFault(401, "Authorization Required");

		if(stricmp(req_path, "RPC2") && stricmp(req_path, "bayonnerpc"))
			rpcFault(404, "Invalid RPC");

		if(!xmlsize)
			rpcFault(411, "No size specified");

		if(xmlsize >= MAX_XML_REQUEST - 1)
			rpcFault(413, "Request too large");

		if(!xmltext)
			rpcFault(406, "Not text/xml document");

		if(readData(buffer, xmlsize) != (ssize_t)xmlsize)
			rpcFault(400, "Request incomplete");

		buffer[xmlsize] = 0;
		if(!parseCall(buffer))
			rpcFault(400, "Malformed Request");

		transport.buffer = new char[MAX_XML_REPLY];
		transport.bufsize = MAX_XML_REPLY;
		transport.bufused = 0;
		transport.buffer[0] = 0;

		result.code = 200;
		result.string = "OK";

		if(!invokeXMLRPC())
			sendFault(1, "Unknown Method");

		if(result.code && result.code != 200)
			rpcFault(result.code, result.string);

		sendHeader("text/xml", strlen(transport.buffer));
		sendText(transport.buffer);								
		syncExit();
	}			

	if(!stricmp(req_path, "status.html"))
		htmlStatus();

	if(!stricmp(req_path, "sessions.html"))
		htmlSessions();

	if(!stricmp(req_path, "calls.html"))
		htmlCalls();

	if(!stricmp(req_path, "control.html"))
		htmlControl();

	slog.notice("webservice/%d: unknown page %s", so, req_path);

	cp = strrchr(req_path, '.');

	rpcFault(404, "Not Found");	
	syncExit();
}

void Session::setComplete(BayonneSession *s)
{
	EventStream::create(so, s->getSessionId());
	s->setConst("session.exit_manager", "webservice");
	s->leave();
	so = INVALID_SOCKET;
	state = Socket::INITIAL;
	syncExit();
}

void Session::postDocument(const char *file, unsigned long size)
{
    char nbuf[256];
	char tbuf[256];
	char dbuf[512];
    FILE *fp = NULL;
	int len;
	char *ep;
	const char *ext = strrchr(file, '.');

	if(strstr(file, ".."))
		goto nowrite;

	if(strstr(file, "/."))
		goto nowrite;

	if(!strchr(file, '/'))
		goto nowrite;

	if(!ext || ext < strchr(file, '/'))
		goto nowrite;

	if(strchr(file, ':'))
		goto nowrite;
	
	snprintf(nbuf, sizeof(nbuf), "%s/%s",
		Service::webservice.getString("datafiles"), file);

	strcpy(tbuf, nbuf);
	ep = strrchr(tbuf, '/') + 1;
	*ep = 0;
	snprintf(ep, 256 - strlen(tbuf), ".tmp.%d.%s", so, ext);

	remove(tbuf);
	fp = fopen(tbuf, "w");
	if(!fp)
nowrite:
		rpcFault(403, "Not Writable");

	while(size)
	{
		if(size > sizeof(dbuf))
			len = sizeof(dbuf);
		else
			len = (int)size;

		if(readData(dbuf, len) < len)
		{
fault:
			remove(tbuf);
			fclose(fp);
			rpcFault(403, "Write Failed");
		}
		if(fwrite(dbuf, 1, len, fp) < 1)
			goto fault;
	}
	fclose(fp);
	rename(tbuf, nbuf);
	rpcReply(NULL);
}

void Session::binDocument(const char *file, const char *ctype)
{
    char buf[256];
    FILE *fp;
	int len;
	struct stat ino;

	if(strchr(file, '/'))
		snprintf(buf, sizeof(buf), "%s/%s",
			server->getLast("prefix"), file);
	else
    	snprintf(buf, sizeof(buf), "%s/%s",
        	Service::webservice.getString("datafiles"), file);

	if(stat(buf, &ino))
		rpcFault(404, "Not Found");
	
	fp = fopen(buf, "r");
	if(!fp)
		rpcFault(403, "Not Accessible");

	sendHeader(ctype, ino.st_size);
	for(;;)
	{
		len = fread(buf, 1, sizeof(buf), fp);	
		if(len < 1)
			break;

		if(writeData(buf, len) < len)
			syncExit();
	}
	fclose(fp);
	syncExit();
}

void Session::textDocument(const char *file, const char *ctype)
{
	char buf[256];
	FILE *fp;
	char *ep;

	if(strchr(file, '/'))
		snprintf(buf, sizeof(buf), "%s/%s",
			server->getLast("prefix"), file);
	else
		snprintf(buf, sizeof(buf), "%s/%s", 
			Service::webservice.getString("datafiles"), file);

	fp = fopen(buf, "r");
	if(!fp)
		rpcFault(404, "Not Found");

	sendHeader(ctype);

	for(;;)
	{
		fgets(buf, sizeof(buf), fp);
		if(feof(fp))
			break;

		ep = strrchr(buf, '\r');
		if(!ep || (ep[1] && ep[1] != '\n') )
			ep = strchr(buf, '\n');

		if(ep)
		{
			*ep = 0;
			sendText(buf);
			sendText("\r\n");
		}
		else
			sendText(buf);
	}
	fclose(fp);
	syncExit();
}
	
} // namespace 
