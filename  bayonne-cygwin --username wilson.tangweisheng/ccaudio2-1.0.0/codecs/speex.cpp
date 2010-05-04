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

#include <speex/speex.h>

namespace ccAudioCodec {
using namespace ost;

class SpeexCommon: public AudioCodec
{
protected:
	const SpeexMode *spx_mode;
	SpeexBits enc_bits, dec_bits;
	unsigned int spx_clock, spx_channel;
	void *encoder, *decoder;
	int spx_frame;

public:
	SpeexCommon(Encoding enc, const char *name);
	SpeexCommon();
	~SpeexCommon();

	unsigned encode(Linear buffer, void *dest, unsigned lsamples);
	unsigned decode(Linear buffer, void *source, unsigned lsamples);

	AudioCodec *getByInfo(Info &info);
	AudioCodec *getByFormat(const char *format);
};

class SpeexAudio: public SpeexCommon
{
public:
	SpeexAudio();
};

class SpeexVoice: public SpeexCommon
{
public:
	SpeexVoice();
};

SpeexCommon::SpeexCommon(Encoding enc, const char *id) :
AudioCodec("speex", enc)
{
	info.framesize = 20;
	info.framecount = 160;
	info.rate = 8000;
	info.bitrate = 24000;
	info.annotation = "speex/8000";

	spx_channel = 1;

	switch(enc) {
	case speexVoice:
		spx_clock = 8000;
		spx_mode = &speex_nb_mode;
		break;
	case speexAudio:
		info.annotation = "speex/16000";
		info.framesize = 40;
		info.rate = 16000;
		spx_clock = 16000;
		spx_mode = &speex_wb_mode;
	default:
		break;
	}

	encoder = decoder = NULL;
}

SpeexCommon::SpeexCommon() :
AudioCodec()
{
}

SpeexCommon::~SpeexCommon()
{
	if(decoder) {
		speex_bits_destroy(&dec_bits);
		speex_decoder_destroy(decoder);
	}
	if(encoder) {
		speex_bits_destroy(&enc_bits);
		speex_encoder_destroy(encoder);
	}
	decoder = encoder = NULL;
}

AudioCodec *SpeexCommon::getByFormat(const char *format)
{
	if(!strnicmp(format, "speex/16", 8))
		return (AudioCodec *)new SpeexAudio();
	return (AudioCodec *)new SpeexVoice();
}

AudioCodec *SpeexCommon::getByInfo(Info &info)
{
	switch(info.encoding) {
	case speexAudio:
	        return (AudioCodec *)new SpeexAudio();
	default:
		return (AudioCodec *)new SpeexVoice();
	}
}

unsigned SpeexCommon::decode(Linear buffer, void *src, unsigned lsamples)
{
	unsigned count = lsamples / info.framecount;
	unsigned result = 0;
	char *encoded = (char *)src;

	if(!count)
		return 0;

	while(count--) {
		speex_bits_read_from(&dec_bits, encoded, info.framesize);
		if(speex_decode_int(decoder, &dec_bits, buffer))
			break;
		result += info.framesize;
	}
	return result;
}

unsigned SpeexCommon::encode(Linear buffer, void *dest, unsigned lsamples)
{
	unsigned count = lsamples / info.framecount;
	unsigned result = 0;
	char *encoded = (char *)dest;

	if(!count)
		return 0;

	while(count--) {
		speex_bits_reset(&enc_bits);
		speex_encoder_ctl(encoder, SPEEX_SET_SAMPLING_RATE, &spx_clock);
		speex_encode_int(encoder, buffer, &enc_bits);
		int nb = speex_bits_write(&enc_bits, encoded, info.framesize);
		buffer += 160;
		encoded += nb;
		result += nb;
	}
	return result;
}

SpeexAudio::SpeexAudio() :
SpeexCommon()
{
	info.encoding = speexVoice;
	info.framesize = 40;
	info.framecount = 160;
	info.rate = 16000;
	info.bitrate = 48000;
	info.annotation = "SPEEX/16000";
	spx_clock = 16000;
	spx_channel = 1;
	spx_mode = &speex_wb_mode;
	speex_bits_init(&dec_bits);
	decoder = speex_decoder_init(spx_mode);
	speex_bits_init(&enc_bits);
	encoder = speex_encoder_init(spx_mode);
	speex_decoder_ctl(decoder, SPEEX_GET_FRAME_SIZE, &spx_frame);
	info.framecount = spx_frame;
	info.set();
}

SpeexVoice::SpeexVoice() :
SpeexCommon()
{
	info.encoding = speexVoice;
	info.framesize = 20;
	info.framecount = 160;
	info.rate = 8000;
	info.bitrate = 24000;
	info.annotation = "SPEEX/8000";
	spx_clock = 8000;
	spx_channel = 1;
	spx_mode = &speex_nb_mode;
	speex_bits_init(&dec_bits);
	decoder = speex_decoder_init(spx_mode);
	speex_bits_init(&enc_bits);
	encoder = speex_encoder_init(spx_mode);
	speex_decoder_ctl(decoder, SPEEX_GET_FRAME_SIZE, &spx_frame);
	info.framecount = spx_frame;
	info.set();

}

static SpeexCommon codec(Audio::speexVoice, "speex");

// namespace
}

