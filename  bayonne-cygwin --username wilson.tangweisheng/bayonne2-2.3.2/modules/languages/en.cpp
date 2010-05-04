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

#include <bayonne.h>
#include <cc++/slog.h>

namespace phrase {
using namespace ost;
using namespace std;

static class EnglishTranslator : public BayonneTranslator
{
public:
	unsigned sayhour(BayonneSession *s, unsigned count, const char *cp);
	unsigned saytime(BayonneSession *s, unsigned count, const char *cp);
	unsigned saydate(BayonneSession *s, unsigned count, const char *cp);
	unsigned saynumber(BayonneSession *s, unsigned count, const char *cp);
	EnglishTranslator(const char *cp);
	EnglishTranslator();
} en;

static class UsEnglishTranslator : public EnglishTranslator
{
public:
	unsigned sayhour(BayonneSession *s, unsigned count, const char *cp);
	unsigned saydate(BayonneSession *s, unsigned count, const char *cp);
	unsigned saytime(BayonneSession *s, unsigned count, const char *cp);
	UsEnglishTranslator();
} en_us;

UsEnglishTranslator::UsEnglishTranslator() :
EnglishTranslator("en_us")
{
}

unsigned UsEnglishTranslator::saytime(BayonneSession *s, unsigned count, const char *cp)
{
	return BayonneTranslator::saytime(s, count, cp);
}

unsigned UsEnglishTranslator::sayhour(BayonneSession *s, unsigned count, const char *cp)
{
	return BayonneTranslator::sayhour(s, count, cp);
}

static char *months[] = {"january", "february", "march", "april",
        "may", "june", "july", "august",
        "september", "october", "november", "december"};  

unsigned EnglishTranslator::saydate(BayonneSession *s, unsigned count, const char *cp)
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
        snprintf(buf, sizeof(buf), "%d", day);
        count = sayorder(s, count, buf);
        count = addItem(s, count, months[month]);
	snprintf(buf, sizeof(buf), "%d", year / 100);
	count = number(s, count, buf);
	if(year % 100 < 10)
		count = addItem(s, count, "o");
	snprintf(buf, sizeof(buf), "%d", year % 100);
	return number(s, count, buf);
}

unsigned UsEnglishTranslator::saydate(BayonneSession *s, unsigned count, const char *cp)
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
        snprintf(buf, sizeof(buf), "%d", day);
	count = addItem(s, count, months[month]);
        count = sayorder(s, count, buf);
	if(year / 100 == 20 && year % 100 < 10)
	{
		count = addItem(s, count, "2");
		count = addItem(s, count, "thousand");
		if(year % 100)
		{
			count = addItem(s, count, "and");
			snprintf(buf, sizeof(buf), "%d", year % 100);
			goto final;
		}
		else
			return count;
	}
	snprintf(buf, sizeof(buf), "%d", year / 100);
	count = number(s, count, buf);
	if(year % 100 < 10)
		count = addItem(s, count, "o");
	snprintf(buf, sizeof(buf), "%d", year % 100);
final:
	return number(s, count, buf);
}


unsigned EnglishTranslator::saytime(BayonneSession *s, unsigned count, const char *cp)
{
        int hour = 0;
        int min = 0;
        char buf[5];

        if(!cp || count > MAX_LIST - 10)
                return count;

        hour = atoi(cp);
        cp = strchr(cp, ':');
        if(cp)
                min = atoi(++cp);

        snprintf(buf, sizeof(buf), "%d", hour);
        count = number(s, count, buf);  


        if(min)
        {
                count = addItem(s, count, "and");
                snprintf(buf, sizeof(buf), "%d", min);
                count = number(s, count, buf);
        }  

        return count;   
}

unsigned EnglishTranslator::sayhour(BayonneSession *s, unsigned count, const char *cp)
{
	return EnglishTranslator::saytime(s, count, cp);
}

EnglishTranslator::EnglishTranslator(const char *cp) :
BayonneTranslator(cp)
{
}

EnglishTranslator::EnglishTranslator() :
BayonneTranslator("en")
{
	slog.debug("loading english translator");
}

unsigned EnglishTranslator::saynumber(BayonneSession *s, unsigned count, const char *cp)
{
        long num;
	char nbuf[32];
	bool zero = true;

        if(!cp || count > MAX_LIST - 20)
                return count;

        num = atol(cp);
                         
	if(num < 0)
	{
		count = addItem(s, count, "negative");
		num = -num;
	}

	if(num > 999999999)
	{
		zero = false;
		snprintf(nbuf, sizeof(nbuf), "%ld", num / 1000000000); 
		count = number(s, count, nbuf);
		count = addItem(s, count, "billion");
		num %= 1000000000;
	}
        if(num > 999999)
        {
                zero = false;
                snprintf(nbuf, sizeof(nbuf), "%ld", num / 1000000);
                count = number(s, count, nbuf);
		count = addItem(s, count, "million");
                num %= 1000000; 
        }  
        if(num > 999)   
        {
                zero = false;
                snprintf(nbuf, sizeof(nbuf), "%ld", num / 1000);   
                count = number(s, count, nbuf);
		count = addItem(s, count, "thousand");
                num %= 1000;    
        } 

	if(num || zero)
	{
		snprintf(nbuf, sizeof(nbuf), "%ld", num);
		count = number(s, count, nbuf);
	}

	cp = strchr(cp, '.');
	if(!cp)
		return count;

	count = addItem(s, count, "point");
	return spell(s, count, ++cp);
}

} // namespace
