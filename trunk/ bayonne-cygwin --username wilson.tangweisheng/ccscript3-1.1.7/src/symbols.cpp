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

ScriptSymbols::ScriptSymbols() :
MemPager(Script::pagesize)
{
	memset(index, 0, sizeof(index));
}

ScriptSymbols::~ScriptSymbols()
{
	Symbol *sym = index[SCRIPT_INDEX_SIZE];
	Symbol *next;

	while(sym) {
		next = sym->next;
		delete[] sym;
		sym = next;
	}
}

void ScriptSymbols::purge(void)
{
	MemPager::purge();
	memset(index, 0, sizeof(index));
}

unsigned ScriptSymbols::gather(Symbol **idx, unsigned max, const char *prefix, const char *suffix)
{
	const char *ext;
	unsigned key = 0;
	unsigned count = 0;
	unsigned pointer, marker;
	Symbol *node;

	while(max && key <= SCRIPT_INDEX_SIZE) {
		node = index[key++];
		while(node && max) {
			if(strnicmp(node->id, prefix, strlen(prefix))) {
				node = node->next;
				continue;
			}

			if(suffix) {
				ext = strrchr(node->id, '.');
				if(!ext) {
					node = node->next;
					continue;
				}
				if(stricmp(++ext, suffix)) {
					node = node->next;
					continue;
				}
			}

			pointer = 0;
			while(pointer < count) {
				if(stricmp(node->id, idx[pointer]->id) < 0)
					break;
				++pointer;
			}
			marker = pointer;
			pointer = count;
			while(pointer > marker) {
				idx[pointer] = idx[pointer - 1];
				--pointer;
			}
			idx[marker] = deref(node);
			--max;
			++count;
			node = node->next;
		}
	}
	return count;
}

unsigned ScriptSymbols::gathertype(Symbol **idx, unsigned max, const char *prefix, symType type)
{
	unsigned key = 0;
	unsigned count = 0;
	Symbol *node;

	if(!prefix)
		return 0;

	while(max && key <= SCRIPT_INDEX_SIZE) {
		node = index[key++];
		while(node && max) {
			if(strnicmp(node->id, prefix, strlen(prefix))) {
next:
				node = node->next;
				continue;
			}

			if(node->id[strlen(prefix)] != '.')
				goto next;

			if(node->type != type)
				goto next;

			--max;
			idx[count++] = node;
			goto next;
		}
	}
	idx[count] = NULL;
	return count;
}


Script::Symbol *ScriptSymbols::find(const char *id, unsigned short size)
{
	unsigned key;
	Symbol *sym;

	if(!id)
		return NULL;

	if(*id == '%' || *id == '&')
		++id;

	key = getIndex(id);
large:
	sym = index[key];
	while(sym) {
		if(!stricmp(sym->id, id))
			break;

		sym = sym->next;
	}
	if(!sym && key < SCRIPT_INDEX_SIZE) {
		key = SCRIPT_INDEX_SIZE;
		goto large;
	}

	if(!sym && size)
		return make(id, size);

	return sym;
}

Script::Symbol *ScriptSymbols::make(const char *id, unsigned short size)
{
	Symbol *sym;
	unsigned key = getIndex(id);

	if(size > symlimit && !useBigmem)
		return NULL;

	if(size > symlimit) {
		key = SCRIPT_INDEX_SIZE;
		sym = (Symbol *)new char[sizeof(Symbol) + size];
	}
	else
		sym = (Symbol *)alloc(sizeof(Symbol) + size);
	sym->id = alloc(id);

	sym->next = index[key];
	index[key] = sym;

	sym->size = size;
	sym->type = symINITIAL;
	sym->data[0] = 0;
	return sym;
}

Script::Symbol *ScriptSymbols::setReference(const char *id, Symbol *target)
{
	Symbol *sym = find(id, sizeof(target));

	if(sym->type != symINITIAL && sym->type != symREF)
		return NULL;

	target = deref(target);
	memcpy(sym->data, &target, sizeof(target));
	sym->type = symREF;
	return sym;
}
