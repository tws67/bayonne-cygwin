/*
 * Please do not edit this file.
 * It was generated using rpcgen.
 */

#include "bayonne_rpc.h"
#include <stdio.h>
#include <stdlib.h>
#include <rpc/pmap_clnt.h>
#include <string.h>
#include <memory.h>
#include <sys/socket.h>
#include <netinet/in.h>

#ifndef SIG_PF
#define SIG_PF void(*)(int)
#endif
/* Copyright (C) 2005 Open Source Telecom Corporation.
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License version 2 as published by the Free Software Foundation.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 * 
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.                                                 
 */

void
bayonne_program_2(struct svc_req *rqstp, register SVCXPRT *transp)
{
	union {
		bayonne_start bayonne_start_2_arg;
		bayonne_session bayonne_stop_2_arg;
	} argument;
	char *result;
	xdrproc_t _xdr_argument, _xdr_result;
	char *(*local)(char *, struct svc_req *);

	switch (rqstp->rq_proc) {
	case NULLPROC:
		(void) svc_sendreply (transp, (xdrproc_t) xdr_void, (char *)NULL);
		return;

	case BAYONNE_RELOAD:
		_xdr_argument = (xdrproc_t) xdr_void;
		_xdr_result = (xdrproc_t) xdr_bayonne_error;
		local = (char *(*)(char *, struct svc_req *)) bayonne_reload_2_svc;
		break;

	case BAYONNE_SHUTDOWN:
		_xdr_argument = (xdrproc_t) xdr_void;
		_xdr_result = (xdrproc_t) xdr_bayonne_error;
		local = (char *(*)(char *, struct svc_req *)) bayonne_shutdown_2_svc;
		break;

	case BAYONNE_STATUS:
		_xdr_argument = (xdrproc_t) xdr_void;
		_xdr_result = (xdrproc_t) xdr_bayonne_status;
		local = (char *(*)(char *, struct svc_req *)) bayonne_status_2_svc;
		break;

	case BAYONNE_START:
		_xdr_argument = (xdrproc_t) xdr_bayonne_start;
		_xdr_result = (xdrproc_t) xdr_bayonne_result;
		local = (char *(*)(char *, struct svc_req *)) bayonne_start_2_svc;
		break;

	case BAYONNE_STOP:
		_xdr_argument = (xdrproc_t) xdr_bayonne_session;
		_xdr_result = (xdrproc_t) xdr_bayonne_error;
		local = (char *(*)(char *, struct svc_req *)) bayonne_stop_2_svc;
		break;

	default:
		svcerr_noproc (transp);
		return;
	}
	memset ((char *)&argument, 0, sizeof (argument));
	if (!svc_getargs (transp, (xdrproc_t) _xdr_argument, (caddr_t) &argument)) {
		svcerr_decode (transp);
		return;
	}
	result = (*local)((char *)&argument, rqstp);
	if (result != NULL && !svc_sendreply(transp, (xdrproc_t) _xdr_result, result)) {
		svcerr_systemerr (transp);
	}
	if (!svc_freeargs (transp, (xdrproc_t) _xdr_argument, (caddr_t) &argument)) {
		fprintf (stderr, "%s", "unable to free arguments");
		exit (1);
	}
	return;
}
