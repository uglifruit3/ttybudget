#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <float.h>
#include <limits.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "backend.h"
#include "io.h"

void free_array(char **array, int n)
{
	for (int i = 0; i < n; i++) 
		free(array[i]);

	free(array);

	return;
}

void arr_cpy(struct record_t *dest, struct record_t *src, int n)
{
	for (int i = 0; i < n; i++)
		dest[i] = src[i];

	return;
}
void alt_arr_cpy(struct sort_t *dest, struct sort_t *src, int n)
{
	for (int i = 0; i < n; i++)
		dest[i] = src[i];

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

void free_recs_array(struct record_t *records, int n_recs, int del_tags)
{
	if (records == NULL) 
		return;
	for (int i = 0; i < n_recs; i++) {
		if (del_tags == FALSE || records[i].n_tags == 0)
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
	params->print_flag   = FALSE;
	params->sort_flag    = FALSE;
	params->show_footer  = TRUE;
	params->reverse_flag = FALSE;

	params->amnt_bound1 = NAN;
	params->amnt_bound2 = NAN;

	params->date1 = params->date2 = 0;

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

			
struct record_t *get_records_array(FILE *infile, int num_records, float *start_amnt)
{
	/* stored record format:
	 * AMOUNT | DATE | MESSAGE | TAGS 
	 *   0       1        2       3   */

	char line[600];
	fgets(line, 600, infile);
	*start_amnt = atof(line);

	if (num_records == 0)
		return NULL;

	struct record_t *records = malloc(num_records*sizeof(struct record_t));

	int i = 0;
	int error = FALSE;
	while (fgets(line, 600, infile) != NULL) {
		line[strlen(line)-1] = '\0'; /* erases newline */
		char **rec_elements = get_elements(line);

		int index = 0;

		/* checks for bad records file values as well */
		initialize_record(&records[i]);
		records[i].amount = atof(rec_elements[0]);
		if (get_date(rec_elements, &index, IN_DATE_ISO, &(records[i].date), TRUE) != FALSE) {
			fprintf(stderr, "Warning: bad records file date value detected.\n");
			error = TRUE;
		}
		if (get_message(rec_elements, &index, records[i].message) != FALSE) {
			fprintf(stderr, "Warning: bad records file message value detected.\n");
			error = TRUE;
		}
		if (rec_elements[3][0] != '\0') {
			if (get_tags(rec_elements, &index, &(records[i].tags), &(records[i].n_tags)) != FALSE) {
				fprintf(stderr, "Warning: bad records file tag value(s) detected.\n");
				error = TRUE;
			}
		}


		free_array(rec_elements, 4);

		i++;
	}
	
	if (error) {
		free(records);
		records = NULL;
	}
	return records;
}

int dir_exists(char dir[])
{
	struct stat dirstat;
	if (stat(dir, &dirstat) == 0 && S_ISDIR(dirstat.st_mode) != 0)
		return TRUE;
	else
		return FALSE;
}

FILE *open_defaults_file(char *mode)
{
	/* getting the filepath */
	char defs_path[256];
	memset(defs_path, '\0', 256);
	strncpy(defs_path, getenv("HOME"), 128);
	strcat(defs_path, "/.config");
	if (!dir_exists(defs_path)) 
		mkdir(defs_path, S_IRWXU|S_IRGRP|S_IXGRP|S_IXOTH);
	strcat(defs_path, "/ttybudget");
	if (!dir_exists(defs_path)) 
		mkdir(defs_path, S_IRWXU|S_IRGRP|S_IXGRP|S_IXOTH);
	strcat(defs_path, "/defaults.conf");
	errno = 0;

	FILE *defs_file = fopen(defs_path, mode);
	/* if the file does not exist, initialize it */
	if (!defs_file && errno == ENOENT) {
		errno = 0;
		fprintf(stderr, "Warning: no config file found (%s does not exist).\n", defs_path);
		return NULL;
	/* otherwise, there is a problem opening the file */
	} else if (errno || !defs_file) {
		errno = 0;
		fprintf(stderr, "Warning: could not open config file \"%s\".\n", defs_path);
		return NULL;
	}

	/* must be closed outside of this function */
	return defs_file;
}

char *get_defs_buffer(FILE *defs_file)
{
	if (fseek(defs_file, 0L, SEEK_END) == 0) {
		long file_size = ftell(defs_file);

		char *buf = malloc((file_size+1) * sizeof(char));
		rewind(defs_file);
		size_t read_size = fread(buf, sizeof(char), file_size, defs_file);

		if (ferror(defs_file) != 0) {
			fprintf(stderr, "Error: could not read config file.\n");
			free(buf);
			return NULL;
		}

		buf[read_size] = '\0';
		/* remember to free() outside here */
		rewind(defs_file);
		return buf;
	} else {
		fprintf(stderr, "Error: could not read config file.\n");
		return NULL;
	}
}

int read_defaults(struct defaults_t *defs)
{
	/* initialize defaults */
	defs->in_date_frmt = defs->out_date_frmt = defs->currency_char = defs->change_flag = 0;
	memset(defs->recs_file, '\0', 256);

	FILE *defs_file = open_defaults_file("r");
	if (defs_file == NULL) {
		fprintf(stderr, "Warning: default out date format not defined--initializing to abbr format.\n");
		fprintf(stderr, "Warning: default in date format not defined--initializing to iso format.\n");
		fprintf(stderr, "Warning: default records file path not defined.\n");

		defs->out_date_frmt = OUT_DATE_ABBR;
		defs->in_date_frmt = IN_DATE_ISO;

		return 0;
	}

	char *buf = get_defs_buffer(defs_file);
	fclose(defs_file);

	char *def_opts[] = {"date-in", "date-out", "default-path", "currency-char"};
	char *date_frmts[] = {"iso", "us", "long", "abbr"};
	int i = 0;
	char option[16];
	char param[128];

	/* parsing buffer */
	while (buf[i] != '\0') {
		if (buf[i] != '#') {
			memset(option, '\0', 16);
			memset(param, '\0', 128);
			int ind = i;
			/* get the option */
			while (buf[i] != '=') {
				if (i > ind+16) {
					fprintf(stderr, "Error: bad syntax in config file.\n");
					free(buf);
					return 1;
				} else {
					option[i-ind] = buf[i];
					i++;
				}
			}	
			option[i-ind] = '\0';
			i++;
			ind = i;
			/* get the parameter corresponding to that option */
			while (buf[i] != '\n' && buf[i] != '#') {
				if (i > ind+128) {
					fprintf(stderr, "Error: bad syntax in config file.\n");
					free(buf);
					return 1;
				} else {
					param[i-ind] = buf[i];
					i++;
				}
			}

			int tmp;
			/* identify which option has been invoked and store its parameter in the defaults structure */
			switch (string_in_list(option, def_opts, 4)) {
			case 0: /* date-in */
				tmp = string_in_list(param, date_frmts, 4);
				if (tmp == 0 || tmp == 1) {
					defs->in_date_frmt = tmp + IN_DATE_ISO;
				} else {
					fprintf(stderr, "Error: default date-in option \"%s\" not valid--exiting.\n", param);
					free(buf);
					return 1;
				}
				break;
			case 1: /* date-out */
				tmp = string_in_list(param, date_frmts, 4);
				if (tmp != -1) {
					defs->out_date_frmt = tmp + OUT_DATE_ISO;
				} else {
					fprintf(stderr, "Error: default date-out option \"%s\" not valid--exiting.\n", param);
					free(buf);
					return 1;
				}
				break;
			case 2: /* default path */
				tmp = 1;
				if (param[i-ind-1] != '\"' || param[0] != '\"') {
					fprintf(stderr, "Error: default records file path must be enclosed in quotes--exiting.\n");
					free(buf);
					return 1;
				}
				while (param[tmp] != '\"') {
					param[tmp-1] = param[tmp];
					tmp++;
				}
				param[tmp] = param[tmp-1] = '\0';

				strncpy(defs->recs_file, param, 256);
				break;
			case 3: /* default currency character */
				if (param[1] != '\n' && param[1] != '\0') {
					fprintf(stderr, "Error: currency character incorrectly specified. Must be one character.\n");
					free(buf);
					return 1;
				}
				defs->currency_char = param[0];
				break;
			}
		} 
		/* advance to the next line */
		while (buf[i] != '\n') 
			i++;
		i++;
	}

	free(buf);
	/* check if any of the defaults have not been defined */
	if (defs->out_date_frmt == 0) {
		fprintf(stderr, "Warning: default out date format not defined--initializing to abbr format.\n");
		defs->out_date_frmt = OUT_DATE_ABBR;
	}
	if (defs->in_date_frmt == 0) {
		fprintf(stderr, "Warning: default in date format not defined--initializing to iso format.\n");
		defs->in_date_frmt = IN_DATE_ISO;
	}
	if (defs->recs_file[0] == '\0') {
		fprintf(stderr, "Warning: default records file path not defined.\n");
	}
	/* need to perform check for definition of default records path after command line has been parsed for possible file specification */

	/* need to expand the ~ macro if in the specified path */
	char *tilde_ptr = strchr(defs->recs_file, '~');
	if (tilde_ptr != NULL) {
		int len = strlen(defs->recs_file);
		char *tmpstr = malloc(len*sizeof(char));
		memset(tmpstr, '\0', len);
		/* copy everything after ~ into tmpstr */
		char *ptr = tilde_ptr + 1;
		int i = 0;
		while (*ptr != '\0') {
			tmpstr[i] = *ptr;
			ptr++;
			i++;
		}
		/* clear defs->recs_file and put in home directory path */
		memset(defs->recs_file, '\0', 256);
		strncpy(defs->recs_file, getenv("HOME"), 256);
		/* append path following ~ */
		strncat(defs->recs_file, tmpstr, len);

		free(tmpstr);
	}
	return 0;
}
					
void write_defaults(struct defaults_t defs)
{
	FILE *defs_file = open_defaults_file("r+");
	if (defs_file == NULL)
		return;
	char *buf = get_defs_buffer(defs_file);

	ftruncate(fileno(defs_file), 0);

	char option[16];
	char *date_frmts[] = {"iso", "us", "long", "abbr"};
	char *def_opts[] = {"date-in", "date-out", "default-path", "currency-char"};
	int opt_inds[4];   /* order of option indices occurs in same order as in def_opts; indices indicate position of corresponding '=' */
	int i = 0;
	/* determine where in the file options are invoked */
	while (buf[i] != '\0') {
		if (buf[i] != '#') {
			memset(option, '\0', 16);
			int ind = i;
			while (buf[i] != '=') {
				option[i-ind] = buf[i];
				i++;
			}
			i++;

			switch (string_in_list(option, def_opts, 4)) {
			case 0: /* date-in */
				opt_inds[0] = i;	
				break;
			case 1: /* date-out */
				opt_inds[1] = i;	
				break;
			case 2: /* default path */
				opt_inds[2] = i;	
				break;
			case 3: /* default currency character */
				opt_inds[3] = i;	
				break;
			}
		} 
		while (buf[i] != '\n') 
			i++;
		i++;
	}
	/* re-output to file, splicing new parameters where the old ones went */
	i = 0;
	while (buf[i] != '\0') {
		if (i == opt_inds[0]) {
			fputs(date_frmts[defs.in_date_frmt - IN_DATE_ISO], defs_file);
			while (buf[i] != ' ' && buf[i] != '\n' && buf[i] != '#')
				i++;
		} else if (i == opt_inds[1]) {
			fputs(date_frmts[defs.out_date_frmt - OUT_DATE_ISO], defs_file);
			while (buf[i] != ' ' && buf[i] != '\n' && buf[i] != '#')
				i++;
		} else if (i == opt_inds[2]) {
			fputc('\"', defs_file);
			fputs(defs.recs_file, defs_file);
			fputc('\"', defs_file);
			while (buf[i] != ' ' && buf[i] != '\n' && buf[i] != '#')
				i++;
		} else if (i == opt_inds[3]) {
			fputc(defs.currency_char, defs_file);
			while (buf[i] != ' ' && buf[i] != '\n' && buf[i] != '#')
				i++;
		} else {
			fputc(buf[i], defs_file);
			i++;
		}
	}

	fclose(defs_file);
	free(buf);
	return;
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
			} else if (i_right >= n || (list[i_left].amount >= list[i_right].amount)) {
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

struct record_t *add_records(struct NewRecs_t *new_recs, struct record_t *records, int *n_recs)
{	
	if (new_recs == NULL)
		return records;

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

	if (tmp != all_recs)
		free(tmp);
	free(records);
	*n_recs += n_newrecs;

	return all_recs;
}
