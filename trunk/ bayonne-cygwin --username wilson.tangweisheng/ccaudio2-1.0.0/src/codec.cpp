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
#include <cmath>
#include <cstdio>

#ifdef	HAVE_PTHREAD_H
#include <pthread.h>
#endif

#ifndef	M_PI
#define	M_PI	3.14159265358979323846
#endif

#if defined(_MSC_VER) && _MSC_VER >= 1300
#if defined(_WIN64)
#define	RLL_SUFFIX ".x64"
#elif defined(_M_IX86)
#define	RLL_SUFFIX ".x86"
#else
#define RLL_SUFFIX ".xlo"
#endif
#endif

#if !defined(RLL_SUFFIX) && defined(W32)  && !defined(__MINGW32__) && !defined(__CYGWIN32__)
#define	RLL_SUFFIX ".rll"
#endif

#ifndef	RLL_SUFFIX
#define	RLL_SUFFIX ".so"
#endif

static ccAudio_Mutex_ lock;

#if defined(HAVE_PTHREAD_H)

ccAudio_Mutex_::ccAudio_Mutex_()
{
	pthread_mutexattr_t attr;

	pthread_mutexattr_init(&attr);
	pthread_mutex_init(&mutex, &attr);
}

ccAudio_Mutex_::~ccAudio_Mutex_()
{
	pthread_mutex_destroy(&mutex);
}

void ccAudio_Mutex_::enter(void)
{
	pthread_mutex_lock(&mutex);
}

void ccAudio_Mutex_::leave(void)
{
	pthread_mutex_unlock(&mutex);
}

#elif defined(W32)

ccAudio_Mutex_::ccAudio_Mutex_()
{
	InitializeCriticalSection(&mutex);
}

ccAudio_Mutex_::~ccAudio_Mutex_()
{
	DeleteCriticalSection(&mutex);
}

void ccAudio_Mutex_::enter(void)
{
	EnterCriticalSection(&mutex);
}

void ccAudio_Mutex_::leave(void)
{
	LeaveCriticalSection(&mutex);
}

#else

ccAudio_Mutex_::ccAudio_Mutex_() {};
ccAudio_Mutex_::~ccAudio_Mutex_() {};
void ccAudio_Mutex_::enter(void) {};
void ccAudio_Mutex_::leave(void) {};

#endif

using namespace ost;

AudioCodec *AudioCodec::first = NULL;

AudioCodec::AudioCodec(const char *n, Encoding e)
{
	encoding = e;
	name = n;
	next = first;
	first = this;

	info.clear();
	info.format = raw;
	info.encoding = e;
}

AudioCodec::AudioCodec()
{
	name = NULL;

	info.clear();
	info.format = raw;
}

void AudioCodec::endCodec(AudioCodec *codec)
{
	if(!codec->name)
		delete codec;
}

bool AudioCodec::load(Encoding e)
{
	switch(e) {
	case mulawAudio:
	case alawAudio:
		return load("g.711");
	case g721ADPCM:
	case g723_3bit:
	case g723_5bit:
	case g723_2bit:
		return load("adpcm");
	case okiADPCM:
	case voxADPCM:
		return load("oki");
	case mp1Audio:
	case mp2Audio:
	case mp3Audio:
		return load("mpg");
	case gsmVoice:
		return load("gsm");
	case msgsmVoice:
		return load("msgsm");
	case sx73Voice:
	case sx96Voice:
		return load("sx7396");
	default:
		return false;
	}
}

bool AudioCodec::load(const char *name)
{
	char path[256];

	char fn[16];
	char *p = fn;
	char *q = fn;

	snprintf(fn, sizeof(fn) - 3, "%s", name);
	while(*p) {
		if(*p != '.')
			*(q++) = *p;
		++p;
	}
	*q = 0;

	snprintf(path, sizeof(path), "%s/%s%s", Audio::getCodecPath(), fn, RLL_SUFFIX);
	return loadPlugin(path);
}

AudioCodec *AudioCodec::getCodec(Encoding e, const char *format, bool loaded)
{
	AudioCodec *codec;
	lock.enter();
retry:
	codec = first;

	while(codec) {
		if(e == codec->encoding)
			break;
		codec = codec->next;
	}

	if(!codec && !loaded)
		if(load(e)) {
			loaded = true;
			goto retry;
		}

	lock.leave();

	if(codec && format)
		return codec->getByFormat(format);

	return codec;
}

AudioCodec *AudioCodec::getCodec(Info &info, bool loaded)
{
	AudioCodec *codec;
	lock.enter();
retry:
	codec = first;

	while(codec) {
		if(info.encoding == codec->encoding)
			break;
		codec = codec->next;
	}

	if(!codec && !loaded)
		if(load(info.encoding)) {
			loaded = true;
			goto retry;
		}

	lock.leave();

	if(codec)
		return codec->getByInfo(info);

	return codec;
}

bool AudioCodec::isSilent(Level hint, void *data, unsigned samples)
{
	Level power = getImpulse(data, samples);

	if(power < 0)
		return true;

	if(power > hint)
		return false;

	return true;
}

Audio::Level AudioCodec::getImpulse(void *data, unsigned samples)
{
	unsigned long sum = 0;
	Linear ldata = new Sample[samples];
	Linear lptr = ldata;
	long count = decode(ldata, data, samples);

	samples = count;
	while(samples--) {
		if(*ldata < 0)
			sum -= *(ldata++);
		else
			sum += *(ldata++);
	}

	delete[] lptr;
	return (Level)(sum / count);
}

Audio::Level AudioCodec::getPeak(void *data, unsigned samples)
{
	Level max = 0, value;
	Linear ldata = new Sample[samples];
	Linear lptr = ldata;
	long count = decode(ldata, data, samples);

	samples = count;
	while(samples--) {
		value = *(ldata++);
		if(value < 0)
			value = -value;
		if(value > max)
			max = value;
	}

	delete[] lptr;
	return max;
}

unsigned AudioCodec::getEstimated(void)
{
	return info.framesize;
}

unsigned AudioCodec::getRequired(void)
{
	return info.framecount;
}

unsigned AudioCodec::encodeBuffered(Linear buffer, Encoded source, unsigned samples)
{
	return encode(buffer, source, samples);
}

unsigned AudioCodec::decodeBuffered(Linear buffer, Encoded source, unsigned bytes)
{
	return decode(buffer, source, toSamples(info, bytes));
}

unsigned AudioCodec::getPacket(Encoded packet, Encoded data, unsigned bytes)
{
	if(bytes != info.framesize)
		return 0;

	memcpy(packet, data, bytes);
	return bytes;
}

