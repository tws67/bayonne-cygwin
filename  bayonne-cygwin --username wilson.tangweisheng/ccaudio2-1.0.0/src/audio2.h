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

/**
 * @file audio2.h
 * @short Framework for portable audio processing and file handling classes.
 **/


#ifndef	CCXX_AUDIO_H_
#define	CCXX_AUDIO_H_

#ifndef	CCXX_PACKING
#if defined(__GNUC__)
#define CCXX_PACKED
#elif !defined(__hpux) && !defined(_AIX)
#define CCXX_PACKED
#endif
#endif

#ifndef	W32
#if defined(_WIN32) && defined(_MSC_VER)
#pragma warning(disable: 4996)
#define	W32
#endif
#if defined(__BORLANDC__) && defined(__Windows)
#define	W32
#endif
#endif

#if !defined(__EXPORT) && defined(W32)
#define	__EXPORT __declspec(dllimport)
#endif

#ifndef	__EXPORT
#define	__EXPORT
#endif

#ifdef	W32
#include <windows.h>
#ifndef	ssize_t
#define	ssize_t	int
#endif
#else
#include <cstddef>
#include <cstdlib>
#include <sys/types.h>
#include <netinet/in.h>
#endif

#include <ctime>

namespace ost {

#define	AUDIO_SIGNED_LINEAR_RAW	1
#define	AUDIO_LINEAR_CONVERSION 1
#define	AUDIO_CODEC_MODULES	1
#define	AUDIO_LINEAR_FRAMING	1
#define	AUDIO_NATIVE_METHODS	1
#define	AUDIO_RATE_RESAMPLER	1

class __EXPORT AudioCodec;
class __EXPORT AudioDevice;

/**
 * Generic audio class to hold master data types and various useful
 * class encapsulated friend functions as per GNU Common C++ 2 coding
 * standard.
 *
 * @author David Sugar <dyfet@ostel.com>
 * @short Master audio class.
 */
class __EXPORT Audio
{
public:
#ifdef	W32
	typedef	short	Sample;
	typedef	short	*Linear;
	typedef	short	Level;
	typedef	DWORD	timeout_t;
	typedef	WORD	snd16_t;
	typedef	DWORD	snd32_t;
#else
	typedef	int16_t snd16_t;
	typedef	int32_t	snd32_t;
	typedef	int16_t	Level;
	typedef	int16_t	Sample;
	typedef	int16_t	*Linear;
	typedef	unsigned long	timeout_t;
#endif

	static const unsigned ndata;

	typedef struct {
	float v2;
		float v3;
		float fac;
	} goertzel_state_t;

	typedef struct {
		int hit1;
		int hit2;
		int hit3;
		int hit4;
		int mhit;

		goertzel_state_t row_out[4];
		goertzel_state_t col_out[4];
		goertzel_state_t row_out2nd[4];
		goertzel_state_t col_out2nd[4];
		goertzel_state_t fax_tone;
		goertzel_state_t fax_tone2nd;
		float energy;

		int current_sample;
		char digits[129];
		int current_digits;
		int detected_digits;
		int lost_digits;
		int digit_hits[16];
		int fax_hits;
	} dtmf_detect_state_t;

	typedef struct {
		float fac;
	} tone_detection_descriptor_t;

	typedef	unsigned char *Encoded;

	/**
	 * Audio encoding rate, samples per second.
	 */
	enum	Rate {
		rateUnknown,
		rate6khz = 6000,
		rate8khz = 8000,
		rate16khz = 16000,
		rate32khz = 32000,
		rate44khz = 44100
	};

	typedef	enum Rate Rate;

	/**
	 * File processing mode, whether to skip missing files, etc.
	 */
	enum	Mode {
		modeRead,
		modeReadAny,
		modeReadOne,
		modeWrite,
		modeCache,
		modeInfo,
		modeFeed,

		modeAppend,	// app specific placeholders...
		modeCreate
	};

	typedef enum Mode Mode;

	/**
	 * Audio encoding formats.
	 */
	enum	Encoding {
		unknownEncoding = 0,
		g721ADPCM,
		g722Audio,
		g722_7bit,
		g722_6bit,
		g723_2bit,
		g723_3bit,
		g723_5bit,
		gsmVoice,
		msgsmVoice,
		mulawAudio,
		alawAudio,
		mp1Audio,
		mp2Audio,
		mp3Audio,
		okiADPCM,
		voxADPCM,
		sx73Voice,
		sx96Voice,

		// Please keep the PCM types at the end of the list -
		// see the "is this PCM or not?" code in
		// AudioFile::close for why.
		cdaStereo,
		cdaMono,
		pcm8Stereo,
		pcm8Mono,
		pcm16Stereo,
		pcm16Mono,
		pcm32Stereo,
		pcm32Mono,

		// speex codecs
		speexVoice,	// narrow band
		speexAudio,

		g729Audio,
		ilbcAudio,
		speexUltra,

		speexNarrow = speexVoice,
		speexWide = speexAudio,
		g723_4bit = g721ADPCM
	};
	typedef enum Encoding Encoding;

	/**
	 * Audio container file format.
	 */
	enum Format {
		raw,
		snd,
		riff,
		mpeg,
		wave
	};
	typedef enum Format Format;

	/**
	 * Audio device access mode.
	 */
	enum DeviceMode {
		PLAY,
		RECORD,
		PLAYREC
	};
	typedef enum DeviceMode DeviceMode;

	/**
	 * Audio error conditions.
	 */
	enum Error {
		errSuccess = 0,
		errReadLast,
		errNotOpened,
		errEndOfFile,
		errStartOfFile,
		errRateInvalid,
		errEncodingInvalid,
		errReadInterrupt,
		errWriteInterrupt,
		errReadFailure,
		errWriteFailure,
		errReadIncomplete,
		errWriteIncomplete,
		errRequestInvalid,
		errTOCFailed,
		errStatFailed,
		errInvalidTrack,
		errPlaybackFailed,
		errNotPlaying,
		errNoCodec
	};
	typedef enum Error Error;


#ifdef	CCXX_PACKED
#pragma pack(1)
#endif

	typedef	struct {
#if	__BYTE_ORDER == __LITTLE_ENDIAN
		unsigned char mp_sync1 : 8;
		unsigned char mp_crc   : 1;
		unsigned char mp_layer : 2;
		unsigned char mp_ver   : 2;
		unsigned char mp_sync2 : 3;

		unsigned char mp_priv  : 1;
		unsigned char mp_pad   : 1;
		unsigned char mp_srate : 2;
		unsigned char mp_brate : 4;

		unsigned char mp_emp   : 2;
		unsigned char mp_original : 1;
		unsigned char mp_copyright: 1;
		unsigned char mp_extend   : 2;
		unsigned char mp_channels : 2;

#else
		unsigned char mp_sync1 : 8;

		unsigned char mp_sync2 : 3;
		unsigned char mp_ver   : 2;
		unsigned char mp_layer : 2;
		unsigned char mp_crc   : 1;

		unsigned char mp_brate : 4;
		unsigned char mp_srate : 2;
		unsigned char mp_pad   : 1;
		unsigned char mp_priv  : 1;

		unsigned char mp_channels : 2;
		unsigned char mp_extend   : 2;
		unsigned char mp_copyright : 1;
		unsigned char mp_original : 1;
		unsigned char mp_emp : 2;
#endif
	}	mpeg_audio;

	typedef struct {
		char tag_id[3];
		char tag_title[30];
		char tag_artist[30];
		char tag_album[30];
		char tag_year[4];
		char tag_note[30];
		unsigned char genre;
	}	mpeg_tagv1;

#ifdef	CCXX_PACKED
#pragma pack()
#endif

	/**
	 * Audio source description.
	 */
	class __EXPORT Info
	{
	public:
		Format format;
		Encoding encoding;
		unsigned long rate;
		unsigned long bitrate;
		unsigned order;
		unsigned framesize, framecount, headersize, padding;
		timeout_t framing;
		char *annotation;

		Info();
		void clear(void);
		void set(void);
		void setFraming(timeout_t frame);
		void setRate(Rate rate);
	};

	/**
	 * Convert dbm power level to integer value (0-32768).
	 *
	 * @param dbm power level
	 * @return integer value.
	 */
	static Level tolevel(float dbm);

	/**
	 * Convert integer power levels to dbm.
	 *
	 * @param power level.
	 * @return dbm power level.
	 */
	static float todbm(Level power);

	/**
	 * Test for the presense of a specified (indexed) audio device.
	 * This is normally used to test for local soundcard access.
	 *
	 * @param device index or 0 for default audio device.
	 * @return true if device exists.
	 */
	static bool hasDevice(unsigned device = 0);

	/**
	 * Get a audio device object that can be used to play or record
	 * audio.  This is normally a local soundcard, though an
	 * abstract base class is returned, so the underlying device may
	 * be different.
	 *
	 * @param device index or 0 for default audio device.
	 * @param mode of device; play, record, or full duplex.
	 * @return pointer to abstract audio device object interface class.
	 */
	static AudioDevice *getDevice(unsigned device = 0, DeviceMode mode = PLAY);

	/**
	 * Get pathname to where loadable codec modules are stored.
	 *
	 * @return file path to loadable codecs.
	 */
	static const char *getCodecPath(void);

	/**
	 * Get the mime descriptive type for a given Audio encoding
	 * description, usually retrieved from a newly opened audio file.
	 *
	 * @param info source description object
	 * @return text of mime type to use for this audio source.
	 */
	static const char *getMIME(Info &info);

	/**
	 * Get the short ascii description used for the given audio
	 * encoding type.
	 *
	 * @param encoding format.
	 * @return ascii name of encoding format.
	 */
	static const char *getName(Encoding encoding);

	/**
	 * Get the preferred file extension name to use for a given
	 * audio encoding type.
	 *
	 * @param encoding format.
	 * @return ascii file extension to use.
	 */
	static const char *getExtension(Encoding encoding);

	/**
	 * Get the audio encoding format that is specified by a short
	 * ascii name.  This will either accept names like those returned
	 * from getName(), or .xxx file extensions, and return the
	 * audio encoding type associated with the name or extension.
	 *
	 * @param name of encoding or file extension.
	 * @return audio encoding format.
	 * @see #getName
	 */
	static Encoding getEncoding(const char *name);

	/**
	 * Get the stereo encoding format associated with the given format.
	 *
	 * @param encoding format being tested for stereo.
	 * @return associated stereo audio encoding format.
	 */
	static Encoding getStereo(Encoding encoding);

	/**
	 * Get the mono encoding format associated with the given format.
	 *
	 * @param encoding format.
	 * @return associated mono audio encoding format.
	 */
	static Encoding getMono(Encoding encoding);

	/**
	 * Test if the audio encoding format is a linear one.
	 *
	 * @return true if encoding format is linear audio data.
	 * @param encoding format.
	 */
	static bool isLinear(Encoding encoding);

	/**
	 * Test if the audio encoding format must be packetized (that
	 * is, has irregular sized frames) and must be processed
	 * only through buffered codecs.
	 *
	 * @return true if packetized audio.
	 * @param encoding format.
	 */
	static bool isBuffered(Encoding encoding);

	/**
	 * Test if the audio encoding format is a mono format.
	 *
	 * @return true if encoding format is mono audio data.
	 * @param encoding format.
	 */
	static bool isMono(Encoding encoding);

	/**
	 * Test if the audio encoding format is a stereo format.
	 *
	 * @return true if encoding format is stereo audio data.
	 * @param encoding format.
	 */
	static bool isStereo(Encoding encoding);

	/**
	 * Return default sample rate associated with the specified
	 * audio encoding format.
	 *
	 * @return sample rate for audio data.
	 * @param encoding format.
	 */
	static Rate getRate(Encoding encoding);

	/**
	 * Return optional rate setting effect.  Many codecs are
	 * fixed rate.
	 *
	 * @return result rate for audio date.
	 * @param encoding format.
	 * @param requested rate.
	 */
	static Rate getRate(Encoding e, Rate request);

	/**
	 * Return frame timing for an audio encoding format.
	 *
	 * @return frame time to use in milliseconds.
	 * @param encoding of frame to get timing segment for.
	 * @param timeout of frame time segment to request.
	 */
	static timeout_t getFraming(Encoding encoding, timeout_t timeout = 0);

	/**
	 * Return frame time for an audio source description.
	 *
	 * @return frame time to use in milliseconds.
	 * @param info descriptor of frame encoding to get timing segment for.
	 * @param timeout of frame time segment to request.
	 */
	static timeout_t getFraming(Info &info, timeout_t timeout = 0);

	/**
	 * Test if the endian byte order of the encoding format is
	 * different from the machine's native byte order.
	 *
	 * @return true if endian format is different.
	 * @param encoding format.
	 */
	static bool isEndian(Encoding encoding);

	/**
	 * Test if the endian byte order of the audio source description
	 * is different from the machine's native byte order.
	 *
	 * @return true if endian format is different.
	 * @param info source description object.
	 */
	static bool isEndian(Info &info);

	/**
	 * Optionally swap endian of audio data if the encoding format
	 * endian byte order is different from the machine's native endian.
	 *
	 * @return true if endian format was different.
	 * @param encoding format of data.
	 * @param buffer of audio data.
	 * @param number of audio samples.
	 */
	static bool swapEndian(Encoding encoding, void *buffer, unsigned number);

	/**
	 * Optionally swap endian of encoded audio data based on the
	 * audio encoding type, and relationship to native byte order.
	 *
	 * @param info source description of object.
	 * @param buffer of audio data.
	 * @param number of bytes of audio data.
	 */
	static void swapEncoded(Info &info, Encoded data, size_t bytes);

	   /**
	 * Optionally swap endian of audio data if the audio source
	 * description byte order is different from the machine's native
	 * endian byte order.
	 *
	 * @return true if endian format was different.
	 * @param info source description object of data.
	 * @param buffer of audio data.
	 * @param number of audio samples.
	 */
	static bool swapEndian(Info &info, void *buffer, unsigned number);

	/**
	 * Get the energey impulse level of a frame of audio data.
	 *
	 * @return impulse energy level of audio data.
	 * @param encoding format of data to examine.
	 * @param buffer of audio data to examine.
	 * @param number of audio samples to examine.
	 */
	static Level getImpulse(Encoding encoding, void *buffer, unsigned number);

	/**
	 * Get the energey impulse level of a frame of audio data.
	 *
	 * @return impulse energy level of audio data.
	 * @param info encoding source description object.
	 * @param buffer of audio data to examine.
	 * @param number of audio samples to examine.
	 */
	static Level getImpulse(Info &info, void *buffer, unsigned number = 0);

	/**
	 * Get the peak (highest energy) level found in a frame of audio
	 * data.
	 *
	 * @return peak energy level found in data.
	 * @param encoding format of data.
	 * @param buffer of audio data.
	 * @param number of samples to examine.
	 */
	static Level getPeak(Encoding encoding, void *buffer, unsigned number);

	/**
	 * Get the peak (highest energy) level found in a frame of audio
	 * data.
	 *
	 * @return peak energy level found in data.
	 * @param info description object of audio data.
	 * @param buffer of audio data.
	 * @param number of samples to examine.
	 */
	static Level getPeak(Info &info, void *buffer, unsigned number = 0);

	/**
	 * Provide ascii timestamp representation of a timeout value.
	 *
	 * @param duration timeout value
	 * @param address for ascii data.
	 * @param size of ascii data.
	 */
	static void toTimestamp(timeout_t duration, char *address, size_t size);

	/**
	 * Convert ascii timestamp representation to a timeout number.
	 *
	 * @param timestamp ascii data.
	 * @return timeout_t duration from data.
	 */
	static timeout_t toTimeout(const char *timestamp);

	/**
	 * Returns the number of bytes in a sample frame for the given
	 * encoding type, rounded up to the nearest integer.  A frame
	 * is defined as the minimum number of bytes necessary to
	 * create a point or points in the output waveform for all
	 * output channels.  For example, 16-bit mono PCM has a frame
	 * size of two (because those two bytes constitute a point in
	 * the output waveform).  GSM has it's own definition of a
	 * frame which involves decompressing a sequence of bytes to
	 * determine the final points on the output waveform.  The
	 * minimum number of bytes you can feed to the decompression
	 * engine is 32.5 (260 bits), so this function will return 33
	 * (because we round up) given an encoding type of GSM.  Other
	 * compressed encodings will return similar results.  Be
	 * prepared to deal with nonintuitive return values for
	 * rare encodings.
	 *
	 * @param encoding The encoding type to get the frame size for.
	 * @param samples Reserved.  Use zero.
	 *
	 * @return The number of bytes in a frame for the given encoding.
	 */
	static int getFrame(Encoding encoding, int samples = 0);

	/**
	 * Returns the number of samples in all channels for a frame
	 * in the given encoding.  For example, pcm32Stereo has a
	 * frame size of 8 bytes: Note that different codecs have
	 * different definitions of a frame - for example, compressed
	 * encodings have a rather large frame size relative to the
	 * sample size due to the way bytes are fed to the
	 * decompression engine.
	 *
	 * @param encoding The encoding to calculate the frame sample count for.
	 * @return samples The number of samples in a frame of the given encoding.
	 */
	static int getCount(Encoding encoding);

	/**
	 * Compute byte counts of audio data into number of samples
	 * based on the audio encoding format used.
	 *
	 * @return number of audio samples in specified data.
	 * @param encoding format.
	 * @param bytes of data.
	 */
	static unsigned long toSamples(Encoding encoding, size_t bytes);

	/**
	 * Compute byte counts of audio data into number of samples
	 * based on the audio source description used.
	 *
	 * @return number of audio samples in specified data.
	 * @param info encoding source description.
	 * @param bytes of data.
	 */
	static unsigned long toSamples(Info &info, size_t bytes);

	/**
	 * Compute the number of bytes a given number of samples in
	 * a given audio encoding will occupy.
	 *
	 * @return number of bytes samples will occupy.
	 * @param info encoding source description.
	 * @param number of samples.
	 */
	static size_t toBytes(Info &info, unsigned long number);

	/**
	 * Compute the number of bytes a given number of samples in
	 * a given audio encoding will occupy.
	 *
	 * @return number of bytes samples will occupy.
	 * @param encoding format.
	 * @param number of samples.
	 */
	static size_t toBytes(Encoding encoding, unsigned long number);

	/**
	 * Fill an audio buffer with "empty" (silent) audio data, based
	 * on the audio encoding format.
	 *
	 * @param address of data to fill.
	 * @param number of samples to fill.
	 * @param encoding format of data.
	 */
	static void fill(unsigned char *address, int number, Encoding encoding);

	/**
	 * Load a dso plugin (codec plugin), used internally...
	 *
	 * @return true if loaded.
	 * @param path to codec.
	 */
	static bool loadPlugin(const char *path);

	/**
	 * Maximum framesize for a given coding that may be needed to
	 * store a result.
	 *
	 * @param info source description object.
	 * @return maximum possible frame size to allocate for encoded data.
	 */
	static size_t maxFramesize(Info &info);
};

/**
 * The AudioResample class is used to manage linear intropolation
 * buffering for rate conversions.
 *
 * @author David Sugar <dyfet@ostel.com>
 * @short linear intropolation and rate conversion.
 */
class __EXPORT AudioResample : public Audio
{
protected:
	unsigned mfact, dfact, max;
	unsigned gpos, ppos;
	Sample last;
	Linear buffer;

public:
	AudioResample(Rate mul, Rate div);
	~AudioResample();

	size_t process(Linear from, Linear to, size_t count);
	size_t estimate(size_t count);
};

/**
 * The AudioTone class is used to create a frame of audio encoded single or
 * dualtones.  The frame will be iterated for each request, so a
 * continual tone can be extracted by frame.
 *
 * @author David Sugar <dyfet@ostel.com>
 * @short audio tone generator class.
 */
class __EXPORT AudioTone : public Audio
{
protected:
	Rate rate;
	unsigned samples;
	Linear frame;
	double df1, df2, p1, p2;
	Level m1, m2;
	bool silencer;

	/**
	 * Set the frame to silent.
	 */
	void silence(void);

	/**
	 * Reset the tone generator completely.  Produces silence.,
	 */
	void reset(void);

	/**
	 * Cleanup for virtual destructors to use.
	 */
	void cleanup(void);

	/**
	 * Set frame to generate single tone...
	 *
	 * @param freq of tone.
	 * @param level of tone.
	 */
	void single(unsigned freq, Level level);

	/**
	 * Set frame to generate dual tone...
	 *
	 * @param f1 frequency of tone 1
	 * @param f2 frequency of tone 2
	 * @param l1 level of tone 1
	 * @param l2 level of tone 2
	 */
	void dual(unsigned f1, unsigned f2, Level l1, Level l2);

public:
	/**
	 * Get the sample encoding rate being used for the tone generator
	 *
	 * @return sample rate in samples per second.
	 */
	inline Rate getRate(void)
		{return rate;};

	/**
	 * Get the frame size for the number of audio samples generated.
	 *
	 * @return number of samples processed in frame.
	 */
	inline size_t getSamples(void)
		{return samples;};

	/**
	 * Test if the tone generator is currently set to silence.
	 *
	 * @return true if generator set for silence.
	 */
	bool isSilent(void);

	/**
	 * Iterate the tone frame, and extract linear samples in
	 * native frame.  If endian flag passed, then convert for
	 * standard endian representation (byte swap) if needed.
	 *
	 * @return pointer to samples.
	 */
	virtual Linear getFrame(void);

	/**
	 * This is used to copy one or more pages of framed audio
	 * quickly to an external buffer.
	 *
	 * @return number of frames copied.
	 * @param buffer to copy into.
	 * @param number of frames requested.
	 */
	unsigned getFrames(Linear buffer, unsigned number);

	/**
	 * See if at end of tone.  This is used for non-continues audio
	 * tones, or to detect "break" events.
	 *
	 * @return true if end of data.
	 */
	virtual bool isComplete(void);

	/**
	 * Construct a silent tone generator of specific frame size.
	 *
	 * @param duration of frame in milliseconds.
	 * @param rate of samples.
	 */
	AudioTone(timeout_t duration = 20, Rate rate = rate8khz);

	/**
	 * Construct a dual tone frame generator.
	 *
	 * @param f1 frequency of tone 1.
	 * @param f2 frequency of tone 2.
	 * @param l1 level of tone 1.
	 * @param l2 level of tone 2.
	 * @param duration of frame in milliseconds.
	 * @param sample rate being generated.
	 */
	AudioTone(unsigned f1, unsigned f2, Level l1, Level l2,
		timeout_t duration = 20, Rate sample = rate8khz);

	/**
	 * Construct a single tone frame generator.
	 *
	 * @param freq of tone.
	 * @param level of tone.
	 * @param duration of frame in milliseconds.
	 * @param sample rate being generated.
	 */
	AudioTone(unsigned freq, Level level, timeout_t duration = 20, Rate sample = rate8khz);

	virtual ~AudioTone();
};

/**
 * AudioBase base class for many other audio classes which stream
 * data.
 *
 * @short common audio stream base.
 */
class __EXPORT AudioBase : public Audio
{
protected:
	Info info;

public:
	/**
	 * Create audio base object with no info.
	 */
	AudioBase();

	/**
	 * Create audio base object with audio source description.
	 *
	 * @param info source description.
	 */
	AudioBase(Info *info);

	/**
	 * Destroy an audio base object.
	 */
	virtual ~AudioBase();

	/**
	 * Generic get encoding.
	 *
	 * @return audio encoding of this object.
	 */
	inline Encoding getEncoding(void)
		{return info.encoding;};

	/**
	 * Generic sample rate.
	 *
	 * @return audio sample rate of this object.
	 */
	inline unsigned getSampleRate(void)
		{return info.rate;};

	/**
	 * Abstract interface to put raw data.
	 *
	 * @param data to put.
	 * @param size of data to put.
	 * @return number of bytes actually put.
	 */
	virtual ssize_t putBuffer(Encoded data, size_t size) = 0;

	/**
	 * Puts raw data and does native to refined endian swapping
	 * if needed based on encoding type and local machine endian.
	 *
	 * @param data to put.
	 * @param size of data to put.
	 * @return number of bytes actually put.
	 */
	ssize_t putNative(Encoded data, size_t size);

	/**
	 * Abstract interface to get raw data.
	 *
	 * @return data received in buffer.
	 * @param data to get.
	 * @param size of data to get.
	 */
	virtual ssize_t getBuffer(Encoded data, size_t size) = 0;

	/**
	 * Get's a packet of audio data.
	 *
	 * @return count of data received.
	 * @param data to get.
	 */
	inline ssize_t getPacket(Encoded data)
		{return getBuffer(data, 0);};

	/**
	 * Get raw data and assure is in native machine endian.
	 *
	 * @return data received in buffer.
	 * @param data to get.
	 * @param size of data to get.
	 */
	ssize_t getNative(Encoded data, size_t size);
};

/**
 * The AudioBuffer class is for mixing one-to-one
 * soft joins.
 *
 * @author Mark Lipscombe <markl@gasupnow.com>
 * @short audio buffer mixer class
 */
class __EXPORT AudioBuffer : public AudioBase
{
public:
	AudioBuffer(Info *info, size_t size = 4096);
	virtual ~AudioBuffer();

	/**
	 * save audio data from buffer data.
	 *
	 * @return number of bytes actually saved.
	 * @param data save buffer.
	 * @param number of bytes to save.
	 */
	ssize_t getBuffer(Encoded data, size_t number);

	/**
	 * Put data into the audio buffer.
	 *
	 * @return number of bytes actually put.
	 * @param data of data to load.
	 * @param number of bytes to load.
	 */
	ssize_t putBuffer(Encoded data, size_t number);

private:
	char *buf;
	size_t size, start, len;
	void *mutexObject;

	void enter(void);
	void leave(void);
};

/**
 * A class used to manipulate audio data.  This class provides file
 * level access to audio data stored in different formats.  This class
 * also provides the ability to write audio data into a disk file.
 *
 * @author David Sugar <dyfet@ostel.com>
 * @short audio file access.
 */
class __EXPORT AudioFile: public AudioBase
{
protected:
	char *pathname;
	Error error;
	unsigned long header;		// offset to start of audio
	unsigned long minimum;		// minimum sample size required
	unsigned long length;           // current size of file, including header

	void initialize(void);
	void getWaveFormat(int size);
	void mp3info(mpeg_audio *mp3);

	union {
		int fd;
		void *handle;
	} file;

	Mode mode;
	unsigned long iolimit;

	virtual bool afCreate(const char *path, bool exclusive = false);
	virtual bool afOpen(const char *path, Mode m = modeWrite);
	virtual bool afPeek(unsigned char *data, unsigned size);

	AudioCodec *getCodec(void);

	/**
	 * Read a given number of bytes from the file, starting from
	 * the current file pointer.  May be overridden by derived
	 * classes.
	 *
	 * @param data A pointer to the buffer to copy the bytes to.
	 * @param size The number of bytes to read.
	 * @return The number of bytes read, or -1 if an error occurs.
	 * On UNIX platforms, use strerror(errno) to get the
	 * human-readable error string or
	 * FormatMessage(GetLastError()) on Windows platforms.
	 */
	virtual int afRead(unsigned char *data, unsigned size);

	/**
	 * Write a number of bytes into the file at the current file
	 * pointer.  May be overridden by derived classes.
	 *
	 * @param data A pointer to the buffer with the bytes to write.
	 * @param size The number of bytes to write from the buffer.
	 * @return The number of bytes written, or -1 if an error
	 * occurs.  On UNIX platforms, use strerror(errno) to get the
	 * human-readable error string or
	 * FormatMessage(GetLastError()) on Windows platforms.
	 */
	virtual int afWrite(unsigned char *data, unsigned size);

	/**
	 * Seek to the given position relative to the start of the
	 * file and set the file pointer.  This does not use 64-bit
	 * clean seek functions, so seeking to positions greater than
	 * (2^32)-1 will result in undefined behavior.
	 *
	 * @param pos The position to seek to.
	 * @return true if successful, false otherwise.
	 */
	virtual bool afSeek(unsigned long pos);

	/**
	 * Close the derived file handling system's file handle.
	 */
	virtual void afClose(void);

	/**
	 * This function is used to splice multiple audio files together
	 * into a single stream of continues audio data.  The
	 * continuation method returns the next audio file to open.
	 *
	 * @return next file to open or NULL when done.
	 */
	virtual char *getContinuation(void)
		{return NULL;};

	/**
	 * Return a human-readable error message given a numeric error
	 * code of type Audio::Error.
	 *
	 * @param err The numeric error code to translate.
	 * @return A pointer to a character string containing the
	 * human-readable error message.
	 */
	const char * getErrorStr(Error err);

	Error setError(Error err);

	/**
	 * Get number of bytes in the file header.  Data packets will
	 * begin after this header.
	 *
	 * @return number of bytes in file header.
	 */
	inline unsigned long getHeader(void)
		{return header;};

	/**
	 * Convert binary 2 byte data stored in the order specified
	 * in the source description into a short variable.  This is
	 * often used to manipulate header data.
	 *
	 * @return short value.
	 * @param data binary 2 byte data pointer.
	 */
	unsigned short getShort(unsigned char *data);

	/**
	 * Save a short as two byte binary data stored in the endian
	 * order specified in the source description.  This is often
	 * used to manipulate header data.
	 *
	 * @param data binary 2 byte data pointer.
	 * @param value to convert.
	 */
	void setShort(unsigned char *data, unsigned short value);

	/**
	 * Convert binary 4 byte data stored in the order specified
	 * in the source description into a long variable.  This is
	 * often used to manipulate header data.
	 *
	 * @return long value.
	 * @param data binary 4 byte data pointer.
	 */
	unsigned long getLong(unsigned char *data);

	/**
	 * Save a long as four byte binary data stored in the endian
	 * order specified in the source description.  This is often
	 * used to manipulate header data.
	 *
	 * @param data binary 4 byte data pointer.
	 * @param value to convert.
	 */
	void setLong(unsigned char *data, unsigned long value);

public:
	/**
	 * Construct and open an existing audio file for read/write.
	 *
	 * @param name of file to open.
	 * @param offset to start access.
	 */
	AudioFile(const char *name, unsigned long offset = 0);

	/**
	 * Create and open a new audio file for writing.
	 *
	 * @param name of file to create.
	 * @param info source description for new file.
	 * @param minimum file size to accept at close.
	 */
	AudioFile(const char *name, Info *info, unsigned long minimum = 0);

	/**
	 * Construct an audio file without attaching to the filesystem.
	 */
	inline AudioFile()
		{initialize();};

	virtual ~AudioFile();

	/**
	 * Open an audio file and associate it with this object.
	 * Called implicitly by the two-argument version of the
	 * constructor.
	 *
	 * @param name of the file to open.  Don't forget to
	 * double your backslashes for DOS-style pathnames.
	 * @param mode to open file under.
	 * @param framing time in milliseconds.
	 */
	void open(const char *name, Mode mode = modeWrite, timeout_t framing = 0);

	/**
	 * Create a new audio file and associate it with this object.
	 * Called implicitly by the three-argument version of the
	 * constructor.
	 *
	 * @param name The name of the file to open.
	 * @param info The type of the audio file to be created.
	 * @param exclusive create option.
	 * @param framing time in milliseconds.
	 */
	void create(const char *name, Info *info, bool exclusive = false, timeout_t framing = 0);

	/**
	 * Returns age since last prior access.  Used for cache
	 * computations.
	 *
	 * @return age in seconds.
	 */
	time_t getAge(void);

	/**
	 * Get maximum size of frame buffer for data use.
	 *
	 * @return max frame size in bytes.
	 */
	inline size_t getSize(void)
		{return maxFramesize(info);};

	/**
	 * Close an object associated with an open file.  This
	 * updates the header metadata with the file length if the
	 * file length has changed.
	 */
	void close(void);

	/**
	 * Clear the AudioFile structure.  Called by
	 * AudioFile::close().  Sets all fields to zero and deletes
	 * the dynamically allocated memory pointed to by the pathname
	 * and info.annotation members.  See AudioFile::initialize()
	 * for the dynamic allocation code.
	 */
	void clear(void);

	/**
	 * Retrieve bytes from the file into a memory buffer.  This
	 * increments the file pointer so subsequent calls read further
	 * bytes.  If you want to read a number of samples rather than
	 * bytes, use getSamples().
	 *
	 * @param buffer area to copy the samples to.
	 * @param len The number of bytes (not samples) to copy or 0 for frame.
	 * @return The number of bytes (not samples) read.  Returns -1
	 * if no bytes are read and an error occurs.
	 */
	ssize_t getBuffer(Encoded buffer, size_t len = 0);

	/**
	 * Retrieve and convert content to linear encoded audio data
	 * from it's original form.
	 *
	 * @param buffer to copy linear data into.
	 * @param request number of linear samples to extract or 0 for frame.
	 * @return number of samples retrieved, 0 if no codec or eof.
	 */
	unsigned getLinear(Linear buffer, unsigned request = 0);

	/**
	 * Insert bytes into the file from a memory buffer.  This
	 * increments the file pointer so subsequent calls append
	 * further samples.  If you want to write a number of samples
	 * rather than bytes, use putSamples().
	 *
	 * @param buffer area to append the samples from.
	 * @param len The number of bytes (not samples) to append.
	 * @return The number of bytes (not samples) read.  Returns -1
	 * if an error occurs and no bytes are written.
	 */
	ssize_t putBuffer(Encoded buffer, size_t len = 0);

	/**
	 * Convert and store content from linear encoded audio data
	 * to the format of the audio file.
	 *
	 * @param buffer to copy linear data from.
	 * @param request Number of linear samples to save or 0 for frame.
	 * @return number of samples saved, 0 if no codec or eof.
	 */
	unsigned putLinear(Linear buffer, unsigned request = 0);

	/**
	 * Retrieve samples from the file into a memory buffer.  This
	 * increments the file pointer so subsequent calls read
	 * further samples.  If a limit has been set using setLimit(),
	 * the number of samples read will be truncated to the limit
	 * position.  If you want to read a certain number of bytes
	 * rather than a certain number of samples, use getBuffer().
	 *
	 * @param buffer pointer to copy the samples to.
	 * @param samples The number of samples to read or 0 for frame.
	 * @return errSuccess if successful, !errSuccess if
	 * error.  Use getErrorStr() to retrieve the human-readable
	 * error string.
	 */
	Error getSamples(void *buffer, unsigned samples = 0);

	/**
	 * Insert samples into the file from a memory buffer.  This
	 * increments the file pointer so subsequent calls append
	 * further samples.  If you want to write a certain number of
	 * bytes rather than a certain number of samples, use
	 * putBuffer().
	 *
	 * @param buffer pointer to append the samples from.
	 * @param samples The number of samples (not bytes) to append.
	 * @return errSuccess if successful, !errSuccess if
	 * error.  Use getErrorStr() to retrieve the human-readable
	 * error string.
	 */
	Error putSamples(void *buffer, unsigned samples = 0);

	/**
	 * Change the file position by skipping a specified number
	 * of audio samples of audio data.
	 *
	 * @return errSuccess or error condition on failure.
	 * @param number of samples to skip.
	 */
	Error skip(long number);

	/**
	 * Seek a file position by sample count.  If no position
	 * specified, then seeks to end of file.
	 *
	 * @return errSuccess or error condition on failure.
	 * @param samples position to seek in file.
	 */
	Error setPosition(unsigned long samples = ~0l);

	/**
	 * Seek a file position by timestamp.  The actual position
	 * will be rounded by framing.
	 *
	 * @return errSuccess if successful.
	 * @param timestamp position to seek.
	 */
	Error position(const char *timestamp);

	/**
	 * Return the timestamp of the current absolute file position.
	 *
	 * @param timestamp to save ascii position into.
	 * @param size of timestamp buffer.
	 */
	void getPosition(char *timestamp, size_t size);

	/**
	 * Set the maximum file position for reading and writing of
	 * audio data by samples.  If 0, then no limit is set.
	 *
	 * @param maximum file i/o access size sample position.
	 * @return errSuccess if successful.
	 */
	Error setLimit(unsigned long maximum = 0l);

	/**
	 * Copy the source description of the audio file into the
	 * specified object.
	 *
	 * @param info pointer to object to copy source description into.
	 * @return errSucess.
	 */
	Error getInfo(Info *info);

	/**
	 * Set minimum file size for a created file.  If the file
	 * is closed with fewer samples than this, it will also be
	 * deleted.
	 *
	 * @param minimum number of samples for new file.
	 * @return errSuccess if successful.
	 */
	Error setMinimum(unsigned long minimum);

	/**
	 * Get the current file pointer in bytes relative to the start
	 * of the file.  See getPosition() to determine the position
	 * relative to the start of the sample buffer.
	 *
	 * @return The current file pointer in bytes relative to the
	 * start of the file.  Returns 0 if the file is not open, is
	 * empty, or an error has occured.
	 */
	unsigned long getAbsolutePosition(void);

	/**
	 * Get the current file pointer in samples relative to the
	 * start of the sample buffer.  Note that you must multiply
	 * this result by the result of a call to
	 * toBytes(info.encoding, 1) in order to determine the offset
	 * in bytes.
	 *
	 * @return the current file pointer in samples relative to the
	 * start of the sample buffer.  Returns 0 if the file is not
	 * open, is empty, or an error has occured.
	 */
	unsigned long getPosition(void);

	/**
	 * Test if the file is opened.
	 *
	 * @return true if a file is open.
	 */
	virtual bool isOpen(void);

	/**
	 * Return true if underlying derived class supports direct
	 * access to file positioning.  Derived classes based on URL's
	 * or fifo devices may not have this ability.
	 *
	 * @return true if file positioning is supported.
	 */
	virtual bool hasPositioning(void)
		{return true;};

	/**
	 * Return audio encoding format for this audio file.
	 *
	 * @return audio encoding format.
	 */
	inline Encoding getEncoding(void)
		{return info.encoding;};

	/**
	 * Return base file format of containing audio file.
	 *
	 * @return audio file container format.
	 */
	inline Format getFormat(void)
		{return info.format;};

	/**
	 * Get audio encoding sample rate, in samples per second, for
	 * this audio file.
	 *
	 * @return sample rate.
	 */
	inline unsigned getSampleRate(void)
		{return info.rate;};

	/**
	 * Get annotation extracted from header of containing file.
	 *
	 * @return annotation text if any, else NULL.
	 */
	inline char *getAnnotation(void)
		{return info.annotation;};

	/**
	 * Get last error code.
	 *
	 * @return alst error code.
	 */
	inline Error getError(void)
		{return error;};

	inline bool operator!(void)
		{return (bool)!isOpen();};

	/**
	 * Return if the current content is signed or unsigned samples.
	 *
	 * @return true if signed.
	 */
	bool isSigned(void);
};

/**
 * AudioStream accesses AudioFile base class content as fixed frames
 * of streaming linear samples.  If a codec must be assigned to perform
 * conversion to/from linear data, AudioStream will handle conversion
 * automatically.  AudioStream will also convert between mono and stereo
 * audio content.  AudioStream uses linear samples in the native
 * machine endian format and perform endian byte swapping as needed.
 *
 * @author David Sugar <dyfet@ostel.com>
 * @short Audio Streaming with Linear Conversion
 */
class __EXPORT AudioStream : public AudioFile
{
protected:
	AudioCodec *codec;	// if needed
	Encoded	framebuf;
	bool streamable;
	Linear bufferFrame;
	unsigned bufferPosition;
	unsigned bufferChannels;
	Linear encBuffer, decBuffer;
	unsigned encSize, decSize;

	unsigned bufAudio(Linear samples, unsigned count, unsigned size);

public:
	/**
	 * Create a new audiostream object.
	 */
	AudioStream();

	/**
	 * Create an audio stream object and open an existing audio file.
	 *
	 * @param name of file to open.
	 * @param mode of file access.
	 * @param framing time in milliseconds.
	 */
	AudioStream(const char *name, Mode mode = modeRead, timeout_t framing = 0);

	/**
	 * Create an audio stream object and a new audio file.
	 *
	 * @param name of file to open.
	 * @param info source description for properties of new file.
	 * @param exclusive access if true.
	 * @param framing time in milliseconds.
	 */
	AudioStream(const char *name, Info *info, bool exclusive = false, timeout_t framing = 0);

	virtual ~AudioStream();

	/**
	 * Virtual for packet i/o intercept.
	 *
	 * @return bytes read.
	 * @param data encoding buffer.
	 * @param count requested.
	 */
	ssize_t getBuffer(Encoded data, size_t count);

	/**
	 * Open existing audio file for streaming.
	 *
	 * @param name of file to open.
	 * @param mode to use file.
	 * @param framing timer in milliseconds.
	 */
	void open(const char *name, Mode mode = modeRead, timeout_t framing = 0);

	/**
	 * Create a new audio file for streaming.
	 *
	 * @param name of file to create.
	 * @param info source description for file properties.
	 * @param exclusive true for exclusive access.
	 * @param framing timing in milliseconds.
	 */
	void create(const char *name, Info *info, bool exclusive = false, timeout_t framing = 0);

	/**
	 * Close the currently open audio file for streaming.
	 */
	void close(void);

	/**
	 * flush any unsaved buffered data to disk.
	 */
	void flush(void);

	/**
	 * Check if the audio file may be streamed.  Files can be
	 * streamed if a codec is available or if they are linear.
	 *
	 * @return true if streamable.
	 */
	bool isStreamable(void);

	/**
	 * Get the number of samples expected in a frame.
	 */
	unsigned getCount(void);	// frame count

	/**
	 * Stream audio data from the file and convert into an alternate
	 * encoding based on the codec supplied.
	 *
	 * @param codec to apply before saving.
	 * @param address of data to save.
	 * @param frames to stream by the codec.
	 * @return number of frames processed.
	 */
	unsigned getEncoded(AudioCodec *codec, Encoded address, unsigned frames = 1);

	/**
	 * Stream audio data in an alternate codec into the currently
	 * opened file.
	 *
	 * @param codec to convert incoming data from.
	 * @param address of data to convert and stream.
	 * @param frames of audio to stream.
	 * @return number of frames processed.
	 */
	unsigned putEncoded(AudioCodec *codec, Encoded address, unsigned frames = 1);

	/**
	 * Get data from the streamed file in it's native encoding.
	 *
	 * @param address to save encoded audio.
	 * @param frames of audio to load.
	 * @return number of frames read.
	 */
	unsigned getEncoded(Encoded address, unsigned frames = 1);

	/**
	 * Put data encoded in the native format of the stream file.
	 *
	 * @param address to load encoded audio.
	 * @param frames of audio to save.
	 * @return number of frames written.
	 */
	unsigned putEncoded(Encoded address, unsigned frames = 1);

	/**
	 * Get a packet of data from the file.  This uses the codec
	 * to determine what a true packet boundry is.
	 *
	 * @param buffer to save encoded data.
	 * @return number of bytes read as packet.
	 */
	ssize_t getPacket(Encoded data);

	/**
	 * Get and automatically convert audio file data into
	 * mono linear audio samples.
	 *
	 * @param buffer to save linear audio into.
	 * @param frames of audio to read.
	 * @return number of frames read from file.
	 */
	unsigned getMono(Linear buffer, unsigned frames = 1);

	/**
	 * Get and automatically convert audio file data into
	 * stereo (two channel) linear audio samples.
	 *
	 * @param buffer to save linear audio into.
	 * @param frames of audio to read.
	 * @return number of frames read from file.
	 */
	unsigned getStereo(Linear buffer, unsigned frames = 1);

	/**
	 * Automatically convert and put mono linear audio data into
	 * the audio file.  Convert to stereo as needed by file format.
	 *
	 * @param buffer to save linear audio from.
	 * @param frames of audio to write.
	 * @return number of frames written to file.
	 */
	unsigned putMono(Linear buffer, unsigned frames = 1);

	/**
	 * Automatically convert and put stereo linear audio data into
	 * the audio file.  Convert to mono as needed by file format.
	 *
	 * @param buffer to save linear audio from.
	 * @param frames of audio to write.
	 * @return number of frames written to file.
	 */
	unsigned putStereo(Linear buffer, unsigned frames = 1);

	/**
	 * Automatically convert and put arbitrary linear mono data
	 * into the audio file.  Convert to stereo and buffer incomplete
	 * frames as needed by the streaming file.
	 *
	 * @param buffer to save linear audio from.
	 * @param count of linear audio to write.
	 * @return number of linear audio samples written to file.
	 */
	unsigned bufMono(Linear buffer, unsigned count);

	/**
	 * Automatically convert and put arbitrary linear stereo data
	 * into the audio file.  Convert to mono and buffer incomplete
	 * frames as needed by the streaming file.
	 *
	 * @param buffer to save linear audio from.
	 * @param count of linear audio to write.
	 * @return number of linear audio samples written to file.
	 */
	unsigned bufStereo(Linear buffer, unsigned count);

	/**
	 * Return the codec being used if there is one.
	 *
	 * @return codec used.
	 */
	inline AudioCodec *getCodec(void)
		{return codec;};
};

/**
 * The codec class is a virtual used for transcoding audio samples between
 * linear frames (or other known format) and an encoded "sample" buffer.
 * This class is only abstract and describes the core interface for
 * loadable codec modules.  This class is normally merged with AudioSample.
 * A derived AudioCodecXXX will typically include a AudioRegisterXXX static
 * class to automatically initialize and register the codec with the codec
 * registry.
 *
 * @author David Sugar <dyfet@ostel.com>
 * @short process codec interface.
 */
class __EXPORT AudioCodec : public Audio
{
protected:
	static AudioCodec *first;
	AudioCodec *next;
	Encoding encoding;
	const char *name;
	Info info;

	AudioCodec();

	/**
	 * often used to create a "new" codec of a subtype based on
	 * encoding format, default returns the current codec entity.
	 *
	 * @return pointer to an active codec or NULL if not found.
	 * @param format name from spd.
	 */
	virtual AudioCodec *getByFormat(const char *format)
		{return this;};

	/**
	 * get a codec by audio source info descriptor.
	 *
	 * @return pointer to an active codec or NULL if not found.
	 * @param info audio source descriptor.
	 */
	virtual AudioCodec *getByInfo(Info &info)
		{return this;};

public:
	/**
	 * Base for codecs, create a named coded of a specific encoding.
	 *
	 * @param name of codec.
	 * @param encoding type of codec.
	 */
	AudioCodec(const char *name, Encoding encoding);

	virtual ~AudioCodec() {};

	/**
	 * End use of a requested codec.  If constructed then will be
	 * deleted.
	 *
	 * @param codec pointer to getCodec returned coded pointer.
	 */
	static void endCodec(AudioCodec *codec);

	/**
	 * Get the codec base class for accessing a specific derived
	 * codec identified by encoding type and optional spd info.
	 *
	 * @return pointer to codec for processing.
	 * @param encoding format requested.
	 * @param format spd options to pass to codec being created.
	 * @param loaded true to load if not already in memory.
	 */
	static AudioCodec *getCodec(Encoding encoding, const char *format = NULL, bool loaded = false);

	/**
	 * Get the codec base class for accessing a specific derived
	 * codec identified by audio source descriptor.
	 *
	 * @return pointer to codec for processing.
	 * @param info source descriptor for codec being requested.
	 * @param loaded true to load codec if not already in memory.
	 */
	static AudioCodec *getCodec(Info &info, bool loaded = false);

	/**
	 * Load a named codec set into process memory.
	 *
	 * @return true if successful.
	 * @param name of codec set to load.
	 */
	static bool load(const char *name);

	/**
	 * Find and load a codec file by it's encoding type.  Converts
	 * the type into a codec name and invokes the other loader...
	 *
	 * @return true if successful.
	 * @param encoding type for file.
	 */
	static bool load(Encoding encoding);

	/**
	 * Get the impulse energy level of a frame of X samples in
	 * the specified codec format.
	 *
 	 * @return average impulse energy of frame (sumnation).
	 * @param buffer of encoded samples.
	 * @param number of encoded samples.
	 */
	virtual Level getImpulse(void *buffer, unsigned number = 0);

	/**
	 * Get the peak energy level within the frame of X samples.
	 *
	 * @return peak energy impulse in frame (largest).
	 * @param buffer of encoded samples.
	 * @param number of encoded samples.
	 */
	virtual Level getPeak(void *buffer, unsigned number = 0);

	/**
	 * Signal if the current audio frame is silent.  This can be
	 * deterimed either by an impulse computation, or, in some
	 * cases, some codecs may signal and flag silent packets.
	 *
	 * @return true if silent
	 * @param threashold to use if not signaled.
	 * @param buffer of encoded samples.
	 * @param number of encoded samples.
	 */
	virtual bool isSilent(Level threashold, void *buffer, unsigned number = 0);

	/**
	 * Encode a linear sample frame into the codec sample buffer.
	 *
	 * @return number of bytes written.
	 * @param buffer linear sample buffer to use.
	 * @param dest buffer to store encoded results.
	 * @param number of samples.
	 */
	virtual unsigned encode(Linear buffer, void *dest, unsigned number = 0) = 0;

	/**
	 * Encode linear samples buffered into the coded.
	 *
	 * @return number of bytes written or 0 if incomplete.
	 * @param buffer linear samples to post.
	 * @param destination of encoded audio.
	 * @param number of samples being buffered.
	 */
	virtual unsigned encodeBuffered(Linear Buffer, Encoded dest, unsigned number);

	/**
	 * Decode the sample frame into linear samples.
	 *
	 * @return number of bytes scanned or returned
	 * @param buffer sample buffer to save linear samples into.
	 * @param source for encoded data.
	 * @param number of samples to extract.
	 */
	virtual unsigned decode(Linear buffer, void *source, unsigned number = 0) = 0;

	/**
	 * Buffer and decode data into linear samples.  This is needed
	 * for audio formats that have irregular packet sizes.
	 *
	 * @return number of samples actually decoded.
	 * @param destination for decoded data.
	 * @param source for encoded data.
	 * @param number of bytes being sent.
	 */
	virtual unsigned decodeBuffered(Linear buffer, Encoded source, unsigned len);

	/**
	 * Get estimated data required for buffered operations.
	 *
	 * @return estimated number of bytes required for decode.
	 */
	virtual unsigned getEstimated(void);

	/**
	 * get required samples for encoding.
	 *
	 * @return required number of samples for encoder buffer.
	 */
	virtual unsigned getRequired(void);

	/**
	 * Get a packet of data rather than decode.  This is tied with
	 * getEstimated.
	 *
	 * @return size of data packet or 0 if not complete.
	 * @param destination to save.
	 * @param data to push into buffer.
	 * @param number of bytes to push.
	 */
	virtual unsigned getPacket(Encoded destination, Encoded data, unsigned size);

	/**
	 * Get an info description for this codec.
	 *
	 * @return info.
	 */
	inline Info getInfo(void)
		{return info;};
};

class __EXPORT AudioDevice : public AudioBase
{
protected:
	bool enabled;

public:
	virtual ~AudioDevice() {};

	/**
	 * Copy linear samples to an audio device through its virtual.
	 *
	 * @param buffer to linear audio data to play.
	 * @param count of audio samples to play.
	 * @return number of audio samples played.
	 */
	virtual unsigned putSamples(Linear buffer, unsigned count) = 0;

	/**
	 * Copy linear samples from an audio device through its virtual.
	 *
	 * @param buffer for recording.
	 * @param count of audio samples to record.
	 * @return number of audio samples recorded.
	 */
	virtual unsigned getSamples(Linear buffer, unsigned count) = 0;

	/**
	 * Copy audio encoded in the currently selected encoding for
	 * the audio device.
	 *
	 * @param data pointer to encoded data to play.
	 * @param count of encoded bytes to play.
	 * @return number of encoded bytes played.
	 */
	virtual ssize_t putBuffer(Encoded data, size_t count);

	/**
	 * Record audio encoded in the currently selected encoding for
	 * the audio device.
	 *
	 * @param data buffer for recording encoded audio.
	 * @param count of encoded bytes to record.
	 * @return number of encoded bytes recorded.
	 */
	virtual ssize_t getBuffer(Encoded data, size_t count);

	/**
	 * Use encoding source descriptor to select the audio encoding
	 * format the audio device should be using.
	 *
	 * @return false if encoding format specified is unsupported by device
	 * @param info source description for device settings.
	 */
	virtual bool setEncoded(Info &info)
		{return false;};

	/**
	 * Set properties for audio device.
	 *
	 * @param rate of audio samples device should operate at.
	 * @param stereo flag.
	 * @param framing timer for default i/o framing for device.
	 * @return false if settings not supported by device.
	 */
	virtual bool setAudio(Rate rate = rate8khz, bool stereo = false, timeout_t framing = 20) = 0;

	/**
	 * Synchronize timing for audio device to next audio frame.
	 * this is needed for audio devices which do not block i/o to
	 * assure one does not push too much data before the device
	 * can handle it.
	 */
	virtual void sync(void)
		{return;};

	/**
	 * Flush any pending buffered samples in audio device.
	 */
	virtual void flush(void) = 0;

	/**
	 * Process linear mono audio and automatically convert to the
	 * encoding format the audio device is currently using.
	 * If needed, automatically convert from mono to stereo.
	 *
	 * @return number of samples played.
	 * @param buffer to linear mono audio data to play.
	 * @param count of linear mono audio samples to play.
	 */
	unsigned bufMono(Linear buffer, unsigned count);

	/**
	 * Process linear stereo audio and automatically convert to the
	 * encoding format the audio device is currently using.
	 * If needed, automatically convert from stereo to mono.
	 *
	 * @return number of samples played.
	 * @param buffer to linear stereo audio data to play.
	 * @param count of linear stereo audio samples to play.
	 */
	unsigned bufStereo(Linear buffer, unsigned count);

	/**
	 * Get audio device source descriptor in effect for the device.
	 *
	 * @return audio device descriptor.
	 */
	inline Info *getInfo(void)
		{return &info;};

	/**
	 * Whether device is currently enabled.  If invalid audio
	 * settings are selected, it will be disabled until supported
	 * values are supplied.
	 *
	 * @return enable state.
	 * @see #setAudio #setInfo
	 */
	inline bool isEnabled(void)
		{return enabled;};
};

/**
 * An object that is used to sequence and extract telephony tones
 * based on a telephony tone descriptor retrieved from the parsed
 * international telephony tone database.
 *
 * @author David Sugar <dyfet@ostel.com>
 * @short telephony tone sequencing object.
 */
class __EXPORT TelTone : public AudioTone
{
public:
	typedef struct _tonedef {
		struct _tonedef *next;
		timeout_t duration, silence;
		unsigned count;
		unsigned short f1, f2;
	} tonedef_t;

	typedef struct _tonekey {
		struct _tonekey *next;
		struct _tonedef *first;
		struct _tonedef *last;
		char id[1];
	} tonekey_t;

	/**
	 * Create a tone sequencing object for a specific telephony tone
	 * key id.
	 *
	 * @param key for telephony tone.
	 * @param level for generated tones.
	 * @param frame timing to use in processing.
	 */
	TelTone(tonekey_t *key, Level level, timeout_t frame = 20);
	~TelTone();

	/**
	 * Generate and retrieve one frame of linear audio data for
	 * the telephony tone sequence being created.
	 *
	 * @return pointer to samples generated.
	 */
	Linear getFrame(void);

	/**
	 * Check if all audio frames for this tone has been created.
	 * Some telephony tones, such as dialtone, may be infinite...
	 *
	 * @return true if audio is complete.
	 */
	bool isComplete(void);


	/**
	 * Load a teltones database file into memory.
	 *
	 * @return true if successful
	 * @param pathname of file to load.
	 * @param locale to optionally load.
	 */
	static bool load(const char *pathname, const char *locale = NULL);

	/**
	 * find an entry in the teltones database.
	 *
	 * @return key of tone list if found, else NULL
	 * @param tone name
	 * @param locale to optionally search under
	 */
	static tonekey_t *find(const char *tone, const char *locale = NULL);

protected:
	tonekey_t *tone;
	tonedef_t *def;
	unsigned remaining, silent, count;
	timeout_t framing;
	Level level;
	bool complete;
};

/**
 * DTMFTones is used to generate a series of dtmf audio data from a
 * "telephone" number passed as an ASCII string.  Each time getFrame()
 * is called, the next audio frame of dtmf audio will be created
 * and pulled.
 *
 * @author David Sugar <dyfet@ostel.com>
 * @short Generate DTMF audio
 */
class __EXPORT DTMFTones : public AudioTone
{
protected:
	unsigned remaining, dtmfframes;
	timeout_t frametime;
	const char *digits;
	Level level;
	bool complete;

public:
	/**
	 * Generate a dtmf dialer for a specified dialing string.
	 *
	 * @param digits to generate tone dialing for.
	 * @param level for dtmf.
	 * @param duration timing for generated audio.
	 * @param interdigit timing, should be multiple of frame.
	 */
	DTMFTones(const char *digits, Level level, timeout_t duration = 20, timeout_t interdigit = 60);

	~DTMFTones();

	Linear getFrame(void);
	bool isComplete(void);
};

/**
 * MFTones is used to generate a series of mf audio data from a
 * "telephone" number passed as an ASCII string.  Each time getFrame()
 * is called, the next audio frame of dtmf audio will be created
 * and pulled.
 *
 * @author David Sugar <dyfet@ostel.com>
 * @short Generate MF audio
 */
class __EXPORT MFTones : public AudioTone
{
protected:
	unsigned remaining, mfframes;
	timeout_t frametime;
	const char *digits;
	Level level;
	bool complete, kflag;

public:
	/**
	 * Generate a mf dialer for a specified dialing string.
	 *
	 * @param digits to generate tone dialing for.
	 * @param level for mf.
	 * @param duration timing for generated audio.
	 * @param interdigit timing, should be multiple of frame.
	 */
	MFTones(const char *digits, Level level, timeout_t duration = 20, timeout_t interdigit = 60);

	~MFTones();

	Linear getFrame(void);
	bool isComplete(void);
};


/**
 * DTMFDetect is used for detecting DTMF tones in a stream of audio.
 * It currently only supports 8000Hz input.
 */
class __EXPORT DTMFDetect : public Audio
{
public:
	DTMFDetect();
	~DTMFDetect();

	/**
	 * This routine is used to push linear audio data into the
	 * dtmf tone detection analysizer.  It may be called multiple
	 * times and results fetched later.
	 *
	 * @param buffer of audio data in native machine endian to analysize.
	 * @param count of samples to analysize from buffer.
	 */
	int putSamples(Linear buffer, int count);

	/**
	 * Copy detected dtmf results into a data buffer.
	 *
	 * @param data buffer to copy into.
	 * @param size of data buffer to copy into.
	 */
	int getResult(char *data, int size);

protected:
	void goertzelInit(goertzel_state_t *s, tone_detection_descriptor_t *t);
	void goertzelUpdate(goertzel_state_t *s, Sample x[], int samples);
	float goertzelResult(goertzel_state_t *s);

private:
	dtmf_detect_state_t *state;
	tone_detection_descriptor_t dtmf_detect_row[4];
	tone_detection_descriptor_t dtmf_detect_col[4];
	tone_detection_descriptor_t dtmf_detect_row_2nd[4];
	tone_detection_descriptor_t dtmf_detect_col_2nd[4];
	tone_detection_descriptor_t fax_detect;
	tone_detection_descriptor_t fax_detect_2nd;
};

}

#endif

