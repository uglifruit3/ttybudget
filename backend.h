#ifndef backend
#define backend

#include <stdio.h>

#define TRUE 1
#define FALSE 0

#define MAX_TAG_LEN 32
#define MAX_MSG_LEN 256

/* enum for handling/returning different error types */
/*                  0       1        2               */
enum err_types { NO_ERR, USR_ERR, SYS_ERR };

/* structure for handling records data */
struct record_t {
	float amount;
	int date;
	char message[MAX_MSG_LEN];
	int n_tags;
	char **tags;
};

// May go unused; consider deleting or appropriating for tag listing usage
/* structure for generic merge sort */
struct tagnode_t {
	char tag[32];
	int times;
};

/* structure for specifying records during print/lookup operations */
struct search_param_t {
	int print_flag;
	int show_footer;
	int sort_flag;
	int reverse_flag;
	int list_tags;
	float amnt_bound1;
	float amnt_bound2;
	int date1;
	int date2;
	int n_tags;
	char **tags;
};

/* structure for storing operating behavior of ttybudget */
struct defaults_t {
	int out_date_frmt;
	int in_date_frmt;
	char recs_file[256];
	char currency_char;
	int change_flag;
};

#include "io.h"

/* frees an array wherein each element is allocated manually */
void free_array(char **array, int n);
// TODO convert this to rec_arr_cpy for clarity 
void arr_cpy(struct record_t *dest, struct record_t *src, int n);
// TODO need to make a tag_arr_cpy for the tag list sorting algorithm
int *binary_search(int term, int *list, int hi, int lo);

/* initializes a record for population */
void initialize_record(struct record_t *record);
/* frees an array of records */
void free_recs_array(struct record_t *records, int n_recs, int del_tags);
/* initializes a search parameter for population */
void init_search_params(struct search_param_t *search_param);
/* frees the dynamically allocated tags in search_params */
void free_search_params(struct search_param_t search_param);

/* gets number of records from a records file */
unsigned int get_num_records(FILE *infile);
/* gets the number of elements in a record from record file */
unsigned int get_num_elements(char line[600]);
/* obtains a space-separated array of all elements in the record stored in  line[600] */
char **get_elements(char line[600]);
/* gets an array of all records stored in file, using the record_t structure */
struct record_t *get_records_array(FILE *infile, int num_records, float *start_amnt);

/* functions for reading/writing defaults */
int dir_exists(char dir[]);
/* opens the defaults file, given a mode adherent to fopen() */
FILE *open_defaults_file(char *mode);
/* returns a complete buffer of the defaults file */
char *get_defs_buffer(FILE *defs_file);
/* reads default values from file. Returns 0 if normal, 1 if error */
int read_defaults(struct defaults_t *defs);
/* writes defaults to file if they have been changed */
/* not used in current configuration; defaults file must be edited manually */
void write_defaults(struct defaults_t defs);

/* reorders a records array, sorted from hightest to lowest amounts */
void sort_recs_amounts(struct record_t *list, struct record_t *tmp, int n, int runs);
/* reorders a records array, sorted from lowest to highest amounts */
void sort_recs_date(struct record_t *list, struct record_t *tmp, int n, int runs);
/* reorders an array of tags, sorted from highest to lowest occurencies */
void sort_taglist(struct tagnode_t *taglist, struct tagnode_t *tmp, int n, int runs);
/* returns the indices of the FIRST and LAST occurences of the date searched; returns NULL if not present */
int *search_recs_date(int date, struct record_t *records, int hi, int lo);
/* returns the indices of all amounts within lower bound1 and upper bound2; returns NULL if none present.
 * Index 0 of the returned array denotes the number of matches */
int *search_recs_amount(float amnt1, float amnt2, struct record_t *records, int bound1, int bound2);
/* returns the indices of all records that contain one of the specified tags within bound1 
 * and bound2; return behavior is same as amount search */
int *search_recs_tags(char **tags, int n_tags, struct record_t *records, int bound1, int bound2);
/* gets a list of tags occuring in the given records array */
struct tagnode_t *get_tag_list(struct record_t *records, int n_recs, int *n_tags);
/* master search function. Returns an array consisting of the matched indices in the passed records array,
 * with the first index being the number of matches; or, it returns a pointer to an error code. */
/* search priority: date -> amounts -> tags */
	/* table of error return values:
	 *   -1: no records within date range returned
	 *   -2: singular date parameter returns no results
	 *   -3: amount parameter returns no results
	 *   -4: tags parameter returns no results
	 *   -5: amount and tags parameters return no results */
int *search_records(struct record_t *records, int n_recs, struct search_param_t params);

struct record_t *add_records(struct NewRecs_t *new_recs, struct record_t *records, int *n_recs);

#endif
