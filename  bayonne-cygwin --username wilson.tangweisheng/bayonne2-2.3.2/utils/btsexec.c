/* Copyright (C) 2005 David Sugar, Tycho Softworks
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
#include <sys/stat.h>
#include "private.h"

static char btspaths[] = CONFIG_FILES "/server.conf";
static char varpath[128] = VAR_FILES;
static char ramfs[128] = "/dev/shm/.bayonne";
static char tmpfs[128] = "/tmp/.bayonne";
static char audio[128] = PROMPT_FILES;
static char libexec[128] = LIBEXEC_FILES;
static char control[128] = RUN_FILES;

static unsigned strlower(char *cp)
{
	unsigned len = 0;
	while(isalnum(*cp))
	{
		*cp = tolower(*cp);
		++cp;
		++len;
	}
	return len;
}

static char *getpath(char *cp)
{
	char *ep;
	if(isspace(*cp) || *cp == '=')
		++cp;
	ep = strchr(cp, ';');
	if(!ep)
		ep = strchr(cp, '#');
	if(!ep)
		ep = cp + strlen(cp) - 1;
	else
	{
		*ep = 0;
		--ep;
	}
	while(ep > cp && isspace(*ep))
	{
		*ep = 0;
		--ep;
	}
	return cp;
}

static void libexec_parse(void)
{
	char lbuf[256];
	char ebuf[128];
	const char *tsid = getenv("PORT_TSESSION");
	char *value, *ep;
	fprintf(stdout, "%s HEAD\n", tsid);
	fflush(stdout);
	for(;;)
	{
		if(!fgets(lbuf, sizeof(lbuf), stdin) || feof(stdin))
			break;
		if(!isalnum(lbuf[0]))
			break;
		value = strchr(lbuf, ':');
		if(!value)
			continue;
		*value = 0;
		value += 2;
		ep = strchr(value, '\r');
		if(ep)
			*ep = 0;
		ep = strchr(value, '\n');
		if(ep)
			*ep = 0;
		snprintf(ebuf, sizeof(ebuf), "HEAD_%s", lbuf);
		setenv(ebuf, value, 1);
	}
	fprintf(stdout, "%s ARGS\n", tsid);
	fflush(stdout);
	for(;;)
	{
		if(!fgets(lbuf, sizeof(lbuf), stdin) || feof(stdin))
			break;
		if(!isalnum(lbuf[0]))
			break;
		value = strchr(lbuf, ':');
		if(!value)
			continue;
		*value = 0;
		value += 2;
		ep = strchr(value, '\r');
		if(ep)
			*ep = 0;
		ep = strchr(value, '\n');
		if(ep)
			*ep = 0;
		snprintf(ebuf, sizeof(ebuf), "ARGS_%s", lbuf);
		setenv(ebuf, value, 1);
	}
}
	
int main(int argc, char **argv)
{
	char lbuf[256];
	char *cp;
	unsigned len;
	char *ext = NULL;
	char *program = "/bin/sh";
	struct group *grp;
	FILE *fp;

        cp = getenv("SERVER_SOFTWARE");
        if(!strcmp(cp, "bayonne"))
                goto starting; 

	if(access("/dev/shm", W_OK))
		strcpy(ramfs, tmpfs);

	umask(007);
	grp = getgrnam("bayonne");
	if(grp)
		setgid(grp->gr_gid);
	endgrent();

	fp = fopen(btspaths, "r");
	if(!fp)
		fp = fopen("/etc/bayonne/server.conf", "r");
	if(!fp)
		goto launch;

	for(;;)
	{
		if(!fgets(lbuf, sizeof(lbuf), fp) || feof(fp))
		{
			fclose(fp);
			goto launch;
		}
		cp = lbuf;		
		while(isspace(*cp))
			++cp;

		if(*cp != '[')
			continue;

		++cp;
		while(isspace(*cp))
			++cp;
		len = strlower(cp);
		if(!len)
			continue;
		if(!strncmp(cp, "paths", len))
			break;
	}
	for(;;)
	{
		if(!fgets(lbuf, sizeof(lbuf), fp) || feof(fp))
			break;

		cp = lbuf;
		while(isspace(*cp))
			++cp;
		if(*cp == '[')
			break;
		if(!isalpha(*cp))
			continue;
		len = strlower(cp);
		if(!strncmp(cp, "libexec", len))
			strcpy(libexec, getpath(cp + len));
		else if(!strncmp(cp, "datafiles", len))
			strcpy(varpath, getpath(cp + len));
		else if(!strncmp(cp, "prompts", len))
			strcpy(audio, getpath(cp + len));
		else if(!strncmp(cp, "runfiles", len))
			strcpy(control, getpath(cp + len));
		else if(!strncmp(cp, "tmpfs", len))
			strcpy(ramfs, getpath(cp + len));
		else if(!strncmp(cp, "tmp", len))
			strcpy(tmpfs, getpath(cp + len));
	}
	fclose(fp);		
	
launch:
	setenv("BAYONNE_HOME", varpath, 1);
	setenv("BAYONNE_AUDIO", audio, 1);
	setenv("BAYONNE_LIBEXEC", libexec, 1);
	setenv("BAYONNE_CONFIG", btspaths, 1);
	setenv("BAYONNE_CONTROL", control, 1);
	setenv("BAYONNE_TMPFS", tmpfs, 1);
	setenv("BAYONNE_RAMFS", ramfs, 1);
	setenv("PERL5LIB", libexec, 1);
	setenv("PYTHONLIB", libexec, 1);

starting:
	if(argv[1])
		ext = strrchr(argv[1], '.');
	if(ext)
	{
		if(!strcmp(ext, ".pl"))
			program = PERL_PROGRAM;
		else if(!strcmp(ext, ".py"))
			program = PYTHON_PROGRAM;
		else if(!strcmp(ext, ".php"))
			program = PHP_PROGRAM;
		else if(!strcmp(ext, ".exe") || !strcmp(ext, ".il"))
			program = PNET_PROGRAM;
	}
	ext = strrchr(argv[0], '-');
	if(ext)
	{
		if(!strcmp(ext, "-perl"))
			program = PERL_PROGRAM;
		else if(!strcmp(ext, "-python"))
			program = PYTHON_PROGRAM;
		else if(!strcmp(ext, "-php"))
			program = PHP_PROGRAM;
	}
	if(!strcmp(program, "/bin/sh") && getenv("PORT_TSESSION"))
		libexec_parse();
	argv[0] = program;
	execv(program, argv);
	exit(-1);
}
 
