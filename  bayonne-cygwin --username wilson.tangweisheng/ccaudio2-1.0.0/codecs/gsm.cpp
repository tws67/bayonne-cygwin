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

#include "codecs.h"

extern "C" {

#ifdef	HAVE_GSM_GSM_H
#include <gsm/gsm.h>
#else
#include <gsm.h>
#endif

}

namespace ccAudioCodec {
using namespace ost;

class GSMCodec : protected AudioCodec
{
protected:
	gsm encoder, decoder;

public:
	AudioCodec *getByInfo(Info &info);
	AudioCodec *getByFormat(const char *format);

	unsigned encode(Linear data, void *dest, unsigned samples);
	unsigned decode(Linear data, void *source, unsigned samples);

	GSMCodec(const char *id, Encoding e);
	GSMCodec();
	~GSMCodec();

};

GSMCodec::GSMCodec()
{
	encoder = gsm_create();
	decoder = gsm_create();
	info.framesize = 33;
	info.framecount = 160;
	info.rate = 8000;
	info.bitrate = 13200;
	info.annotation = (char *)"gsm";
	info.encoding = gsmVoice;
}

GSMCodec::GSMCodec(const char *id, Encoding e) : AudioCodec(id, e)
{
	encoder = gsm_create();
	decoder = gsm_create();
	info.framesize = 33;
	info.framecount = 160;
	info.rate = 8000;
	info.bitrate = 13200;
	info.annotation = (char *)"gsm";
}

GSMCodec::~GSMCodec()
{
	gsm_destroy(encoder);
	gsm_destroy(decoder);
}

AudioCodec *GSMCodec::getByInfo(Info &info)
{
	return (AudioCodec *)new GSMCodec();
}

AudioCodec *GSMCodec::getByFormat(const char *format)
{
	return (AudioCodec *)new GSMCodec();
}

unsigned GSMCodec::encode(Linear from, void *dest, unsigned samples)
{
	unsigned count = samples / 160;
	unsigned result = count * 33;
	gsm_byte *encoded = (gsm_byte *)dest;

	if(!count)
		return 0;

	while(count--) {
		gsm_encode(encoder, from, encoded);
		from += 160;
		encoded += 33;
	}
	return result;
}

unsigned GSMCodec::decode(Linear dest, void *from, unsigned samples)
{
	unsigned count = samples / 160;
	unsigned result = count * 33;
	gsm_byte *encoded = (gsm_byte *)from;
	if(!count)
		return 0;

	while(count--) {
		gsm_decode(decoder, encoded, dest);
		encoded += 160;
		dest += 160;
	}
	return result;
}

static GSMCodec codec("gsm", Audio::gsmVoice);

}
