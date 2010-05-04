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
#include <cstdarg>

namespace ost {
using namespace std;

static size_t xmlformat(char *dp, size_t max, const char *fmt, ...)
{
	va_list args;

	if(max < 0)
		return 0;

	va_start(args, fmt);
	vsnprintf(dp, max, fmt, args);
	va_end(args);
	return strlen(dp);
}

static size_t xmltext(char *dp, size_t max, const char *src)
{
    unsigned count = 0;
    while(*src && count < max) {
        switch(*src)
        {
        case '&':
            snprintf(dp + count, max - count, "&amp;");
            count += strlen(dp + count);
            ++src;
            break;
        case '<':
            snprintf(dp + count, max - count, "&lt;");
            count += strlen(dp + count);
            ++src;
            break;
        case '>':
            snprintf(dp + count, max - count, "&gt;");
            count += strlen(dp + count);
            ++src;
            break;
        case '\"':
            snprintf(dp + count, max - count, "&quot;");
            count += strlen(dp + count);
            ++src;
            break;
         case '\'':
            snprintf(dp + count, max - count, "&apos;");
            count = strlen(dp + count);
            ++src;
            break;
        default:
            dp[count++] = *(src++);
        }
    }
    return count;
}           

static size_t b64encode(char *dest, const unsigned char *src, size_t size, size_t max)
{
	static const unsigned char alphabet[65] =
        	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

	size_t count = 0;
	unsigned bits;

	while(size >= 3 && max > 4)
	{
		bits = (((unsigned)src[0])<<16) | 
			(((unsigned)src[1])<<8) | ((unsigned)src[2]);

		src += 3;
		size -= 3;
		
        	*(dest++) = alphabet[bits >> 18];
        	*(dest++) = alphabet[(bits >> 12) & 0x3f];
        	*(dest++) = alphabet[(bits >> 6) & 0x3f];
        	*(dest++) = alphabet[bits & 0x3f];
		max -= 4;
		count += 4;
	}
	*dest = 0;
	if(!size || max < 5)
		return count;

        bits = ((unsigned)src[0])<<16;
        *(dest++) = alphabet[bits >> 18];
	++count;
        if (size == 1) 
	{
       		*(dest++) = alphabet[(bits >> 12) & 0x3f];
        	*(dest++) = '=';
		count += 2;
        }
        else 
	{
        	bits |= ((unsigned)src[1])<<8;
        	*(dest++) = alphabet[(bits >> 12) & 0x3f];
        	*(dest++) = alphabet[(bits >> 6) & 0x3f];
		count += 2;
        }
	*(dest++) = '=';
	++count;
	*(dest++) = 0;
	return count;
}	

static char *parseText(char *cp)
{
	char *dp = cp;
	char *rp = cp;

	if(!cp)
		return NULL;

	while(*cp)
	{
		if(*cp != '&') 
		{
			*(dp++) = *(cp++);
			continue;
        	}
		if(!strncmp(cp, "&amp;", 5)) 
		{
            		*(dp++) = '&';
			cp += 5;
			continue;
		}
 		else if(!strncmp(cp, "&gt;", 4)) 
		{
			*(dp++) = '>';
			cp += 4;
			continue;
		}
		else if(!strncmp(cp, "&lt;", 4))
		{
			*(dp++) = '<';
			*cp += 4;
			continue;
		}
		else if(!strncmp(cp, "&quot;", 6)) 
		{
			*(dp++) = '\"';
			*cp += 6;
			continue;
		}
		else if(!strncmp(cp, "&apos;", 6)) 
		{
			*(dp++) = '\'';
			*cp += 6;
			continue;
		}
		*(dp++) = *(cp++);
	}
	*dp = 0;
	return rp;
}

static char *parseValue(char *cp, char **value, char **map)
{
	*value = NULL;
	bool base64 = false;

	if(map)
		*map = NULL;	

	while(*cp)
	{
		while(isspace(*cp))
			++cp;

		if(!strncmp(cp, "<base64>", 8))
		{
			base64 = true;
			cp += 8;
			continue;
		}

		if(!strncmp(cp, "<struct>", 8))
			return cp + 8;
		else if(!strncmp(cp, "<array>", 7))
			return cp + 7;

		if(*cp == '<' && cp[1] != '/')
		{
			if(map)
				*map = ++cp;
			while(*cp && *cp != '>')
				++cp;
			if(*cp == '>')
				*(cp++) = 0;
			continue;
		}

		*value = cp;
		while(*cp && *cp != '<')
			++cp;

		if(*cp)
			*(cp++) = 0;

		while(*cp && *cp != '>')
			++cp;
		if(!*cp)
			return cp;
		++cp;
		parseText(*value);
		return cp;
	}
	return cp;
}

static char *parseName(char *cp, char **value)
{
	char *t = NULL;

	while(isspace(*cp))
		++cp;

	if(isalnum(*cp))
		t = cp;
	while(*cp && !isspace(*cp) && *cp != '<')
		++cp;
	while(isspace(*cp))
		*(cp++) = 0;
	if(*cp != '<')
		t = NULL;
	*(cp++) = 0;
	*value = parseText(t);
	return cp;
}

BayonneRPC::BayonneRPC()
{
}

BayonneRPC::~BayonneRPC()
{
}

bool BayonneRPC::parseCall(char *cp)
{
	bool name_flag = false;
	char *value;
	char *map = NULL;
	char *method;

	params.argc = 0;
	params.count = 0;
	header.prefix = header.method = NULL;

	while(*cp)
	{
		while(isspace(*cp))
			++cp;

		if(!strncmp(cp, "<methodName>", 12))
		{
			cp = parseName(cp + 12, &method);
			if(strncmp(cp, "/methodName>", 12) || !method)
				return false;

			header.prefix = method;
			method = strchr(method, '.');
			if(method)
				*(method++) = 0;
			header.method = method;

			cp += 12;
			break;
		}
		if(!strncmp(cp, "<methodCall>", 12))
		{
			cp += 12;
			continue;
		}
		if(!strncmp(cp, "<?", 2) || !strncmp(cp, "<!", 2))
		{
			while(*cp && *cp != '>')
				++cp;
			if(*cp)
				++cp;
			continue;
		} 
		return false;
	}
	if(!header.method || !*cp)
		return false;		

	while(*cp && params.count < RPC_MAX_PARAMS)
	{
		while(isspace(*cp))
			++cp;

		if(!*cp)
			break;

		if(!strncmp(cp, "<name>", 6))
		{
			name_flag = true;
			cp = parseName(cp + 6, &params.name[params.count]);
			params.map[params.count] = map;
			if(strncmp(cp, "/name>", 6))
				return false;

			cp += 6;
			continue;
		}
		if(!strncmp(cp, "</struct>", 9) && map && !name_flag)
		{
			map = NULL;
			cp += 9;
			continue;
		}
		if(!strncmp(cp, "<param>", 7))
		{
			params.name[params.count] = params.value[params.count] = params.map[params.count] = NULL;
			++params.argc;
			cp += 7;
			continue;
		}
		if(!strncmp(cp, "<value>", 7))
		{
			params.param[params.count] = params.argc;
			cp = parseValue(cp, &value, NULL);
			if(value)
				params.value[params.count++] = value;
			else if(name_flag)
				map = params.name[params.count];
			name_flag = false;
			params.name[params.count] = params.map[params.count] = params.value[params.count] = NULL;
			continue;
		}
		if(!strncmp(cp, "</params>", 9))
			return true; 
		if(*cp == '<')
		{
			while(*cp && *cp != '>')
				++cp;
			if(*cp)
				++cp;
			else
				return false;
			continue;
		}
		return false;
	}
	return false;
}

void BayonneRPC::setComplete(BayonneSession *s)
{
	buildResponse("(sb)", 
		"session", s->getSessionId(), 
		"complete", (rpcbool_t)0);
}

const char *BayonneRPC::getParamId(unsigned short param, unsigned short offset)
{
    unsigned count = 0;
    unsigned member = 1;

    if(!offset)
        offset = 1;

	while(count < params.count)
	{
		if(params.param[count] > param)
			break;

		if(params.param[count] == param)
			if(member++ == offset)
				return (const char *)params.name[count];

		++count;
	}
	return NULL;
}

const char *BayonneRPC::getIndexed(unsigned short param, unsigned short offset)
{
	unsigned count = 0;
	unsigned member = 1;

	if(!offset)
		offset = 1;

	while(count < params.count)
	{
		if(params.param[count] > param)
			break;

		if(params.param[count] == param)
			if(member++ == offset)
				return (const char *)params.value[count];

		++count;
	}
	return NULL;
}

const char *BayonneRPC::getNamed(unsigned short param, const char *member)
{
	unsigned count = 0;

	while(count < params.count)
	{
		if(params.param[count] > param)
			break;

		if(params.param[count] == param)
			if(!strcmp(params.name[count], member))
				return (const char *)params.value[count];

		++count;
	}
	return NULL;
}
		
const char *BayonneRPC::getMapped(const char *map, const char *member)
{
	unsigned count = 0;

	while(count < params.count)
	{
		if(!strcmp(params.map[count], map))
			if(!strcmp(params.name[count], member))
				return (const char *)params.value[count];
		++count;
	}
	return NULL;
}

bool BayonneRPC::invokeXMLRPC(void)
{
	RPCNode *node = RPCNode::first;
	RPCDefine *method;

	while(node)
	{
		if(!node->prefix || stricmp(header.prefix, node->prefix))
			goto skip;

		method = node->methods;
		while(method && method->name)
		{
			if(!stricmp(method->name, header.method))
			{
				method->method(this);
				return true;
			}
			++method;
		}
skip:
		node = node->next;
	}
	return false;		
}

void BayonneRPC::sendSuccess(void)
{
	xmlformat(transport.buffer, transport.bufsize,
		"<?xml version=\"1.0\"?>\r\n"	
		"<methodResponse><params></params></methodResponse>\r\n");
}

void BayonneRPC::sendFault(int code, const char *string)
{
	size_t count = xmlformat(transport.buffer, transport.bufsize, 
		"<?xml version=\"1.0\"?>\r\n"
		"<methodResponse>\r\n"
		" <fault><value><struct>\r\n"
		"  <member><name>faultCode</name>\r\n"
		"   <value><int>%d</int></value></member>\r\n"
		"  <member><name>faultString</name>\r\n"
		"   <value><string>", code);
	count += xmltext(transport.buffer + count, transport.bufsize - count, string);
	count += xmlformat(transport.buffer + count, transport.bufsize - count,
		"</string></value></member>\r\n"
		" </struct></value></fault>\r\n"
		"</methodResponse>\r\n");
}

size_t xmlwrite(char **bufp, size_t *max, const char *fmt, ...)
{
	const char *sv;
	int iv;
	long lv;
	time_t tv;
	va_list args;
	unsigned count;
	size_t used = *max;
	char *buf = *bufp;
	struct tm *dt, dbuf;

	va_start(args, fmt);

	while(*fmt && *max > 1)
	{
		if(*fmt == '%')
		{
			count = 0;
			switch(*(++fmt))
			{
			case 't':
				tv = va_arg(args, time_t);
				dt = localtime_r(&tv, &dbuf);
                if(dt->tm_year < 1800)
                    dt->tm_year += 1900;
                count = xmlformat(buf, *max, "%04d%02d%02dT%02d:%02d:%02d",
                    dt->tm_year, dt->tm_mon + 1, dt->tm_mday,
                    dt->tm_hour, dt->tm_min, dt->tm_sec);
				break;
			case 'd':
				iv = va_arg(args, int);
				count = xmlformat(buf, *max, "%d", iv);
				break;
            case 'l':
                lv = va_arg(args, int);
                count = xmlformat(buf, *max, "%ld", lv);
                break;
			case 'q':
				sv = va_arg(args, const char *);
				if(*max > 1)
				{
					*(buf++) = '\"';
					--*max;
				}
				count = xmltext(buf, *max, sv);
				*max -= count;
				buf += count;
				if(*max > 1)
				{
					(*buf++) = '\"';
					--*max;
				}
				count = 0;
				break;	
			case 's':
				sv = va_arg(args, const char *);
				count = xmltext(buf, *max, sv);
			default:
				break;
			}
			++fmt;	
			buf += count;
			*max -= count;
		}
		else
		{
			*(buf++) = *(fmt++);
			--*max;
		}
	}

	*buf = 0;
	*bufp = buf;
	va_end(args);
	return used - *max;
}

bool BayonneRPC::buildResponse(const char *fmt, ...)
{
	rpcint_t iv;
	time_t tv;
	struct tm *dt, dbuf;
	double dv;
	const unsigned char *xp;
	size_t xsize;
	const char *sv;
	const char *valtype = "string";
	const char *name;
	bool end_flag = false;	// end of param...
	bool map_flag = false;
	bool struct_flag = false;
	bool array_flag = false;
	size_t count = transport.bufused;
	size_t max = transport.bufsize;
	char *buffer = transport.buffer;
	va_list args;
	va_start(args, fmt);

	switch(*fmt)
	{
	case '^':
	case '(':
	case '[':
	case '<':
	case '{':
	case 's':
	case 'i':
	case 'd':
	case 't':
	case 'b':
	case 'x':
		count += xmlformat(buffer + count, max - count,
			"<?xml version=\"1.0\"?>\r\n"
			"<methodResponse>\r\n"
			" <params><param>\r\n");
		break;
	case '!':
		array_flag = true;
		break;
	case '+':
		map_flag = true;
	case '.':
		struct_flag = true;
	}

	if(!*fmt)
		end_flag = true;

	while(*fmt && *fmt != '$' && count < max - 1 && !end_flag)
	{
		switch(*fmt)
		{
		case '[':
			count += xmlformat(buffer + count, max - count,
				" <value><array><data>\r\n");
		case ',':
			array_flag = true;
			break;
		case ']':
			array_flag = false;
			count += xmlformat(buffer + count, max - count,
				" </data></array></value>\r\n");
			end_flag = true;
			break;
		case '}':
		case '>':
			map_flag = struct_flag = false;
			count += xmlformat(buffer + count, max - count,	
				" </struct></value></member>\r\n"
				" </struct></value>\r\n");
			end_flag = true;
			break;
		case ';':
		case ':':
			name = va_arg(args, const char *);
			count += xmlformat(buffer + count, max - count,
				" </struct></value></member>\r\n"
				" <member><name>%s<value><struct>\r\n", name);
			break;
		case '{':
		case '<':
			name = va_arg(args, const char *);
			count += xmlformat(buffer + count, max - count,
				" <value><struct>\r\n"
				" <member><name>%s</name><value><struct>\r\n", name);
			struct_flag = map_flag = true;
			break;
		case '(':
			struct_flag = true;
			count += xmlformat(buffer + count, max - count, " <value><struct>\r\n");
			break;
		case ')':
			struct_flag = false;
			if(!map_flag && !array_flag)
				end_flag = true;
			count += xmlformat(buffer + count, max - count, 
				" </struct></value>\r\n");
			break;
		case 's':
		case 'i':
		case 'b':
		case 'd':
		case 't':
		case 'x':
		case 'm':
			switch(*fmt)
			{
			case 'm':
				valtype = "string";
				break;
			case 'd':
				valtype = "double";
				break;
			case 'b':
				valtype = "boolean";
				break;
			case 'i':
				valtype = "i4";
				break;
			case 's':
				valtype = "string";
				break;
			case 't':
				valtype = "dateTime.iso8601";
				break;
			case 'x':
				valtype = "base64";
			}
			if(struct_flag && *fmt == 'm')
			{
				if(count > max - 60)
					goto skip;
				sv = va_arg(args, const char *);
				while(sv && *sv)
				{
					count += xmlformat(buffer + count, max - count,
						"  <member><name>");
					while(*sv && *sv != ':' && *sv != '=' && count < max - 35)
					{
						buffer[count++] = tolower(*sv);
						++sv;
					}
					buffer[count] = 0;
					count += xmlformat(buffer + count, max - count, 
						"</name>\r\n"
						"   <value><%s>", valtype);
					if(*sv == ':' || *sv == '=')
						++sv;
					else
						sv="";
					while(*sv && *sv != ';' && count < max - 20)
					{
						switch(*sv)
						{
						case '<':
							count += xmlformat(buffer + count, max - count, "&lt;");
							break;
						case '>':
							count += xmlformat(buffer + count, max - count, "&gt;");
							break;
						case '&':
							count += xmlformat(buffer + count, max - count, "&amp;");
							break;
						case '\"':
							count += xmlformat(buffer + count, max - count, "&quot;");
							break;
						case '\'':
							count += xmlformat(buffer + count, max - count, "&apos;");
							break;
						default:
							buffer[count++] = *sv;
						}
						++sv;
					}
					count += xmlformat(buffer + count, max - count,
						"</%s></value></member>\r\n", valtype);
				}
				goto skip;
			}
			if(struct_flag)
			{
				name = va_arg(args, const char *);
				count += xmlformat(buffer + count, max - count,
					"  <member><name>%s</name>\r\n"  
					"   <value><%s>", name, valtype);
			}
			else
				count += xmlformat(buffer + count, max - count,
					"  <value><%s>", valtype);

			switch(*fmt)
			{
			case 'x':
				xp = va_arg(args, const unsigned char *);
				xsize = va_arg(args, size_t);
				count += b64encode(buffer + count, xp, xsize, max - count);
				break;
			case 's':
				sv = va_arg(args, const char *);
				if(!sv)
					sv = "";
				count += xmltext(buffer + count, max - count, sv);
				break;
			case 'd':
				dv = va_arg(args, double);
				count += xmlformat(buffer + count, max - count, "%f", dv);
				break;
			case 'i':
			case 'b':
				iv = va_arg(args, rpcint_t);
				if(*fmt == 'b' && iv)
					iv = 1;
				count += xmlformat(buffer + count, max - count, "%ld", (long)iv);
				break;
			case 't':
				tv = va_arg(args, time_t);
				dt = localtime_r(&tv, &dbuf);
				if(dt->tm_year < 1800)
					dt->tm_year += 1900;
				count += xmlformat(buffer + count, max - count,
					"%04d%02d%02dT%02d:%02d:%02d",
					dt->tm_year, dt->tm_mon + 1, dt->tm_mday,
					dt->tm_hour, dt->tm_min, dt->tm_sec); 
				break;
			}
			if(struct_flag)
				count += xmlformat(buffer + count, max - count,
					"</%s></value></member>\r\n", valtype);
			else
				count += xmlformat(buffer + count, max - count,
					"</%s></value>\r\n", valtype);
skip:
			if(!struct_flag && !array_flag)
				end_flag = true;
		}
		++fmt;
	}
	
	if(*fmt == '$' || end_flag)
		count += xmlformat(buffer + count, max - count,
			" </param></params>\r\n"
			"</methodResponse>\r\n");

	va_end(args);
	transport.bufused = count;
	if(transport.bufused < transport.bufsize - 1)
		return true;

	return false;
}
		
} // namespace
