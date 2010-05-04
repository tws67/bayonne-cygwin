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

static class g711u : public AudioCodec {
public:
	g711u();

	unsigned encode(Linear buffer, void *source, unsigned lsamples);
	unsigned decode(Linear buffer, void *dest, unsigned lsamples);
	Level getImpulse(void *buffer, unsigned samples);
	Level getPeak(void *buffer, unsigned samples);

} g711u;

static class g711a : public AudioCodec {
public:
	g711a();

	unsigned encode(Linear buffer, void *source, unsigned lsamples);
	unsigned decode(Linear buffer, void *dest, unsigned lsamples);
	Level getImpulse(void *buffer, unsigned samples);
	Level getPeak(void *buffer, unsigned samples);

} g711a;

// some asm optimized routines can be specified first, and set
// ASM_OPTIMIZED.  This is the default codec routines if no asm optimized
// code present.

#ifndef	ASM_OPTIMIZED

g711u::g711u() : AudioCodec("g.711", mulawAudio)
{
	info.framesize = 1;
	info.framecount = 1;
	info.rate = 8000;
	info.bitrate = 64000;
	info.annotation = (char *)"mu-law";
}

g711a::g711a() : AudioCodec("g.711", alawAudio)
{
	info.framesize = 1;
	info.framecount = 1;
	info.bitrate = 64000;
	info.rate = 8000;
	info.annotation = (char *)"a-law";
}

static unsigned ullevels[128] =
{
			32124,   31100,   30076,   29052,   28028,
	27004,   25980,   24956,   23932,   22908,   21884,   20860,
	19836,   18812,   17788,   16764,   15996,   15484,   14972,
	14460,   13948,   13436,   12924,   12412,   11900,   11388,
	10876,   10364,    9852,    9340,    8828,    8316,    7932,
	7676,    7420,    7164,    6908,    6652,    6396,    6140,
	5884,    5628,    5372,    5116,    4860,    4604,    4348,
	4092,    3900,    3772,    3644,    3516,    3388,    3260,
	3132,    3004,    2876,    2748,    2620,    2492,    2364,
	2236,    2108,    1980,    1884,    1820,    1756,    1692,
	1628,    1564,    1500,    1436,    1372,    1308,    1244,
	1180,    1116,    1052,     988,     924,     876,     844,
	812,     780,     748,     716,     684,     652,     620,
	588,     556,     524,     492,     460,     428,     396,
	372,     356,     340,     324,     308,     292,     276,
	260,     244,     228,     212,     196,     180,     164,
	148,     132,     120,     112,     104,      96,      88,
	80,      72,      64,      56,      48,      40,      32,
	24,      16,       8,       0
};


Audio::Level g711u::getImpulse(void *data, unsigned samples)
{
	unsigned long count = samples;
	unsigned long sum = 0;

	if(!samples)
		samples = count = 160;

	unsigned char *dp = (unsigned char *)data;

	while(samples--)
		sum += (ullevels[*(dp++) & 0x7f]);

	return (Level)(sum / count);
}

Audio::Level g711u::getPeak(void *data, unsigned samples)
{
	unsigned long count = samples;
	Level max = 0, value;

	if(!samples)
		samples = count = 160;

	unsigned char *dp = (unsigned char *)data;

	while(samples--) {
		value = ullevels[*(dp++) & 0x7f];
		if(value > max)
			max = value;
	}
	return max;
}

unsigned g711u::encode(Linear buffer, void *dest, unsigned lsamples)
{
	static int ulaw[256] = {
		0,0,1,1,2,2,2,2,3,3,3,3,3,3,3,3,
		4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
		5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
		5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
		6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
		6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
		6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
		6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
		7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
		7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
		7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
		7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
		7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
		7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
		7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
		7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7};

	register Sample sample;
	int sign, exponent, mantissa, retval;
	register unsigned char *d = (unsigned char *)dest;
	unsigned count;

	count = lsamples;

	while(lsamples--) {
		sample = *(buffer++);
		        sign = (sample >> 8) & 0x80;
	        if(sign != 0) sample = -sample;
			sample += 0x84;
			exponent = ulaw[(sample >> 7) & 0xff];
			mantissa = (sample >> (exponent + 3)) & 0x0f;
			retval = ~(sign | (exponent << 4) | mantissa);
			if(!retval)
			retval = 0x02;
		*(d++) = (unsigned char)retval;
	}
	return count;
}

unsigned g711u::decode(Linear buffer, void *source, unsigned lsamples)
{
	register unsigned char *src = (unsigned char *)source;
	unsigned count;

	count = lsamples;

	static Sample values[256] =
	{
	-32124, -31100, -30076, -29052, -28028, -27004, -25980, -24956,
	-23932, -22908, -21884, -20860, -19836, -18812, -17788, -16764,
	-15996, -15484, -14972, -14460, -13948, -13436, -12924, -12412,
	-11900, -11388, -10876, -10364,  -9852,  -9340,  -8828,  -8316,
	 -7932,  -7676,  -7420,  -7164,  -6908,  -6652,  -6396,  -6140,
	 -5884,  -5628,  -5372,  -5116,  -4860,  -4604,  -4348,  -4092,
	 -3900,  -3772,  -3644,  -3516,  -3388,  -3260,  -3132,  -3004,
	 -2876,  -2748,  -2620,  -2492,  -2364,  -2236,  -2108,  -1980,
	 -1884,  -1820,  -1756,  -1692,  -1628,  -1564,  -1500,  -1436,
	 -1372,  -1308,  -1244,  -1180,  -1116,  -1052,   -988,   -924,
	  -876,   -844,   -812,   -780,   -748,   -716,   -684,   -652,
	  -620,   -588,   -556,   -524,   -492,   -460,   -428,   -396,
	  -372,   -356,   -340,   -324,   -308,   -292,   -276,   -260,
  	  -244,   -228,   -212,   -196,   -180,   -164,   -148,   -132,
	  -120,   -112,   -104,    -96,    -88,    -80,    -72,    -64,
	   -56,    -48,    -40,    -32,    -24,    -16,     -8,      0,
	 32124,  31100,  30076,  29052,  28028,  27004,  25980,  24956,
	 23932,  22908,  21884,  20860,  19836,  18812,  17788,  16764,
	 15996,  15484,  14972,  14460,  13948,  13436,  12924,  12412,
	 11900,  11388,  10876,  10364,   9852,   9340,   8828,   8316,
	  7932,   7676,   7420,   7164,   6908,   6652,   6396,   6140,
	  5884,   5628,   5372,   5116,   4860,   4604,   4348,   4092,
	  3900,   3772,   3644,   3516,   3388,   3260,   3132,   3004,
	  2876,   2748,   2620,   2492,   2364,   2236,   2108,   1980,
	  1884,   1820,   1756,   1692,   1628,   1564,   1500,   1436,
	  1372,   1308,   1244,   1180,   1116,   1052,    988,    924,
	   876,    844,    812,    780,    748,    716,    684,    652,
	   620,    588,    556,    524,    492,    460,    428,    396,
	   372,    356,    340,    324,    308,    292,    276,    260,
	   244,    228,    212,    196,    180,    164,    148,    132,
	   120,    112,    104,     96,     88,     80,     72,     64,
	    56,     48,     40,     32,     24,     16,      8,      0
	};

	while(lsamples--)
		*(buffer++) = values[*(src++)];

	return count;
}

#define	AMI_MASK	0x55

unsigned g711a::encode(Linear buffer, void *dest, unsigned lsamples)
{
	int mask, seg, pcm_val;
	unsigned count;
	unsigned char *d = (unsigned char *)dest;

	static int seg_end[] = {
		0xFF, 0x1FF, 0x3FF, 0x7FF, 0xFFF, 0x1FFF, 0x3FFF, 0x7FFF};

	count = lsamples;

	while(lsamples--) {
		pcm_val = *(buffer++);
		if(pcm_val >= 0)
			mask = AMI_MASK | 0x80;
		else {
			mask = AMI_MASK;
			pcm_val = -pcm_val;
		}
		for(seg = 0; seg < 8; seg++)
		{
			if(pcm_val <= seg_end[seg])
				break;
		}
		*(d++) = ((seg << 4) | ((pcm_val >> ((seg)  ?  (seg + 3)  :  4)) & 0x0F)) ^ mask;
	}
	return count;
}

static unsigned allevels[128] =
{
	5504,   5248,   6016,   5760,   4480,   4224,   4992,   4736,
	7552,   7296,   8064,   7808,   6528,   6272,   7040,   6784,
	2752,   2624,   3008,   2880,   2240,   2112,   2496,   2368,
	3776,   3648,   4032,   3904,   3264,   3136,   3520,   3392,
	22016,  20992,  24064,  23040,  17920,  16896,  19968,  18944,
	30208,  29184,  32256,  31232,  26112,  25088,  28160,  27136,
	11008,  10496,  12032,  11520,   8960,   8448,   9984,   9472,
	15104,  14592,  16128,  15616,  13056,  12544,  14080,  13568,
	344,    328,    376,    360,    280,    264,    312,    296,
	472,    456,    504,    488,    408,    392,    440,    424,
	88,     72,    120,    104,     24,      8,     56,     40,
	216,    200,    248,    232,    152,    136,    184,    168,
	1376,   1312,   1504,   1440,   1120,   1056,   1248,   1184,
	1888,   1824,   2016,   1952,   1632,   1568,   1760,   1696,
	688,    656,    752,    720,    560,    528,    624,    592,
	944,    912,   1008,    976,    816,    784,    880,    848
};

Audio::Level g711a::getImpulse(void *data, unsigned samples)
{
	unsigned long count = samples;
	unsigned long sum = 0;

	if(!samples)
		samples = count = 160;



	unsigned char *dp = (unsigned char *)data;

	while(samples--)
		sum += (allevels[*(dp++) & 0x7f]);

	return (Level)(sum / count);
}

Audio::Level g711a::getPeak(void *data, unsigned samples)
{
	unsigned long count = samples;
	Level max = 0, value;

	if(!samples)
		samples = count = 160;

	unsigned char *dp = (unsigned char *)data;

	while(samples--) {
		value = allevels[*(dp++) & 0x7f];
		if(value > max)
			max = value;
	}
	return max;
}

unsigned g711a::decode(Linear buffer, void *source, unsigned lsamples)
{
	register unsigned char *src = (unsigned char *)source;
	unsigned count;

	static Sample values[256] =
	{
	    -5504,  -5248,  -6016,  -5760,  -4480,  -4224,  -4992,  -4736,
	    -7552,  -7296,  -8064,  -7808,  -6528,  -6272,  -7040,  -6784,
	    -2752,  -2624,  -3008,  -2880,  -2240,  -2112,  -2496,  -2368,
	    -3776,  -3648,  -4032,  -3904,  -3264,  -3136,  -3520,  -3392,
	   -22016, -20992, -24064, -23040, -17920, -16896, -19968, -18944,
	   -30208, -29184, -32256, -31232, -26112, -25088, -28160, -27136,
	   -11008, -10496, -12032, -11520,  -8960,  -8448,  -9984,  -9472,
	   -15104, -14592, -16128, -15616, -13056, -12544, -14080, -13568,
	     -344,   -328,   -376,   -360,   -280,   -264,   -312,   -296,
	     -472,   -456,   -504,   -488,   -408,   -392,   -440,   -424,
	      -88,    -72,   -120,   -104,    -24,     -8,    -56,    -40,
	     -216,   -200,   -248,   -232,   -152,   -136,   -184,   -168,
	    -1376,  -1312,  -1504,  -1440,  -1120,  -1056,  -1248,  -1184,
	    -1888,  -1824,  -2016,  -1952,  -1632,  -1568,  -1760,  -1696,
	     -688,   -656,   -752,   -720,   -560,   -528,   -624,   -592,
	     -944,   -912,  -1008,   -976,   -816,   -784,   -880,   -848,
	     5504,   5248,   6016,   5760,   4480,   4224,   4992,   4736,
	     7552,   7296,   8064,   7808,   6528,   6272,   7040,   6784,
	     2752,   2624,   3008,   2880,   2240,   2112,   2496,   2368,
	     3776,   3648,   4032,   3904,   3264,   3136,   3520,   3392,
	    22016,  20992,  24064,  23040,  17920,  16896,  19968,  18944,
	    30208,  29184,  32256,  31232,  26112,  25088,  28160,  27136,
	    11008,  10496,  12032,  11520,   8960,   8448,   9984,   9472,
	    15104,  14592,  16128,  15616,  13056,  12544,  14080,  13568,
	      344,    328,    376,    360,    280,    264,    312,    296,
	      472,    456,    504,    488,    408,    392,    440,    424,
	       88,     72,    120,    104,     24,      8,     56,     40,
	      216,    200,    248,    232,    152,    136,    184,    168,
	     1376,   1312,   1504,   1440,   1120,   1056,   1248,   1184,
	     1888,   1824,   2016,   1952,   1632,   1568,   1760,   1696,
	      688,    656,    752,    720,    560,    528,    624,    592,
	      944,    912,   1008,    976,    816,    784,    880,    848
 	};

	count = lsamples;

	while(lsamples--)
		*(buffer++) = values[*(src++)];

	return count;
}

#endif
}

