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

static class RussianTranslator : public BayonneTranslator
{
private:
	unsigned number(BayonneSession *s, unsigned count, const char *cp);
	unsigned number_neuter(BayonneSession *s, unsigned count, const char *cp);
	unsigned number_case1(BayonneSession *s, unsigned count, const char *cp);
	unsigned number_f(BayonneSession *s, unsigned count, const char *cp);
	unsigned number_n(BayonneSession *s, unsigned count, const char *cp);
	unsigned weekday(BayonneSession *s, unsigned count, const char *cp);
	unsigned saynumber(BayonneSession *s, unsigned count, const char *cp);
	unsigned saydate(BayonneSession *s, unsigned count, const char *cp);
	unsigned saytime(BayonneSession *s, unsigned count, const char *cp);
	unsigned sayday(BayonneSession *s, unsigned count, const char *cp);
	unsigned sayhour(BayonneSession *s, unsigned count, const char *cp);
	unsigned sayorder(BayonneSession *s, unsigned count, const char *cp);
	unsigned spell(BayonneSession *s, unsigned count, const char *cp);

public:
	RussianTranslator();
} ru;

RussianTranslator::RussianTranslator() :
BayonneTranslator("ru")
{
	slog.debug("loading russian translator");
}

static char *huns[] = {"0", "100", "200", "300", "400", "500", "600", "700", "800", "900"};

static char *letters[] = {"a", "b", "c", "d", "e", "f", "g", "h", "i", 
        "j", "k", "l", "m", "n", "o", "p", "q", "r", "s", "t", "u", "v",
        "w", "x", "y", "z"};  

typedef struct {
	char value;
	char *name;
} sym_t;

static sym_t syms[] = {
		{'Á', "cyrillic_a"},
		{'Â', "cyrillic_be"},
		{'×', "cyrillic_ve"},
		{'Ç', "cyrillic_ghe"},
		{'Ä', "cyrillic_de"},
		{'Å', "cyrillic_ie"},
		{'£', "cyrillic_io"},
		{'Ö', "cyrillic_zhe"},
		{'Ú', "cyrillic_ze"},
		{'É', "cyrillic_i"},
		{'Ê', "cyrillic_shorti"},
		{'Ë', "cyrillic_ka"},
		{'Ì', "cyrillic_el"},
		{'Í', "cyrillic_em"},
		{'Î', "cyrillic_en"},
		{'Ï', "cyrillic_o"},
		{'Ð', "cyrillic_pe"},
		{'Ò', "cyrillic_er"},
		{'Ó', "cyrillic_es"},
		{'Ô', "cyrillic_te"},
		{'Õ', "cyrillic_u"},
		{'Æ', "cyrillic_ef"},
		{'È', "cyrillic_ha"},
		{'Ã', "cyrillic_tse"},
		{'Þ', "cyrillic_che"},
		{'Û', "cyrillic_sha"},
		{'Ý', "cyrillic_shcha"},
		{'Ø', "cyrillic_softsign"},
		{'ß', "cyrillic_hardsign"},
		{'Ù', "cyrillic_yi"},
		{'Ü', "cyrillic_e"},
		{'À', "cyrillic_yu"},
		{'Ñ', "cyrillic_ya"}
        };

static char *huns_oc1[] = {"0", "100-order-case_1", "200-order-case_1",
	"300-order-case_1", "400-order-case_1", "500-order-case_1",
	"600-order-case_1", "700-order-case_1", "800-order-case_1", "900-order-case_1"};

static char *huns_neu[] = {"0", "100-order-neuter", "200-order-neuter",
	"300-order-neuter", "400-order-neuter", "500-order-neuter",
	"600-order-neuter", "700-order-neuter", "800-order-neuter", "900-order-neuter"};

static char *huns_ord[] = {"0", "100-order", "200-order", "300-order", "400-order",
	"500-order", "600-order", "700-order", "800-order", "900-order"};

static char *lows_oc1[] = {"0", "1-order-case_1", "2-order-case_1", "3-order-case_1",
	"4-order-case_1", "5-order-case_1", "6-order-case_1", "7-order-case_1",
	"8-order-case_1", "9-order-case_1", "10-order-case_1", "11-order-case_1",
	"12-order-case_1", "13-order-case_1", "14-order-case_1", "15-order-case_1",
	"16-order-case_1", "17-order-case_1", "18-order-case_1", "19-order-case_1"};

static char *lows_neu[] = {"0", "1-order-neuter", "2-order-neuter", "3-order-neuter",
	"4-order-neuter", "5-order-neuter", "6-order-neuter","7-order-neuter",
	"8-order-neuter", "9-order-neuter", "10-order-neuter", "1-order-neuter",
	"12-order-neuter", "13-order-neuter", "14-order-neuter", "15-order-neuter",
	"16-order-neuter", "17-order-neuter", "18-order-neuter", "19-order-neuter"};

static char *lows_ord[] = {"0", "1-order", "2-order", "3-order", "4-order", 
	"5-order", "6-order", "7-order", "8-order", "9-order",
	"10-order", "11-order", "12-order", "13-order", "14-order",
	"15-order", "16-order", "17-order", "18-order", "19-order"};

static char *lows[] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
	"10", "11", "12", "13", "14", "15", "16", "17", "18", "19"};

static char *tens_oc1[] = {"0", "10-order-case_1", "20-order-case_1", "30-order-case_1",
	"40-order-case_1", "50-order-case_1", "60-order-case_1", "70-order-case_1",
	"80-order-case_1", "90-order-case_1"};

static char *tens_neu[] = {"0", "10-order-neuter", "20-order-neuter", "30-order-neuter",
	"40-order-neuter", "50-order-neuter", "60-order-neuter", "70-order-neuter",
	"80-order-neuter", "90-order-neuter"};

static char *tens_ord[] = {"0", "10-order", "20-order", "30-order", "40-order",
	"50-order", "60-order", "70-order", "80-order", "90-order"};

static char *tens[] = {"0", "10", "20", "30", "40", "50", "60", "70", "80", "90"};

static char *months[] = {"yanvar-case_1", "fevral-case_1", "mart-case_1", "aprel-case_1",
	"mai-case_1", "iyun-case_1", "iyul-case_1", "avgust-case_1",
	"sentyabr-case_1", "oktyabr-case_1", "noyabr-case_1", "dekabr-case_1"};

static char *wdays[] = {"voskresenie", "ponedelnik", "vtornik",
	"sreda", "chetverg", "pyatnica", "subbota"};

static unsigned lastnum(unsigned num)
{
	unsigned divider = 100;
	while(num > 19)
	{
		if(num < divider)
			divider /= 10;
		num %= divider;
	}
	return num;
}

unsigned RussianTranslator::spell(BayonneSession *s, unsigned count, const char *text)
{
        char ch, ch1;
	unsigned i;

        if(!text)
                return count;

        while(*text && count < (MAX_LIST - 1))
        {
                ch = tolower(*(text));
		ch1 = *text;
                ++text;
				if(ch >= 'a' && ch <= 'z')
				{
                        count = addItem(s, count, letters[ch - 'a']);
						continue;
				}
                else if(ch >= '0' && ch <= '9')
				{
                        count = addItem(s, count, lows[ch - '0']);
						continue;
				}
		for(i = 0; i < sizeof(syms) / sizeof(sym_t); ++i)
		{
			if(syms[i].value == ch1)
			{
				count = addItem(s, count, syms[i].name);
				break;
			}
		}

                switch(ch1)
                {
                case '.':
                        count = addItem(s, count, "celaya");
                        break;
                }
        }
        return count;
} 

unsigned RussianTranslator::sayhour(BayonneSession *s, unsigned count, const char *cp)
{
	return RussianTranslator::saytime(s, count, cp);
}

unsigned RussianTranslator::number(BayonneSession *s, unsigned count, const char *cp)
{
	long num = atol(cp);

	if(num > 999 || count > MAX_LIST - 10)
		return count;

	if(num >= 100)
	{
		count = addItem(s, count, huns[num / 100]);
                num %= 100;      
                if(!num)
                        return count;
        }
 
	if(num > 20)
	{
		count = addItem(s, count, tens[num / 10]);
		num %= 10;
		if(!num)
			return count;
	}

	return addItem(s, count, lows[num]);
}

unsigned RussianTranslator::number_f(BayonneSession *s, unsigned count, const char *cp)
{
	long num = atol(cp);

	if(num > 999 || count > MAX_LIST - 10)
		return count;

	if(num >= 100)
	{
		count = addItem(s, count, huns[num / 100]);
                num %= 100;      
                if(!num)
                        return count;
        }
 
	if(num > 20)
	{
		count = addItem(s, count, tens[num / 10]);
		num %= 10;
		if(!num)
			return count;
	}

	switch(num)
	{
	case 1:
		return addItem(s, count, "1-feminine");
	case 2:
		return addItem(s, count, "2-feminine");
	default:
		return addItem(s, count, lows[num]);
	}
}


unsigned RussianTranslator::number_n(BayonneSession *s, unsigned count, const char *cp)
{
	long num = atol(cp);

	if(num > 999 || count > MAX_LIST - 10)
		return count;

	if(num >= 100)
	{
		count = addItem(s, count, huns[num / 100]);
                num %= 100;      
                if(!num)
                        return count;
        }
 
	if(num > 20)
	{
		count = addItem(s, count, tens[num / 10]);
		num %= 10;
		if(!num)
			return count;
	}

	switch(num)
	{
	case 1:
		return addItem(s, count, "1-neuter");
	default:
		return addItem(s, count, lows[num]);
	}
}

unsigned RussianTranslator::number_neuter(BayonneSession *s, unsigned count, const char *cp)
{
	long num = atol(cp);

	if(num > 999 || count > MAX_LIST - 10)
		return count;

	if(num >= 100)
	{
		if(!(num % 100))
			return addItem(s, count, huns_neu[num / 100]);

		count = addItem(s, count, huns[num / 100]);
                num %= 100;      
        }
 
	if(num > 20)
	{
		if(!(num % 10))
			return addItem(s, count, tens_neu[num / 10]);
			
		count = addItem(s, count, tens[num / 10]);
		num %= 10;
	}

	if(num)
		return addItem(s, count, lows_neu[num]);
	return count;
}

unsigned RussianTranslator::sayorder(BayonneSession *s, unsigned count, const char *cp)
{
	long num = atol(cp);

	if(num > 999 || count > MAX_LIST - 10)
		return count;

	if(num >= 100)
	{
		if(!(num % 100))
			return addItem(s, count, huns_ord[num / 100]);

		count = addItem(s, count, huns[num / 100]);
                num %= 100;      
        }
 
	if(num > 20)
	{
		if(!(num % 10))
			return addItem(s, count, tens_ord[num / 10]);
			
		count = addItem(s, count, tens[num / 10]);
		num %= 10;
	}

	if(num)
		return addItem(s, count, lows_ord[num]);
	return count;
}

unsigned RussianTranslator::number_case1(BayonneSession *s, unsigned count, const char *cp)
{
	long num = atol(cp);

	if(num > 999 || count > MAX_LIST - 10)
		return count;

	if(num >= 100)
	{
		if(!(num % 100))
			return addItem(s, count, huns_oc1[num / 100]);

		count = addItem(s, count, huns[num / 100]);
                num %= 100;      
        }
 
	if(num > 20)
	{
		if(!(num % 10))
			return addItem(s, count, tens_oc1[num / 10]);
			
		count = addItem(s, count, tens[num / 10]);
		num %= 10;
	}

	if(num)
		return addItem(s, count, lows_oc1[num]);
	return count;
}

unsigned RussianTranslator::weekday(BayonneSession *s, unsigned count,
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
	return addItem(s, count, wdays[ti->tm_wday]);
}

unsigned RussianTranslator::saytime(BayonneSession *s, unsigned count, const char *cp)
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
	switch(lastnum(atoi(buf)))
	{
	case 1:
		count = addItem(s, count, "chas");
		break;
	case 2:
	case 3:
	case 4:
		count = addItem(s, count, "chas-case_1");
		break;
	default:
		count = addItem(s, count, "chas-multiple-case_1");
	}

	if(!min)
		return count;
	
	snprintf(buf, sizeof(buf), "%d", min);
	count = number_f(s, count, buf);
        switch(lastnum(atoi(buf)))
        {
        case 1:
                count = addItem(s, count, "minuta");
                break;
        case 2:
        case 3:
        case 4:
                count = addItem(s, count, "minuta-case_1");
                break;
        default:
                count = addItem(s, count, "minuta-multiple-case_1");
        } 

	return count;
}

unsigned RussianTranslator::saydate(BayonneSession *s, unsigned count, const char *cp)
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
	count = number_neuter(s, count, buf);
	count = addItem(s, count, months[month]);
	if(year == 2000)
	{
		count = addItem(s, count, "2000-order-case_1");
		return addItem(s, count, "god-case_1");
	}
	if(year > 2000)
		count = addItem(s, count, "2000");
	else
		count = addItem(s, count, "1000");
	snprintf(buf, sizeof(buf), "%d", year % 1000);
	count = number_case1(s, count, buf);
	return addItem(s, count, "god-case_1");
}

unsigned RussianTranslator::sayday(BayonneSession *s, unsigned count, const char *cp)
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
	count = number_neuter(s, count, buf);
	return addItem(s, count, months[month]);
}

unsigned RussianTranslator::saynumber(BayonneSession *s, unsigned count, const char *cp)
{
        long num;
	char nbuf[32];
	bool zero = true;
	float val = 0;

        if(!cp || count > MAX_LIST - 20)
                return count;
            
	num = atol(cp);
	if(num < 0)
	{
		count = addItem(s, count, "minus");
		++cp;
		num = -num;
	}

	if(strchr(cp, '.'))
	{
		val = (float)atof(cp);
		val = val - int(val);	
	}

	if(num > 999999999)
	{
		zero = false;
		snprintf(nbuf, sizeof(nbuf), "%ld", num / 1000000000); 
		count = number(s, count, nbuf);
		switch(lastnum(atoi(nbuf)))
		{
		case 1:
			count = addItem(s, count, "milliard");
			break;
		case 2:
		case 3:
		case 4:
			count = addItem(s, count, "milliard-case_1");
			break;
		default:
			count = addItem(s, count, "milliard-multiple-case_1");
		}
		num %= 1000000000;
	}
        if(num > 999999)
        {
                zero = false;
                snprintf(nbuf, sizeof(nbuf), "%ld", num / 1000000);
                count = number(s, count, nbuf);
		switch(lastnum(atoi(nbuf)))
		{
		case 1:
			count = addItem(s, count, "million");
			break;
		case 2:
		case 3:
		case 4:
			count = addItem(s, count, "million-case_1");
			break;
		default:
			count = addItem(s, count, "million-multiple-case_1");
		}
                num %= 1000000; 
        }  
        if(num > 999)   
        {
                zero = false;
                snprintf(nbuf, sizeof(nbuf), "%ld", num / 1000);   
                count = number_f(s, count, nbuf);
		switch(lastnum(atoi(nbuf)))
		{
		case 1:
			count = addItem(s, count, "1000");
			break;
		case 2:
		case 3:
		case 4:
			count = addItem(s, count, "1000-case_1");
			break;
		default:
			count = addItem(s, count, "1000-multiple-case_1");
			break;
		}
                num %= 1000;    
        } 

	cp = strchr(cp, '.');

	if(num || zero)
	{
		snprintf(nbuf, sizeof(nbuf), "%ld", num);
		if(cp)
			count = number_f(s, count, nbuf);
		else
			count = number(s, count, nbuf);
	}

	if(num)
		zero = false;

	if(!cp)
		return count;

	if(lastnum(atoi(nbuf)) > 1)
		count = addItem(s, count, "celaya-multiple-case_1");
	else
		count = addItem(s, count, "celaya");


	snprintf(nbuf, sizeof(nbuf), "%.3f", val);
	count = number_f(s, count, nbuf);

	num = 0;
	if(lastnum(atoi(nbuf)) > 1)
		num = 1;
	num += 10 * strlen(cp);
	switch(num)
	{
	case 10:
		return addItem(s, count, "desyataya");
	case 11:
		return addItem(s, count, "desyataya-multiple-case_1");
	case 20:
		return addItem(s, count, "sotaya");
	case 21:
		return addItem(s, count, "sotaya-multiple-case_1");
	case 30:
		return addItem(s, count, "tisyachnaya");
	case 31:
		return addItem(s, count, "tisyachnaya-multiple-case_1");
	}
	return count;
}


} // namespace
