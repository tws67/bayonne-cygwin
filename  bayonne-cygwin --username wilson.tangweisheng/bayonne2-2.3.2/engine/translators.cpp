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

#include "engine.h"

using namespace ost;
using namespace std;

static char *letters[] = {"a", "b", "c", "d", "e", "f", "g", "h", "i", 
	"j", "k", "l", "m", "n", "o", "p", "q", "r", "s", "t", "u", "v",
	"w", "x", "y", "z"};

static char *numbers[] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", 
	"9", "10", "11", "12", "13", "14", "15", "16", "17", "18", "19"};

static char *tens[] = {"", "10", "20", "30", "40", "50", "60", "70", "80", "90"};

static char *weekdays[] = {
	"sunday", "monday", "tuesday", "wednesday", "thursday", "friday", "saturday"}; 

static char *months[] = {
        "january", "february", "march", "april",
        "may", "june", "july", "august",
        "september", "october", "november", "december"};

BayonneTranslator *BayonneTranslator::first = NULL;

BayonneTranslator::BayonneTranslator(const char *name)
{
	id = name;
	next = first;
	first = this;
}

BayonneTranslator::~BayonneTranslator()
{}

BayonneTranslator *BayonneTranslator::get(const char *id)
{
	BayonneTranslator *bts = first;
	char nid[3];

retry:
	while(bts)
	{
		if(!stricmp(bts->id, id))
			return bts;

		bts = bts->next;
	}
	if(id[2] == '_')
	{
		nid[0] = id[0];
		nid[1] = id[1];
		nid[2] = 0;
		id = nid;
		bts = first;
		goto retry;
	}
	return NULL;
}

unsigned BayonneTranslator::saydate(BayonneSession *s, unsigned count, const char *cp)
{
        int month, day, year;
	char buf[8];

	year = month = day = 0;

	if(count > MAX_LIST - 16)
		return count;

	if(strchr(cp, '-'))
	{
		year = atoi(cp);
		cp = strchr(cp, '-');
		if(cp)
			month = atoi(++cp);
		if(cp)
			cp = strchr(cp, '-');
		if(cp)
			day = atoi(++cp);
	}
	else if(strchr(cp, '/'))
	{
		month = atoi(cp);
		cp = strchr(cp, '/');
		if(cp)
			day = atoi(++cp);
		if(cp)
			cp = strchr(cp, '/');
		if(cp)
			year = atoi(++cp);
	}
	else if(strchr(cp, '.'))
	{
		day = atoi(cp);
		cp = strchr(cp, '.');
		if(cp)
			month = atoi(++cp);
		if(cp)
			cp = strchr(cp, '.');
		if(cp)
			year = atoi(++cp);
	}
	else return count;

        --month;
	s->state.audio.list[count++] = months[month];
	snprintf(buf, sizeof(buf), "%d", day);
	count = sayorder(s, count, buf);
	
	if(year < 2000 || year > 2009)
	{
		snprintf(buf, sizeof(buf), "%d", year / 100);
                count = number(s, count, buf);
		if(year % 100)
		{
			if(year % 100 < 10)
				s->state.audio.list[count++] = "o";
			snprintf(buf, sizeof(buf), "%d", year % 100);
			count = number(s, count, buf);
		}
		else
			s->state.audio.list[count++] = "hundred";
	}
	else
	{
		s->state.audio.list[count++] = "2";
		s->state.audio.list[count++] = "thousand";
		if(year % 10)
			s->state.audio.list[count++] = numbers[year % 10];
	}
	return count;
}

unsigned BayonneTranslator::sayday(BayonneSession *s, unsigned count, const char *cp)
{
        int month, day, year;
	char buf[8];

	year = month = day = 0;

	if(count > MAX_LIST - 16)
		return count;

	if(strchr(cp, '-'))
	{
		year = atoi(cp);
		cp = strchr(cp, '-');
		if(cp)
			month = atoi(++cp);
		if(cp)
			cp = strchr(cp, '-');
		if(cp)
			day = atoi(++cp);
	}
	else if(strchr(cp, '/'))
	{
		month = atoi(cp);
		cp = strchr(cp, '/');
		if(cp)
			day = atoi(++cp);
		if(cp)
			cp = strchr(cp, '/');
		if(cp)
			year = atoi(++cp);
	}
	else if(strchr(cp, '.'))
	{
		day = atoi(cp);
		cp = strchr(cp, '.');
		if(cp)
			month = atoi(++cp);
		if(cp)
			cp = strchr(cp, '.');
		if(cp)
			year = atoi(++cp);
	}
	else return count;

        --month;
	s->state.audio.list[count++] = months[month];
	snprintf(buf, sizeof(buf), "%d", day);
	return sayorder(s, count, buf);
}

unsigned BayonneTranslator::weekday(BayonneSession *s, unsigned count,
const char *cp) {
	time_t cl;
        struct tm *ti, tid;
        int month, day, year; 

	time(&cl);
	ti = localtime_r(&cl, &tid);

	year = month = day = 0;

	if(count > MAX_LIST - 1)
		return count;

	if(strchr(cp, '-'))
	{
		year = atoi(cp);
		cp = strchr(cp, '-');
		if(cp)
			month = atoi(++cp);
		if(cp)
			cp = strchr(cp, '-');
		if(cp)
			day = atoi(++cp);
	}
	else if(strchr(cp, '/'))
	{
		month = atoi(cp);
		cp = strchr(cp, '/');
		if(cp)
			day = atoi(++cp);
		if(cp)
			cp = strchr(cp, '/');
		if(cp)
			year = atoi(++cp);
	}
	else if(strchr(cp, '.'))
	{
		day = atoi(cp);
		cp = strchr(cp, '.');
		if(cp)
			month = atoi(++cp);
		if(cp)
			cp = strchr(cp, '.');
		if(cp)
			year = atoi(++cp);
	}
	else return count;

        ti->tm_year = year;
        ti->tm_mon = month - 1;
        ti->tm_mday = day;

        if(ti->tm_year > 1000)
                ti->tm_year -= 1900;

        cl = mktime(ti);
	ti = localtime_r(&cl, &tid);
	s->state.audio.list[count++] = weekdays[ti->tm_wday];
	return count;	
}

unsigned BayonneTranslator::sayorder(BayonneSession *s, unsigned count, const char *cp)
{
	long num;

	static char *low[] = { "th",
                "1st", "2nd", "3rd", "4th", "5th",
                "6th", "7th", "8th", "9th", "10th",
                "11th", "12th", "13th", "14th", "15th",
                "16th", "17th", "18th", "19th"};

	static char *hi[] = {"", "10th", "20th", "30th", "40th", "50th",
		"60th", "70th", "80th", "90th"};

	if(!cp || count > MAX_LIST - 10)
		return count;

	num = atol(cp);

	if(num > 999)
		return count;

	if(num > 100)
	{
		if(num % 100)
		{
			s->state.audio.list[count++] = numbers[num / 100];
			s->state.audio.list[count++] = "hundred";
		}
		else
		{
			s->state.audio.list[count++] = numbers[num / 100];
			s->state.audio.list[count++] = "hundred";
			s->state.audio.list[count++] = "th";
		}
		num %= 100;
	}
	if(num > 19)
	{
		if(num % 10)
			s->state.audio.list[count++] = tens[num / 10];
		else
			s->state.audio.list[count++] = hi[num / 10];
		num %= 10;
	}
	if(num)
		s->state.audio.list[count++] = low[num];

	return count;
}

unsigned BayonneTranslator::saynumber(BayonneSession *s, unsigned count, const char *cp)
{
	return number(s, count, cp);
}

unsigned BayonneTranslator::number(BayonneSession *s, unsigned count, const char *cp)
{
	long num;

	if(!cp || count > MAX_LIST - 10)
		return count;

	num = atol(cp);
	
	if(num > 999)
		return count;

	if(num >= 100)
	{
		s->state.audio.list[count++] = numbers[num / 100];
		s->state.audio.list[count++] = "hundred";
		num %= 100;
		if(!num)
			return count;
	}

	if(num < 20)
	{
		s->state.audio.list[count++] = numbers[num];
		return count;
	}

	s->state.audio.list[count++] = tens[num / 10];
	if(num %10)
		s->state.audio.list[count++] = numbers[num % 10];
	return count;
}

unsigned BayonneTranslator::saycount(BayonneSession *s, unsigned count, const char *cp)
{
	long num;

	if(!cp || count > MAX_LIST - 10)
		return count;

	num = atol(cp);
	s->state.audio.lastnum = num;

	if(num)
		return number(s, count, cp);
	return count;
}

unsigned BayonneTranslator::spell(BayonneSession *s, unsigned count, const char *text) 
{
	char ch;
	bool comma = false;

	if(!text)
		return count;

	while(*text && count < (MAX_LIST - 1))
	{
		ch = tolower(*(text));
		++text;
		if(ch >= 'a' && ch <= 'z')
		{
			comma = true;
			s->state.audio.list[count++] = letters[ch - 'a'];
		}
		else if(ch >= '0' && ch <= '9')
			s->state.audio.list[count++] = numbers[ch - '0'];
		
		switch(ch)
		{
		case '.':
			s->state.audio.list[count++] = "dot";
			break;
		case ',':
			if(comma)
				s->state.audio.list[count++] = "comma";
			break;
		}
	}
	return count;
}		

unsigned BayonneTranslator::digits(BayonneSession *s, unsigned count, const char *text) 
{
	char ch;

	if(!text)
		return count;

	while(*text && count < (MAX_LIST - 1))
	{
		ch = tolower(*(text));
		++text;
		if(ch >= '0' && ch <= '9')
			s->state.audio.list[count++] = numbers[ch - '0'];
		
		switch(ch)
		{
		case '.':
			s->state.audio.list[count++] = "dot";
			break;
		}
	}
	return count;
}		

unsigned BayonneTranslator::saybool(BayonneSession *s, unsigned count, const char *cp)
{
	if(!cp || count >= MAX_LIST)
		return count;

	switch(*cp)
	{
	case '0':
	case 'n':
	case 'N':
	case 'f':
	case 'F':
		s->state.audio.list[count++] = "no";
		break;
	default:
		s->state.audio.list[count++] = "yes";
	}
	return count;
}

unsigned BayonneTranslator::phone(BayonneSession *s, unsigned count, const char *ch)
{
	return digits(s, count, ch);
}

unsigned BayonneTranslator::extension(BayonneSession *s, unsigned count, const char *ch)
{
	return digits(s, count, ch);
}

unsigned BayonneTranslator::saytime(BayonneSession *s, unsigned count, const char *cp)
{
	int hour = 0;
	int min = 0;
	bool pm = false;
	char buf[4];

	if(!cp || count > MAX_LIST - 10)
		return count;

	hour = atoi(cp);
	cp = strchr(cp, ':');
	if(cp)
		min = atoi(++cp);

	if(hour > 11)
	{
		pm = true;
		hour -= 12;
	}
	if(!hour)
		hour = 12;

	snprintf(buf, sizeof(buf), "%d", hour);
	count = number(s, count, buf);
	if(min)
	{
		if(min < 10)
			s->state.audio.list[count++] = "o";
		snprintf(buf, sizeof(buf), "%d", min);
		count = number(s, count, buf);
	}
	if(pm)
		s->state.audio.list[count++] = "p";
	else
		s->state.audio.list[count++] = "a";
	s->state.audio.list[count++] = "m";
	return count;
}

unsigned BayonneTranslator::sayhour(BayonneSession *s, unsigned count, const char *cp)
{
	int hour = 0;
	int min = 0;
	bool pm = false;
	char buf[4];

	if(!cp || count > MAX_LIST - 10)
		return count;

	hour = atoi(cp);
	cp = strchr(cp, ':');
	if(cp)
		min = atoi(++cp);

	if(hour > 11)
	{
		pm = true;
		hour -= 12;
	}
	if(!hour)
		hour = 12;

	snprintf(buf, sizeof(buf), "%d", hour);
	count = number(s, count, buf);
	if(pm)
		s->state.audio.list[count++] = "p";
	else
		s->state.audio.list[count++] = "a";
	s->state.audio.list[count++] = "m";
	return count;
}


unsigned BayonneTranslator::addItem(BayonneSession *s, unsigned count, const char *text)
{
	s->state.audio.list[count++] = text;
	return count;
}

const char *BayonneTranslator::getLast(BayonneSession *s, unsigned count)
{
	return s->state.audio.list[--count];
}

const char *BayonneTranslator::getToken(BayonneSession *s, Line *line, unsigned *idx)
{
	const char *cp;

	for(;;)
	{
		if(*idx >= line->argc)
			return NULL;

		cp = line->args[*idx];
		++*idx;
		if(*cp == '=')
		{
			++*idx;
			continue;
		}
		else if(*cp == '{')
			return ++cp;
		else
		{
			cp = s->getContent(cp);
			if(!cp)
				return "";
			return cp;
		}
	}
}

const char *BayonneTranslator::speak(BayonneSession *s, Line *l)
{
	const char *cp;
	
	if(!l)
		l = s->getLine();

	unsigned idx = 0;
	unsigned count = 0;
	while(count < (MAX_LIST - 1) && NULL != (cp = getToken(s, l, &idx)))
	{
		if(*cp != '&')
			s->state.audio.list[count++] = cp;
		else if(!stricmp(cp, "&spell"))
			count = spell(s, count, getToken(s, l, &idx));	
		else if(!stricmp(cp, "&digits"))
			count = digits(s, count, getToken(s, l, &idx));
		else if(!stricmp(cp, "&bool"))
			count = saybool(s, count, getToken(s, l, &idx));
		else if(!stricmp(cp, "&time"))
			count = saytime(s, count, getToken(s, l, &idx));
		else if(!stricmp(cp, "&hour"))
			count = sayhour(s, count, getToken(s, l, &idx));
		else if(!stricmp(cp, "&weekday"))
			count = weekday(s, count, getToken(s, l, &idx));
                else if(!stricmp(cp, "&day"))
                        count = sayday(s, count, getToken(s, l, &idx));
                else if(!stricmp(cp, "&daydate"))
                {
                        cp = getToken(s, l, &idx);
                        count = weekday(s, count, cp);
                        count = sayday(s, count, cp);
                }
		else if(!stricmp(cp, "&fulldate"))
		{
			cp = getToken(s, l, &idx);
			count = weekday(s, count, cp);
			count = saydate(s, count, cp);
		}
		else if(!stricmp(cp, "&date"))
			count = saydate(s, count, getToken(s, l, &idx));
		else if(!stricmp(cp, "&order"))
			count = sayorder(s, count, getToken(s, l, &idx));
		else if(!stricmp(cp, "&number"))
			count = saynumber(s, count, getToken(s, l, &idx));
		else if(!stricmp(cp, "&phone"))
			count = phone(s, count, getToken(s, l, &idx));
		else if(!stricmp(cp, "&extension"))
			count = extension(s, count, getToken(s, l, &idx));
		else if(!stricmp(cp, "&bool"))
			count = saybool(s, count, getToken(s, l, &idx));
		else if(!stricmp(cp, "&count"))
			count = saycount(s, count, getToken(s, l, &idx));
		else if(!stricmp(cp, "&zero"))
		{
			cp = getToken(s, l, &idx);
			if(!s->state.audio.lastnum)
			{
				s->state.audio.lastnum = -1;
				s->state.audio.list[count++] = cp;
			}
		}
		else if(!stricmp(cp, "&one"))
			cp = getToken(s, l, &idx);
		else if(!stricmp(cp, "&single"))
		{
			cp = getToken(s, l, &idx);
			if(s->state.audio.lastnum == 1)
			{
				s->state.audio.lastnum = -1;
				s->state.audio.list[count++] = cp;
			}
		}
		else if(!stricmp(cp, "&plural"))
		{
			cp = getToken(s, l, &idx);
			if(s->state.audio.lastnum > 1)
			{
				s->state.audio.lastnum = -1;
				s->state.audio.list[count++] = cp;
			}
		}
		else 
			return "unknown rule";
	}
	s->state.audio.list[count] = NULL;
	return NULL;
}
				
BayonneTranslator *BayonneTranslator::loadTranslator(const char *id)
{
        DSO *dso;
        BayonneTranslator *d;
        char pathbuf[256];
        const char *path;
	char nid[3];
	const char *lid = id;
	const char *prefix = NULL;

#ifdef  WIN32
        if(!prefix)
                prefix="C:\\Program Files\\Common Files\\GNU Telephony\\Bayonne Translators";
#else
        if(!prefix)
                prefix=LIBDIR_FILES;
#endif  

retry:
#ifdef  WIN32
        snprintf(pathbuf, sizeof(pathbuf), "%s/%s." RLL_SUFFIX, prefix, id);
#else
        snprintf(pathbuf, sizeof(pathbuf), "%s/%s.phrases", prefix, id);
#endif
        path = pathbuf;
 
        d = get(lid);
        if(d)
                return d;  

	if(id[2] != lid[2])
	d = get(id);
	if(d)
		return d;

        if(!canAccess(path))
        {
		if(id[2] == '_')
		{
			nid[0] = id[0];
			nid[1] = id[1];
			nid[2] = 0;
			id = nid;
			goto retry;
		}

		if(server)
	                errlog("access", "cannot load %s", path);
                return NULL;
        }

        dso = new DSO(path);
        if(!dso->isValid())
        {
		if(server)
	                errlog("error", "%s: %s", path, dso->getError());
                return NULL;
        }
        d = get(lid);  
	if(d)
		return d;

	if(id[2] != lid[2])
		d = get(id);

	return d;
}
