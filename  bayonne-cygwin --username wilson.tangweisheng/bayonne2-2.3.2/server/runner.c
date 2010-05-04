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
#include <sys/wait.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>
#include "private.h"

#ifdef	LIBEXEC_USES_PERL
#include "perlxsi.c"

static PerlInterpreter *my_perl;
#endif

typedef struct
{
	int pid;
	char tsid[16];
}	slot_t;

static slot_t *slot = NULL;
static unsigned timeslots = 0, bufsize;
static const char *libexec;
static const char *sysexec;
static int server_pid = 0;
static int interval = 5;
static struct sigaction timer_act, timer_old;
static struct sigaction child_act, child_old;

static void stop(void)
{
	unsigned count = 0;
	if(!slot || !timeslots)
		return;

	while(count < timeslots)
	{
		if(slot[count].pid)
			kill(slot[count].pid, SIGTERM);
		++count;
	}
}

static void timer(int signo)
{
	if(server_pid && kill(server_pid, 0))
	{
		fprintf(stderr, "libexec exiting; server lost\n");
		stop();
		exit(0);
	}	
	alarm(interval);
}

static void child(int signo)
{
	int status, pid;
	unsigned count;
	char buf[65];

	while((pid = wait3(&status, WNOHANG, 0)) > 0)
	{
		count = 0;
		while(count < timeslots)
		{
			if(slot[count].pid == pid)
				break;
			++count;
		}

		if(count >= timeslots)
		{
			fprintf(stderr, "libexec exiting; unknown pid=%d", pid);
			continue;
		}
		status = WEXITSTATUS(status);
		fprintf(stderr, "libexec exiting; timeslot=%d, pid=%d, status=%d\n", count, pid, status);
		snprintf(buf, sizeof(buf), "%s exit %d\n", slot[count].tsid, status);
		write(1, buf, strlen(buf));
		slot[count].tsid[0] = 0;
		slot[count].pid = 0;
	}
}

static char *newString(const char *c)
{
	unsigned len = strlen(c);
	char *n = (char *)malloc(len + 1);
	strcpy(n, c);
	return n;
}

static char *setString(char *buf, unsigned len, const char *t)
{
	snprintf(buf, len, "%s", t);
	return buf;
}

static void execute(unsigned ts, const char *cmd)
{
	int pid, fd;
	char path[256];
	char fname[32];
	char buf[256];
	char *args[12];
	unsigned count = 3;
	char *ext, *dp;
	unsigned diff;
	int argc;
	int rts = -1;	
	const char *path_tmp = getenv("SERVER_TMP");
	const char *path_bts = getenv("SHELL_BTSEXEC");
	const char *path_mac = getenv("SERVER_LIBEXEC");

	if(!strncmp(cmd, "lib::", 5))
		snprintf(path, sizeof(path), "%s/%s", path_mac, cmd + 5);
	else if(!strncmp(cmd, "exec::", 6))
		snprintf(path, sizeof(path), "%s/%s", sysexec, cmd + 6);
	else
		snprintf(path, sizeof(path), "%s/%s", libexec, cmd);
	snprintf(fname, sizeof(fname), "%s/.libexec-%d", path_tmp, ts);

	ext = strrchr(path, '.');
	dp = strrchr(path, '/');
	if(!ext)
		goto normal;

	diff = dp - path;

	if(ext && !strcmp(ext, ".sh") && path_bts)
	{
		args[0] = (char *)path_bts;
		args[1] = newString(path);
		args[2] = NULL;
		argc = 2;
		setString(path, sizeof(path), path_bts);
		goto launch;
	}

	if(ext && (!strcmp(ext, ".exe") || !strcmp(ext, ".il")))
	{
		args[0] = PNET_PROGRAM;
		args[1] = newString(path);
		args[2] = NULL;
		argc = 2;
		setString(path, sizeof(path), PNET_PROGRAM);
		goto launch;
	}

        if(ext && !strcmp(ext, ".class"))
        {
		*ext = 0;
		args[0] = JAVA_PROGRAM;
		snprintf(buf, sizeof(buf), "-Dbayonne.home=%s",
			getenv("SERVER_PREFIX"));
		args[1] = newString(buf);
		snprintf(buf, sizeof(buf), "-Dbayonne.ramfs=%s",
			getenv("SERVER_TMPFS"));
		args[2] = newString(buf);
		snprintf(buf, sizeof(buf), "-Dbayonne.tmpfs=%s",
			getenv("SERVER_TMP"));
		args[3] = newString(buf);
                snprintf(buf, sizeof(buf), "-Dbayonne.tsid=%s", slot[ts].tsid);
                args[4] = newString(buf);
                args[5] = newString(++dp);
                args[6] = NULL;
		argc = 6;
		setString(path, sizeof(path), JAVA_PROGRAM);
                goto launch;       
        }  

	if(ext && !strcmp(ext, ".jar"))
	{
		args[0] = JAVA_PROGRAM;
                snprintf(buf, sizeof(buf), "-Dbayonne.home=%s",
                        getenv("SERVER_PREFIX"));
                args[1] = newString(buf);
                snprintf(buf, sizeof(buf), "-Dbayonne.ramfs=%s", 
                        getenv("SERVER_TMPFS"));       
                args[2] = newString(buf);      
                snprintf(buf, sizeof(buf), "-Dbayonne.tmpfs=%s",
                        getenv("SERVER_TMP"));
                args[3] = newString(buf);  
		snprintf(buf, sizeof(buf), "-Dbayonne.tsid=%s", slot[ts].tsid);
		args[4] = newString(buf);
		args[5] = "-jar";
		args[6] = newString(++dp);
		args[7] = NULL;
		argc = 7;
		setString(path, sizeof(path), JAVA_PROGRAM);
		goto launch;
	}

normal:
	args[0] = path;
	args[1] = NULL;
	argc = 1;

launch:
	pid = fork();
	if(pid)
	{
		slot[ts].pid = pid;

		fprintf(stderr, "libexec starting; timeslot=%d, pid=%d, cmd=%s\n",
				ts, pid, cmd);
		return;
	}

	alarm(0);
	sigaction(SIGCHLD, &child_old, NULL);
	sigaction(SIGALRM, &timer_old, NULL);
	remove(fname);
	mkfifo(fname, 0660);

	snprintf(buf, sizeof(buf), "%d", ts);
	setenv("PORT_TIMESLOT", newString(buf), 1);
	setenv("PORT_TSESSION", slot[ts].tsid, 1);

	snprintf(buf, sizeof(buf), "%s start %d %s\n",
		slot[ts].tsid, getpid(), fname);

	fd = open(fname, O_RDWR);
	dup2(fd, 0);

	while(count < 20)
		close(count++);
	write(1, buf, strlen(buf));

#ifdef	LIBEXEC_USES_PERL
	ext = strrchr(path, '.');
	if(ext && !strcmp(ext, ".pl") && argc < 2)
	{
		args[1] = newString(path);
		args[0] = PERL_PROGRAM;
		args[2] = NULL;
		perl_parse(my_perl, xs_init, 2, args, (char **)NULL);
		perl_run(my_perl);
		perl_destruct(my_perl);
		perl_free(my_perl);
		exit(0);
	}
#endif	
	execv(path, args);
	sleep(1);
	exit(rts);		
}

int main(int argc, char **argv)
{
	char *buf, *p;
	int len;
	unsigned ts;

	if(argc < 6)
		exit(-1);

	timeslots = atoi(argv[1]);
	bufsize = atoi(argv[2]);
	interval = atoi(argv[3]);
	libexec = argv[4];
	sysexec = argv[5];

#ifdef	LIBEXEC_USES_PERL
	my_perl = perl_alloc();
	perl_construct(my_perl);
#endif

	memset(&child_act, 0, sizeof(child_act));
	memset(&timer_act, 0, sizeof(timer_act));
	timer_act.sa_handler = timer;
	child_act.sa_handler = child;
	sigemptyset(&timer_act.sa_mask);
	sigemptyset(&child_act.sa_mask);
#ifdef	SA_INTERRUPT
	timer_act.sa_flags = SA_INTERRUPT;
#endif
	sigaddset(&child_act.sa_mask, SIGALRM);
#ifdef	SA_RESTART
	child_act.sa_flags |=SA_RESTART;
#endif
	sigaction(SIGALRM, &timer_act, &timer_old);
	sigaction(SIGCHLD, &child_act, &child_old); 

	slot = (slot_t *)malloc(sizeof(slot_t) * timeslots);
	memset(slot, 0, sizeof(slot_t) * timeslots);
	alarm(interval);
	buf = (char *)malloc(bufsize);

	for(;;)
	{
		len = read(0, buf, bufsize);
		if(len != bufsize)
			continue;

		if(!strcmp(buf, "down"))
		{
			fprintf(stderr, "libexec exiting; server shutdown\n");
			stop();
			exit(0);
		}

		if(!strncmp(buf, "serv", 4))
			server_pid = atoi(buf + 4);

                p = strchr(buf, ' ');
                if(!p)
                        continue;

                *(p++) = 0;
                ts = atoi(buf);
                strcpy(slot[ts].tsid, buf);
                slot[ts].pid = 0;
                execute(ts, p); 
	}
}

