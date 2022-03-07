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
		QUERY_S, QUERY_L,
		INTERVAL_S, INTERVAL_L,
		FIND_RANGE_S, FIND_RANGE_L,
		SORT_S, SORT_L,
		REVERSE_S, REVERSE_L,
		NOFOOTER_S, NOFOOTER_L,
		LIST_TAGS_S, LIST_TAGS_L,
	HELP_S, HELP_L,
	VERSION_S, VERSION_L,
	IN_DATE_ISO, IN_DATE_US,
	OUT_DATE_ISO, OUT_DATE_US, OUT_DATE_LONG, OUT_DATE_ABBR,
	CURRENCY_S, CURRENCY_L,
       	USE_FILE_S, USE_FILE_L
};

/* linked list structure for accumulating new records to be added */
struct NewRecs_t {
	struct record_t data;
	struct NewRecs_t *next;
};

/* ======================================================================
 * unless otherwise noted, functions return values specified in backend.h
 * under enum err_types
 * ======================================================================*/

/* linked list helpers */
void add2front(struct NewRecs_t **list, struct NewRecs_t *new);
/* frees a linked list of new nodes. OF NOTE: del_mode specifies whether the 
 * dynamically allocated tags will be free'd or not. This is necessary because
 * the tag addresses are shared with the tags of the same new records when 
 * added to the record array. Under normal operation, freeing that records
 * array handles freeing the shared tags. But in an error state, it is 
 * sometimes necessary for this function to free tags */
void free_list(struct NewRecs_t *list, bool del_mode);

/* commandline parse helpers */
/* general purpose checker for whether string is in list. returns index of match or -1 if none */
signed int string_in_list(char *string, char *list[], int num_list_items);
/* returns a value associated with the command line option as denoted in the
 * Commandline_Options enumerator, or -1 for none */
signed int is_cmdline_option(char *str1);

/* opens records file for reading and writing; creates and populates if nonexistent */
FILE *open_records_file(char file_name[256], int *err);
/* changes date format from format_enum to new_format */

/* obtains tags from the -t option when adding or -q option when printing; *index 
 * should point to the index of the "-t" flag and will be incremented to the 
 * element following tag arguments */
int get_tags(char *argv[], int *index, char ***tags, int *n_tags);
/* obtains a message from the -m option when adding records; *index is used and altered
 * as in get_tags */
int get_message(char *argv[], int *index, char message[256]);

/* gets the date with the -d option with adding records or -i with lookup */
	/* table of return values for get_date functions:
	 * 0/NO_ERR: no error
	 * 1:        bad delimeters
	 * 2:        bad format
	 * 3:        invalid date expression
	 * 4:        invalid month value
	 * 5:        invalid day value 
	 * 6:        get_date_LONG returned an error (handles own output) */
int get_date_ISO(char *date_str, int *last_days, int *date, char *argv[], int *index);
int get_date_US(char *date_str, int *last_days, int *date, char *argv[], int *index);
int get_date_LONG(char *argv[], int *index, int *last_days, int *date);
int get_date(char *argv[], int *index, int format, int *date, bool accept_inf);
/* obtains the current date in ISO integer representation */
int get_current_date();

/* obtains an amount from a record when adding records */
int get_amount(char *argv[], int *index, float *amount, bool assume_negative, bool accept_inf);

/* obtains a linked list representation of all new records when the add option is specified */
int get_new_records(int argc, char *argv[], int *index, int date_frmt, struct NewRecs_t **new_records);       
/* obtains search parameters for printing */
int get_print_commands(int argc, char *argv[], int *index, int date_frmt, struct search_param_t *params);

/* parses a command line invocation, populating new records lists, search parameters, and opening the records file */
int parse_command_line(char *argv[], int argc, char filename[], struct NewRecs_t **new_records, struct search_param_t *print_params, struct defaults_t *defaults);

/* formats and prints a record structure to the record file */
void print_rec_to_file(FILE *outfile, struct record_t record);
/* writes a records array to an output file */
void write_to_file(struct record_t *records, int n_recs, float start_amnt, char *rec_filename, int *err);

void print_date_ISO(struct record_t record);
void print_date_US(struct record_t record);
void print_date_LONG(struct record_t record);
void print_date_ABBR(struct record_t record);

void print_amnt_no_cc(char sign, char curr_char, float amnt);
void print_amnt_cc(char sign, char curr_char, float amnt);

void print_table_footer(struct record_t *records, int n_recs, int *matches, float start_amnt, char cur_char);
void print_tags(struct record_t *records, int n_recs);

/* prints formatted output to the terminal and applies search parameters */
void print_records(struct record_t *records, int n_recs, float start_amnt, struct search_param_t params, struct defaults_t defs);

#endif
