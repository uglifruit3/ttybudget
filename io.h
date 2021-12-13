#ifndef io_header
#define io_header

#include <stdio.h>

#include "backend.h"

enum Commandline_Options {
	ADD_S, ADD_L,
		TAGS_S, TAGS_L,
		MESSAGE_S, MESSAGE_L,
		DATE_S, DATE_L,
	PRINT_S, PRINT_L,
	EDIT_S, EDIT_L,
		QUERY_S, QUERY_L,
		INTERVAL_S, INTERVAL_L,
		RANGE_S, RANGE_L,
		SORT_S, SORT_L,
	HELP_S, HELP_L,
	VERSION_S, VERSION_L,
	DATE_ISO, DATE_US, DATE_LONG,
	BASE_DIR, CURRENCY, OUT_DATE, IN_DATE, USE_FILE
};

/* linked list structure for accumulating new records to be added */
struct NewRecs_t {
	struct record_t data;
	struct NewRecs_t *next;
};

/* linked list helpers */
void add2front(struct NewRecs_t **list, struct NewRecs_t *new);
void free_list(struct NewRecs_t *list);

/* commandline parse helpers */
int string_in_list(char *string, char *list[], int num_list_items);
int is_cmdline_option(char *str1);

/* opens records file for reading and writing; creates and populates if nonexistent */
FILE *open_records_file(char file_name[256]);
/* changes date format from format_enum to new_format */
void chng_date_frmt(int *format_enum, int new_format);

/* obtains tags from the -t option when adding or -q option when printing; *index 
 * should point to the index of the "-t" flag and will be incremented to the 
 * element following tag arguments */
int get_tags(char *argv[], int *index, char tags[8][32]);
/* obtains a message from the -m option when adding records; *index is used and altered
 * as in get_tags */
int get_message(char *argv[], int *index, char message[256]);

/* gets the date with the -d option with adding records or -i with lookup */
int get_date_ISO(char *date_str, int *last_days, int *date);
int get_date_US(char *date_str, int *last_days, int *date);
int get_date_LONG(char *argv[], int *index, int *last_days, int *date);
int get_date(char *argv[], int *index, int format, int *date, int accept_inf);

/* obtains an amount from a record when adding records */
int get_amount(char *argv[], int *index, float *amount, int assume_negative, int accept_inf);

/* obtains a linked list representation of all new records when the add option is specified */
int get_new_records(int argc, char *argv[], int *index, int date_frmt, struct NewRecs_t **new_records);
/* obtains search parameters for printing */
int get_print_commands(int argc, char *argv[], int *index, int date_frmt, struct search_param_t *params);

/* parses a command line invocation, populating new records lists, search parameters, and opening the records file */
int parse_command_line(char *argv[], int argc, char filename[], struct NewRecs_t **new_records, struct search_param_t *print_params);

#endif
