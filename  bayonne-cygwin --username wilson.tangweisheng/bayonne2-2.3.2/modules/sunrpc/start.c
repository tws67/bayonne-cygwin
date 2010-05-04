#include <stdio.h>
#include <rpc/rpc.h>
#include <sys/time.h>
#include <getopt.h>
#include "bayonne_rpc.h"

static CLIENT *rpc = NULL;

static char *caller = "";
static char *display = "";

static void start(char *script, char *number)
{
	bayonne_start s;
	bayonne_result *result;
	s.start_script = script;
	s.start_number = number;
	s.start_caller = caller;
	s.start_display = display;
	
	result = bayonne_start_2(&s, rpc);
	if(!result)
	{
        	printf( "RPC CALL: %s\n", clnt_sperror(rpc, "" ) );
                clnt_destroy(rpc);
                exit( 1 );
        }

	if(result->result_code != BAYONNE_SUCCESS)
	{
		printf("cancel: failed; error=%d\n", result->result_code);			
		exit(2);
	}

	printf("%s\n", result->result_id);
	
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

	while((opt = getopt(argc, argv, "c:d:t:h:p:")) != -1)
		switch(opt)
		{
		case 'c':
			caller = optarg;
			break;
		case 'd':
			display = optarg;
			break;
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

	if(optind == (argc - 1) || optind == (argc - 2))
		start(argv[optind], argv[optind + 1]);

use:
	fprintf(stderr, "use: bts_start script [-c caller] [-d display] [-h host] [-p proto] [-t timeout]\n");

	exit(-1);

}
