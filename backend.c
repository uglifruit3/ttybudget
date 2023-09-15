#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <float.h>
#include <limits.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
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

int *binary_search(int term, int *list, int hi, int lo)
{
	int ref = (((float)hi - (float)lo) / 2.0) + lo;
	int max_ind = hi;

	if (list[ref] == term) {
		while (ref >= 0 && list[ref-1] == term)
			ref--;
		int hi = ref;
		while (hi+1 <= max_ind && list[hi+1] == term)
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
	params->print_flag   = false;
	params->sort_flag    = false;
	params->show_footer  = true;
	params->reverse_flag = false;
	params->list_tags    = false;

	params->negate_date  = false;
	params->negate_tags  = false;
	params->negate_range = false;

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
	bool error = false;
	while (fgets(line, 600, infile) != NULL) {
		line[strlen(line)-1] = '\0'; /* erases newline */
		char **rec_elements = get_elements(line);

		int index = 0;

		/* checks for bad records file values as well */
		initialize_record(&records[i]);
		errno = 0;
		records[i].amount = strtof(rec_elements[0], NULL);
		if (errno != 0) {
			fprintf(stderr, "Error: bad records file initial amount value detected.\n");
		}
		if (get_date(rec_elements, &index, IN_DATE_ISO, &(records[i].date), true) != NO_ERR) {
			fprintf(stderr, "Error: bad records file date value detected.\n");
			error = true;
		}
		if (get_message(rec_elements, &index, records[i].message) != NO_ERR) {
			fprintf(stderr, "Error: bad records file message value detected.\n");
			error = true;
		}
		if (rec_elements[3][0] != '\0') {
			if (get_tags(rec_elements, &index, &(records[i].tags), &(records[i].n_tags)) != NO_ERR) {
				fprintf(stderr, "Error: bad records file tag value(s) detected.\n");
				error = true;
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

void initialize_defaults(struct defaults_t *defaults)
{
	defaults->in_date_frmt = defaults->out_date_frmt = defaults->change_flag = 0;
	defaults->currency_char = '\0';
	defaults->currency_defined = false;
	defaults->print_mode = 0;
	defaults->color_on = false;
	memset(defaults->recs_file, '\0', 256);

	return;
}

bool dir_exists(char dir[], int *err)
{
	struct stat dirstat;
	int f_stat = stat(dir, &dirstat);
	if (f_stat == 0 && S_ISDIR(dirstat.st_mode) != 0) {
		return true;
	} else if (f_stat == -1) {
		fprintf(stderr, "Error: unable to stat \"%s\"--exiting.\n", dir);
		*err = SYS_ERR;
		return false;
	} else {
		return false;
	}
}

FILE *open_defaults_file(char *mode, int *err, int print_mode)
{
	/* getting the filepath */
	char defs_path[256];
	memset(defs_path, '\0', 256);
	strncpy(defs_path, getenv("HOME"), 128);
	strcat(defs_path, "/.config");

	if (!dir_exists(defs_path, err) && *err != SYS_ERR) 
		mkdir(defs_path, S_IRWXU|S_IRGRP|S_IXGRP|S_IXOTH);
	else if (*err == SYS_ERR)
		return NULL;

	strcat(defs_path, "/ttybudget");
	if (!dir_exists(defs_path, err) && *err != SYS_ERR) 
		mkdir(defs_path, S_IRWXU|S_IRGRP|S_IXGRP|S_IXOTH);
	else if (*err == SYS_ERR)
		return NULL;
	strcat(defs_path, "/defaults.conf");
	errno = 0;

	FILE *defs_file = fopen(defs_path, mode);
	/* if the file does not exist */
	if (!defs_file && errno == ENOENT) {
		errno = 0;
		if (print_mode != NO_WARNINGS)
			fprintf(stderr, "(%i): Warning: no config file found (%s does not exist).\n", ++(*Warning_Count), defs_path);
		*err = USR_ERR;
		return NULL;
	/* otherwise, there is a problem opening the file */
	} else if (errno || !defs_file) {
		errno = 0;
		if (print_mode != NO_WARNINGS)
			fprintf(stderr, "(%i): Warning: could not open config file \"%s\".\n", ++(*Warning_Count), defs_path);
		*err = SYS_ERR;
		return NULL;
	}

	/* must be closed outside of this function */
	return defs_file;
}

char *get_defs_buffer(FILE *defs_file, int *err)
{
	if (fseek(defs_file, 0L, SEEK_END) == 0) {
		long file_size = ftell(defs_file);

		char *buf = malloc((file_size+1) * sizeof(char));
		rewind(defs_file);
		size_t read_size = fread(buf, sizeof(char), file_size, defs_file);

		if (ferror(defs_file) != 0) {
			fprintf(stderr, "Error: could not read config file.\n");
			free(buf);
			*err = SYS_ERR;
			return NULL;
		}

		buf[read_size] = '\0';
		rewind(defs_file);
		return buf;
	} else {
		fprintf(stderr, "Error: could not read config file.\n");
		*err = SYS_ERR;
		return NULL;
	}
}

int parse_defaults_buffer(char *buf, struct defaults_t *defs)
{
	char *def_opts[] = {"date-in", "date-out", "default-path", "currency-char", "color"};
	int no_def_opts = 5;
	char *date_frmts[] = {"iso", "us", "long", "abbr"};
	char *color_options[] = {"true", "on", "false", "off"};
	int i = 0;
	char option[16];
	char param[128];

	/* parsing buffer */
	while (buf[i] != '\0') {
		/* if buf[i] is not a comment */
		if (buf[i] != '#') {
			memset(option, '\0', 16);
			memset(param, '\0', 128);
			int ind = i;
			/* get the option */
			while (buf[i] != '=') {
				if (i > ind+16) {
					fprintf(stderr, "Error: bad syntax in config file.\n");
					return USR_ERR;
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
					return USR_ERR;
				} else {
					param[i-ind] = buf[i];
					i++;
				}
			}

			int tmp;
			/* identify which option has been invoked and store its parameter in the defaults structure */
			switch (string_in_list(option, def_opts, no_def_opts)) {
			case 0: /* date-in */
				tmp = string_in_list(param, date_frmts, 4);
				if (tmp == 0 || tmp == 1) {
					if (defs->in_date_frmt == 0)
						defs->in_date_frmt = tmp + IN_DATE_ISO;
				} else if (defs->print_mode != NO_WARNINGS) {
					fprintf(stderr, "(%i): Warning: default date-in setting \"%s\" not valid. Option will be left undefined.\n", ++(*Warning_Count), param);
				}
				break;
			case 1: /* date-out */
				tmp = string_in_list(param, date_frmts, 4);
				if (tmp != -1) {
					if (defs->out_date_frmt == 0)
						defs->out_date_frmt = tmp + OUT_DATE_ISO;
				} else if (defs->print_mode != NO_WARNINGS) {
					fprintf(stderr, "(%i): Warning: default date-out setting \"%s\" not valid. Option will be left undefined.\n", ++(*Warning_Count), param);
				}
				break;
			case 2: /* default path */
				tmp = 1;
				if ((param[i-ind-1] != '\"' || param[0] != '\"') && defs->print_mode != NO_WARNINGS) {
					fprintf(stderr, "(%i): Warning: default records file path must be enclosed in quotes. Option will be left undefined.\n", ++(*Warning_Count));
					break;
				} 
				
				if (defs->recs_file[0] != '\0')
					break;

				while (param[tmp] != '\"') {
					param[tmp-1] = param[tmp];
					tmp++;
				}
				param[tmp] = param[tmp-1] = '\0';

				strncpy(defs->recs_file, param, 256);
				break;
			case 3: /* default currency character */
				if (param[1] != '\n' && param[1] != '\0' && defs->print_mode != NO_WARNINGS) {
					fprintf(stderr, "(%i): Warning: default currency character incorrectly specified. Must be one character. Defaulting to none.\n", ++(*Warning_Count));
				} else if (!defs->currency_defined) {
					defs->currency_char = param[0];
					defs->currency_defined = true;
				}
				break;
			case 4: /* color printing */
				tmp = string_in_list(param, color_options, 4);
				if (tmp == 0 || tmp == 1) {
					defs->color_on = true;
					defs->color_defined = true;
				} else if (tmp == 2 || tmp == 3) {
					defs->color_on = false;
					defs->color_defined = true;
				} else {
					if (defs->print_mode != NO_WARNINGS)
						fprintf(stderr, "(%i): Warning: default color printing option \"%s\" not valid. Defaulting to none.\n", ++(*Warning_Count), param);
					defs->color_on = false;
					defs->color_defined = true;
				}
				break;
			default:
				fprintf(stderr, "Error: bad syntax in config file.\n");
				return USR_ERR;
			}
		} 
		/* advance to the next line */
		while (buf[i] != '\n') 
			i++;
		i++;
	}

	return NO_ERR;
}

int read_defaults(struct defaults_t *defs)
{
	int freaderr = NO_ERR;

	FILE *defs_file = open_defaults_file("r", &freaderr, defs->print_mode);
	if (defs_file) {
		char *buf = get_defs_buffer(defs_file, &freaderr);
		fclose(defs_file);
		if (buf == NULL || freaderr) 
			return SYS_ERR;

		int err = parse_defaults_buffer(buf, defs);
		free(buf);

		if (err)
			return err;
	}
	
	/* check if any of the defaults have not been defined */
	if (defs->out_date_frmt == 0) {
		if (defs->print_mode != NO_WARNINGS)
			fprintf(stderr, "(%i): Warning: default out date format not defined--initializing to abbr format.\n", ++(*Warning_Count));
		defs->out_date_frmt = OUT_DATE_ABBR;
	}
	if (defs->in_date_frmt == 0) {
		if (defs->print_mode != NO_WARNINGS)
			fprintf(stderr, "(%i): Warning: default in date format not defined--initializing to iso format.\n", ++(*Warning_Count));
		defs->in_date_frmt = IN_DATE_ISO;
	}
	if (defs->recs_file[0] == '\0' && defs->print_mode != NO_WARNINGS) {
		fprintf(stderr, "(%i): Warning: default records file path not defined.\n", ++(*Warning_Count));
	}
	if (!defs->color_defined) {
		if (defs->print_mode != NO_WARNINGS)
			fprintf(stderr, "(%i): Warning: default color printing mode not defined--initializing to off.\n", ++(*Warning_Count));
		defs->color_on = false;
	}


	/* need to expand the ~ macro if in the specified path */
	char *tilde_ptr = strchr(defs->recs_file, '~');
	/* if ~ is present in specified path */
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
	return NO_ERR;
}

/* call with runs=0 initially */
void sort_recs_amounts(struct record_t *list, struct record_t *tmp, int n, int runs)
{
	int sub_width = pow(2, runs);
	if (sub_width > n) {
		if (runs % 2 != 0) {
			for (int i = 0; i < n; i++) 
				tmp[i] = mk_record_cpy(list[i]);
		}
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
		if (runs % 2 != 0) {
			for (int i = 0; i < n; i++) 
				tmp[i] = mk_record_cpy(list[i]);
		}
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

void sort_taglist(struct tagnode_t *taglist, struct tagnode_t *tmp, int n, int runs)
{
	int sub_width = pow(2, runs);
	if (sub_width > n) {
		if (runs % 2 != 0) {
			for (int i = 0; i < n; i++) {
				strcpy(tmp[i].tag, taglist[i].tag);
				tmp[i].times = taglist[i].times;
			}
		}	
		return;
	}

	for (int i = 0; i < n; i = i + 2*sub_width) {
		int i_left = i;
		int i_right = i + sub_width;
		int j = i;
		while (i_left < i + sub_width && i_right < i + 2*sub_width) {
			if (i_left >= n || i_right >= n) {
				break;
			} else if (i_right >= n || (taglist[i_left].times >= taglist[i_right].times)) {
				tmp[j] = taglist[i_left];
				i_left++;
			} else {
				tmp[j] = taglist[i_right];
				i_right++;
			}
			j++;
		}

		if (i_left < i+sub_width) {
			while (i_left < n && i_left < i + sub_width)
				tmp[j++] = taglist[i_left++];
		}
		else if (i_right < i+2*sub_width) {
			while (i_right < n && i_right < i + 2*sub_width)
				tmp[j++] = taglist[i_right++];
		}
	}

	sort_taglist(tmp, taglist, n, ++runs);
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

void negate_matches(int **matches, int n_recs)
{
		/* if inverted results will contain all or no records */
		if (*matches == NULL) {
			*matches = malloc(sizeof(int));
			(*matches)[0] = 0;
		} else if ((*matches)[0] == n_recs) {
			free(*matches);
			*matches = NULL;
		}
			
		int *tmp = malloc((n_recs - (*matches)[0] + 1)*sizeof(int));
		tmp[0] = n_recs - (*matches)[0];
		int ind = 1;

		for (int i = 0; i < n_recs; i++) {
			for (int j = 1; j <= (*matches)[0]; j++) {
				if (i == (*matches)[j]) {
					i++;
					j = 1;
				}
			}

		if (ind == (n_recs - (*matches)[0] + 1))
			break;
		tmp[ind] = i;
		ind++;
		}	

		free(*matches);
		*matches = tmp;
}

int *search_recs_amount(float amnt1, float amnt2, bool negate, struct record_t *records, int n_recs, int *search_arr)
{
	if (isnan(amnt1) && isnan(amnt2)) {
		amnt1 = -INFINITY;
		amnt2 =  INFINITY;
	}

	int *match_indices = malloc((search_arr[0]+1)*sizeof(int));
	match_indices[0] = 0;

	int j = 1;
	for (int i = 1; i <= search_arr[0]; i++) {
		if (records[search_arr[i]].amount >= amnt1 && records[search_arr[i]].amount <= amnt2) {
			match_indices[j] = search_arr[i];
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

	if (negate)
		negate_matches(&match_indices, n_recs);

	return match_indices;
}

int *search_recs_tags(char **tags, int n_tags, bool negate, struct record_t *records, int n_recs, int *search_arr)
{
	/* if tags are unitialized, don't bother searching for tags */
	if (n_tags == 0) {
		int *matches = malloc((search_arr[0]+1)*sizeof(int));
		matches[0] = search_arr[0]+1;
		for (int i = 1; i <= search_arr[0]; i++)
			matches[i] = search_arr[i];
		return matches;
	}

	char **tags_cpy = malloc(n_tags*sizeof(char*));
	for (int i = 0; i < n_tags; i++) {
		tags_cpy[i] = malloc(MAX_TAG_LEN*sizeof(char));
		strcpy(tags_cpy[i], tags[i]);
	}

	int *match_indices = malloc((search_arr[0]+1)*sizeof(int));
	match_indices[0] = 0;

	int j = 1;
	for (int i = 1; i <= search_arr[0]; i++) {
		for (int k = 0; k < records[search_arr[i]].n_tags; k++) {
			if (string_in_list(records[search_arr[i]].tags[k], tags_cpy, n_tags) != -1) {
				match_indices[j] = search_arr[i];
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

	if (negate)
		negate_matches(&match_indices, n_recs);

	return match_indices;
}

int in_taglist(struct tagnode_t *taglist, char *tag, int n_tags)
{
	for (int i = 0; i < n_tags; i++) {
		if (!strcmp(taglist[i].tag, tag))
			return i;
	}

	return -1;
}

struct tagnode_t *get_tag_list(struct record_t *records, int n_recs, int *n_tags)
{
	*n_tags = 0;
	/* get the number of tags globally ocurring */
	for (int i = 0; i < n_recs; i++) 
		*n_tags += records[i].n_tags;		
	if (*n_tags == 0)
		return NULL;

	/* will need to resize at function conclusion */
	struct tagnode_t *alltags = malloc(*n_tags * sizeof(struct tagnode_t));
	for (int i = 0; i < *n_tags; i++)
		memset(alltags[i].tag, '\0', 32);

	int n_uniqtags = 0, k = 0;
	for (int i = 0; i < n_recs; i++) {
		for (int j = 0; j < records[i].n_tags; j++) {
			int tmp = in_taglist(alltags, records[i].tags[j], n_uniqtags);
			/* the tag has not been read until now */
			if (tmp == -1) {
				strcpy(alltags[k].tag, records[i].tags[j]);
				alltags[k].times = 1;
				k++;
				n_uniqtags++;
			/* the tag is already stored at alltags[tmp] */
			} else {
				alltags[tmp].times++;
			}
		}
	}

	*n_tags = n_uniqtags;
	return alltags;	
}

int *search_records(struct record_t *records, int n_recs, struct search_param_t params)
{
	/* status tracks the matching status of the search.
	 * see table of values in backend.h above this function
	 * prototype */
	int *status = malloc(sizeof(int));
	int *date_matches = NULL;
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
			*status = -2;
			return status;
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
		/* need to add 1 to keep date2 reflective of final date found */
		params.date2++;
		while (tmp1 == NULL) {
			if (params.date1 > params.date2)
				break;
			tmp1 = search_recs_date(params.date1, records, n_recs-1, 0);
			params.date1++;
		}

		if (tmp1 == NULL || tmp2 == NULL) {
			*status = -1;
			free(tmp1);
			free(tmp2);
			return status;
		}
		
		lo = tmp1[0];
		hi = tmp2[1];
		free(tmp1);
		free(tmp2);
	}

	if (params.negate_date) {
		date_matches = malloc((n_recs - (hi-lo))*sizeof(int));
		date_matches[0] = n_recs - (hi-lo) - 1;
		/* get those leading up to lo */
		for (int i = 0; i < lo; i++)
			date_matches[i+1] = i;
		/* get those after hi */
		for (int i = hi+1; i < n_recs; i++)
			date_matches[i-(hi-lo)] = i;
	} else {
		date_matches = malloc((hi - lo + 2)*sizeof(int));
		date_matches[0] = hi - lo + 1;
		for (int i = lo; i <= hi; i++)
			date_matches[i-lo+1] = i;
	}

	/* get matches for amt and tag searches */
	/* amount param is always initialized, no need for branching */
	amt_matches = search_recs_amount(params.amnt_bound1, params.amnt_bound2, params.negate_range, records, n_recs, date_matches);
	tag_matches = search_recs_tags(params.tags, params.n_tags, params.negate_tags, records, n_recs, date_matches);
	free(date_matches);

	/* results are not returned for the amt or tag parameters */
	if (amt_matches == NULL || tag_matches == NULL) {
		if (tag_matches != NULL)
			*status = -3;
		else if (amt_matches != NULL)
			*status = -4;
		else
			*status = -5;

		free(amt_matches);
		free(tag_matches);
		return status;
	/* both parameters return results;
	 * need to find records which match both searches */
	} else {
		int n_matches = 0;
		match_inds = malloc((amt_matches[0]+1)*sizeof(int));

		/* becomes smallest of either match array */
		int *ref_arr = (amt_matches[0] < tag_matches[0] ? amt_matches : tag_matches);
		/* becomes that which ref_arr is not */
		int *sch_arr = (ref_arr == amt_matches ? tag_matches : amt_matches);
		int sch_lo = 1;

		/* searches for and collects the records that meet the search criteria for
		 * ref_arr within  sch_arr */
		for (int ref_i = 1; ref_i <= ref_arr[0]; ref_i++) {
			int *result = binary_search(ref_arr[ref_i], sch_arr, sch_arr[0], sch_lo);
			if (result != NULL) {
				match_inds[n_matches+1] = ref_arr[ref_i];
				sch_lo = result[0];
				free(result);
				n_matches++;
			}
		}
		if (amt_matches[0] != n_matches)
			match_inds = realloc(match_inds, (n_matches+1)*sizeof(int));
		match_inds[0] = n_matches;
	}

	free(amt_matches);
	free(tag_matches);
	free(status);
	return match_inds;
}

struct record_t mk_record_cpy(struct record_t rec_orig)
{
	struct record_t rec_dest;
	initialize_record(&rec_dest);

	rec_dest.amount = rec_orig.amount;
	rec_dest.date   = rec_orig.date;

	strcpy(rec_dest.message, rec_orig.message);

	rec_dest.n_tags = rec_orig.n_tags;
	if (rec_dest.n_tags > 0) {
		rec_dest.tags = malloc(rec_dest.n_tags * sizeof(char *));
		for (int j = 0; j < rec_orig.n_tags; j++) {
			rec_dest.tags[j] = calloc(MAX_TAG_LEN, sizeof(char));
			strcpy(rec_dest.tags[j], rec_orig.tags[j]);
		}
	}

	return rec_dest;
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
	/* copy everything in original records array */
	for (i = 0; i < *n_recs; i++)
		all_recs[i] = mk_record_cpy(records[i]);
	/* copy everything in new records list */
	tmp_new = new_recs;
	for (i = i; i < *n_recs+n_newrecs; i++) {
		all_recs[i] = mk_record_cpy(tmp_new->data);
		tmp_new = tmp_new->next;
	}

	sort_recs_date(all_recs, tmp, *n_recs+n_newrecs, 0);

	if (tmp != all_recs)
		free(tmp);
	free_recs_array(records, *n_recs);
	*n_recs += n_newrecs;

	return all_recs;
}
