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

#include "module.h"

namespace binder {
using namespace ost;
using namespace std;

bool Methods::xmlVoice(void)
{
	const char *lang = getValue(NULL);
	const char *voice = getValue(NULL);

	BayonneTranslator *t = BayonneTranslator::get(lang);
	
	if(t)
	{
		translator = t;
		voicelib = voice;
	}
	advance();
	return true;
}

bool Methods::xmlHangup(void)
{
	if(signal("exit"))
		return true;

	setState(STATE_HANGUP);
	return false;
}

bool Methods::xmlError(void)
{
        const char *opt;

        slog(Slog::levelError);
        slog() << logname << ": ";
        while(NULL != (opt = getValue(NULL)))
                slog() << opt;
        slog() << endl;
        advance();
        return true;
}
bool Methods::xmlNotice(void)
{
        const char *opt;

        slog(Slog::levelNotice);
        slog() << logname << ": ";
        while(NULL != (opt = getValue(NULL)))
                slog() << opt;
        slog() << endl;
        advance();
        return true;
}

bool Methods::xmlDebug(void)
{
	const char *opt;

	slog(Slog::levelDebug);
	slog() << logname << ": ";
	while(NULL != (opt = getValue(NULL)))
		slog() << opt;
	slog() << endl;
	advance();
	return true;
}

bool Methods::xmlAssign(void)
{
        const char *var = getKeyword("var");
        const char *value = getKeyword("value");
        const char *size = getKeyword("size");

        if(!value)
                value = "";

        if(!size)
                setSymbol(var, value, symsize);
        else
                setSymbol(var, value, atoi(size));
        advance();
        return true;
}

} // end namespace
