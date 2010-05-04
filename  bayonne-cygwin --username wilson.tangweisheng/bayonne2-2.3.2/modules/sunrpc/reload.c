#include <stdio.h>
#include <rpc/rpc.h>
#include <sys/time.h>
#include <getopt.h>
#include "bayonne_rpc.h"

static CLIENT *rpc = NULL;

static void reload(void)
{
	bayonne_error *error;
	char dur[12];
	unsigned long upt;

	error = bayonne_reload_2("", rpc);
	if(!error)
	{
        	printf( "RPC CALL: %s\n", clnt_sperror(rpc, "" ) );
                clnt_destroy(rpc);
                exit( 1 );
        }

	if(*error != BAYONNE_SUCCESS)
	{
		printf("reload: failed; error=%d\n", *error);			
		exit(2);
	}
	
	clnt_destroy(rpc);
	exit(0);
}

int main(int argc, char **argv)
{
        struct timeval          stvTimeout;
	char *tp = "udp";
	char *host;
	char opt;
	
        stvTimeout.tv_sec =  23;
        stvTimeout.tv_usec = 0;

	host = getenv("BAYONNE_HOST");
	if(!host)
		host = "localhost";

	while((opt = getopt(argc, argv, "t:h:p:")) != -1)
		switch(opt)
		{
		case 't':
			stvTimeout.tv_sec = atoi(optarg);
			break;
		case 'h':
			host = optarg;
			break;
		case 'p':
			tp = optarg;
			break;
		default:
			goto use;
		}

	rpc = clnt_create(host, BAYONNE_PROGRAM, BAYONNE_VERSION, tp);

        if (!rpc)
        {
                printf( "CLNT_CREATE %s\n", clnt_spcreateerror( "" ) );
                exit( 1 );
        }

        if(!clnt_control(rpc, CLSET_TIMEOUT,( char *) &stvTimeout ) )
        {
                printf( "CLNT_CONTROL: %s\n",clnt_sperror(rpc, "" ) );
                clnt_destroy(rpc);
                exit( 1 );
        }

	if(optind >= argc)
		reload();

use:
	fprintf(stderr, "use: bts_reload [-h host] [-p proto] [-t timeout]\n");

	exit(-1);

}
