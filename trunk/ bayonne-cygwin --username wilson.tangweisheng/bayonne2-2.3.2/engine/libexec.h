// Copyright (C) 2005 Open Source Telecom Corp.
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
// 
// As a special exception, you may use this file as part of a free software
// library without restriction.  Specifically, if other files instantiate
// templates or use macros or inline functions from this file, or you compile
// this file and link it with other files to produce an executable, this
// file does not by itself cause the resulting executable to be covered by
// the GNU General Public License.  This exception does not however
// invalidate any other reasons why the executable file might be covered by
// the GNU General Public License.
//
// This exception applies only to the code released under the name GNU
// ccScript.  If you copy code from other releases into a copy of GNU
// ccScript, as the General Public License permits, the exception does
// not apply to the code that you add in this way.  To avoid misleading
// anyone as to the status of such modified files, you must delete
// this exception notice from them.
//
// If you write modifications of your own for GNU ccScript, it is your choice
// whether to permit this exception to apply to your modifications.
// If you do not wish that, delete this exception notice.
//

#ifndef	CCXX_LIBEXEC_H_
#define	CCXX_LIBEXEC_H_

#ifndef	CCXX_BAYONNE_H_
#include <cc++/bayonne.h>
#endif

#ifndef	CCXX_SLOG_H_
#include <cc++/slog.h>
#endif

#ifndef	CCXX_PROCESS_H_
#include <cc++/process.h>
#endif

namespace ost {

/**
 * Container class for applications implimenting the libexec process
 * method of Bayonne interfacing.  This is intended for writing external
 * apps and is neatly tucked away into libbayonne as well.
 *
 * @author David Sugar <dyfet@gnutelephony.org>
 * @short Libexec process interface class.
 */
class __EXPORT Libexec : public Bayonne
{
protected:
	Keydata head, args;
	const char *tsid;
	const char *voice;
	Audio::Level level;

public:
	result_t result;
	char digits[64];
	char query[512];
	char position[32];
	unsigned exitcode, reply;

	/**
	 * Initialize libexec.
	 */
	Libexec();

	/**
	 * Get a header record item.
	 *
	 * @param id of header or sys env item.
	 * @return string value of requested item or NULL.
	 */
	const char *getEnv(const char *id);

	/**
	 * Get a named libexec command line argument.
	 *
	 * @param id of libexec argument.
	 * @return string value of requested argument or NULL.
	 */
	const char *getArg(const char *id);	

	/**
	 * Get a fully qualified and resolved pathname.
	 *
	 * @return pointer to buffer or NULL if invalid.
	 * @param filename path to evaluate.
	 * @param buffer to save into.
	 * @param size of buffer.
	 */
	const char *getPath(const char *filename, char *buffer, unsigned size);

        /**
         * Get and verify partial pathname for file oriented libexec commands.
         *
         * @return pointer to buffer or NULL if invalid.
         * @param filename path to evaluate.
         */
        const char *getFile(const char *filename);

	/**
	 * Set the effective voice library to use.
	 *
	 * @param voice to set or NULL for default.
	 */
	inline void setVoice(const char *voice)
		{voice = voice;};

	/**
	 * Set the effective audio level for tones...
	 *
	 * @param level to set.
	 */
	inline void setLevel(Audio::Level level)
		{level = level;};

	/**
	 * Hangup running session...
	 */
	void hangupSession(void);

	/**
	 * Resume server session, libexec continues detached.
	 */
	void detachSession(unsigned code);

	/**
	 * Send a command through to server and capture result.
	 *
	 * @return result code.
	 * @param command to send.
	 * @param optional query buffer.
	 * @param optional query size.
	 */
	result_t sendCommand(const char *text, char *buffer = NULL, unsigned size = 0);

	/**
	 * Send a result to the server.
	 *
	 * @return result code from server.
	 * @param result to send.
	 */
	result_t sendResult(const char *text);

	/**
	 * Send an error to the server.
	 *
	 * @param error msg to send.
	 */
	void sendError(const char *msg);

	/**
	 * Transfer a call.
	 *
	 * @return result code from server.
	 * @param destination to transfer.
	 */
	result_t xferCall(const char *dest);

	/**
	 * Replay an audio file.
	 *
	 * @return result code.
	 * @param name of file to play.
	 */
	result_t replayFile(const char *file);

	/**
	 * Replay an audio file from a specified offset.
	 *
	 * @return result code.
	 * @param name of file to play.
	 * @param offset to play from.
	 */
	result_t replayOffset(const char *file, const char *offset);                                                                            

	/**
	 * Record an audio file.
	 *
	 * @return result code.
	 * @param name of file to record.
	 * @param total duration of file.
	 * @param optional silence detect.
	 */
	result_t recordFile(const char *file, timeout_t duration, timeout_t silence = 0);

	/**
	 * Play a phrase.
	 *
	 * @return result code.
	 * @param text of phrase to play.
	 */
	result_t speak(const char *format, ...);

	/**
	 * Play a tone.
	 *
	 * @return result code.
	 * @param name of tone to play.
	 * @param duration for tone.
	 * @param audio level of tone.
	 */
	result_t playTone(const char *name, timeout_t duration = 0, unsigned level = 0);
	result_t playSingleTone(short f1, timeout_t duration, unsigned level = 0);
	result_t playDualTone(short f1, short f2, timeout_t duration, unsigned level = 0);

	/**
	 * Record an audio file to a specified offset.
	 *
	 * @return result code.
	 * @param name of file to record.
	 * @param offset to record info.
	 * @param total duration of file.
	 * @param optional silence detect.
	 */
	result_t recordOffset(const char *file, const char *offset, timeout_t duration, timeout_t silence = 0);                                                                            

	/**
	 * Erase an audio file.
	 *
	 * @return result code.
	 * @param name of file to erase.
	 */
	result_t eraseFile(const char *file);

        /**
         * Move an audio file.
         *
         * @return result code.
         * @param name of file to move.
	 * @param destination of move.
         */ 
        result_t moveFile(const char *file1, const char *file2);

	/**
	 * Flush input.
	 */
	result_t clearInput(void);

	/**
	 * Wait for input.
	 * 
	 * @return true if input waiting.
	 */
	bool waitInput(timeout_t timeout);
                                           
	/**
	 * Read a line of input.
	 *
	 * @return result code.
	 * @param input buffer.
	 * @param size of input buffer.
	 * @param timeout for input.
	 */
	result_t readInput(char *buffer, unsigned size, timeout_t timeout);

	/**
	 * Read a single key of input.
	 *
	 * @return key input or 0.
	 * @param timeout for read.
	 */
	char readKey(timeout_t timeout);

	result_t sizeSym(const char *id, unsigned size);
	result_t addSym(const char *id, const char *value);
	result_t setSym(const char *id, const char *value);
	result_t getSym(const char *id, char *buf, unsigned size);

        /**
         * Post a symbol asychrononous event to server.  This sets the
         * symbol value, and also generates a @posted:symname event.
         *
         * @param id of symbol to post.
         * @param value of symbol.
         */  
	void postSym(const char *id, const char *value);
};	 

class __EXPORT BayonneTSession : public BayonneSession
{
protected:
	friend class __EXPORT BayonneSysexec;

	void sysPost(const char *sid, char *id, const char *value);
        void sysVar(const char *tsid, char *id, const char *value, int size);
        void sysHeader(const char *tsid);
        void sysArgs(const char *tsid);
        void sysStatus(const char *tsid);  
	void sysRecord(const char *tsid, char *token);
	void sysReplay(const char *tsid, char *token);
	void sysFlush(const char *tsid);
	void sysWait(const char *tsid, char *token);
	void sysTone(const char *tsid, char *token);
	void sysSTone(const char *tsid, char *token);
	void sysDTone(const char *tsid, char *token);
	void sysPrompt(const char *tsid, const char *voice, const char *text);
	void sysInput(const char *tsid, char *token);
	void sysHangup(const char *tsid);
	void sysExit(const char *tsid, char *token);
	void sysError(const char *tsid, char *token);
	void sysReturn(const char *tsid, const char *text);
	void sysXfer(const char *tsid, const char *dest);
};

/**
 * Core class for any server which impliments libexec functionality.
 *
 * @author David Sugar
 * @short Server system execution interface
 */
class __EXPORT BayonneSysexec : protected Thread, protected Bayonne
{
private:
	static bool exiting;
#ifndef	WIN32
	static int iopair[2];
#endif
	static BayonneSysexec *libexec;

	static void readline(char *buf, unsigned max);

	void run(void);

	BayonneSysexec();
	~BayonneSysexec();

public:
	static bool create(BayonneSession *s);
	static void allocate(const char *path, size_t bs = 0, int pri = 0, const char *modpath = NULL);
	static void cleanup(void);
	static void startup(void);
};

}; // namespace

#endif
