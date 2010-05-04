// Copyright (C) 1999-2005 Open Source Telecom Corporation.
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

/**
 * @file script3.h
 * @short Threaded step execute scripting engine framework.
 **/


#ifndef	CCXX_SCRIPT3_H_
#define	CCXX_SCRIPT3_H_

#ifndef	CCXX_MISC_H_
#include <cc++/misc.h>
#endif

#ifndef	CXXX_FILE_H_
#include <cc++/file.h>
#endif

#ifndef	CCXX_BUFFER_H_
#include <cc++/buffer.h>
#endif

#define TRAP_BITS (sizeof(unsigned long) * 8)
#define	SCRIPT_INDEX_SIZE KEYDATA_INDEX_SIZE
#define	SCRIPT_MAX_ARGS	250
#define	SCRIPT_TEMP_SPACE 16
#define	SCRIPT_STACK_SIZE 32
#define	SCRIPT_ROUTE_SLOTS 16
#define	SCRIPT_EXEC_WRAPPER
#define	SCRIPT_APPS_WRAPPER
#define	SCRIPT_RIPPLE_LEVEL 2
#define	SCRIPT_RIPPLE_KEYDATA
#define	SCRIPT_BINDER_SELECT
#define	SCRIPT_SERVER_PREFIX
#define	SCRIPT_DEFINE_TOKENS

#ifndef CCXX_PACKING
#if defined(__GNUC__)
#define CCXX_PACKED
#elif !defined(__hpux) && !defined(_AIX)
#define CCXX_PACKED
#endif
#endif

namespace ost {

class __EXPORT ScriptRegistry;
class __EXPORT ScriptCommand;
class __EXPORT ScriptImage;
class __EXPORT ScriptInterp;
class __EXPORT ScriptSymbols;
class __EXPORT ScriptProperty;
class __EXPORT ScriptThread;
class __EXPORT ScriptCompiler;

/**
 * Generic script class to hold master data types and various useful
 * class encpasulated friend functions.
 *
 * @author David Sugar <dyfet@ostel.com>
 * @short Master script class.
 */
class __EXPORT Script
{
public:
	class __EXPORT Line;
	class __EXPORT Name;

	typedef bool (ScriptInterp::*Method)(void);
	typedef const char *(ScriptCommand::*Check)(Line *line, ScriptImage *img);
	typedef bool (*Cond)(ScriptInterp *interp, const char *v);
	typedef long (*Function)(long *args, unsigned prec);
	typedef const char *(*Meta)(ScriptInterp *interp, const char *token);
	typedef const char *(*Parse)(ScriptCompiler *img, const char *token);
	typedef void (*Init)(void);

	enum scrAccess {
		scrPUBLIC,
		scrPROTECTED,
		scrPRIVATE,
		scrFUNCTION,
		scrLOCAL
	};

	typedef enum scrAccess scrAccess;

	enum symType {
		symNORMAL = 0,
		symCONST,
		symDYNAMIC,
		symFIFO,
		symSEQUENCE,
		symSTACK,
		symCOUNTER,
		symPOINTER,
		symREF,
		symARRAY,
		symASSOC,
		symINITIAL,
		symNUMBER,
		symLOCK,
		symPROPERTY,
		symORIGINAL,
		symMODIFIED,
		symTIMER,
		symBOOL
	};

	typedef enum symType symType;

	typedef struct _symbol {
		struct _symbol *next;
		const char *id;
		unsigned short size;
		symType type: 8;
		char data[1];
	}	Symbol;

#ifdef CCXX_PACKED
#pragma	pack(1)
#endif

	typedef struct _array {
		unsigned short head, tail, rec, count;
	}	Array;

#ifdef	CCXX_PACKED
#pragma pack()
#endif

public:
	class __EXPORT Line
	{
	public:
		Line *next;
		union {
			ScriptRegistry *registry;
			Method method;
			Name *name;
		}	scr;
		const char *cmd, **args;
		unsigned long cmask, mask;
		unsigned short loop, line, lnum;
		unsigned short argc;
	};

	class __EXPORT NamedEvent
	{
	public:
		NamedEvent *next;
		Line *line;
		char type;
		const char *name;
	};

	class __EXPORT Name
	{
	public:
		Name *next;
		NamedEvent *events;
		Line *first, *select;
		Line *trap[TRAP_BITS];
		unsigned long mask;
		const char *name, *filename;
		scrAccess access;
	};

	class __EXPORT Initial
	{
		public:
		const char *name;
		unsigned size;
		const char *value;
	};

	class __EXPORT Define
	{
		public:
		const char *keyword;
		bool init;
		Method method;
		Check check;
	};

	class __EXPORT Test
	{
		public:
		const char *id;
		Cond handler;
		Test *next;
	};

	class __EXPORT Fun
	{
	public:
		const char *id;
		unsigned args;
		Function fn;
		Fun *next;
	};

	class __EXPORT InitScript
	{
	public:
		Init handler;
		InitScript *next;
	};

	class __EXPORT Package : public DSO
	{
	public:
		static Package *first;
		Package *next;
		const char *filename;

		Package(const char *name);
	};

	static bool fastStart;
	static bool useBigmem;
	static unsigned fastStepping;
	static unsigned autoStepping;
	static size_t pagesize;
	static unsigned symsize;
	static unsigned symlimit;

	static bool isScript(Name *scr);
	static bool isSymbol(const char *id);
	static bool use(const char *name);
	static unsigned getIndex(const char *id);
	static Symbol *deref(Symbol *sym);
	static bool commit(Symbol *sym, const char *value);
	static bool append(Symbol *sym, const char *value);
	static bool symindex(Symbol *sym, short offset);
	static const char *extract(Symbol *sym);
	static unsigned count(Symbol *sym);
	static unsigned storage(Symbol *sym);
	static void clear(Symbol *sym);
	static char decimal;
	static bool use_definitions;
	static bool use_macros;
	static bool use_prefix;
	static bool use_merge;
	static bool use_funcs;
	static const char *plugins;
	static const char *altplugins;
	static const char *access_user;
	static const char *access_pass;
	static const char *access_host;
	static bool exec_funcs;
	static const char *exec_extensions;
	static const char *exec_token;
	static const char *exec_prefix;
	static const char *exit_token;
	static const char *apps_extensions;
	static const char *apps_prefix;

	static const char *etc_prefix;
	static const char *var_prefix;
	static const char *log_prefix;

	static void addFunction(const char *name, unsigned count, Function i);
	static void addConditional(const char *name, Cond test);

	static bool isPrivate(Name *scr);
	static bool isFunction(Name *scr);

protected:
	static Test *test;
	static Fun *ifun;

};

class __EXPORT ScriptSymbols : public MemPager, public Script
{
protected:
	Symbol *index[SCRIPT_INDEX_SIZE + 1];

	void purge(void);

public:
	ScriptSymbols();
	~ScriptSymbols();

	inline const char *cstring(const char *str)
		{return MemPager::alloc(str);};

	unsigned gathertype(Symbol **idx, unsigned max, const char *prefix, symType group);
	unsigned gather(Symbol **idx, unsigned max, const char *prefix, const char *suffix);
	Symbol *find(const char *id, unsigned short size = 0);
	Symbol *make(const char *id, unsigned short size);

	Symbol *setReference(const char *id, Symbol *target);
};

/**
 * This class holds the bound keyword set for a given Bayonne style
 * script interpreter.  Application specific dialects are created
 * by deriving a application specific version of ScriptCommand which
 * then binds application specific keywords and associated methods
 * in an application derived ScriptInterp which are typecast to
 * (scriptmethod_t).
 *
 * @author David Sugar <dyfet@ostel.com>
 * @short Bayonne script keyword binding tables and compiler constants.
 */
class __EXPORT ScriptCommand : public Keydata, public Mutex, public Script
{
private:
	friend class __EXPORT ScriptImage;
	friend class __EXPORT ScriptInterp;
	friend class __EXPORT ScriptCompiler;
	friend class __EXPORT ScriptBinder;

#ifdef	CCXX_PACKED
#pragma pack(1)
#endif

	typedef struct _keyword {
		struct _keyword *next;
		Method method;
		Check check;
		bool init : 1;
		char keyword[1];
	}	Keyword;

#ifdef	CCXX_PACKED
#pragma pack()
#endif

	ThreadQueue *tq;
	Keyword *keywords[SCRIPT_INDEX_SIZE];
	char *traps[TRAP_BITS];
	ScriptImage *active;
	unsigned keyword_count;
	unsigned trap_count;
	unsigned long imask;
	unsigned dbcount;
	void *dbc;

protected:
	bool ripple;
	unsigned activity;	// activity counter
	static ScriptCommand *runtime;

	virtual const char *getExternal(const char *opt);

public:
	/**
	 * Checks if the line statement is an input statement.  Used in
	 * some servers...
	 *
	 * @return true if line is input.
	 * @param line to examine.
	 */
	virtual bool isInput(Line *line);

	/**
	 * Get the method handler associated with a given keyword.  This
	 * is used by ScriptImage when compiling.
	 *
	 * @param keyword to search for.
	 * @return method handler to execute for this keyword.
	 */
	Method getHandler(const char *keyword);

	/**
	 * Issue a control event against current image for attached
	 * modules until claimed.
	 *
	 * @param args list of control command and arguments.
	 * @return true if processed.
	 */
	bool control(char **args);

	/**
	 * Get the active script.
	 *
	 * @return pointer to active script image.
	 */
	inline ScriptImage *getActive(void)
		{return active;};

	/**
	 * Get the name of a trap from it's id.
	 *
	 * @param id of trap.
	 * @return name of trap.
	 */
	const char *getTrapName(unsigned id);

	/**
	 * Alias use modules...
	 *
	 * @param id to alias.
	 * @param id to use.
	 */
	void aliasModule(const char *id, const char *use);

protected:
	/**
	 * Fetch whether the given keyword is valid for constructor.
	 *
	 * @param keyword to search for.
	 * @return init flag.
	 */
	bool isInitial(const char *keyword);

	/**
	 * Check keyword syntax.
	 *
	 * @return syntax error string or NULL.
	 * @param command name of keyword to check.
	 * @param line pointer to line being compiled.
	 * @param img pointer to image being compiled.
	 */
	const char *check(char *command, Line *line, ScriptImage *img);

	/**
	 * Get the trap id number associated with a trap name.
	 *
	 * @return trap id number, 0 (exit) if invalid.
	 * @param name of trap identifier.
	 */
	virtual unsigned getTrapId(const char *name);

	/**
	 * Get the mask bits for the default script.
	 *
	 * @return trap mask to use.
	 */
	virtual unsigned long getTrapDefault(void);

	/**
	 * Get the mask bits for a trap "handler".
	 *
	 * @return script object of trap mask to use.
	 */
	virtual unsigned long getTrapHandler(Name *script);

	/**
	 * Get a trap mask for a given identifer.  This is a virtual
	 * since some derived handlers may manipulate mask bits.
	 *
	 * @return signal bit mask based on id number.
	 * @param id number of trap mask.
	 */
	virtual unsigned long getTrapMask(unsigned id);

	/**
	 * A helper method for the compiler.  Converts a named
	 * trap into it's bit shifted mask.  By making it a virtual,
	 * derived dialects can add "aliases" to default trap names.
	 *
	 * @param name of trap identifier.
	 * @return bit shifted mask or 0 if invalid.
	 */
	virtual unsigned long getTrapModifier(const char *name);

	/**
	 * A helper method for the compiler used specifically for
	 * "^" trap subsection requests.  These will occasionally
	 * carry different attribute settings.
	 *
	 * @param name of trap identifier.
	 * @return bit shifted mask or 0 if invalid.
	 */
	virtual unsigned long getTrapMask(const char *name);

	/**
	 * Test current command to see if it uses keyword syntax.
	 *
	 * @return true if keyword syntax used.
	 * @param line record to examine in check routine.
	 */
	static bool hasKeywords(Line *line);

public:
	/**
	 * Test for a specific keyword.
	 *
	 * @return content of keyword that is found.
	 * @param line pointer to record to examine in check routine.
	 * @param keyword to search for.
	 */
	static const char *findKeyword(Line *line, const char *keyword);

	/**
	 * Test for a specific keyword or keydata filler.
	 *
	 * @return content of keyword that is found.
	 * @param script image for keydata.
	 * @param line pointer to record to examine in check routine.
	 * @param keyword to search for.
	 */
	static const char *findKeyword(ScriptImage *img, Line *line, const char *keyword);

	/**
	 * Server level logging interface override.
	 *
	 * @param level of log message.
	 * @param text of message.
	 */
	virtual void errlog(const char *level, const char *text);

public:
	/**
	 * Test current command against a list of valid keywords.
	 *
	 * @return first keyword found not in list.
	 * @param line record to examine in check routine.
	 * @param list of =xxx keyword entries.
	 */
	static bool useKeywords(Line *line, const char *list);

	/**
	 * Count non-keyword arguments.
	 *
	 * @return number of non-keyword arguments.
	 * @param line record to examine.
	 */
	static unsigned getCount(Line *line);

	/**
	 * Get the member id code of a line.
	 *
	 * @return member id code.
	 * @param line record to examine in check routine.
	 */
	static const char *getMember(Line *line);


protected:
	/**
	 * Check the member list.
	 *
	 * @return true if member found or none.
	 * @param line record to examine in check routine.
	 * @param list of .members...
	 */
	static bool useMember(Line *line, const char *list);

	/**
	 * Get an option to examine in check routine.
	 *
	 * @return option or NULL if past end of line record.
	 * @param line record pointer to line to examine.
	 * @param index pointer to index value.  Start at 0.
	 */
	static const char *getOption(Line *line, unsigned *index);

	/**
	 * Load a set of keywords into the system keyword table.  This
	 * provides a convenient method of initializing and adding to
	 * the keyword indexes.
	 *
	 * @param keywords defined pair entries to load.
	 */
	void load(Script::Define *keywords);

	/**
	 * Add a trap handler symbolic identity to the keyword table.
	 * These are used to handle signal mask coercion and event
	 * branch points in the compiler.
	 *
	 * @param name of requested trap to add to the trap table.
	 * @param inherited status of trap.
	 * @return assigned id number for the trap.
	 */
	int trap(const char *name, bool inherited = true);

	/**
	 * Get count of active traps.
	 *
	 * @return count of active trap identifiers.
	 */
	inline unsigned getCount(void)
		{return trap_count;};

	/**
	 * Return true if the trap id is inherited.
	 */
	bool isInherited(unsigned id);

	/**
	 * Perform compile time check of a specified symbol.
	 *
	 * @return syntax error message string.
	 * @param chk object pointer to check member function.
	 * @param line pointer to line being checked.
	 * @param img pointer to image being compiled.
	 */
	virtual const char *check(Check chk, Line *line, ScriptImage *img);

public:
	/**
	 * Create an initialized script command box.
	 */
	ScriptCommand();

	/**
	 * Create a ScriptCommand box initialized from another.
	 */
	ScriptCommand(ScriptCommand *ini);

	/**
	 * Get activity counter.
	 *
	 * @return activity counter.
	 */
	inline unsigned getActivity(void)
		{return activity;};

};

class __EXPORT ScriptBinder : public Script
{
private:
	friend class __EXPORT ScriptInterp;
	friend class __EXPORT ScriptCompiler;
	friend class __EXPORT ScriptCommand;

	static ScriptBinder *first;
	ScriptBinder *next;
	const char *id;

protected:
	void bind(Script::Define *extensions);

	virtual void attach(ScriptInterp *interp);
	virtual void detach(ScriptInterp *interp);
	virtual bool select(ScriptInterp *interp);
	virtual bool reload(ScriptCompiler *img);
	virtual bool control(ScriptImage *img, char **args);
	virtual void down(void);
	virtual const char *use(Line *line, ScriptImage *img);

public:
	ScriptBinder(const char *id = NULL);
	ScriptBinder(Script::Define *extensions);
	virtual ~ScriptBinder();

	static const char *check(Line *line, ScriptImage *img);
	static void shutdown(void);
	static bool rebuild(ScriptCompiler *img);
};

class __EXPORT ScriptRuntime : public ScriptCommand
{
public:
	ScriptRuntime();
};

class __EXPORT ScriptRipple : public ScriptCommand
{
public:
	ScriptRipple();
};

class __EXPORT ScriptChecks : public ScriptCommand
{
public:
	/**
	 * Default compiler syntax to accept any syntax.
	 */
	const char *chkIgnore(Line *line, ScriptImage *img);

	/**
	 * Performs DSO load phase for USE modules.
	 */
	const char *chkUse(Line *line, ScriptImage *img);

	/**
	 * A check used by "inc" and "dec".
	 *
	 * @return synxtax error message string or NULL.
	 * @param line statement being examined.
	 * @param img pointer to image being examined.
	 */
	const char *chkHasModify(Line *line, ScriptImage *img);

	/**
	 * Check if member is NULL or a property reference.
	 *
	 * @param line pointer to line checked for property reference.
	 * @param img pointer to image being compiled.
	 * @return syntax error message string or NULL.
	 */
	const char *chkProperty(Line *line, ScriptImage *img);

	/**
	 * A check for first var...
	 *
	 * @return syntax error message string or NULL.
	 * @param line statement being examined.
	 * @param img pointer to image being compiled.
	 */
	const char *chkFirstVar(Line *line, ScriptImage *img);

	/**
	 * A basic type check for simple type declarations...
	 *
	 * @return syntax error message string or NULL.
	 * @param line statement being examined.
	 * @param img pointer to image being compiled.
	 */
	const char *chkType(Line *line, ScriptImage *img);

	/**
	 * Script compiler syntax check for certain variable using
	 * statements such as "clear".  Assumes list of valid variable
	 * arguments.
	 *
	 * @return syntax error message string or NULL.
	 * @param line statement being examined.
	 * @param img pointer to image being compiled.
	 */
	const char *chkHasVars(Line *line, ScriptImage *img);

	/**
	 * Script compiler syntax check for assignment statements
	 * such as "set", "for", etc.
	 *
	 * @return syntax error message string or NULL.
	 * @param line statement being examined.
	 * @param img pointer to image being compiled.
	 */
	const char *chkHasList(Line *line, ScriptImage *img);

	/**
	 * Script compiler syntax check for commands that require
	 * no arguments to be present.
	 *
	 * @return syntax error message string or NULL.
	 * @param line statement being examined.
	 * @param img pointer to image being compiled.
	 */
	const char *chkNoArgs(Line *line, ScriptImage *img);

	/**
	 * Script compiler syntax check for commands that require
	 * all arguments to be symbols.
	 *
	 * @return syntax error message string or NULL.
	 * @param line pointer being examined.
	 * @param img pointer to image being compiled.
	 */
	const char *chkAllVars(Line *line, ScriptImage *img);

	/**
	 * Script compiler syntax check for commands that require
	 * one or more arguments to be present.
	 *
	 * @return syntax error message string or NULL.
	 * @param line statement being examined.
	 * @param img pointer to image being compiled.
	 */
	const char *chkHasArgs(Line *line, ScriptImage *img);

	/**
	 * Script compiler syntax check for commands that require
	 * one or more arguments but use no keywords.
	 *
	 * @return syntax error message string or NULL.
	 * @param line statement being examined.
	 * @param img pointer to image being compiled.
	 */
	const char *chkOnlyArgs(Line *line, ScriptImage *img);

	const char *chkOnlyOneArg(Line *line, ScriptImage *img);

	const char *chkRefArgs(Line *line, ScriptImage *img);

	const char *chkSlog(Line *line, ScriptImage *img);

	const char *chkExpression(Line *line, ScriptImage *img);

	const char *chkConditional(Line *line, ScriptImage *img);

	const char *chkGoto(Line *line, ScriptImage *img);

	const char *chkLabel(Line *line, ScriptImage *img);

	const char *chkCall(Line *line, ScriptImage *img);

	const char *chkReturn(Line *line, ScriptImage *img);

	const char *chkRestart(Line *line, ScriptImage *img);

	const char *chkVar(Line *line, ScriptImage *img);

	const char *chkVarType(Line *line, ScriptImage *img);

	const char *chkDecimal(Line *line, ScriptImage *img);

	const char *chkNumber(Line *line, ScriptImage *img);

	const char *chkString(Line *line, ScriptImage *img);

	const char *chkChar(Line *line, ScriptImage *img);

	const char *chkExpr(Line *line, ScriptImage *img);

	const char *chkIndex(Line *line, ScriptImage *img);

	const char *chkError(Line *line, ScriptImage *img);

	const char *chkConst(Line *line, ScriptImage *img);

	const char *chkSequence(Line *line, ScriptImage *img);

	const char *chkSignal(Line *line, ScriptImage *img);

	const char *chkThrow(Line *line, ScriptImage *img);

	const char *chkSet(Line *line, ScriptImage *img);

	const char *chkRepeat(Line *line, ScriptImage *img);

	const char *chkArray(Line *line, ScriptImage *img);

	const char *chkFor(Line *line, ScriptImage *img);

	const char *chkForeach(Line *line, ScriptImage *img);

	const char *chkCat(Line *line, ScriptImage *img);

	const char *chkRemove(Line *line, ScriptImage *img);

	const char *chkOnlyCommand(Line *line, ScriptImage *img);

	const char *chkCounter(Line *line, ScriptImage *img);

	const char *chkTimer(Line *line, ScriptImage *img);

	const char *chkClear(Line *line, ScriptImage *img);

	const char *chkPack(Line *line, ScriptImage *img);

	const char *chkConstruct(Line *line, ScriptImage *img);

	const char *chkLock(Line *line, ScriptImage *img);

	const char *chkSession(Line *line, ScriptImage *img);

	const char *chkKeywords(Line *line, ScriptImage *img);

	const char *chkDefine(Line *line, ScriptImage *img);
};

/**
 * A linkable list of objects that can be destroyed when a script image
 * is removed from memory.
 *
 * @author David Sugar <dyfet@ostel.com>
 * @short Object list in image.
 */
class __EXPORT ScriptObject : public Script
{
private:
	friend class __EXPORT ScriptImage;
	ScriptObject *next;

protected:
	ScriptObject(ScriptImage *image);
	virtual ~ScriptObject();
};

/**
 * A derivable class to hold compiled script images for active processes.
 * This includes the script image compiler itself.  Typically, a script is
 * compiled one file at a time from a directory, and the committed, during
 * the constructor in a derived class.
 *
 * @author David Sugar <dyfet@ostel.com>.
 * @short Script compiler image set.
 */
class __EXPORT ScriptImage : public Keydata, public Script, public Assoc
{
protected:
	friend class __EXPORT ScriptObject;

	ScriptCommand *cmds;
	unsigned refcount;
	Name *index[SCRIPT_INDEX_SIZE + 1], *current;
	Line *select, *selecting, *registration;
	Line *advertising[SCRIPT_ROUTE_SLOTS];
	Mutex duplock;
	ScriptObject *objects;
	static unsigned long serial;
	unsigned long instance;

	class	__EXPORT InitialList : public Script::Initial
	{
	public:
		InitialList *next;
	}	*ilist;

	friend class __EXPORT ScriptInterp;

	/**
	 * Get the interpreter method pointer for a given keyword.
	 *
	 * @return method handler.
	 * @param keyword to search.
	 */
	Method getHandler(const char *keyword)
		{return cmds->getHandler(keyword);};

public:
	/**
	 * Get memory for assoc data...
	 *
	 * @return memory pointer.
	 * @param size of memory request.
	 */
	void *getMemory(size_t size);

	/**
	 * Duplicate string...
	 *
	 * @return memory pointer.
	 * @param string requested.
	 */
	const char *dupString(const char *str);

	/**
	 * Fast branch linkback code.
	 */
	virtual void fastBranch(ScriptInterp *interp);

	/**
	 * Get current entity being compiled...
	 *
	 * @return pointer to script currently compiling.
	 */
	inline Name *getCurrent(void)
		{return current;};

	/**
	 * Add a select record to the currently compiled script.
	 *
	 * @param line statement to add to list.
	 */
	void addSelect(Line *line);

	/**
	 * Add a registration record to the compiled script.
	 *
	 * @param line statement to add to list.
	 */
	void addRegistration(Line *line);

	/**
	 * Get a registration record to use.
	 *
	 * @return registration object.
	 */
	ScriptRegistry *getRegistry(void);

	/**
	 * Add an advertised route in a priority slot.
	 *
	 * @param line statement to add to list.
	 * @param pri of route to add to.
	 */
	void addRoute(Line *line, unsigned pri);

	/**
	 * Get the selection list from the image.
	 *
	 * @return selection list.
	 */
	inline Line *getSelect(void)
		{return select;};

	/**
	 * Get the registration list from the image.
	 *
	 * @return registration list.
	 */
	inline Line *getRegistration(void)
		{return registration;};

	/**
	 * Get an advertised priority record from the image.
	 *
	 * @return priority list.
	 */
	inline Line *getRoute(unsigned pri)
		{return advertising[pri];};

	/**
	 * Get the session instance of the image.
	 */
	inline unsigned long getInstance(void)
		{return instance;};

	/**
	 * Construct a new working image.  This must be derived to an
	 * application specific compiler that can scan directories and
	 * invoke the compiler as needed.
	 *
	 * @param cmdset of keyword table object used.
	 * @param symset of predefined symbols being used.
	 */
	ScriptImage(ScriptCommand *cmdset, const char *symset);

	/**
	 * Destruct the ScriptImage itself by removing linked objects.
	 */
	~ScriptImage();

	/**
	 * Purge and reload the script image workspace.
	 */
	void purge(void);

	/**
	 * Used in the derived constructor to "commit" the current image
	 * for new processes.  This is usually the last statement in the
	 * derived constructor.
	 */
	void commit(void);

	/**
	 * Used by a derived constructor to load an initialization list.
	 *
	 * @param ilist initialization list of symbol pairs to load.
	 */
	void load(Initial *ilist);

	/**
	 * Used to load a single initialization list entry.
	 *
	 * @param keyword name to initialize.
	 * @param value of keyword.
	 * @param size of keyword data field.
	 */
	void initial(const char *keyword, const char *value, unsigned size = 0);

	/**
	 * Fetch named script.
	 *
	 * @param name of script to find.
	 * @return script or NULL.
	 */
	virtual Name *getScript(const char *name);

	/**
	 * Get the command object associated with the image.
	 *
	 * @return command object.
	 */
	inline ScriptCommand *getCommand(void)
		{return cmds;};

	/**
	 * Get the ripple flag for the current image.
	 *
	 * @return true if a ripple image.
	 */
	bool isRipple(void)
		{return cmds->ripple;};

	/**
	 * Fetch list of relational scripts.
	 *
	 * @param suffix extension of scripts being fetched.
	 * @param array of script objects gathered from image.
	 * @param size limit of array to gather.
	 * @return count of entries found.
	 */
	unsigned gather(const char *suffix, Name **array, unsigned size);

	/**
	 * inc the reference count.
	 */
	inline void incRef(void)
		{++refcount;};

	/**
	 * dec the reference count.
	 */
	inline void decRef(void)
		{--refcount;};

	/**
	 * See if the image is referenced...
	 *
	 * @return true if is referenced.
	 */
	inline bool isRef(void)
		{return (bool)(refcount > 0);};

	/**
	 * Get the active image from command.  Useful when compiling.
	 *
	 * @return ScriptImage of currently active image.
	 */
	inline ScriptImage *getActive(void)
		{return cmds->getActive();};
};

/**
 * A derivable class to hold compiled script images for active processes.
 * This includes the script image compiler itself.  Typically, a script is
 * compiled one file at a time from a directory, and the committed, during
 * the constructor in a derived class.
 *
 * @author David Sugar <dyfet@ostel.com>.
 * @short Script compiler image set.
 */
class __EXPORT ScriptCompiler : public ScriptImage
{
protected:
	std::ifstream scrSource;
	std::istream *scrStream;
	char *buffer;
	unsigned bufsize;
	char *bp;
	bool quote;
	unsigned paren;
	unsigned inccount;
	const char *incfiles[256];

	typedef struct _merge {
		struct _merge *next;
		Name *target;
		const char *source;
		const char *prefix;
	}	merge_t;

	merge_t *mlist;

	friend class __EXPORT ScriptInterp;

	virtual bool checkSegment(Name *scr);

public:
	char *getToken(char **pre = NULL);

	/**
	 * Fast branch linkback code.
	 */
	virtual void fastBranch(ScriptInterp *interp);

	/**
	 * Construct a new working image.  This must be derived to an
	 * application specific compiler that can scan directories and
	 * invoke the compiler as needed.
	 *
	 * @param cmdset of keyword table object used.
	 * @param symset of symbols to set.
	 */
	ScriptCompiler(ScriptCommand *cmdset, const char *symset);

	/**
	 * A method to invoke the script compiler to include a script
	 * only if it has not been included already.
	 *
	 * @return named script object.
	 * @param name of script file to compile.
	 */
	Name *include(const char *name);

	/**
	 * The script compiler itself.  This linearly compiles a Bayonne
	 * script file that is specified.  Normally used along with a dir
	 * scanner in the constructor.
	 *
	 * @return lines of script compiled.
	 * @param file name of script file to compile.
	 */
	int compile(const char *file);

	/**
	 * Compile a script from disk and give it a different internal
	 * "name" as passed.
	 *
	 * @return lines of script compiled.
	 * @param file name of script file to compile.
	 * @param name of script to compile under.
	 */
	int compile(const char *file, char *name);

	/**
	 * Compile a defintions library, commonly used for remapping
	 * tokens to macros.
	 *
	 * @return lines of script compiled.
	 * @param file name of defintions file to compile.
	 */
	int compileDefinitions(const char *file);

	/**
	 * Compile an open stream object into a script.
	 *
	 * @return lines of script compiled.
	 * @param stream object to use.
	 * @param name of script save under.
	 * @param file name to use in object.
	 */
	int compile(std::istream *stream, char *name, const char *file = NULL);

	/**
	 * Used in the derived constructor to "commit" the current image
	 * for new processes.  This is usually the last statement in the
	 * derived constructor.
	 */
	void commit(void);

	/**
	 * Used to process '$const' inserts.
	 *
	 * @return string if found.
	 * @param token string being substituted.
	 */
	virtual const char *getDefined(const char *token);

	/**
	 * Check for special preprocessor token.
	 *
	 * @return error message or NULL if no error.
	 * @param token name of keyword to check.
	 */
	const char *preproc(const char *token);

	/**
	 * Used by embedded interpreters to fetch script from the current
	 * source file.
	 *
	 * @return reference to source file stream.
	 */
	inline std::istream *getSource(void)
		{return (std::istream *)&scrSource;};
};

class __EXPORT ScriptInterp : public Mutex, public ScriptSymbols
{
protected:
	friend class __EXPORT ScriptThread;
	friend class __EXPORT ScriptCommand;
	friend class __EXPORT ScriptBinder;

public:
	class __EXPORT Frame
	{
	public:
		Name *script;
		Line *line, *first;
		unsigned short index;
		ScriptSymbols *local;
		unsigned long mask;
		bool caseflag : 1;
		bool tranflag : 1;
		bool unused1 : 1;
		bool unused2 : 1;
		unsigned decimal : 4;
		unsigned base : 8;
	};

	static long getRealValue(double val, unsigned prec);
	static double getDouble(long value, unsigned prec);
	static long getInteger(long value, unsigned prec);
	static long getTens(unsigned prec);
	long getIntValue(const char *text, unsigned prec, ScriptProperty *property = NULL);
	int numericExpression(long *list, int max, unsigned prec, ScriptProperty *property = NULL);
	bool conditionalExpression(void);
	bool conditional(void);

protected:
	Mutex *lock;	// any additional lock that is siezed
	ScriptCommand *cmd;
	ScriptImage *image;
	ScriptInterp *session;
	ScriptThread *thread;
	Frame frame[SCRIPT_STACK_SIZE];
	char *temps[SCRIPT_TEMP_SPACE];
	unsigned tempidx;
	unsigned stack;
	bool initialized, trace, exiting, updated;
	unsigned long sequence;
	char logname[32];

public:
	virtual unsigned getId(void);

	inline unsigned long getSequence(void)
		{return sequence;};

	virtual const char *getLogname(void)
		{return logname;};

	virtual ScriptInterp *getInterp(const char *id);

	virtual const char *getExternal(const char *opt);

	inline ScriptImage *getImage(void)
		{return image;};

protected:
	virtual ScriptSymbols *getSymbols(const char *id);

	ScriptSymbols *getLocal(void);

public:
	const char *getMember(void);
	const char *getKeyword(const char *kw);
	const char *getKeyoption(const char *kw);
	const char *getValue(const char *def = NULL);
	const char *getOption(const char *def = NULL);
	const char *hasOption(void);
	const char *getContent(const char *opt);
	const char *getSymContent(const char *opt);
	Symbol *getKeysymbol(const char *kw, unsigned size = 0);
	Symbol *getSymbol(unsigned short size = 0);
	char getPackToken(void);

protected:
	/**
	 * Initialize execution environment for a script.
	 */
	void initRuntime(Name *name);

	/**
	 * New virtual to initialize script environment syms
	 * before running init sections.
	 */
	virtual void initialize(void);

public:
	inline Frame *getFrame(void)
		{return &frame[stack];};

	inline Line *getLine(void)
		{return frame[stack].line;};

	void setFrame(void);

	inline Name *getName(void)
		{return frame[stack].script;};

	inline bool getTrace(void)
		{return trace;};

	/**
	 * Runtime execution of script handler.  This can be called in
	 * the current or derived class to invoke extensible methods.
	 *
	 * @return true if immediately ready for next step.
	 * @param method derived method member to call.
	 */
	bool execute(Method method);

protected:
	/**
	 * Attempt to push a value onto the stack.
	 *
	 * @return false if stack overflow.
	 */
	bool push(void);

	/**
	 * Attempt to recall a previous stack level.
	 *
	 * @return false if stack underflow.
	 */
	bool pull(void);

	/**
	 * Clear the stack of local loops or recursion for branching.
	 */
	void clearStack(void);

	/**
	 * Advance program to the next script statement.
	 */
	void advance(void);

	/**
	 * Skip line without checking or setting updates.
	 */
	void skip(void);

	/**
	 * Set error variable and advance to either the error handler
	 * or next script statement.
	 *
	 * @param error message.
	 */
	void error(const char *error);

	/**
	 * Events reference to named \@event handlers which have been
	 * attached to a script.  This allows low level applications
	 * to invoke an event handler much the way a signal handler
	 * occurs.
	 *
	 * @return true if event handler exists.
	 * @param name of event handler.
	 * @param inhereted search flag.
	 */
	bool scriptEvent(const char *name, bool inhereted = true);

	/**
	 * Branch to a selected event record immediately.
	 *
	 * @param event record pointer to access.
	 */
	void gotoEvent(NamedEvent *event);

	/**
	 * Set the execution interpreter to a trap identifier.  If no
	 * trap id exists, then advance to next script statement (unless
	 * exit trap).
	 *
	 * @param id of trap to select numerically.
	 */
	void trap(unsigned id);

	/**
	 * Tries a catch handler...
	 *
	 * @return true if caught.
	 * @param id of catch handler to try.
	 */
	bool tryCatch(const char *id);

	/**
	 * Select trap by symbolic name and execute if found, else advance
	 * to next script step (unless exit trap).
	 *
	 * @param name of trap to select.
	 */
	void trap(const char *name);

public:
	virtual void logmissing(const char *id, const char *level = "undefined", const char *group = "symbol");
	virtual void logerror(const char *msg, const char *name = NULL);

	Symbol *mapSymbol(const char *id, unsigned short = 0);
	Symbol *mapDirect(const char *id, unsigned short = 0);

protected:
	virtual bool isLocked(const char *id);
	virtual const char *remapLocal(void);
	virtual bool exit(void);
	virtual void enterThread(ScriptThread *thread);
	virtual void exitThread(const char *msg);
	virtual void waitThread(void);
	virtual void startThread(void);

	bool eventThread(const char *evt, bool flag = true);

	bool redirect(const char *scr);

	void ripple(void);

	bool redirect(bool evflag);

	unsigned long getMask(void);

public:
	bool setNumber(const char *id, const char *value = NULL, unsigned dec = 0);
	bool setSymbol(const char *id, const char *value = NULL, unsigned short size = 0);
	bool setConst(const char *id, const char *value);
	bool putSymbol(const char *id, const char *value, unsigned short size = 0);
	bool getSymbol(const char *id, char *buffer, unsigned short max);
	bool catSymbol(const char *id, const char *value, unsigned short size = 0);

	const char *getSymbol(const char *id);

	Name *getScript(const char *name);

	ScriptInterp();

	bool step(void);
	bool attach(ScriptCommand *cmd, const char *scrname);
	void detach(void);
	void attach(ScriptCommand *cmd, ScriptImage *img, Name *scr);

	/**
	 * Release any aquired lock...
	 */
	void release(void);

	   /**
	 * Signals are used during "delayed" execution steps when a
	 * signal event has occured aynchronously with the execution
	 * of a script controlled state event handler.  This mechanism
	 * can be used in place of calling implicit "Step" traps.
	 *
	 * @return true if signal handler is not blocked.
	 * @param name of signal identifier.
	 */
	bool signal(const char *name);

	/**
	 * Signals can be referenced by numeric id as well as by symbolic
	 * name.
	 *
	 * @return true if signal handler is not blocked.
	 * @param id number of handler.
	 */
	bool signal(unsigned id);

	bool done(void);

	timeout_t getTimeout(void);

	/**
	 * A virtual holding a branch conditional member.  This may be
	 * invoked typically from goto or restart.  Can be used to check
	 * contextual changes.
	 */
	virtual void branching(void);

	inline bool isRunning(void)
		{return (image != NULL) && initialized;};

	inline bool isExiting(void)
		{return exiting;};

	char *getTemp(void);

	unsigned getTempSize(void);
};

class __EXPORT ScriptMethods : public ScriptInterp
{
public:
	bool scrNop(void);
	bool scrError(void);
	bool scrExit(void);
	bool scrDecimal(void);
	bool scrDefine(void);
	bool scrVar(void);
	bool scrType(void);
	bool scrNumber(void);
	bool scrSlog(void);
	bool scrExpr(void);
	bool scrIndex(void);
	bool scrOffset(void);
	bool scrRef(void);
	bool scrRestart(void);
	bool scrInit(void);
	bool intGoto(void);
	bool scrGoto(void);
	bool scrCall(void);
	bool scrReturn(void);
	bool scrBegin(void);
	bool scrEnd(void);
	bool scrConst(void);
	bool scrSequence(void);
	bool scrSet(void);
	bool scrArray(void);
	bool scrClear(void);
	bool scrConstruct(void);
	bool scrDeconstruct(void);
	bool scrPack(void);
	bool scrUnpack(void);
	bool scrLock(void);
	bool scrSession(void);
	bool scrSignal(void);
	bool scrThrow(void);
	bool scrInvoke(void);
	bool scrCounter(void);
	bool scrTimer(void);
	bool scrCase(void);
	bool scrEndcase(void);
	bool scrRemove(void);
	bool scrDo(void);
	bool scrRepeat(void);
	bool scrFor(void);
	bool scrForeach(void);
	bool scrLoop(void);
	bool scrContinue(void);
	bool scrBreak(void);
	bool scrIf(void);
	bool scrIfThen(void);
	bool scrThen(void);
	bool scrElse(void);
	bool scrEndif(void);
};

class __EXPORT ScriptThread : public Thread, public Script
{
private:
	volatile bool exiting;
	size_t stacksize;

protected:
	friend class __EXPORT ScriptInterp;

	ScriptInterp *interp;

	void exit(const char *errmsg = NULL);

	void exitEvent(const char *evt, bool inherited = true);

	inline bool isExiting(void)
		{return exiting;};

	inline bool putSymbol(const char *id, const char *value, unsigned short size = 0)
		{return interp->putSymbol(id, value, size);};

	inline bool getSymbol(const char *id, char *buffer, unsigned short max)
		{return interp->getSymbol(id, buffer, max);};

	inline bool addSymbol(const char *id, char *buffer, unsigned short max)
		{return interp->getSymbol(id, buffer, max);};

	void block(void);

	void unblock(void);

	void lock(void);

	void release(void);

public:
	virtual timeout_t getTimeout(void);

	inline size_t getStack(void)
		{return stacksize;};

	ScriptThread(ScriptInterp *interp, int pri = 0, size_t stack = 0);
	~ScriptThread();
};

/**
 * This class is used for registering scripts with an external
 * registry.  Sometimes this is used as a base class for a more
 * complete one.
 *
 * @author David Sugar <dyfet@ostel.com>
 * @short Registry for script objects.
 */
class __EXPORT ScriptRegistry : public Script, public TimerPort
{
public:
	const char *protocol;
	timeout_t duration;
	Name *scr;		// script being registered
	Line *line;		// line of registry statement
};

/**
 * This class is used for DSO modules that impliment property
 * extensions for scripting objects.
 *
 * @author David Sugar <dyfet@ostel.com>
 * @short ccScript property module
 */
class __EXPORT ScriptProperty : public Script
{
private:
	friend class ScriptInterp;

	static ScriptProperty *first;
	ScriptProperty *next;
	const char *id;

public:
	/**
	 * Set property method.  Performs set.xxx and init.xxx methods.
	 *
	 * @param data buffer to work from.
	 * @param temp workspace buffer to use.
	 * @param size of temp area.
	 */
	virtual void set(const char *data, char *temp, unsigned size) = 0;

	/**
	 * Precision for property type expressions.
	 *
	 * @return precision.
	 */
	virtual unsigned prec(void);

	/**
	 * Set property from integer value.
	 *
	 * @param data to save.
	 * @param size of data.
	 * @param value being set.
	 */
	virtual void setValue(char *data, unsigned short size, long value);

	/**
	 * See if should be computed as property.
	 *
	 * @return true if property valid.
	 * @param data string to test.
	 */
	virtual bool isProperty(const char *data);

	/**
	 * Initialize a new property through var definition.
	 *
	 * @return property value.
	 * @param data location to clear.
	 * @param size of workspace to clear.
	 */
	virtual void clear(char *data, unsigned size = 0);

	/**
	 * Fetch a property specific seperator token.
	 *
	 * @return seperator token used in foreach loops...
	 */
	virtual char token(void);

	/**
	 * adjust value method.  Performs inc.xxx conversions.
	 *
	 * @param data buffer to work from.
	 * @param size of data buffer.
	 * @param adjustment offset to apply.
	 */
	virtual void adjust(char *data, size_t size, long adjustment);

	/**
	 * normalize values for scope and range.
	 *
	 * @return noramized value.
	 * @param value prior to normalization.
	 */
	virtual long adjustValue(long value);

	/**
	 * Get the "numeric" (or #var) value of this property symbol.
	 *
	 * @return numeric value of this property object.
	 * @param data being examined from property object.
	 */
	virtual long getValue(const char *data);

	ScriptProperty(const char *name);
	virtual ~ScriptProperty();

	static ScriptProperty *find(const char *name);
};

}

#endif

