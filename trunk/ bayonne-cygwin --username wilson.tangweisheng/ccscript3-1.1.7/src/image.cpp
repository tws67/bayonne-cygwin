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

#include "engine.h"

using namespace std;
using namespace ost;

unsigned long ScriptImage::serial = 0l;

ScriptObject::ScriptObject(ScriptImage *img)
{
	next = img->objects;
	img->objects = this;
}

ScriptObject::~ScriptObject()
{
}

ScriptImage::ScriptImage(ScriptCommand *cmd, const char *symset) :
Keydata(symset), Assoc()
{
	static Script::Initial initial[] = {
		{"script.index", 6 * SCRIPT_STACK_SIZE, ""},
		{"script.stack", 3, "-"},
		{"script.error", 64, "none"},
		{"script.home", 64, "none"},
		{"script.token", 1, ","},
		{"script.decimal", 1, "."},
		{"script.authorize", 23, ""},
		{NULL, 0, NULL}
	};


	cmds = cmd;
	memset(index, 0, sizeof(index));
	refcount = 0;
	ilist = NULL;
	objects = NULL;
	select = selecting = registration = NULL;
	memset(advertising, 0, sizeof(advertising));
	instance = ++serial;

	load(initial);
}

ScriptImage::~ScriptImage()
{
	ScriptObject *node = objects, *next;

	while(node) {
		next = node->next;
		delete objects;
		node = next;
	}
}

void *ScriptImage::getMemory(size_t size)
{
	return alloc(size);
}

const char *ScriptImage::dupString(const char *str)
{
	char *cp = (char *)alloc(strlen(str) + 1);
	strcpy(cp, str);
	return (const char *)cp;
}

void ScriptImage::fastBranch(ScriptInterp *interp)
{
	return;
}

void ScriptImage::addRegistration(Line *line)
{
	line->next = registration;
	line->scr.name = current;
	registration = line;
}

ScriptRegistry *ScriptImage::getRegistry(void)
{
	return (ScriptRegistry *)alloc(sizeof(ScriptRegistry));
}

void ScriptImage::addRoute(Line *line, unsigned pri)
{
	line->next = advertising[pri];
	line->scr.name = current;
	advertising[pri] = line;
}

void ScriptImage::addSelect(Line *line)
{
	line->next = NULL;
	if(selecting)
		selecting->next = line;
	else
		select = line;

	selecting = line;
	line->scr.name = current;

	if(!current->select)
		current->select = line;
}

void ScriptImage::initial(const char *keyword, const char *value, unsigned size)
{
	InitialList *init;

	if(!size)
		size = (unsigned)strlen(value);

	init = (InitialList *)alloc(sizeof(InitialList));
	init->name = alloc((char *)keyword);
	init->size = size;
	init->value = alloc((char *)value);
	init->next = ilist;
	ilist = init;
}

void ScriptImage::load(Initial *init)
{
	while(init->name) {
		initial(init->name, init->value, init->size);
		++init;
	}
}

void ScriptImage::purge(void)
{
	MemPager::purge();
	memset(index, 0, sizeof(index));
	refcount = 0;
}

void ScriptImage::commit(void)
{
	cmds->enterMutex();
	if(cmds->active) {
		if(!cmds->active->refcount)
			delete cmds->active;
	}
	cmds->active = this;
	cmds->leaveMutex();
}

unsigned ScriptImage::gather(const char *suffix, Name **array, unsigned max)
{
	unsigned count = 0;
	unsigned sort = 0;
	unsigned key = 0;
	Name *scr;
	const char *ext;
	int ins;

	while(count < max && key < SCRIPT_INDEX_SIZE) {
		scr = index[key];
		while(scr && count < max) {
			ext = strstr(scr->name, "::");
			if(!ext) {
				scr = scr->next;
				continue;
			}
			ext += 2;
			if(stricmp(ext, suffix)) {
				scr = scr->next;
				continue;
			}
			sort = 0;
			while(sort < count) {
				if(stricmp(scr->name, array[sort]->name) < 0)
					break;
				++sort;
			}
			ins = count;
			while(ins > (int)sort) {
				array[ins] = array[ins - 1];
				--ins;
			}
			array[sort] = scr;
			++count;
			scr = scr->next;
		}
		++key;
	}
	return count;
}

Script::Name *ScriptImage::getScript(const char *name)
{

	int key = Script::getIndex(name);
	Name *scr = index[key];

	while(scr) {
		if(!stricmp(scr->name, name))
			break;

		scr = scr->next;
	}
	return scr;
}
