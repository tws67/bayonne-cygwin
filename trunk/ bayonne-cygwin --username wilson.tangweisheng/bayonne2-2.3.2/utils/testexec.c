#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

int main(int argc, char **argv)
{
	char lbuf[512];

	for(;;)
	{
		if(!fgets(lbuf, sizeof(lbuf), stdin) || feof(stdin))
			break;
		if(!isalnum(lbuf[0]))
			break;
		fprintf(stderr, "%s", lbuf);
	}
	exit(0);
}
