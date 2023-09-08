/* ttybudget main. handles all high level operations */
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdbool.h>

#include "backend.h"
#include "io.h"

int *Warning_Count;

int main(int argc, char *argv[])
{
	/* takeaways from this project:
	 *  - use getopt() for command line parsing; in general, look for implementations of common things before doing them yourself
	 *  - be more sophisticated/thoughtful about types usage at the outset of writing stuff
	 *  - be more detailed in error tracking/reporting, and be prepared for edge cases and catching errors in library calls
	 *  - learn how to do file i/o better */

	// TODO create an interface to edit entries
	// TODO create a color mode, make it a defaults option, display tags more nicely

	/* error state variable */
	int err = NO_ERR;

	/* initialize defaults */
	struct defaults_t defaults;
	defaults.in_date_frmt = defaults.out_date_frmt = defaults.change_flag = 0;
	defaults.currency_char = '\0';
	defaults.print_mode = 0;
	memset(defaults.recs_file, '\0', 256);
	/* initialize warning tracker and read defaults from file */
	Warning_Count = malloc(sizeof(int));
	*Warning_Count = 0;
	err = read_defaults(&defaults);
	if (err)
		return err;

	/* define new records and search parameters storage */
	struct NewRecs_t *new_records = NULL;
	struct search_param_t print_params;
	init_search_params(&print_params);

	/* read input records file */
	FILE *infile = NULL;
	infile = open_records_file(defaults.recs_file, &err);
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

	/* get command line input */
	err = parse_command_line(argv, argc, defaults.recs_file, &new_records, &print_params, &defaults);
	if (err) {
		free_list(new_records, true);
		free_search_params(print_params);

		if (err == EXIT_NOW)
			return NO_ERR;
		else
			return err;
	}

	records = add_records(new_records, records, &n_recs);
	write_to_file(records, n_recs, tot_cash, defaults.recs_file, &err);
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
	free(Warning_Count);

	return err;
}
