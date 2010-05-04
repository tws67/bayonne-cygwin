// Copyright (C) 2006-2008 David Sugar, Tycho Softworks.
//
// This file is part of GNU ucommon.
//
// GNU ucommon is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published 
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// GNU ucommon is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with GNU ucommon.  If not, see <http://www.gnu.org/licenses/>.

#if defined(OLD_STDCPP) || defined(NEW_STDCPP)

#include <config.h>
#include <ucommon/thread.h>
#include <ucommon/socket.h>
#include <ucommon/string.h>
#include <ucommon/stream.h>
#include <stdarg.h>

#ifndef	_MSWINDOWS_
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <fcntl.h>
#endif

#ifdef	HAVE_SYS_RESOURCE_H
#include <sys/time.h>
#include <sys/resource.h>
#endif

using namespace UCOMMON_NAMESPACE;
using namespace std;

tcpstream::tcpstream(const tcpstream &copy) :
	streambuf(),
#ifdef OLD_STDCPP
	iostream()
#else
	iostream((streambuf *)this)
#endif
{
	bufsize = 0;
	gbuf = pbuf = NULL;
	so = Socket::create(Socket::getfamily(copy.so), SOCK_STREAM, IPPROTO_TCP);

#ifdef OLD_STDCPP
	init((streambuf *)this);
#endif
	timeout = copy.timeout;
}

tcpstream::tcpstream(int family, timeout_t tv) :
	streambuf(), 
#ifdef OLD_STDCPP
	iostream()
#else
	iostream((streambuf *)this)
#endif
{
	bufsize = 0;
	gbuf = pbuf = NULL;
	so = Socket::create(family, SOCK_STREAM, IPPROTO_TCP);

#ifdef OLD_STDCPP
	init((streambuf *)this);
#endif
	timeout = tv;
}

tcpstream::tcpstream(Socket::address& list, unsigned segsize, timeout_t tv) :
	streambuf(),
#ifdef OLD_STDCPP
	iostream()
#else
	iostream((streambuf *)this)
#endif
{
	bufsize = 0;
	gbuf = pbuf = NULL;
	so = Socket::create(list.getfamily(), SOCK_STREAM, IPPROTO_TCP);

#ifdef OLD_STDCPP
	init((streambuf *)this);
#endif
	timeout = tv;
	open(list);
}

tcpstream::tcpstream(ListenSocket& listener, unsigned segsize, timeout_t tv) :
	streambuf(), 
#ifdef OLD_STDCPP
	iostream()
#else
	iostream((streambuf *)this)
#endif
{
	bufsize = 0;
	gbuf = pbuf = NULL;
	so = listener.accept();

#ifdef OLD_STDCPP
	init((streambuf *)this);
#endif
	timeout = tv;
	if(so == INVALID_SOCKET) {
		clear(ios::failbit | rdstate());
		return;
	}
	allocate(segsize);
}

tcpstream::~tcpstream()
{
	tcpstream::release();
}

void tcpstream::release(void)
{
	if(gbuf)
		delete[] gbuf;

	if(pbuf)
		delete[] pbuf;

	clear();
	Socket::release(so);
}

int tcpstream::uflow()
{
    int ret = underflow();

    if (ret == EOF)
        return EOF;

    if (bufsize != 1)
        gbump(1);

    return ret;
}

#ifndef	MSG_WAITALL
#define	MSG_WAITALL	0
#endif

int tcpstream::underflow()
{
	ssize_t rlen = 1;
	unsigned char ch;

	if(bufsize == 1) {
        if(timeout && !Socket::wait(so, timeout)) {
            clear(ios::failbit | rdstate());
            return EOF;
        }
        else
            rlen = Socket::recvfrom(so, &ch, 1, MSG_WAITALL);
        if(rlen < 1) {
            if(rlen < 0)
				reset();
            return EOF;
        }
        return ch;
    }

    if(!gptr())
        return EOF;

    if(gptr() < egptr())
        return (unsigned char)*gptr();

    rlen = (ssize_t)((gbuf + bufsize) - eback());
    if(timeout && !Socket::wait(so, timeout)) {
        clear(ios::failbit | rdstate());
        return EOF;
    }
    else {
        rlen = Socket::recvfrom(so, eback(), rlen, MSG_WAITALL);
	}
    if(rlen < 1) {
//      clear(ios::failbit | rdstate());
        if(rlen < 0)
			reset();
        else
            clear(ios::failbit | rdstate());
        return EOF;
    }

    setg(eback(), eback(), eback() + rlen);
    return (unsigned char) *gptr();
}

int tcpstream::overflow(int c)
{
    unsigned char ch;
    ssize_t rlen, req;

    if(bufsize == 1) {
        if(c == EOF)
            return 0;

		ch = (unsigned char)(c);
        rlen = Socket::sendto(so, &ch, 1);
        if(rlen < 1) {
            if(rlen < 0) 
				reset();
            return EOF;
        }
        else
            return c;
    }

    if(!pbase())
        return EOF;

    req = (ssize_t)(pptr() - pbase());
    if(req) {
        rlen = Socket::sendto(so, pbase(), req);
        if(rlen < 1) {
            if(rlen < 0) 
				reset();
            return EOF;
        }
        req -= rlen;
    }
    // if write "partial", rebuffer remainder

    if(req)
//      memmove(pbuf, pptr() + rlen, req);
        memmove(pbuf, pbuf + rlen, req);
    setp(pbuf, pbuf + bufsize);
    pbump(req);

    if(c != EOF) {
        *pptr() = (unsigned char)c;
        pbump(1);
    }
    return c;
}

void tcpstream::open(Socket::address& list, unsigned mss)
{
	if(bufsize)
		close();	// close if existing is open...

	if(Socket::connectto(so, *list))
		return;

	allocate(mss);
}	

void tcpstream::reset(void)
{
	if(!bufsize)
		return;

	if(gbuf)
		delete[] gbuf;

	if(pbuf)
		delete[] pbuf;

	gbuf = pbuf = NULL;
	bufsize = 0;
	clear();
	Socket::disconnect(so);
}

void tcpstream::close(void)
{
	if(!bufsize)
		return;

	sync();

	if(gbuf)
		delete[] gbuf;

	if(pbuf)
		delete[] pbuf;

	gbuf = pbuf = NULL;
	bufsize = 0;
	clear();
	Socket::disconnect(so);
}

void tcpstream::allocate(unsigned mss)
{
	unsigned size = mss;
	unsigned max = 0;
#ifdef	TCP_MAXSEG
	socklen_t alen = sizeof(max);
#endif
	
	if(mss == 1)
		goto allocate;

#ifdef  TCP_MAXSEG
	if(mss)
		setsockopt(so, IPPROTO_TCP, TCP_MAXSEG, (char *)&max, sizeof(max));
	getsockopt(so, IPPROTO_TCP, TCP_MAXSEG, (char *)&max, &alen);
#endif

	if(max && max < mss)
		mss = max;

	if(!mss) {
		if(max)
			mss = max;
		else
			mss = 536;
		goto allocate;
	}

#ifdef  TCP_MAXSEG
	setsockopt(so, IPPROTO_TCP, TCP_MAXSEG, (char *)&mss, sizeof(mss));
#endif

	if(mss < 80)
		mss = 80;

	if(mss * 7 < 64000u)
		bufsize = mss * 7;
	else if(mss * 6 < 64000u)
		bufsize = mss * 6;
	else
		bufsize = mss * 5;

	Socket::sendsize(so, bufsize);
	Socket::recvsize(so, bufsize);

	if(mss < 512)
		Socket::sendwait(so, mss * 4);

allocate:
	if(gbuf)
		delete[] gbuf;

	if(pbuf)
		delete[] pbuf;

	gbuf = pbuf = NULL;

	if(size < 2) {
		bufsize = 1;
		return;
	}

	gbuf = new char[size];
	pbuf = new char[size];
	assert(gbuf != NULL && pbuf != NULL);
	bufsize = size;
	clear();
#if (defined(__GNUC__) && (__GNUC__ < 3)) && !defined(MSWINDOWS) && !defined(STLPORT)
    setb(gbuf, gbuf + size, 0);
#endif
    setg(gbuf, gbuf + size, gbuf + size);
    setp(pbuf, pbuf + size);
}

int tcpstream::sync(void)
{
	if(!bufsize)
		return 0;

	overflow(EOF);
	setg(gbuf, gbuf + bufsize, gbuf + bufsize);
	return 0;
}

pipestream::pipestream() :
streambuf(),
#ifdef OLD_STDCPP
	iostream()
#else
	iostream((streambuf *)this)
#endif
{
	bufsize = 0;
	gbuf = pbuf = NULL;
#ifdef OLD_STDCPP
	init((streambuf *)this);
#endif
}

pipestream::pipestream(const char *cmd, access_t access, const char **envp, size_t size) :
streambuf(),
#ifdef OLD_STDCPP
	iostream()
#else
	iostream((streambuf *)this)
#endif
{
bufsize = 0;
	gbuf = pbuf = NULL;
#ifdef OLD_STDCPP
	init((streambuf *)this);
#endif
	open(cmd, access, envp, size);
}

pipestream::~pipestream()
{
	close();
}

#ifdef	_MSWINDOWS_
void pipestream::open(const char *cmd, access_t mode, const char **envp, size_t size)
{
	PROCESS_INFORMATION pi;
	STARTUPINFO si;
	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = true;
	HANDLE inputWriteTmp, inputRead,inputWrite;
	HANDLE outputReadTmp, outputRead, outputWrite;
	HANDLE errorWrite;
	char cmdspec[128];
	char *ep = NULL;
	unsigned len = 0;

	if(envp)
		ep = new char[4096];

	while(envp && *envp && len < 4090) {
		String::set(ep + len, 4094 - len, *envp);
		len += strlen(*(envp++)) + 1;
	}

	if(ep)
		ep[len] = 0;

	GetEnvironmentVariable("ComSpec", cmdspec, sizeof(cmdspec));

	if(mode == WRONLY || mode == RDWR) {
		CreatePipe(&inputRead, &inputWriteTmp,&sa,0);
		DuplicateHandle(GetCurrentProcess(),inputWriteTmp,
            GetCurrentProcess(), &inputWrite, 0,FALSE, DUPLICATE_SAME_ACCESS);
		CloseHandle(&inputRead);
	}
	if(mode == RDONLY || mode == RDWR) {
		CreatePipe(&outputReadTmp, &outputWrite,&sa,0);
		DuplicateHandle(GetCurrentProcess(),outputWrite,
            GetCurrentProcess(),&errorWrite,0, TRUE,DUPLICATE_SAME_ACCESS);
		DuplicateHandle(GetCurrentProcess(),outputReadTmp,
            GetCurrentProcess(), &outputRead, 0,FALSE, DUPLICATE_SAME_ACCESS);
		CloseHandle(&outputReadTmp);
	}

	memset(&si, 0, sizeof(si));
	si.cb = sizeof(STARTUPINFO);
	si.dwFlags = STARTF_USESTDHANDLES;
	
	if(mode == RDONLY || mode == RDWR) {
		si.hStdOutput = outputWrite;
		si.hStdError = errorWrite;
	}

	if(mode == WRONLY || mode == RDWR)
		si.hStdInput = inputRead;

	if(!CreateProcess(cmdspec, (char *)cmd, NULL, NULL, TRUE,
		CREATE_NEW_CONSOLE, ep, NULL, &si, &pi))
		size = 0;
	else
		pid = pi.dwProcessId;

	if(ep)
		delete[] ep;

	if(mode == WRONLY || mode == RDWR) {
		fsys::assign(wr, &inputWrite);
		CloseHandle(inputRead);
	}
	if(mode == RDONLY || mode == RDWR) {
		fsys::assign(rd, &outputRead);
		CloseHandle(outputWrite);
		CloseHandle(errorWrite);
	}
	if(size)
		allocate(size, mode);
	else {
		fsys::close(rd);
		fsys::close(wr);
	}
}

void pipestream::terminate(void)
{
	HANDLE hProc;
	if(bufsize) {
		hProc = OpenProcess(SYNCHRONIZE|PROCESS_TERMINATE, FALSE, pid);
		if(hProc != NULL) {
			TerminateProcess(hProc,0);
			CloseHandle(hProc);
		}
		release();
	}
}

void pipestream::close(void)
{
	HANDLE hProc;
	if(bufsize) {
		release();
		hProc = OpenProcess(SYNCHRONIZE|PROCESS_TERMINATE, FALSE, pid);
		if(hProc != NULL) {
			WaitForSingleObject(hProc, INFINITE);
			CloseHandle(hProc);
		}
	}
}

#else

void pipestream::terminate(void)
{
	if(bufsize) {
		kill(pid, SIGTERM);
		close();
	}
}

void pipestream::close(void)
{
	if(bufsize) {
		release();
		waitpid(pid, NULL, 0);
	}
}

void pipestream::open(const char *cmd, access_t mode, const char **envp, size_t size)
{
	int input[2], output[2];
	int max = sizeof(fd_set) * 8;
	char symname[129];
	const char *cp;
	char *ep;

#ifdef	RLIMIT_NOFILE
	struct rlimit rlim;

	if(!getrlimit(RLIMIT_NOFILE, &rlim))
		max = rlim.rlim_max;
#endif

	close();
	
	if(mode == RDONLY || mode == RDWR) {
		if(pipe(input))
			return;
		fsys::assign(rd, input[0]);
	}
	else
		input[1] = ::open("/dev/null", O_RDWR);

	if(mode == WRONLY || mode == RDWR) {
		if(pipe(output)) {
			if(mode == RDWR) {
				::close(input[0]);
				::close(input[1]);
			}
			return;
		}
		fsys::assign(wr, output[1]);
	}
	else
		output[0] = ::open("/dev/null", O_RDWR);

	pid = fork();
	if(pid) {
		if(mode == RDONLY || mode == RDWR)
			::close(input[1]);
		if(mode == WRONLY || mode == RDWR)		
			::close(output[0]);
		if(pid == -1) {
			fsys::close(rd);
			fsys::close(wr);
		}
		else	
			allocate(size, mode);
		return;
	}
	dup2(input[1], 1);
	dup2(output[0], 0);
	for(int fd = 3; fd < max; ++fd)
		::close(fd);

	while(envp && *envp) {
		String::set(symname, sizeof(symname), *envp);
		ep = strchr(symname, '=');
		if(ep)
			*ep = 0;
		cp = strchr(*envp, '=');
		if(cp)
			++cp;
		::setenv(symname, cp, 1);
		++envp;
	}

	::signal(SIGQUIT, SIG_DFL);
	::signal(SIGINT, SIG_DFL);
	::signal(SIGCHLD, SIG_DFL);
	::signal(SIGPIPE, SIG_DFL);
	::execlp("/bin/sh", "sh", "-c", cmd, NULL);
	exit(127);
}

#endif

void pipestream::release(void)
{
	if(!bufsize)
		return;

	sync();

	if(gbuf) {
		fsys::close(rd);
		delete[] gbuf;
	}

	if(pbuf) {
		fsys::close(wr);
		delete[] pbuf;
	}

	gbuf = pbuf = NULL;
	bufsize = 0;
	clear();
}

void pipestream::allocate(size_t size, access_t mode)
{
	if(gbuf)
		delete[] gbuf;

	if(pbuf)
		delete[] pbuf;

	gbuf = pbuf = NULL;

	if(size < 2) {
		bufsize = 1;
		return;
	}

	if(mode == RDONLY || mode == RDWR)
		gbuf = new char[size];
	if(mode == WRONLY || mode == RDWR)
		pbuf = new char[size];
	bufsize = size;
	clear();
	if(mode == RDONLY || mode == RDWR) {
#if (defined(__GNUC__) && (__GNUC__ < 3)) && !defined(MSWINDOWS) && !defined(STLPORT)
	    setb(gbuf, gbuf + size, 0);
#endif
		setg(gbuf, gbuf + size, gbuf + size);
	}
	if(mode == WRONLY || mode == RDWR)
		setp(pbuf, pbuf + size);
}

int pipestream::sync(void)
{
	if(!bufsize)
		return 0;

	overflow(EOF);
	setg(gbuf, gbuf + bufsize, gbuf + bufsize);
	return 0;
}

int pipestream::underflow()
{
	ssize_t rlen = 1;
	unsigned char ch;

	if(!gbuf)
		return EOF;

	if(bufsize == 1) {
        rlen = fsys::read(rd, &ch, 1);
        if(rlen < 1) {
            if(rlen < 0)
				close();
            return EOF;
        }
        return ch;
    }

    if(!gptr())
        return EOF;

    if(gptr() < egptr())
        return (unsigned char)*gptr();

    rlen = (ssize_t)((gbuf + bufsize) - eback());
	rlen = fsys::read(rd, eback(), rlen);
    if(rlen < 1) {
//      clear(ios::failbit | rdstate());
        if(rlen < 0)
			close();
        else
            clear(ios::failbit | rdstate());
        return EOF;
    }

    setg(eback(), eback(), eback() + rlen);
    return (unsigned char) *gptr();
}

int pipestream::uflow()
{
    int ret = underflow();

    if (ret == EOF)
        return EOF;

    if (bufsize != 1)
        gbump(1);

    return ret;
}

int pipestream::overflow(int c)
{
    unsigned char ch;
    ssize_t rlen, req;

	if(!pbuf)
		return EOF;
	
    if(bufsize == 1) {
        if(c == EOF)
            return 0;

		ch = (unsigned char)(c);
        rlen = fsys::write(wr, &ch, 1);
        if(rlen < 1) {
            if(rlen < 0) 
				close();
            return EOF;
        }
        else
            return c;
    }

    if(!pbase())
        return EOF;

    req = (ssize_t)(pptr() - pbase());
    if(req) {
        rlen = fsys::write(wr, pbase(), req);
        if(rlen < 1) {
            if(rlen < 0) 
				close();
            return EOF;
        }
        req -= rlen;
    }
    // if write "partial", rebuffer remainder

    if(req)
//      memmove(pbuf, pptr() + rlen, req);
        memmove(pbuf, pbuf + rlen, req);
    setp(pbuf, pbuf + bufsize);
    pbump(req);

    if(c != EOF) {
        *pptr() = (unsigned char)c;
        pbump(1);
    }
    return c;
}

filestream::filestream() :
streambuf(),
#ifdef OLD_STDCPP
	iostream()
#else
	iostream((streambuf *)this)
#endif
{
	bufsize = 0;
	gbuf = pbuf = NULL;
#ifdef OLD_STDCPP
	init((streambuf *)this);
#endif
}

filestream::filestream(const filestream& copy) :
streambuf(),
#ifdef OLD_STDCPP
	iostream()
#else
	iostream((streambuf *)this)
#endif
{
	bufsize = 0;
	gbuf = pbuf = NULL;
#ifdef OLD_STDCPP
	init((streambuf *)this);
#endif
	if(copy.bufsize) 
		fd = copy.fd;
	if(is(fd))
		allocate(copy.bufsize, copy.ac);
}


filestream::filestream(const char *filename, fsys::access_t mode, size_t size) :
streambuf(),
#ifdef OLD_STDCPP
	iostream()
#else
	iostream((streambuf *)this)
#endif
{
bufsize = 0;
	gbuf = pbuf = NULL;
#ifdef OLD_STDCPP
	init((streambuf *)this);
#endif
	open(filename, mode, size);
}

filestream::filestream(const char *filename, fsys::access_t access, unsigned mode, size_t size) :
streambuf(),
#ifdef OLD_STDCPP
	iostream()
#else
	iostream((streambuf *)this)
#endif
{
bufsize = 0;
	gbuf = pbuf = NULL;
#ifdef OLD_STDCPP
	init((streambuf *)this);
#endif
	create(filename, access, mode, size);
}

filestream::~filestream()
{
	close();
}

void filestream::seek(fsys::offset_t offset)
{
	if(bufsize) {
		sync();
		fsys::seek(fd, offset);
	}
}

void filestream::close(void)
{
	if(bufsize) {
		if(gbuf) {
			delete[] gbuf;
			gbuf = NULL;
		}
		if(pbuf) {
			delete[] pbuf;
			pbuf = NULL;
		}
		bufsize = 0;
		fsys::close(fd);
	}
}

void filestream::allocate(size_t size, fsys::access_t mode)
{
	if(gbuf)
		delete[] gbuf;

	if(pbuf)
		delete[] pbuf;

	gbuf = pbuf = NULL;
	ac = mode;

	if(size < 2) {
		bufsize = 1;
		return;
	}

	if(mode == fsys::ACCESS_RDONLY || fsys::ACCESS_RDWR || fsys::ACCESS_SHARED || fsys::ACCESS_DIRECTORY)
		gbuf = new char[size];
	if(mode == fsys::ACCESS_WRONLY || fsys::ACCESS_APPEND || fsys::ACCESS_SHARED || fsys::ACCESS_RDWR)
		pbuf = new char[size];
	bufsize = size;
	clear();
	if(mode == fsys::ACCESS_RDONLY || fsys::ACCESS_RDWR || fsys::ACCESS_SHARED || fsys::ACCESS_DIRECTORY) {
#if (defined(__GNUC__) && (__GNUC__ < 3)) && !defined(MSWINDOWS) && !defined(STLPORT)
	    setb(gbuf, gbuf + size, 0);
#endif
		setg(gbuf, gbuf + size, gbuf + size);
	}
	if(mode == fsys::ACCESS_WRONLY || fsys::ACCESS_APPEND || fsys::ACCESS_SHARED || fsys::ACCESS_RDWR)
		setp(pbuf, pbuf + size);
}

void filestream::create(const char *fname, fsys::access_t access, unsigned mode, size_t size)
{
	close();
	fsys::create(fd, fname, access, mode);
	if(is(fd))
		allocate(size, access);
}

void filestream::open(const char *fname, fsys::access_t access, size_t size)
{
	close();
	fsys::open(fd, fname, access);
	if(is(fd))
		allocate(size, access);
}

int filestream::sync(void)
{
	if(!bufsize)
		return 0;

	overflow(EOF);
	setg(gbuf, gbuf + bufsize, gbuf + bufsize);
	return 0;
}

int filestream::underflow()
{
	ssize_t rlen = 1;

	if(!gbuf)
		return EOF;

    if(!gptr())
        return EOF;

    if(gptr() < egptr())
        return (unsigned char)*gptr();

    rlen = (ssize_t)((gbuf + bufsize) - eback());
	rlen = fsys::read(fd, eback(), rlen);
    if(rlen < 1) {
//      clear(ios::failbit | rdstate());
        if(rlen < 0)
			close();
        else
            clear(ios::failbit | rdstate());
        return EOF;
    }

    setg(eback(), eback(), eback() + rlen);
    return (unsigned char) *gptr();
}

int filestream::uflow()
{
    int ret = underflow();

    if (ret == EOF)
        return EOF;

    if (bufsize != 1)
        gbump(1);

    return ret;
}

int filestream::overflow(int c)
{
    ssize_t rlen, req;

	if(!pbuf)
		return EOF;
	
    if(!pbase())
        return EOF;

    req = (ssize_t)(pptr() - pbase());
    if(req) {
        rlen = fsys::write(fd, pbase(), req);
        if(rlen < 1) {
            if(rlen < 0) 
				close();
            return EOF;
        }
        req -= rlen;
    }
    // if write "partial", rebuffer remainder

    if(req)
//      memmove(pbuf, pptr() + rlen, req);
        memmove(pbuf, pbuf + rlen, req);
    setp(pbuf, pbuf + bufsize);
    pbump(req);

    if(c != EOF) {
        *pptr() = (unsigned char)c;
        pbump(1);
    }
    return c;
}

#endif

