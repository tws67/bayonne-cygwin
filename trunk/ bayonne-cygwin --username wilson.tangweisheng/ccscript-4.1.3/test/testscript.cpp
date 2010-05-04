// Copyright (C) 1995-1999 David Sugar, Tycho Softworks.
// Copyright (C) 1999-2005 Open Source Telecom Corp.
// Copyright (C) 2005-2008 David Sugar, Tycho Softworks.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <config.h>
#include <ccscript.h>

using namespace UCOMMON_NAMESPACE;

static unsigned checks = 0;
static unsigned exchecks = 0;
static unsigned refchecks = 0;
static unsigned mainchecks = 0;
static unsigned debugcount = 0;
static unsigned printcount = 0;
static unsigned eventchecks = 0;
static unsigned loopingchecks = 0;
static unsigned manipchecks = 0;

class debug
{
public:
	debug();

	const char *flag;

	virtual void print(void);
};

class testing : public debug, public script::interp
{
public:
	bool scrCheck(void);
	bool scrCheckExit(void);
	bool scrCheckMain(void);
	bool scrCheckEvent(void);
	bool scrCheckLoop(void);
	bool scrCheckRefs(void);
	bool scrArgs(void);
	bool scrSleep(void);
	bool scrCheckManip(void);
};

debug::debug()
{
	++debugcount;
	flag = NULL;
}

void debug::print(void)
{
	if(flag)
		printf("%s", flag);
	else
		++printcount;
}

bool testing::scrCheckManip(void)
{
	++manipchecks;
	skip();
	return true;
}

bool testing::scrCheckEvent(void)
{
	++eventchecks;
	skip();
	return true;
}

bool testing::scrCheckRefs(void)
{
	++refchecks;
	skip();
	return true;
}

bool testing::scrCheckMain(void)
{
	++mainchecks;
	debug::print();
	if(scriptEvent("event"))
		return false;
	skip();
	return true;
}

bool testing::scrCheckLoop(void)
{
	++loopingchecks;
	skip();
	return true;
}


bool testing::scrCheckExit(void)
{
	++exchecks;
	skip();
	return true;
}

bool testing::scrCheck(void)
{
	++checks;
	skip();
	return true;
}

bool testing::scrArgs(void)
{
	unsigned index = 0;
	script::line_t *line = stack[frame].line;

	while(index < line->argc) {
		const char *cp = getContent(line->argv[index]);
		printf(" ARG %d %s <%s>\n", index, line->argv[index], cp);
		++index;
	}
	skip();
	return false;
}

bool testing::scrSleep(void)
{
	Thread::sleep(500);
	skip();
	return true;
}

int main(int argc, char **argv)
{
	testing interp;
	unsigned errors;

	static script::keyword_t keywords[] = {
		{"check", (script::method_t)&testing::scrCheck, &script::checks::chkNop},
		{"check.exit", (script::method_t)&testing::scrCheckExit, &script::checks::chkNop},
		{"check.main", (script::method_t)&testing::scrCheckMain, &script::checks::chkNop},
		{"check.event", (script::method_t)&testing::scrCheckEvent, &script::checks::chkNop},
		{"check.loop", (script::method_t)&testing::scrCheckLoop, &script::checks::chkNop},
		{"check.refs", (script::method_t)&testing::scrCheckRefs, &script::checks::chkNop},
		{"check.manip", (script::method_t)&testing::scrCheckManip, &script::checks::chkNop},
		{"ignore", (script::method_t)&script::methods::scrNop, &script::checks::chkIgnore}, 
		{"args", (script::method_t)&testing::scrArgs, &script::checks::chkIgnore},
		{"sleep", (script::method_t)&testing::scrSleep, &script::checks::chkNop},
		{NULL}};

	const char *filename = "testscript.scr";

	if(argc > 3) {
		fprintf(stderr, "use: testscript [scrname]\n");
		exit(-1);
	}

	if(argc == 2)
		filename = argv[1];

	script::init();
	script::assign(keywords);

	script *image = script::compile(filename);

	if(!image) {
		fprintf(stderr, "*** failed to load %s\n", filename);
		exit(-1);
	}

	errors = image->getErrors();
	if(errors) {
		fprintf(stderr, "*** %d total errors in %s\n", errors, filename);
		linked_pointer<script::error> ep = image->getListing();
		while(is(ep)) {
			fprintf(stderr, "*** %s(%d): %s\n", image->getFilename(), ep->errline, ep->errmsg);
			ep.next();
		}
		delete image;
		exit(-1);
	}	

	interp.initialize();

	if(!interp.attach(image)) {
		fprintf(stderr, "*** no main section in %s\n", filename);
		exit(-1);
	} 
	while(interp.step()) {
		Thread::yield();
	}
	interp.detach();

	if(argc < 2) {
		// enter correct generic number of "checks"
		assert(checks == 3);

		// enter validation of defined call by ref
		assert(refchecks == 1);

		// @exit handler called, and only once...
		assert(exchecks == 1);

		// @main handler called
		assert(mainchecks == 1);

		// @main ^event handler called
		assert(eventchecks == 1);

		// check looping operation
		assert(loopingchecks == 3);

		// in multiple inheritence base class initialized successfully
		assert(debugcount == 1);

		// in  multiple inheritence, method call this* has combined class
		assert(printcount == 1);

		// check manipulations
		assert(manipchecks == 8);
	}

	exit(0);
}

