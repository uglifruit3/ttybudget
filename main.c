/* ttybudget main. handles all high level operations */
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdbool.h>

#include "backend.h"
#include "io.h"

void cleanup(int exit_code);

int *Warning_Count;
struct record_t *records = NULL;
int n_recs = 0;
struct NewRecs_t *new_records = NULL;
struct search_param_t print_params;

int main(int argc, char *argv[])
{
	/* takeaways from this project:
	 *  - use getopt() for command line parsing; in general, look for implementations of common things before doing them yourself
	 *  - be more sophisticated/thoughtful about types usage at the outset of writing stuff
	 *  - be more detailed in error tracking/reporting, and be prepared for edge cases and catching errors in library calls
	 *  - learn how to do file i/o better */

	/* error state variable */
	int err = NO_ERR;

	struct defaults_t defaults;
	initialize_defaults(&defaults);
	/* define new records and search parameters storage */
	init_search_params(&print_params);

	/* initialize warning tracker and read defaults from file */
	Warning_Count = malloc(sizeof(int));
	*Warning_Count = 0;
	err = read_defaults(&defaults);
	if (err)
		return err;
	/* get command line input */
	err = parse_command_line(argv, argc, defaults.recs_file, &new_records, &print_params, &defaults);
	if (err) {
		if (err == EXIT_NOW)
			cleanup(NO_ERR);
		else
			cleanup(err);
	}

	/* read input records file */
	FILE *infile = NULL;
	infile = open_records_file(defaults.recs_file, &err);
	if (infile == NULL) 
		cleanup(err);
	float tot_cash = 0;
	n_recs = get_num_records(infile);
	records = get_records_array(infile, n_recs, &tot_cash);
	fclose(infile);
	if (records == NULL && n_recs != 0)
		cleanup(USR_ERR);

	/* append new records to records array */
	records = add_records(new_records, records, &n_recs);
	write_to_file(records, n_recs, tot_cash, defaults.recs_file, &err);
	if (err == SYS_ERR)
		cleanup(err);
	/* print if commanded */
	if (print_params.print_flag)
		print_records(records, n_recs, tot_cash, print_params, defaults);

	cleanup(err);
}

void cleanup(int exit_code)
{
	free(Warning_Count);
	free_search_params(print_params);
	if (new_records != NULL)
		free_list(new_records);
	if (records != NULL)
		free_recs_array(records,n_recs);

	exit(exit_code);
}
