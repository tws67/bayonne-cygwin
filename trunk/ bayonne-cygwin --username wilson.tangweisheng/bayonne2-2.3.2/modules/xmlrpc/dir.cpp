// Copyright (C) 2005 Open Source Telecom Corporation.
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

#include <engine.h>
#include <cc++/slog.h>
#include <cc++/socket.h>

#ifndef	WIN32
#include "private.h"

#include <sys/types.h>
#include <sys/param.h>
#ifdef HAVE_SYS_MOUNT_H
#include <sys/mount.h>
#endif
#ifdef HAVE_SYS_STATVFS_H
#include <sys/statvfs.h>
#endif
#ifdef HAVE_SYS_VFS_H
#include <sys/vfs.h>
#endif

#ifdef HAVE_SYS_STATVFS_H
#define STATFS statvfs
#else
#define STATFS statfs
#endif

#endif

namespace moduleXMLRPC {
using namespace ost;
using namespace std;

typedef	Bayonne::rpcint_t rpcint_t;
typedef	rpcint_t rpcbool_t;

static bool isValid(const char *path)
{
	if(!path)
		return false;

	if(!*path)
		return false;

	if(*path == '/' || *path == '\\' || *path == '.')
		return false;

	if(strstr("..", path))
		return false;

#ifdef	WIN32
	if(path[1] == ':')
		return false;
#endif
	return true;
}

static void purgedir(const char *dir)
{
	const char *cp;
	if(isDir(dir) && canAccess(dir))
	{
		DirTree dt(dir, 16);
		while(NULL != (cp = dt.getPath()))
			remove(cp);
		remove(dir);
	}
}

static void dir_version(BayonneRPC *rpc)
{
	if(rpc->getCount())
	{
		rpc->sendFault(3, "Invalid parameters");
		return;
	}
	rpc->buildResponse("(ii)", 
		"current", (rpcint_t)1,
		"prior", (rpcint_t)0);
}

static void dir_test(BayonneRPC *rpc)
{
	const char *path;

	if(!rpc->transport.authorized)
	{
		rpc->transportFault(401, "Not Authorized");
		return;
	}

    if(rpc->getCount() != 1)
    {
        rpc->sendFault(3, "Invalid parameters");
        return;
    }

	path = rpc->getIndexed(1);
	if(!isValid(path))
	{
		rpc->sendFault(4, "Invalid parameter value");
		return;
	}

	rpc->buildResponse("b", (rpcbool_t)isDir(path));
}

static void dir_list(BayonneRPC *rpc)
{
	Dir dir;
	const char *path;
	char pbuf[128];
	const char *cp;

	if(!rpc->transport.authorized)
	{
		rpc->transportFault(401, "Not Authorized");
		return;
	}

    if(rpc->getCount() != 1)
    {
        rpc->sendFault(3, "Invalid parameters");
        return;
    }

	path = rpc->getIndexed(1);
	if(!isValid(path))
	{
		rpc->sendFault(4, "Invalid parameter value");
		return;
	}

	if(!isDir(path) || !canAccess(path))
	{
		rpc->buildResponse("[]");
		return;
	}

	rpc->buildResponse("[");
	dir.open(path);
	while(NULL != (cp = dir.getName()))
	{
		if(*cp == '.')
			continue;

		snprintf(pbuf, sizeof(pbuf), "%s/%s", path, cp);
		rpc->buildResponse("!s", pbuf);
	}
	dir.close();

	rpc->buildResponse("]");
}

static void dir_create(BayonneRPC *rpc)
{
	const char *path;

	if(!rpc->transport.authorized)
	{
		rpc->transportFault(401, "Not Authorized");
		return;
	}

    if(rpc->getCount() != 1)
    {
        rpc->sendFault(3, "Invalid parameters");
        return;
    }

	path = rpc->getIndexed(1);
	if(!isValid(path))
	{
		rpc->sendFault(4, "Invalid parameter value");
		return;
	}

	Dir::create(path);
	rpc->buildResponse("b", (rpcbool_t)isDir(path));
}


static void dir_remove(BayonneRPC *rpc)
{
	const char *path;
	bool result = false;

	if(!rpc->transport.authorized)
	{
		rpc->transportFault(401, "Not Authorized");
		return;
	}

    if(rpc->getCount() != 1)
    {
        rpc->sendFault(3, "Invalid parameters");
        return;
    }

	path = rpc->getIndexed(1);
	if(!isValid(path))
	{
		rpc->sendFault(4, "Invalid parameter value");
		return;
	}

	if(isDir(path))
	{
		purgedir(path);
		result = true;
    }
	rpc->buildResponse("b", (rpcbool_t)result);
}

static void dir_space(BayonneRPC *rpc)
{
	const char *path;
	rpcint_t result = 0;

	if(!rpc->transport.authorized)
	{
		rpc->transportFault(401, "Not Authorized");
		return;
	}

    if(rpc->getCount() != 1)
    {
        rpc->sendFault(3, "Invalid parameters");
        return;
    }

	path = rpc->getIndexed(1);
	if(!isValid(path))
	{
		rpc->sendFault(4, "Invalid parameter value");
		return;
	}

	if(isDir(path))
	{
#ifdef	WIN32			
		ULARGE_INTEGER bavail, btotal, bfree;
		GetDiskFreeSpaceEx(path, &bavail, &btotal, &bfree);
		result = ((bavail / 1024l) / 1024l);
#else
		struct STATFS fs;
		if(0 == STATFS(path, &fs))
			result = (fs.f_bfree * (fs.f_bsize / 1024l) / 1024l);
	}
#endif
	rpc->buildResponse("i", result);
}

static void dir_move(BayonneRPC *rpc)
{
	const char *path1, *path2;
	bool result = false;

	if(!rpc->transport.authorized)
	{
		rpc->transportFault(401, "Not Authorized");
		return;
	}

    if(rpc->getCount() != 2)
    {
        rpc->sendFault(3, "Invalid parameters");
        return;
    }

	path1 = rpc->getIndexed(1);
	path2 = rpc->getIndexed(2);
	if(!isValid(path1) || !isValid(path2))
	{
		rpc->sendFault(4, "Invalid parameter value");
		return;
	}

	if(isDir(path1))
		if(!rename(path1, path2))
			result = true;

	rpc->buildResponse("b", (rpcbool_t)result);
}


static Bayonne::RPCDefine dispatch[] = {
	{"version", dir_version,
		"API version level of dir rpc subsystem", "struct"},
	{"test", dir_test,
		"Test for specified directory path", "boolean, string"},
    {"list", dir_list,
        "List files for specified directory path", "array, string"},
	{"move", dir_move,
		"Move specified directory path", "boolean, string, string"},
	{"remove", dir_remove,
		"Remove specified directory tree", "boolean, string"},
	{"create", dir_create,
		"Create specified subdirectory path", "boolean, string"},
	{"space", dir_space,
		"Return space available in megabytes", "int, string"},
	{NULL, NULL, NULL, NULL}};

static Bayonne::RPCNode xmlrpc_dir("dir", dispatch);

}; // namespace
