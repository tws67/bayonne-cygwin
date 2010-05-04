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

ScriptCommand *ScriptCommand::runtime = NULL;
ScriptBinder *ScriptBinder::first = NULL;

ScriptBinder::ScriptBinder(Script::Define *extensions)
{
	bind(extensions);
}

ScriptBinder::ScriptBinder(const char *name)
{
	id = name;

	if(id) {
		if(!first)
			atexit(ScriptBinder::shutdown);

		next = first;
		first = this;
	}
}

ScriptBinder::~ScriptBinder()
{
}

void ScriptBinder::down(void)
{
}

bool ScriptBinder::select(ScriptInterp *interp)
{
	return false;
}

bool ScriptBinder::control(ScriptImage *img, char **args)
{
	return false;
}

bool ScriptBinder::reload(ScriptCompiler *img)
{
	return false;
}

bool ScriptBinder::rebuild(ScriptCompiler *img)
{
	ScriptBinder *node = ScriptBinder::first;
	bool rtn = false;

	while(node) {
		if(node->reload(img))
			rtn = true;
		node = node->next;
	}
	return rtn;
}

void ScriptBinder::shutdown(void)
{
	static bool flag = false;

	ScriptBinder *node = ScriptBinder::first;

	if(flag)
		return;

	flag = true;
	while(node) {
		node->down();
		node = node->next;
	}
	ScriptBinder::first = NULL;
}

const char *ScriptBinder::check(Line *line, ScriptImage *img)
{
	const char *id = strchr(line->cmd, '.') + 1;
	ScriptBinder *node = first;
	while(node) {
		if(!stricmp(id, node->id))
			break;

		node = node->next;
	}

	if(!node)
		return "";

	return node->use(line, img);
}

const char *ScriptBinder::use(Line *line, ScriptImage *img)
{
	return "";
}

void ScriptBinder::bind(Script::Define *extensions)
{
	if(ScriptCommand::runtime)
		ScriptCommand::runtime->load(extensions);
}

void ScriptBinder::attach(ScriptInterp *interp)
{
}

void ScriptBinder::detach(ScriptInterp *interp)
{
}

