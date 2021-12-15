#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include "backend.h"
#include "io.h"

int main(int argc, char *argv[])
{
	char infile_name[256];
	FILE *infile;
	struct NewRecs_t *new_records = NULL;
	struct search_param_t print_params;
	init_search_params(&print_params);

	int err = parse_command_line(argv, argc, infile_name, &new_records, &print_params);
	if (err == TRUE)
		return 1;
	else if (err == 2)
		return 0;

	float tot_cash = 0;

	infile = open_records_file(infile_name);
	int n_recs = get_num_records(infile);
	struct record_t *records = get_records_array(infile, n_recs, &tot_cash);
	fclose(infile);

	add_records(new_records, records, &n_recs, infile_name, tot_cash);
	
	infile = open_records_file(infile_name);
	n_recs = get_num_records(infile);
	free_recs_array(records, n_recs);
	records = get_records_array(infile, n_recs, &tot_cash);
	fclose(infile);

	int *prints = search_records(records, n_recs, print_params);
	if (*prints == -1)
		fprintf(stderr, "Error: no records in date range %i, %i.\n", print_params.date1, print_params.date2);
	else if (*prints == -2)
		fprintf(stderr, "Error: date %i not found.\n", print_params.date2);
	else if (*prints == -3)
		fprintf(stderr, "Error: no records in amount range %.2f, %.2f.\n", print_params.amnt_bound1, print_params.amnt_bound2);
	else if (*prints == -4)
		fprintf(stderr, "Error: no records with given tags found.\n");
	else if (*prints == -5)
		fprintf(stderr, "Error: no records in amount range %.2f, %.2f.\nError: no records with given tags found.\n", print_params.amnt_bound1, print_params.amnt_bound2);
	else {
		int i = 1;
		printf("Matched indices: ");
		while (i <= prints[0]) {
			printf(" %i", prints[i]);
			i++;
		}	
		putchar('\n');
	}
	
	printf(" Total: $%.2f\n", tot_cash);
	for (int i = 0; i < n_recs; i++) {
		printf("== %2i ==  $%.2f  %i  \"%s\"", i,
		 	records[i].amount,
			records[i].date,
			records[i].message);
		if (records[i].n_tags != 0) {
			printf(" %s", records[i].tags[0]);
			for (int j = 1; j < records[i].n_tags; j++)
				printf(",%s", records[i].tags[j]);
		}
		putchar('\n');
	}

	free_list(new_records);
	free(prints);
	free_recs_array(records, n_recs);
	free_search_params(print_params);
	return 0;
}
