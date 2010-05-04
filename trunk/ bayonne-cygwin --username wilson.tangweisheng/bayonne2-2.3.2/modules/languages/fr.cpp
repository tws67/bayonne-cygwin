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

static class FrenchTranslator : public BayonneTranslator
{
private:
	unsigned number(BayonneSession *s, unsigned count, const char *cp);
	unsigned saynumber(BayonneSession *s, unsigned count, const char *cp);
	unsigned lownumber(BayonneSession *s, unsigned count, long num, bool andfix);
	unsigned sayorder(BayonneSession *s, unsigned count, const char *cp);
	unsigned saydate(BayonneSession *s, unsigned count, const char *cp);
	unsigned saytime(BayonneSession *s, unsigned count, const char *cp);
	unsigned sayhour(BayonneSession *s, unsigned count, const char *cp);
	unsigned sayday(BayonneSession *s, unsigned count, const char *cp);

public:
	FrenchTranslator();
} fr;

FrenchTranslator::FrenchTranslator() :
BayonneTranslator("fr")
{
	slog.debug("loading french translator");
}

static char *lows[] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10",
	"11", "12", "13", "14", "15", "16", "17", "18", "19"};

static char *tens[] = {"0", "10", "20", "30", "40", "50"};

static char *months[] = {"january", "february", "march", "april",
	"may", "june", "july", "august",
	"september", "october", "november", "december"};

unsigned FrenchTranslator::number(BayonneSession *s, unsigned count, const char *cp)
{
	return lownumber(s, count, atol(cp), true);
}

unsigned FrenchTranslator::lownumber(BayonneSession *s, unsigned count, long num, bool andfix)
{
	bool andrule = false;

	if(num >= 100)
	{
		count = addItem(s, count, lows[num / 100]);
		count = addItem(s, count, "hundred");
		num %= 100;
		andfix = false;
		if(!num)
			return count;
	}
	if(num >= 80)
	{
		andrule = andfix;
		count = addItem(s, count, "80");
		num -= 80;
	}
	if(num >= 60)
	{
		andrule = andfix;
		count = addItem(s, count, "60");
		num -= 60;
	}
	if(num >= 20)
	{
		andrule = andfix;
		count = addItem(s, count, tens[num / 10]);
		num -= ((num / 10) * 10);
	}
	if(num > 16)
	{
		count = addItem(s, count, "10");
		num -= 10;
	}
	if(num)
	{
		if(num == 1 && andrule)
			count = addItem(s, count, "and");
		count = addItem(s, count, lows[num]);
	}
	return count;
}

unsigned FrenchTranslator::sayhour(BayonneSession *s, unsigned count, const char *cp)
{
	return FrenchTranslator::saytime(s, count, cp);
}

unsigned FrenchTranslator::saytime(BayonneSession *s, unsigned count, const char *cp)
{
        int hour = 0;
        int min = 0;

        if(!cp || count > MAX_LIST - 10)
                return count;

        hour = atoi(cp);
        cp = strchr(cp, ':');
        if(cp)
                min = atoi(++cp);
                                 
	if(hour)
		count = lownumber(s, count, hour, false);
	else
		count = addItem(s, count, "0");
	
	if(min)
		count = lownumber(s, count, min, false);
	else
		count = addItem(s, count, "0");

	return count;
}

unsigned FrenchTranslator::saydate(BayonneSession *s, unsigned count, const char *cp)
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
	count = lownumber(s, count, day, true);
	count = addItem(s, count, months[month]);
	snprintf(buf, sizeof(buf), "%d", year);
	return saynumber(s, count, buf);
}

unsigned FrenchTranslator::sayday(BayonneSession *s, unsigned count, const char *cp)
{
        int month, day, year;      

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
	count = lownumber(s, count, day, true);
	return addItem(s, count, months[month]);
}

unsigned FrenchTranslator::sayorder(BayonneSession *s, unsigned count, const char *cp)
{
	long num = atol(cp);

	if(!num || count > MAX_LIST - 10)
		return count;

	if(num == 1)
	{
		count = addItem(s, count, "1st");
		return count;
	}

	count = lownumber(s, count, num, false);
	switch(atol(getLast(s, count)))
	{
	case 1:
		return addItem(s, count, "uniemi");
        case 7:
        case 8:       
        case 20: 
        case 30:
        case 40:
        case 50:
        case 60:
        case 80:
        case 100: 
		return addItem(s, count, "tiemi");
        case 4:
        case 5:
        case 9:
        case 1000: 
		return addItem(s, count, "iemi");
	default:
		return addItem(s, count, "ziemi");
	}
}

unsigned FrenchTranslator::saynumber(BayonneSession *s, unsigned count, const char *cp)
{
        long num;;
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
		zero = true;
		count = lownumber(s, count, num / 1000000000, false);
		count = addItem(s, count, "billion");
		num %= 1000000000;
	}
        if(num > 999999)
        {
                zero = true;
                count = lownumber(s, count, num / 1000000, false);
		count = addItem(s, count, "million");
                num %= 1000000; 
        }  
        if(num > 999)   
        {
                zero = true;
                count = lownumber(s, count, num / 1000, false);
		count = addItem(s, count, "thousand");
                num %= 1000;    
        } 

	if(num || zero)
		count = lownumber(s, count, num, true);

	cp = strchr(cp, '.');
	if(!cp)
		return count;

	count = addItem(s, count, "point");
	return spell(s, count, ++cp);
}

} // namespace
