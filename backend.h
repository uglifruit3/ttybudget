#ifndef backend
#define backend

#include <stdio.h>

#define TRUE 1
#define FALSE 0

#define MAX_TAG_LEN 32
#define MAX_MSG_LEN 256

// TODO having a limit of 8 tags if kind of a pussy thing to do... see if possible to have infinite while tampering with codebase as little as possible
/* structure for handling records data */
struct record_t {
	float amount;
	int date;
	char message[MAX_MSG_LEN];
	char tags[8][MAX_TAG_LEN];
};

/* structure for specifying records during print/lookup operations */
struct search_param_t {
	float amnt_bound1;
	float amnt_bound2;
	int date1;
	int date2;
	char tags[8][MAX_TAG_LEN];
	int sort_flag;
};

#include "io.h"

/* frees an array wherein each element is allocated manually */
void free_array(char **array, int n);
void arr_cpy(struct record_t *arr1, struct record_t *arr2, int n);
int *binary_search(int term, int *list, int hi, int lo);

/* initializes a record for population */
void initialize_record(struct record_t *record);
/* initializes a search parameter for population */
void init_search_params(struct search_param_t *search_param);

/* formats and prints a record structure to the record file */
void print_rec_to_file(FILE *outfile, struct record_t record);

/* gets number of records from a records file */
unsigned int get_num_records(FILE *infile);
/* gets the number of elements in a record from record file */
unsigned int get_num_elements(char line[600]);
/* obtains a space-separated array of all elements in the record stored in  line[600] */
char **get_elements(char line[600]);
/* gets an array of all records stored in file, using the record_t structure */
struct record_t *get_records_array(FILE *infile, int num_records, float *total);

/* reorders a records array, sorted from lowest to highest amounts */
void sort_recs_amounts(struct record_t *list, struct record_t *tmp, int n, int runs);
/* reorders a records array, sorted from lowest to highest amounts */
void sort_recs_date(struct record_t *list, struct record_t *tmp, int n, int runs);
/* returns the indices of the FIRST and LAST occurences of the date searched; returns NULL if not present */
int *search_recs_date(int date, struct record_t *records, int hi, int lo);
/* returns the indices of all amounts within lower bound1 and upper bound2; returns NULL if none present.
 * Index 0 of the returned array denotes the number of matches */
int *search_recs_amount(float amnt1, float amnt2, struct record_t *records, int bound1, int bound2);
/* returns the indices of all records that contain one of the specified tags within bound1 
 * and bound2; return behavior is same as amount search */
int *search_recs_tags(char tags[8][MAX_TAG_LEN], struct record_t *records, int bound1, int bound2);
int *search_records(struct record_t *records, int n_recs, struct search_param_t params);

void add_records(struct NewRecs_t *new_recs, struct record_t *records, int *n_recs, char *rec_filename, float tot_cash);

void delete_record(struct record_t old_record, struct record_t *record_list);
void modify_record(struct record_t old_record, struct record_t *record_list);
void edit_records(struct record_t **record_list, int record_index);

#endif
