/* ttybudget main. handles all high level operations */
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdbool.h>

#include "backend.h"
#include "io.h"

int main(int argc, char *argv[])
{
	// TODO include a negation operator/functionality for search tools (so -p [print option [!] arguments]
	
	/* takeaways from this project:
	 *  - use getopt() for command line parsing; in general, look for implementations of common things before doing them yourself
	 *  - be more sophisticated/thoughtful about types usage at the outset of writing stuff
	 *  - use stdbool.h for booleans
	 *  - be more detailed in error tracking/reporting, and be prepared for edge cases and catching errors in library calls
	 *  - learn how to do file i/o better */

	struct defaults_t defaults;
	int err = NO_ERR;
	err = read_defaults(&defaults);
	if (err)
		return err;

	char infile_name[256];
	FILE *infile = NULL;

	struct NewRecs_t *new_records = NULL;
	struct search_param_t print_params;
	init_search_params(&print_params);

	err = parse_command_line(argv, argc, infile_name, &new_records, &print_params, &defaults);
	if (err) {
		free_list(new_records, true);
		free_search_params(print_params);

		if (err == EXIT_NOW)
			return NO_ERR;
		else
			return err;
	}

	infile = open_records_file(infile_name, &err);
	if (infile == NULL) {
		free_list(new_records, true);
		free_search_params(print_params);
		return err;
	}

	float tot_cash = 0;
	struct record_t *records = NULL;
	int n_recs = 0;
	n_recs = get_num_records(infile);
	records = get_records_array(infile, n_recs, &tot_cash);
	fclose(infile);
	if (records == NULL && n_recs != 0) {
		free_list(new_records, true);
		free_search_params(print_params);
		return USR_ERR;
	}

	records = add_records(new_records, records, &n_recs);
	write_to_file(records, n_recs, tot_cash, infile_name, &err);
	if (err == SYS_ERR) {
		free_recs_array(records, n_recs, true);
		free_list(new_records, false);
		free_search_params(print_params);
		return err;
	}

	if (print_params.print_flag)
		print_records(records, n_recs, tot_cash, print_params, defaults);

	free_recs_array(records, n_recs, true);
	free_list(new_records, false);
	free_search_params(print_params);
	return err;
}
