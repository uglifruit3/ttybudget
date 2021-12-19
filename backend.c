#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <float.h>
#include <limits.h>

#include "backend.h"
#include "io.h"

void free_array(char **array, int n)
{
	for (int i = 0; i < n; i++) 
		free(array[i]);

	free(array);

	return;
}

void arr_cpy(struct record_t *arr1, struct record_t *arr2, int n)
{
	for (int i = 0; i < n; i++)
		arr1[i] = arr2[i];

	return;
}

int *binary_search(int term, int *list, int hi, int lo)
{
	int ref = (((float)hi - (float)lo) / 2.0) + lo;
	int max_ind = hi;

	if (list[ref] == term) {
		while (ref > 0 && list[ref-1] == term)
			ref--;
		int hi = ref;
		while (hi < max_ind && list[hi+1] == term)
			hi++;

		int *lohi = malloc(2*sizeof(int));
		lohi[0] = ref;
		lohi[1] = hi;
		return lohi;
	}

	if (hi <= lo)
		return NULL;
	else if (list[ref] < term)
		return binary_search(term, list, hi, ++ref);
	else if (list[ref] > term)
		return binary_search(term, list, --ref, lo);
	else
		return NULL;
}

void initialize_record(struct record_t *record)
{
	record->date = 0;

	memset(record->message, '\0', MAX_MSG_LEN);
	record->n_tags = 0;

	record->amount = NAN;

	return;
}

void free_recs_array(struct record_t *records, int n_recs)
{
	if (records == NULL) 
		return;
	for (int i = 0; i < n_recs; i++) {
		if (records[i].n_tags == 0)
			continue;

		for (int j = 0; j < records[i].n_tags; j++)
			free(records[i].tags[j]);
		free(records[i].tags);
	}

	free(records);
	return;
}

void init_search_params(struct search_param_t *params)
{
	params->amnt_bound1 = NAN;
	params->amnt_bound2 = NAN;
	params->date1 = params->date2 = 0;
	params->sort_flag = 0;

	params->n_tags = 0;

	return;
}

void free_search_params(struct search_param_t search_param)
{
	if (search_param.n_tags == 0) 
		return;

	for (int i = 0; i < search_param.n_tags; i++)
		free(search_param.tags[i]);
	free(search_param.tags);
}

void print_rec_to_file(FILE *outfile, struct record_t record)
{
	fprintf(outfile, "%.2f %i \"%s\" ", record.amount, record.date, record.message);
	if (record.n_tags != 0) {
		fprintf(outfile, "%s", record.tags[0]);
		for (int i = 1; i < record.n_tags; i++)
			fprintf(outfile, ",%s", record.tags[i]);
	}
	fputc('\n', outfile);

	return;
}

unsigned int get_num_records(FILE *infile)
{
	int num_lines = 0;
	char tmp[600];

	while (fgets(tmp, 600, infile) != NULL)
		num_lines++;

	/* returns offset to beginning of file for re-reading */
	rewind(infile);
	return num_lines - 1; /* offset by one, because first file entry is not a record */
}

unsigned int get_num_elements(char line[600])
{
	int num_elements = 1;
	for (int i = 0; i < strlen(line); i++) {
		if (line[i] == ' ') {
			i++;
			num_elements++;
			while (line[i+1] == ' ')
				i++;
		}
	}

	return num_elements;
}

char **get_elements(char line[600])
{
	char **elements = malloc(4*sizeof(char*));
	int line_i = 0;

	for (int arr_i = 0; arr_i < 4; arr_i++) {
		elements[arr_i] = malloc(256*sizeof(char));
		memset(elements[arr_i], '\0', 256);

		int tmp_i = 0;

		/* for treating quote-enclosed phrases as one whole element */
		if (line[line_i] == '\"') {
			line_i++;
			while (line[line_i] != '\"') {
				elements[arr_i][tmp_i] = line[line_i];
				tmp_i++;
				line_i++;
			}
			/* offsets to the start of the next element */
			line_i += 2;
			continue;
		}

		while (line[line_i] != ' ' && line[line_i] != '\n' && line[line_i] != '\0') {	
			elements[arr_i][tmp_i] = line[line_i];
			tmp_i++;
			line_i++;
		}

		line_i++;
	}

	return elements;
}

			
struct record_t *get_records_array(FILE *infile, int num_records, float *total)
{
	/* stored record format:
	 * AMOUNT | DATE | MESSAGE | TAGS 
	 *   0       1        2       3   */

	char line[600];
	fgets(line, 600, infile);
	*total = atof(line);

	if (num_records == 0)
		return NULL;

	struct record_t *records = malloc(num_records*sizeof(struct record_t));

	int i = 0;
	while (fgets(line, 600, infile) != NULL) {
		line[strlen(line)-1] = '\0'; /* erases newline */
		char **rec_elements = get_elements(line);

		int index = 0;

		initialize_record(&records[i]);
		records[i].amount = atof(rec_elements[0]);
		get_date(rec_elements, &index, DATE_ISO, &(records[i].date), TRUE);
		get_message(rec_elements, &index, records[i].message);
		if (rec_elements[3][0] != '\0')
			get_tags(rec_elements, &index, &(records[i].tags), &(records[i].n_tags));

		free_array(rec_elements, 4);

		i++;
	}
	
	return records;
}

/* call with runs=0 initially */
void sort_recs_amounts(struct record_t *list, struct record_t *tmp, int n, int runs)
{
	int sub_width = pow(2, runs);
	if (sub_width > n) {
		if (runs % 2 != 0)
			arr_cpy(tmp, list, n);
		return;
	}

	for (int i = 0; i < n; i = i + 2*sub_width) {
		int i_left = i;
		int i_right = i + sub_width;
		int j = i;
		while (i_left < i + sub_width && i_right < i + 2*sub_width) {
			if (i_left >= n || i_right >= n) {
				break;
			} else if (i_right >= n || (list[i_left].amount <= list[i_right].amount)) {
				tmp[j] = list[i_left];
				i_left++;
			} else {
				tmp[j] = list[i_right];
				i_right++;
			}
			j++;
		}

		if (i_left < i+sub_width) {
			while (i_left < n && i_left < i + sub_width)
				tmp[j++] = list[i_left++];
		}
		else if (i_right < i+2*sub_width) {
			while (i_right < n && i_right < i + 2*sub_width)
				tmp[j++] = list[i_right++];
		}
	}

	sort_recs_amounts(tmp, list, n, ++runs);
}

void sort_recs_date(struct record_t *list, struct record_t *tmp, int n, int runs)
{
	int sub_width = pow(2, runs);
	if (sub_width > n) {
		if (runs % 2 != 0)
			arr_cpy(tmp, list, n);
		return;
	}

	for (int i = 0; i < n; i = i + 2*sub_width) {
		int i_left = i;
		int i_right = i + sub_width;
		int j = i;
		while (i_left < i + sub_width && i_right < i + 2*sub_width) {
			if (i_left >= n || i_right >= n) {
				break;
			} else if (i_right >= n || (list[i_left].date <= list[i_right].date)) {
				tmp[j] = list[i_left];
				i_left++;
			} else {
				tmp[j] = list[i_right];
				i_right++;
			}
			j++;
		}

		if (i_left < i+sub_width) {
			while (i_left < n && i_left < i + sub_width)
				tmp[j++] = list[i_left++];
		}
		else if (i_right < i+2*sub_width) {
			while (i_right < n && i_right < i + 2*sub_width)
				tmp[j++] = list[i_right++];
		}
	}

	sort_recs_date(tmp, list, n, ++runs);
}

int *search_recs_date(int param, struct record_t *records, int hi, int lo)
{
	int num_dates = hi-lo+1;
	int *rec_dates = malloc(num_dates*sizeof(int));
	for (int i = 0; i <= hi; i++)
		rec_dates[i] = records[i].date;

	int *results = binary_search(param, rec_dates, hi, lo);
	free(rec_dates);
	return results;
}

int *search_recs_amount(float amnt1, float amnt2, struct record_t *records, int bound1, int bound2)
{
	if (isnan(amnt1) && isnan(amnt2)) {
		amnt1 = -INFINITY;
		amnt2 =  INFINITY;
	}

	int *match_indices = malloc((bound2-bound1+2)*sizeof(int));
	match_indices[0] = 0;

	int j = 1;
	for (int i = bound1; i <= bound2; i++) {
		if (records[i].amount >= amnt1 && records[i].amount <= amnt2) {
			match_indices[j] = i;
			j++;
			match_indices[0]++;
		}
	}

	if (match_indices[0] > 0) {
		match_indices = realloc(match_indices, (match_indices[0]+1)*sizeof(int));
	} else {
		free(match_indices);
		match_indices = NULL;
	}

	return match_indices;
}

int *search_recs_tags(char **tags, int n_tags, struct record_t *records, int bound1, int bound2)
{
	/* if tags are unitialized, don't bother searching for tags */
	if (n_tags == 0) {
		int *matches = malloc((bound2-bound1+2)*sizeof(int));
		matches[0] = bound2 - bound1 + 1;
		for (int i = bound1; i <= bound2; i++)
			matches[i-bound1+1] = i;
		return matches;
	}

	char **tags_cpy = malloc(n_tags*sizeof(char*));
	for (int i = 0; i < n_tags; i++) {
		tags_cpy[i] = malloc(MAX_TAG_LEN*sizeof(char));
		strcpy(tags_cpy[i], tags[i]);
	}

	int *match_indices = malloc((bound2-bound1+2)*sizeof(int));
	match_indices[0] = 0;

	int j = 1;
	for (int i = bound1; i <= bound2; i++) {
		for (int k = 0; k < records[i].n_tags; k++) {
			if (string_in_list(records[i].tags[k], tags_cpy, n_tags) != -1) {
				match_indices[j] = i;
				j++;
				match_indices[0]++;
				break;
			}
		}

	}
	free_array(tags_cpy, n_tags);

	if (match_indices[0] > 0) {
		match_indices = realloc(match_indices, (match_indices[0]+1)*sizeof(int));
	} else {
		free(match_indices);
		match_indices = NULL;
	}

	return match_indices;
}

int *search_records(struct record_t *records, int n_recs, struct search_param_t params)
{
	int *err_stat = malloc(sizeof(int));
	int *amt_matches = NULL;
	int *tag_matches = NULL;
	int *match_inds = NULL;
	int lo = 0; /* lower bound of array to search in */
	int hi = n_recs-1; /* upper bound of array to search in */

	/* search priority: date -> amounts -> tags */
	/* table of error return values:
	 *   -1: no records within date range returned
	 *   -2: singular date parameter returns no results
	 *   -3: amount parameter returns no results
	 *   -4: tags parameter returns no results
	 *   -5: amount and tags parameters return no results */

	/* gets lo and hi bounds of specified date range, if applicable */
	/* if a singular date is being searched */
	if (params.date1 == 0 && params.date2 != 0) {
		int *tmp = search_recs_date(params.date2, records, hi, 0);

		if (tmp == NULL) {
			free(tmp);
			*err_stat = -2;
			return err_stat;
		}

		lo = tmp[0];
		hi = tmp[1];
		free(tmp);
	/* if a date range is being searched */
	} else if (params.date1 != 0 && params.date2 != 0) {
		if (params.date1 == INT_MIN) 
			params.date1 = records[lo].date;
		if (params.date2 == INT_MAX)
			params.date2 = records[hi].date;
		

		int *tmp1 = NULL;
		int *tmp2 = NULL;

		while (tmp2 == NULL) {
			if (params.date2 < params.date1)
				break;
			tmp2 = search_recs_date(params.date2, records, n_recs-1, 0);
			params.date2--;
		}
		while (tmp1 == NULL) {
			if (params.date1 > params.date2)
				break;
			tmp1 = search_recs_date(params.date1, records, n_recs-1, 0);
			params.date1++;
		}

		if (tmp1 == NULL || tmp2 == NULL) {
			*err_stat = -1;
			free(tmp1);
			free(tmp2);
			return err_stat;
		}
		
		lo = tmp1[0];
		hi = tmp2[1];
		free(tmp1);
		free(tmp2);
	}

	/* get matches for amt and tag searches */
	/* amount param is always initialized, no need for branching */
	amt_matches = search_recs_amount(params.amnt_bound1, params.amnt_bound2, records, lo, hi);
	tag_matches = search_recs_tags(params.tags, params.n_tags, records, lo, hi);

	/* results are not returned for the amt or tag parameters */
	if (amt_matches == NULL || tag_matches == NULL) {
		if (tag_matches != NULL)
			*err_stat = -3;
		else if (amt_matches != NULL)
			*err_stat = -4;
		else
			*err_stat = -5;

		free(amt_matches);
		free(tag_matches);
		return err_stat;
	/* both parameters return results */
	} else {
		int n_matches = 0;
		match_inds = malloc((amt_matches[0]+1)*sizeof(int));

		int *ref_arr = (amt_matches[0] < tag_matches[0] ? amt_matches : tag_matches);
		int *sch_arr = (ref_arr == amt_matches ? tag_matches : amt_matches);
		int sch_lo = 1;

		for (int ref_i = 1; ref_i <= ref_arr[0]; ref_i++) {
			int *result = binary_search(ref_arr[ref_i], sch_arr, sch_arr[0], sch_lo);
			if (result != NULL) {
				match_inds[n_matches+1] = ref_arr[ref_i];
				sch_lo = result[0];
				free(result);
				n_matches++;
			}
		}
		match_inds = realloc(match_inds, (n_matches+1)*sizeof(int));
		match_inds[0] = n_matches;
	}

	free(amt_matches);
	free(tag_matches);
	free(err_stat);
	return match_inds;
}

struct record_t *add_records(struct NewRecs_t *new_recs, struct record_t *records, int *n_recs, char *rec_filename, float tot_cash)
{	
	if (new_recs == NULL)
		return records;

	FILE *rec_file = fopen(rec_filename, "w");

	fprintf(rec_file, "%.2f\n", tot_cash);

	int n_newrecs = 0;
	struct NewRecs_t *tmp_new = new_recs;
	while (tmp_new != NULL) {
		n_newrecs++;
		tmp_new = tmp_new->next;
	}

	struct record_t *all_recs = malloc((n_newrecs+*n_recs)*sizeof(struct record_t));
	struct record_t *tmp      = malloc((n_newrecs+*n_recs)*sizeof(struct record_t));

	int i;
	for (i = 0; i < *n_recs; i++)
		all_recs[i] = records[i];
	tmp_new = new_recs;
	for (i = i; i < *n_recs+n_newrecs; i++) {
		all_recs[i] = tmp_new->data;
		tmp_new = tmp_new->next;
	}

	sort_recs_date(all_recs, tmp, *n_recs+n_newrecs, 0);

	for (int j = 0; j < *n_recs+n_newrecs; j++)
		print_rec_to_file(rec_file, all_recs[j]);

	if (tmp != all_recs)
		free(tmp);
	free(records);
	*n_recs += n_newrecs;

	fclose(rec_file);	
	return all_recs;
}
