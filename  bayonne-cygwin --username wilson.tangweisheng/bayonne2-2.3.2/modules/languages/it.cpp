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

static class ItalianTranslator : public BayonneTranslator
{
private:
	unsigned number(BayonneSession *s, unsigned count, const char *cp);
	unsigned saynumber(BayonneSession *s, unsigned count, const char *cp);
	unsigned sayorder(BayonneSession *s, unsigned count, const char *cp);
	unsigned saydate(BayonneSession *s, unsigned count, const char *cp);
	unsigned saytime(BayonneSession *s, unsigned count, const char *cp);
	unsigned sayday(BayonneSession *s, unsigned count, const char *cp);
	unsigned sayhour(BayonneSession *s, unsigned count, const char *cp);

public:
	ItalianTranslator();
} it;

ItalianTranslator::ItalianTranslator() :
BayonneTranslator("it")
{
	slog.debug("loading italian translator");
}

static char *lows[] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
	"10", "11", "12", "13", "14", "15", "16", "17", "18", "19"};

static char *tens[] = {"0", "10", "20", "30", "40", "50", "60", "70", "80", "90"};

static char *ones[] = {"01", "11", "21", "31", "41", "51", "61", "71", "81", "91"};

static char *ords[] = {"0ord", "1ord", "2ord", "3ord", "4ord", "5ord",
	"6ord", "7ord", "8ord", "9ord", "10ord", "11ord", "12ord",
	"13ord", "14ord", "15ord", "16ord", "17ord", "18ord", "19ord"};

static char *ends[] = {"0end", "1end", "2end", "3end", "4end", "5end", 
        "6end", "7end", "8end", "9end", "10end", "11end", "12end",
        "13end", "14end", "15end", "16end", "17end", "18end", "19end"}; 

static char *months[] = {"january", "february", "march", "april",
	"may", "june", "july", "august",
	"september", "october", "november", "december"};

unsigned ItalianTranslator::number(BayonneSession *s, unsigned count, const char *cp)
{
	long num = atol(cp);

	if(num > 999 || count > MAX_LIST - 10)
		return count;

        if(num >= 100)
        {
		count = addItem(s, count, lows[num / 100]);
		if(num / 100 > 1)
			count = addItem(s, count, "hundreds");
		else
			count = addItem(s, count, "hundred");
                num %= 100;      
                if(!num)
                        return count;
        } 

	if(num < 20)
		return addItem(s, count, lows[num]);

	if((num % 10) == 1)
		return addItem(s, count, ones[num / 10]);

	count = addItem(s, count, tens[num / 10]);
	if(num % 10)
		count = addItem(s, count, lows[num % 10]);

	return count;
}

unsigned ItalianTranslator::sayhour(BayonneSession *s, unsigned count, const char *cp)
{
	return ItalianTranslator::saytime(s, count, cp);
}

unsigned ItalianTranslator::saytime(BayonneSession *s, unsigned count, const char *cp)
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
	
	snprintf(buf, sizeof(buf), "%d", min);
	count = number(s, count, buf);

	return count;
}

unsigned ItalianTranslator::saydate(BayonneSession *s, unsigned count, const char *cp)
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
	snprintf(buf, sizeof(buf), "%d", year);
	return saynumber(s, count, buf);
}

unsigned ItalianTranslator::sayday(BayonneSession *s, unsigned count, const char *cp)
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
	return addItem(s, count, months[month]);
}

unsigned ItalianTranslator::sayorder(BayonneSession *s, unsigned count, const char *cp)
{
	long num = atol(cp);

	if(!num || count > MAX_LIST - 10 || num > 999)
		return count;

	if(num >= 100)
	{
		count = addItem(s, count, lows[num/100]);
		count = addItem(s, count, "hundred");
		num %= 100;
	}

	if(num > 19)
	{
		if(num % 10)
			count = addItem(s, count, lows[num / 10]);
		else
			count = addItem(s, count, ords[num / 10]);
		num %= 10;
		count = addItem(s, count, ends[num]);
		return count;
	}
	if(num)
		count = addItem(s, count, ords[num]);
	return count;
}

unsigned ItalianTranslator::saynumber(BayonneSession *s, unsigned count, const char *cp)
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
		if(num / 1000000000 > 1)
			count = addItem(s, count, "billions");
		else
			count = addItem(s, count, "billion");
		num %= 1000000000;
	}
        if(num > 999999)
        {
                zero = false;
                snprintf(nbuf, sizeof(nbuf), "%ld", num / 1000000);
                count = number(s, count, nbuf);
		if(num / 1000000 > 1)
			count = addItem(s, count, "millions");
		else
			count = addItem(s, count, "million");
                num %= 1000000; 
        }  
        if(num > 999)   
        {
                zero = false;
                snprintf(nbuf, sizeof(nbuf), "%ld", num / 1000);   
                count = number(s, count, nbuf);
		if(num / 1000 > 1)
			count = addItem(s, count, "thousands");
		else
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
