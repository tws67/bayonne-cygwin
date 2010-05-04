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

namespace ccAudioCodec {
using namespace ost;

static int index[8] = {-1, -1, -1, -1, 2, 4, 6, 8};

static int steps[49] = {
  16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45, 50, 55, 60, 66, 73,
  80, 88, 97, 107, 118, 130, 143, 157, 173, 190, 209, 230, 253, 279,
  307, 337, 371, 408, 449, 494, 544, 598, 658, 724, 796, 876, 963,
  1060, 1166, 1282, 1411, 1552
};

class okiCodec : public AudioCodec
{
private:
	typedef struct state {
		short last;
		short ssindex;
	}	state_t;

	state_t encode_state, decode_state;

public:
	AudioCodec *getByInfo(Info &info);
	AudioCodec *getByFormat(const char *format);

	unsigned decode(Linear buffer, void *from, unsigned lsamples);
	unsigned encode(Linear buffer, void *dest, unsigned lsamples);
	unsigned char encode_sample(state_t *state, short sample);
	short decode_sample(state_t *state, unsigned char code);

	okiCodec(const char *id, Encoding e);
	okiCodec(Encoding e);
	~okiCodec();
};

okiCodec::okiCodec(Encoding e) : AudioCodec()
{
	info.framesize = 1;
	info.framecount = 2;
	info.encoding = e;

	if(encoding == voxADPCM) {
		info.rate = 6000;
		info.bitrate = 24000;
		info.annotation = (char *)"vox";
	}
	else {
		info.rate = 8000;
		info.bitrate = 24000;
		info.annotation = (char *)"oki";
	}

	memset(&encode_state, 0, sizeof(encode_state));
	memset(&decode_state, 0, sizeof(decode_state));
	info.set();
}

okiCodec::okiCodec(const char *id, Encoding e) : AudioCodec(id, e)
{
	info.framesize = 1;
	info.framecount = 2;

	if(encoding == voxADPCM) {
		info.rate = 6000;
		info.bitrate = 24000;
		info.annotation = (char *)"vox";
	}
	else {
		info.rate = 8000;
		info.bitrate = 24000;
		info.annotation = (char *)"oki";
	}
	memset(&encode_state, 0, sizeof(encode_state));
	memset(&decode_state, 0, sizeof(decode_state));
	info.set();
}

okiCodec::~okiCodec()
{}

unsigned char okiCodec::encode_sample(state_t *state, short sample)
{
	unsigned char code = 0;
	short diff, step;

	step = steps[state->ssindex];
	diff = sample - state->last;
	if(diff < 0) {
		diff = -diff;
		code = 0x08;
	}

	if(diff >= step) {
		code |= 0x04;
		diff -= step;
	}
	if(diff >= step/2) {
		code |= 0x02;
		diff -= step/2;
	}
	if(diff >= step/4)
		code |= 0x01;

	decode_sample(state, code);
	return code;
}

short okiCodec::decode_sample(state_t *state, unsigned char code)
{
	short diff, step, sample;

	step = steps[state->ssindex];
	diff = step / 8;
	if(code & 0x01)
		diff += step / 4;
	if(code & 0x02)
		diff += step / 2;
	if(code & 0x04)
		diff += step;
	if(code & 0x08)
		diff = -diff;
	sample = state->last + diff;
	if(sample > 2047)
		sample = 2047;
	else if(sample < -2047)
		sample = -2047;
	state->last = sample;
	state->ssindex += index[code & 0x07];
	if(state->ssindex < 0)
		state->ssindex = 0;
	if(state->ssindex > 48)
		state->ssindex = 48;
	return sample;
}

unsigned okiCodec::encode(Linear buffer, void *coded, unsigned lsamples)
{
	unsigned count = (lsamples / 2) * 2;
	bool hi = false;
	unsigned char byte = 0;
	Encoded dest = (Encoded)coded;

	while(count--) {
		if(hi) {
			byte |= encode_sample(&encode_state, *(buffer++) / 16 );
			*(dest++) = byte;
		}
		else
			byte = encode_sample(&encode_state, *(buffer++) / 16 ) << 4 ;
	}
	return (lsamples / 2) * 2;
}

unsigned okiCodec::decode(Linear buffer, void *from, unsigned lsamples)
{
	Encoded src = (Encoded)from;
	unsigned count = lsamples / 2;
	unsigned char byte;

	while(count--) {
		byte = ((*src >> 4) & 0x0f);
		*(buffer++) = (decode_sample(&decode_state, byte) * 16);
		byte = (*src & 0x0f);
		*(buffer++) = (decode_sample(&decode_state, byte) * 16);
		++src;
	}
	return (lsamples / 2) * 2;
}

AudioCodec *okiCodec::getByInfo(Info &info)
{
	return (AudioCodec *)new okiCodec(info.encoding);
}

AudioCodec *okiCodec::getByFormat(const char *format)
{
	return (AudioCodec *)new okiCodec(info.encoding);
}

static okiCodec voxcodec("vox", Audio::voxADPCM);
static okiCodec okicodec("oki", Audio::okiADPCM);

} // namespace

