#include <stdio.h>
#include <string.h>

#define COL_RESET "\x1b[0m"
#define COL_RED   "\x1b[1;31m"
#define COL_GRN   "\x1b[1;32m"

int 
main()
{
	char str[42];
	memset(str, '\0', 42);

	sprintf(str, "%s hello! %s\n", COL_RED, COL_RESET);
	printf(str);
	return 0;
}
