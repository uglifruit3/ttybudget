#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include "backend.h"
#include "io.h"

int main(int argc, char *argv[])
{
	//TODO implement a -l/--list-tags argument, which displays all tags assigned to displayed records
	//TODO change program return values (0 normal; 1 user error; 2 system error)
	struct defaults_t defaults;
	int err = read_defaults(&defaults);
	if (err) 
		return 1;

	char infile_name[256];
	FILE *infile = NULL;

	struct NewRecs_t *new_records = NULL;
	struct search_param_t print_params;
	init_search_params(&print_params);

	err = parse_command_line(argv, argc, infile_name, &new_records, &print_params, &defaults);
	if (err) {
		free_list(new_records, TRUE);
		free_search_params(print_params);

		if (err == 2)
			return 0;
		else
			return 1;
	}

	infile = open_records_file(infile_name);
	if (infile == NULL) {
		free_list(new_records, TRUE);
		free_search_params(print_params);
		return 1;
	}

	float tot_cash = 0;
	struct record_t *records = NULL;
	int n_recs = 0;
	n_recs = get_num_records(infile);
	records = get_records_array(infile, n_recs, &tot_cash);
	fclose(infile);

	records = add_records(new_records, records, &n_recs);
	write_to_file(records, n_recs, tot_cash, infile_name);

	if (print_params.print_flag)
		print_records(records, n_recs, tot_cash, print_params, defaults);

	free_recs_array(records, n_recs, TRUE);
	free_list(new_records, FALSE);
	free_search_params(print_params);
	return 0;
}
