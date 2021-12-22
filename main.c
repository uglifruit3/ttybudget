#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include "backend.h"
#include "io.h"

int main(int argc, char *argv[])
{
	char infile_name[256];
	FILE *infile = NULL;

	struct NewRecs_t *new_records = NULL;

	struct record_t *records = NULL;
	int n_recs = 0;

	struct search_param_t print_params;
	init_search_params(&print_params);

	int err = parse_command_line(argv, argc, infile_name, &new_records, &print_params);
	if (err) {
		free_recs_array(records, n_recs, TRUE);
		free_list(new_records, TRUE);
		free_search_params(print_params);

		if (err == 2)
			return 0;
		else
			return 1;
	}

	float tot_cash = 0;

	infile = open_records_file(infile_name);
	n_recs = get_num_records(infile);
	records = get_records_array(infile, n_recs, &tot_cash);
	fclose(infile);

	// TODO create a separate function for writing to the outfile, and call it here. 
	records = add_records(new_records, records, &n_recs);
	write_to_file(records, n_recs, tot_cash, infile_name);

	if (print_params.print_flag)
		print_records(records, n_recs, tot_cash, print_params, DATE_ISO);

	free_recs_array(records, n_recs, TRUE);
	free_list(new_records, FALSE);
	free_search_params(print_params);
	return 0;
}
