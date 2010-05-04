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

static class SpanishTranslator : public BayonneTranslator
{
private:
	unsigned number(BayonneSession *s, unsigned count, const char *cp);
	unsigned saynumber(BayonneSession *s, unsigned count, const char *cp);
	unsigned sayorder(BayonneSession *s, unsigned count, const char *cp);
	unsigned saydate(BayonneSession *s, unsigned count, const char *cp);
	unsigned saytime(BayonneSession *s, unsigned count, const char *cp);
	unsigned sayhour(BayonneSession *s, unsigned count, const char *cp);
	unsigned sayday(BayonneSession *s, unsigned count, const char *cp);

public:
	SpanishTranslator();
	SpanishTranslator(const char *cp);
} es;

static class UsSpanishTranslator : public SpanishTranslator
{
private:
	unsigned saydate(BayonneSession *s, unsigned count, const char *cp);
	unsigned saytime(BayonneSession *s, unsigned count, const char *cp);
	unsigned sayday(BayonneSession *s, unsigned count, const char *cp);
	unsigned sayhour(BayonneSession *s, unsigned count, const char *cp);
public:
	UsSpanishTranslator();
} es_us;

UsSpanishTranslator::UsSpanishTranslator() :
SpanishTranslator("es_us")
{}

unsigned UsSpanishTranslator::saydate(BayonneSession *s, unsigned count, const char *cp)
{
	return BayonneTranslator::saydate(s, count, cp);
}

unsigned UsSpanishTranslator::saytime(BayonneSession *s, unsigned count, const char *cp)
{
	return BayonneTranslator::saytime(s, count, cp);
}

unsigned UsSpanishTranslator::sayhour(BayonneSession *s, unsigned count, const char *cp)
{
	return BayonneTranslator::sayhour(s, count, cp);
}

unsigned UsSpanishTranslator::sayday(BayonneSession *s, unsigned count, const char *cp)
{
	return BayonneTranslator::sayday(s, count, cp);
}

SpanishTranslator::SpanishTranslator(const char *cp) :
BayonneTranslator(cp)
{
}
	
SpanishTranslator::SpanishTranslator() :
BayonneTranslator("es")
{
	slog.debug("loading spanish translator");
}

static char *lows[] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
	"10", "11", "12", "13", "14", "15", "16", "17", "18", "19"};

static char *tens[] = {"0", "10", "20", "30", "40", "50", "60", "70", "80", "90"};

static char *hundreds[] = {"0", "100", "200", "300", "400", "500", "600", "700", "800", "900"};

static char *ths[] = {"0th", "1th", "2th", "3th", "4th", "5th", "6th", "7th", "8th", "9th", "10th"};

static char *months[] = {"january", "february", "march", "april",
	"may", "june", "july", "august",
	"september", "october", "november", "december"};

unsigned SpanishTranslator::sayhour(BayonneSession *s, unsigned count, const char *cp)
{
	return SpanishTranslator::saytime(s, count, cp);
}

unsigned SpanishTranslator::number(BayonneSession *s, unsigned count, const char *cp)
{
	long num = atol(cp);

	if(num > 999 || count > MAX_LIST - 10)
		return count;

        if(num >= 100)
        {
		count = addItem(s, count, hundreds[num / 100]);
		if(num > 100 && num < 200)
			count = addItem(s, count, "to");
                num %= 100;      
                if(!num)
                        return count;
        } 


	if(num >= 20 && (num % 10))
	{
		count = addItem(s, count, tens[num / 10]);
		count = addItem(s, count, "and");
		num %= 10;
	}

	if(num < 20)
	{
		if(num == 1)
			return addItem(s, count, "un");

		return addItem(s, count, lows[num]);
	}
	return addItem(s, count, lows[num / 10]);
}

unsigned SpanishTranslator::saytime(BayonneSession *s, unsigned count, const char *cp)
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

unsigned SpanishTranslator::saydate(BayonneSession *s, unsigned count, const char *cp)
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
	count = number(s, count, buf);
	count = addItem(s, count, months[month]);
	snprintf(buf, sizeof(buf), "%d", year);
	count = addItem(s, count, "de");
	return saynumber(s, count, buf);
}

unsigned SpanishTranslator::sayday(BayonneSession *s, unsigned count, const char *cp)
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
	count = number(s, count, buf);
	return addItem(s, count, months[month]);
}

unsigned SpanishTranslator::sayorder(BayonneSession *s, unsigned count, const char *cp)
{
	bool lower = true;
	long num = atol(cp);

	if(num > 999 || count > MAX_LIST - 10)
		return count;

        if(num >= 100)
        {
		lower = false;
		count = addItem(s, count, hundreds[num / 100]);
		if(num > 100 && num < 200)
			count = addItem(s, count, "to");
                num %= 100;      
                if(!num)
                        return addItem(s, count, "avo");
        } 


	if(num > 10 && (num % 10))
	{
		count = addItem(s, count, tens[num / 10]);
		count = addItem(s, count, "and");
		num %= 10;
	}

	if(num <= 10)
	{
		if(lower)
			return addItem(s, count, ths[num]);
		if(num == 1)
		{
			count = addItem(s, count, "un");
			return addItem(s, count, "avo");
		}
		count = addItem(s, count, lows[num]);
		return addItem(s, count, "avo");
	}
	count = addItem(s, count, lows[num / 10]);
	return addItem(s, count, "avo");
}

unsigned SpanishTranslator::saynumber(BayonneSession *s, unsigned count, const char *cp)
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
