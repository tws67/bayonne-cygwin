/* Copyright (C) 2006 David Sugar, Tycho Softworks
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */                      

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <grp.h>
#include <fcntl.h>
#include <time.h>
#include "private.h"

static char *cc=NULL;
static char *to=NULL;
static char *reply=NULL;
static char *from = NULL;
static char *subject = NULL;
static char *body = NULL;
static char *data = NULL;
static char *mime=NULL;
static char *name = NULL;
static char *errors = NULL;
static char *mailer = "Bayonne Server " VERSION;

typedef enum
{
	doc_attach,
	doc_inline
} doctype_t;

static doctype_t type = doc_attach;
static char divider[65];

static void to64(const char *file)
{
	unsigned char buf[4];
	int len = 3, col = 0;
	int fd = open(file, O_RDONLY);
	static char base64[] =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

	if(fd < 0)
		return;

	while(len == 3)
	{
		memset(buf, 0, 4);
		len = read(fd, buf, 3);
		if(len < 1)
			break;

		buf[3] = 3 - len;

		fputc(base64[buf[0] >> 2], stdout);
		fputc(base64[((buf[0] & 0x3) << 4) | 
			((buf[1] & 0xf0) >> 4)], stdout);
		if(buf[3] == 2)
			fputs("==", stdout);
		else if(buf[3])
		{
			fputc(base64[((buf[1] & 0xf) << 2) |
				((buf[2] & 0xc0) >> 6)], stdout);
			fputc('=', stdout);
		}
		else
		{
                        fputc(base64[((buf[1] & 0xf) << 2) |
                                ((buf[2] & 0xc0) >> 6)], stdout);
			fputc(base64[buf[2] & 0x3f], stdout);
		}

		col += 4;
		if(col > 72)
		{
			fputc('\n', stdout);
			col = 0;
		}
	}
	if(col)
		fputc('\n', stdout);
	close(fd);
	fflush(stdout);	
}		

int main(int argc, char **argv)
{
	FILE *fp;
	int opt;
	char buffer[256];
	char *fn;
	
	snprintf(divider, sizeof(divider), 
		"_=_BTS_%04x%04lx", getpid(), time(NULL));

	while((opt = getopt(argc, argv, "i:a:e:f:t:c:b:s:m:r:n:")) != -1)
		switch(opt)
		{
		case 'e':
			errors = optarg;
			break;
		case 'a':
			type = doc_attach;
			data = optarg;
			break;
		case 'i':
			type = doc_inline;
			data = optarg;
			break;
		case 'f':
			from = optarg;
			break;
		case 't':
			to = optarg;
			break;
		case 's':
			subject = optarg;
			break;
		case 'r':
			reply = optarg;
			break;
		case 'c':
			cc = optarg;
			break;
		case 'm':
			mime = optarg;
			break;
		case 'b':
			body = optarg;
			break;
		case 'n':
			name = optarg;
			break;
		}

	if(errors && !*errors)
		errors = NULL;

	if(from && !*from)
		from = NULL;

	if(to && !*to)
		to = NULL;

	if(cc && !*cc)
		cc = NULL;

	if(subject && !*subject)
		subject = NULL;

	if(body && !*body)
		body = NULL;

	if(reply && !*reply)
		reply = NULL;

	if(mime && !*mime)
		mime = NULL;

	if(data && !*data)
		data = NULL;	

	if(from)
		printf("From: %s\n", from);

	if(to)
		printf("To: %s\n", to);

	if(cc)
		printf("Cc: %s\n", cc);

	if(reply)
		printf("Reply-To: %s\n", reply);

	if(subject)
		printf("Subject: %s\n", subject);

	if(errors)
		printf("Errors-To: %s\n", errors);

	printf("X-Mailer: %s\n", mailer);

	if(mime)
	{
		printf("MIME-Version: 1.0\n");
		printf("Content-Type: multipart/mixed; boundary=\"%s\"\n", divider);
	}

	printf("\n");

	if(mime)
	{
		printf("This is a multi-part message in MIME format.\n\n");
		printf("--%s\n", divider);
	}
	if(mime && body)
	{
		printf("Content-Type: text/plain; charset=us-ascii\n\n");
		
		fp = fopen(body, "r");
		for(;;)
		{
			if(!fgets(buffer, sizeof(buffer), fp) || feof(fp))
				break;
			fputs(buffer, stdout);
		}
		fclose(fp);
		if(data)				
			printf("\n--%s\n", divider);
		else
		{
			printf("\n--%s--\n", divider);
			exit(0);
		}
	}		

	if(!mime && body)
		data = body;

	if(!mime)
	{
		fp = fopen(data, "r");
		if(!fp)
			exit(-1);
		for(;;)
		{
			if(!fgets(buffer, sizeof(buffer), fp) || feof(fp))
				break;
			fputs(buffer, stdout);
		}
		fclose(fp);
		exit(0);
	}

	if(!data)
	{
		printf("\n--%s--\n", divider);
		exit(0);
	}

	fn = strrchr(data, '/');
	if(fn)
		++fn;
	else
		fn = data;

	printf("Content-Type: %s; name=\"%s\"\n", mime, fn);
	if(type == doc_inline)
		printf("Content-Disposition: inline\n");
	printf("Content-Transfer-Encoding: base64\n\n");
	to64(data);

	printf("\n--%s--\n", divider);

	exit(0);
}
	
