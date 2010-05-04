#include <stdio.h>
#include <rpc/rpc.h>
#include <sys/time.h>
#include <getopt.h>
#include "bayonne_rpc.h"

static CLIENT *rpc = NULL;

static void uptime(void)
{
	bayonne_status *stats;
	char dur[12];
	unsigned long upt;

	stats = bayonne_status_2("", rpc);
	if(!stats)
	{
        	printf( "RPC CALL: %s\n", clnt_sperror(rpc, "" ) );
                clnt_destroy(rpc);
                exit( 1 );
        }

	upt = stats->node_uptime;

        if(upt < 100 * 3600)
                snprintf(dur, sizeof(dur), "%02d:%02d:%02d",
                        upt / 3600, (upt / 60) % 60, upt % 60);
        else
                snprintf(dur, sizeof(dur), "%d days", upt / (3600 * 24));
	
	printf("%s server version %s, timeslots=%d, active=%d, up %s\n",
		stats->node_server, stats->node_version, 
		stats->node_count, stats->node_active, dur);
	
	clnt_destroy(rpc);
	exit(0);
}

int main(int argc, char **argv)
{
        struct timeval          stvTimeout;
	char *tp = "udp";
	char *host;
	char opt;
	
	host = getenv("BAYONNE_HOST");
	if(!host)
		host = "localhost";

        stvTimeout.tv_sec =  23;
        stvTimeout.tv_usec = 0;

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
		uptime();

use:
	fprintf(stderr, "use: bts_uptime [-h host] [-p proto] [-t timeout]\n");

	exit(-1);

}
