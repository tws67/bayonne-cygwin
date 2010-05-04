// Copyright (C) 1999-2005 Open Source Telecom Corporation.
// Copyright (C) 2006-2008 David Sugar, Tycho Softworks.
//
// This file is part of GNU ccAudio2.
//
// GNU ccAudio2 is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published 
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// GNU ccAudio2 is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with GNU ccAudio2.  If not, see <http://www.gnu.org/licenses/>.

#include "private.h"
#include "audio2.h"
#include <cstdlib>
#include <cstdio>

#ifdef  W32
#define FD(x)   ((HANDLE)(x.handle))
#define SETFD(x,y) (x.handle = (void *)(y))
#ifndef	INVALID_SET_FILE_POINTER
#define	INVALID_SET_FILE_POINTER	((DWORD)(-1))
#endif
#else
#include <fcntl.h>
#include <unistd.h>
#endif

using namespace ost;

bool AudioFile::afCreate(const char *name, bool exclusive)
{
	AudioFile::close();
	mode = modeWrite;
#ifdef	WIN32
	if(exclusive)
		SETFD(file, CreateFile(name, GENERIC_READ | GENERIC_WRITE, 0,
			NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL));
	else
		SETFD(file, CreateFile(name, GENERIC_READ | GENERIC_WRITE, 0,
			NULL, TRUNCATE_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL));
#else
	// JRS: do not use creat() here.  It uses O_RDONLY by default
	// which prevents us from reading the wave header later on when
	// setting the length during AudioFile::Close().
	if(exclusive)
		file.fd = ::open(name, O_CREAT | O_EXCL | O_RDWR, 0660);
	else
		file.fd = ::open(name, O_CREAT | O_TRUNC | O_RDWR, 0660);
#endif
	return isOpen();
}

bool AudioFile::afOpen(const char *name, Mode m)
{
	AudioFile::close();
	mode = m;
#ifdef	WIN32
	switch(m) {
	case modeWrite:
	case modeCache:
		SETFD(file, CreateFile(name, GENERIC_READ | GENERIC_WRITE, 0,
			NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL));
		if(isOpen())
			break;
	case modeInfo:
	case modeRead:
	case modeFeed:
	case modeReadAny:
	case modeReadOne:
		SETFD(file, CreateFile(name,GENERIC_READ, 0,
			NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL));
	default:
		break;
	}
#else
	switch(m) {
	case modeWrite:
	case modeCache:
		file.fd = ::open(name, O_RDWR);
		if(file.fd > -1)
			break;
	case modeInfo:
	case modeRead:
	case modeFeed:
	case modeReadAny:
	case modeReadOne:
		file.fd = ::open(name, O_RDONLY);
	default:
		break;
	}
#endif
	return isOpen();
}
int AudioFile::afWrite(unsigned char *data, unsigned len)
{
#ifdef	WIN32
	DWORD count;

	if(!WriteFile(FD(file), data, (DWORD)len, &count, NULL))
		return -1;
	return count;
#else
	return ::write(file.fd, data, len);
#endif
}

int AudioFile::afRead(unsigned char *data, unsigned len)
{
#ifdef	WIN32
	DWORD count;

	if(!ReadFile(FD(file), data, (DWORD)len, &count, NULL))
		return -1;
	return count;
#else
	return ::read(file.fd, data, len);
#endif
}

bool AudioFile::afPeek(unsigned char *data, unsigned len)
{
	if(afRead(data, len) != (int)len)
		return false;
	return true;
}

bool AudioFile::afSeek(unsigned long pos)
{
#ifdef	WIN32
	if(SetFilePointer(FD(file), pos, NULL, FILE_BEGIN) != INVALID_SET_FILE_POINTER)
#else
	if(::lseek(file.fd, pos, SEEK_SET) != -1)
#endif
		return true;
	else
		return false;
}

bool AudioFile::isOpen(void)
{
#ifdef	WIN32
	if(FD(file) == INVALID_HANDLE_VALUE)
		return false;
#else
	if(file.fd < 0)
		return false;
#endif
	return true;
}

void AudioFile::afClose(void)
{
	unsigned long size = ~0;
#ifdef	WIN32
	if(FD(file) != INVALID_HANDLE_VALUE) {
		size = getPosition();
		CloseHandle(FD(file));
		if(size < minimum && pathname && mode == modeWrite)
			DeleteFile(pathname);
	}
	SETFD(file, INVALID_HANDLE_VALUE);
#else
	if(file.fd > -1) {
		size = getPosition();
		if(size < minimum && pathname && mode == modeWrite)
			::remove(pathname);
		::close(file.fd);
	}
	file.fd = -1;
#endif
}

void AudioFile::initialize(void)
{
	minimum = 0;
	pathname = NULL;
	info.annotation = NULL;
	header = 0l;
	iolimit = 0l;
	mode = modeInfo;
#ifdef	WIN32
	SETFD(file, INVALID_HANDLE_VALUE);
#else
	file.fd = -1;
#endif
}

Audio::Error AudioFile::setPosition(unsigned long samples)
{
	long pos;
	long eof;

	if(!isOpen())
		return errNotOpened;

#ifdef  WIN32
	eof = SetFilePointer(FD(file), 0l, NULL, FILE_END);
#else
	eof = ::lseek(file.fd, 0l, SEEK_END);
#endif
	if(samples == (unsigned long)~0l)
		return errSuccess;

	pos = header + toBytes(info, samples);
	if(pos > eof) {
		pos = eof;
		return errSuccess;
	}

#ifdef  WIN32
	SetFilePointer(FD(file), pos, NULL, FILE_BEGIN);
#else
	::lseek(file.fd, pos, SEEK_SET);
#endif
	return errSuccess;
}

unsigned long AudioFile::getAbsolutePosition(void)
{
	unsigned long pos;
	if(!isOpen())
		return 0;

#ifdef  WIN32
	pos = SetFilePointer(FD(file), 0l, NULL, FILE_CURRENT);
	if(pos == INVALID_SET_FILE_POINTER) {
#else
		pos = ::lseek(file.fd, 0l, SEEK_CUR);
	if(pos == (unsigned long)-1l) {
#endif
		close();
		return 0;
	}
		return pos;
	}

unsigned long AudioFile::getPosition(void)
{
		unsigned long pos;
		if(!isOpen())
		return 0;

#ifdef  WIN32
		pos = SetFilePointer(FD(file), 0l, NULL, FILE_CURRENT);
	if(pos == INVALID_SET_FILE_POINTER) {
#else
		pos = getAbsolutePosition();
	if(pos == (unsigned long)-1l) {
#endif
		close();
		return 0;
	}
		pos = toSamples(info, pos - header);
		return pos;
		}


