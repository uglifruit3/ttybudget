#include <unistd.h>
#include <math.h>
#include <errno.h>
#include <time.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

#include "backend.h"
#include "io.h"

void add2front(struct NewRecs_t **list, struct NewRecs_t *new) 
{
	if (*list == NULL) {
		*list = new;
		new->next = NULL;
	} else {
		new->next = *list;
		*list = new;
	}		

	return;
}

void free_list(struct NewRecs_t *list, int del_mode) 
{
	if (list == NULL)
		return;
	struct NewRecs_t *to_free;
	while (list != NULL) {
		to_free = list;
		list = list->next;

		/* tags don't need to be freed here; they share an address with tags in 
		 * the actual records array and are freed when those are freed in the 
		 * free_recs_array function */
		if (del_mode == TRUE && to_free->data.n_tags > 0) {
			for (int i = 0; i < to_free->data.n_tags; i++)
				free(to_free->data.tags[i]);
			free(to_free->data.tags);
		}
		free(to_free);
	}	
}

int string_in_list(char *string, char **list, int num_list_items)
{
	if (!string || string[0] == '\0') return -1;

	for (int i = 0; i < num_list_items; i++) {
		if(!strcmp(string, list[i]))
			return i;
	}

	return -1;
}

int is_cmdline_option(char *str1)
{
	const char *cmdline_opts[] = {
		"-a", "--add",                              /* 0-1 add */
			"-t", "--tags",                     /* 2-7 add options */
			"-m", "--message",
			"-d", "--date",
		"-p", "--print",                            /* 8-9 print */
		"-e", "--edit",                             /* 10-11 edit */
			"-q", "--query-tags",               /* 12-17 print options */
			"-i", "--interval",
			"-r", "--range",
			"-s", "--sort",
		"-h", "--help",
		"-v", "--version",
		"--iso", "--us", "--long", "--long-abbr",   /* 24-27 date formats */
		"--base-dir",                               /* 28-32 config options */
		"--currency",
		"--date-in",
		"--date-out",
		"--use-file"
	};
	const int num_opts = 33;

	int opt = string_in_list(str1, (char **)cmdline_opts, num_opts);
			
	return opt;
}

FILE *open_records_file(char file_name[256])
{
	FILE *records_file = fopen(file_name, "r");

	/* initializes records file to a starting amount of money */
	if (!records_file) {
		float amount;
		char line[256];
		char *end;

		records_file = fopen(file_name, "w");
		printf("The records file has not been initialized.\nEnter a starting amount of money: ");

		errno = 0;
		while (1) {
			fgets(line, sizeof line, stdin);
			amount = strtof(line, &end);
			if (!errno && *end == '\n') break;
			printf("Amount incorrectly specified. Re-enter: ");
		}

		fprintf(records_file, "%.2f\n", amount);
		fclose(records_file);
		records_file = fopen(file_name, "r");
	}

	return records_file;
}

void chng_date_frmt(int *format_enum, int new_format)
{
	*format_enum = new_format;
	return;
}

int get_tags(char *argv[], int *index, char ***tags, int *n_tags)
{
	*index += 1;

	if (*n_tags != 0) {
		fprintf(stderr, "Error: multiple definitions of tags.\n");
		return TRUE;
	}
	if (!argv[*index]) {
		fprintf(stderr, "Error: no argument given for tags option.\n");
		return TRUE;
	}

	int i, j, k;
	i = j = k = 0;
	char *tag_list = argv[*index];
	*n_tags = 1;
	while (i < strlen(tag_list)) {
		if (tag_list[i] == ',')
			*n_tags += 1;
		i++;
	}
	char **tmptags = malloc(*n_tags * sizeof(char*));
	for (i = 0; i < *n_tags; i++)
		tmptags[i] = calloc(MAX_TAG_LEN, sizeof(char));
	i = 0;

	char temp[MAX_TAG_LEN];
	memset(temp, '\0', MAX_TAG_LEN);
	while (i < strlen(tag_list)) {
		if (tag_list[i] == ',') {
			strncpy(tmptags[j], temp, MAX_TAG_LEN);
			j++; i++;
			k = 0;
			memset(temp, '\0', MAX_TAG_LEN);
			continue;
		} else if (!(tag_list[i] >= 'a' && tag_list[i] <= 'z') &&
				!(tag_list[i] >= 'A' && tag_list[i] <= 'Z') &&
				!(tag_list[i] >= '0' && tag_list[i] <= '9') &&
				tag_list[i] != '-' && tag_list[i] != '_') {
			fprintf(stderr, "Error: invalid characters in tag argument.\n");
			free_array(tmptags, *n_tags);
			*n_tags = 0;
			return TRUE;
		} else if (k == MAX_TAG_LEN) {
			fprintf(stderr, "Error: tag name too long (max 32 chars).\n");
			free_array(tmptags, *n_tags);
			*n_tags = 0;
			return TRUE;
		}

		temp[k] = tag_list[i];	
		k++; i++;
	}
	strncpy(tmptags[j], temp, MAX_TAG_LEN);
	*tags = tmptags;

	return FALSE;
}

int get_message(char *argv[], int *index, char message[256])
{
	*index+=1;

	if (!argv[*index]) {
		fprintf(stderr, "Error: no argument given for message option.\n");
		return TRUE;
	}

	char *msg = argv[*index];
	
	if (strlen(msg) > MAX_MSG_LEN) {
		fprintf(stderr, "Error: message length exceeds character limit (256).\n");
		return TRUE;
	} else if (message[0] != '\0') {
		fprintf(stderr, "Error: multiple definitions of message in new record.\n");
		return TRUE;
	}

	strncpy(message, msg, MAX_MSG_LEN);
	return FALSE;
}

int get_date_LONG(char *argv[], int *index, int *last_days, int *date)
{
	char yr[5], dy[3];
	char str_mo[10];
	if (!argv[*index+2] || (strlen(argv[*index]) > 2 || strlen(argv[*index+2]) != 4))
		return 3;

	strncpy(dy, argv[*index], 3);
	strncpy(str_mo, argv[*index+1], 10);
	strncpy(yr, argv[*index+2], 5);

	for (int i = 0; i < strlen(str_mo); i++) {
		str_mo[i] = tolower(str_mo[i]);
	}

	char *months_long[]  = {"january", "february", "march", "april", "may", "june", "july", "august", "september", "october", "november", "december"};
	char *months_short[] = {"jan", "feb", "mar", "apr", "may", "jun", "jul", "aug", "sep", "oct", "nov", "dec"};
	int num_mo = string_in_list(str_mo, months_long, 12) + 1;
	if (num_mo == 0) {
		num_mo = string_in_list(str_mo, months_short, 12) + 1;
		if (num_mo == 0) {
			fprintf(stderr, "Error: month \"%s\" is not valid.\n", argv[*index+1]);
			return 6;
		}
	}

	if (atoi(yr) % 4 == 0)
		last_days[1] = 29;

	if (!(atoi(dy) >= 1 && atoi(dy) <= last_days[num_mo-1])) {
		fprintf(stderr, "Error: day in date expression \"%s\" is not a calendar day.\n", dy);
		return 6;
	}

	*index += 2;
	*date += atoi(yr) * 10000;
	*date += num_mo * 100;
	*date += atoi(dy) * 1;
	return FALSE;
}
int get_date_ISO(char *date_str, int *last_days, int *date, char *argv[], int *index)
{
	char yr[5], mo[3], dy[3];
	int mo_mod = 0, dy_mod = 0;
	int date_len = 8;
	/* YYYYMMDD   YYYY-MM-DD */
	/* 01234567   0123456789 */
	if (date_str[4] == '-' || date_str[4] == '/') {
		if (date_str[7] != date_str[4])
			return 1;
		date_len = 10;
	}

	if (strlen(date_str) != date_len) {
		if (strlen(date_str) <= 2) 
			return get_date_LONG(argv, index, last_days, date);
		return 2;
	} else if (date_len == 10) {
		mo_mod = 1;
		dy_mod = 2;
	}

	for (int i = 0; i < 4; i++) {
		yr[i] = date_str[i];
		if (!(yr[i] >= '0' && yr[i] <= '9')) 
			return 3;
	}
	for (int i = 0; i < 2; i++) {
		mo[i] = date_str[4+i+mo_mod];
		dy[i] = date_str[6+i+dy_mod];
		if (!(mo[i] >= '0' && mo[i] <= '9') || !(dy[i] >= '0' && dy[i] <= '9')) 
			return 3;
	}
	yr[4] = mo[2] = dy[2] = '\0';

	if (atoi(yr) % 4 == 0)
		last_days[1] = 29;

	if (!(atoi(mo) >= 1 && atoi(mo) <= 12))
		return 4;
	else if (!(atoi(dy) >= 1 && atoi(dy) <= last_days[atoi(mo)-1]))
		return 5;

	*date += atoi(yr) * 10000;
	*date += atoi(mo) * 100;
	*date += atoi(dy) * 1;
	return FALSE;
}
int get_date_US(char *date_str, int *last_days, int *date, char *argv[], int *index)
{
	char yr[5], mo[3], dy[3];
	int dy_mod = 0, yr_mod = 0;
	int date_len = 8;
	/* MMDDYYYY   MM-DD-YYYY */
	/* 01234567   0123456789 */
	if (date_str[2] == '-' || date_str[2] == '/') {
		if (date_str[5] != date_str[2]) 
			return 1;
		date_len = 10;
	}

	if (strlen(date_str) != date_len) {
		if (strlen(date_str) <= 2) 
			return get_date_LONG(argv, index, last_days, date);
		return 2;
	} else if (date_len == 10) {
		dy_mod = 1;
		yr_mod = 2;
	}

	for (int i = 0; i < 2; i++) {
		mo[i] = date_str[i];
		dy[i] = date_str[i+2+dy_mod];
		if (!(mo[i] >= '0' && mo[i] <= '9') || !(dy[i] >= '0' && dy[i] <= '9'))
			return 3;
	}
	for (int i = 0; i < 4; i++) {
		yr[i] = date_str[i+4+yr_mod];
		if (!(yr[i] >= '0' && yr[i] <= '9'))
			return 3;
	}
	yr[4] = mo[2] = dy[2] = '\0';

	if (atoi(yr) % 4 == 0)
		last_days[1] = 29;

	if (!(atoi(mo) >= 1 && atoi(mo) <= 12))
		return 4;
	else if (!(atoi(dy) >= 1 && atoi(dy) <= last_days[atoi(mo)-1]))
		return 5;

	*date += atoi(yr) * 10000;
	*date += atoi(mo) * 100;
	*date += atoi(dy) * 1;
	return FALSE;
}
int get_date(char *argv[], int *index, int format, int *date, int accept_inf)
{
	*index += 1;

	if (*date != 0) {
		fprintf(stderr, "Error: multiple definitions of date.\n");
		return TRUE;
	}
	if (!argv[*index]) {
		fprintf(stderr, "Error: no argument given for date option.\n");
		return TRUE;
	}

	float inf_arg = strtof(argv[*index], NULL);
	if (fabs(inf_arg) == INFINITY) {
		if (accept_inf) {
			if (inf_arg < 0)
				*date = INT_MIN;
			else
				*date = INT_MAX;
			return FALSE;
		} 
		fprintf(stderr, "Error: passing infinity for a date is not permitted here.\n");
		return TRUE;
	}

	/* final day of each month, Jan to Dec */
	int max_days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

	int error = FALSE;
	if (format == DATE_ISO) 
		error = get_date_ISO(argv[*index], max_days, date, argv, index);
	else if (format == DATE_US)
		error = get_date_US(argv[*index], max_days, date, argv, index);
	/* this option exists for redundancy's sake, if for some reason the date format is set to long or long-abbr */
	else if (format == DATE_LONG || format == DATE_ABBR)
		error = get_date_LONG(argv, index, max_days, date);

	/* table of return values for get_date functions:
	 * 0/FALSE: no error
	 * 1/TRUE:  bad delimeters
	 * 2:       bad format
	 * 3:       invalid date expression
	 * 4:       invalid month value
	 * 5:       invalid day value 
	 * 6:       get_date_LONG returned an error (handles own output) */

	switch (error) {
	case 0:
		return FALSE;
	case 1:
		fprintf(stderr, "Error: date expression \"%s\" is not consistently delimited.\n", argv[*index]);
		return TRUE;
	case 2:
		fprintf(stderr, "Error: date expression \"%s\" is not correctly formatted.\n", argv[*index]);
		return TRUE;
	case 3:
		fprintf(stderr, "Error: date expression \"%s\" is invalid.\n", argv[*index]);
		return TRUE;
	case 4:
		fprintf(stderr, "Error: month in date expression \"%s\" outside valid range (1-12).\n", argv[*index]);
		return TRUE;
	case 5:
		fprintf(stderr, "Error: day in date expression \"%s\" is not a calendar day.\n", argv[*index]);
		return TRUE;
	case 6: 
		return TRUE;
	}
	return TRUE;
}
int get_current_date()
{
	struct tm time_data;
	time_t secs;
	secs = time(&secs);
	time_data = *localtime(&secs);

	/* YYYYMMDD 
	 * 01234567 */ 
	int date = 0;
	date += ((1900 + time_data.tm_year)*10000) + ((1 + time_data.tm_mon)*100) + time_data.tm_mday;

	return date;
}

int get_amount(char *argv[], int *index, float *amount, int assume_negative, int accept_inf)
{
	errno = 0;
	char *end;
	float tmp = strtof(argv[*index], &end);

	if (errno || *end) {
		fprintf(stderr, "Error: \"%s\" not a valid expression of amount.\n",
				argv[*index]);
		return TRUE;
	}
	if (fabs(tmp) == INFINITY && !accept_inf) {
		fprintf(stderr, "Error: passing infinity for an amount is not permitted here.\n");
		return TRUE;
	}
	if (assume_negative && argv[*index][0] != '+' && argv[*index][0] != '-')
		tmp *= -1;

	*index += 1;
	*amount = tmp;
	return FALSE;
}

int get_new_records(int argc, char *argv[], int *index, int date_frmt, struct NewRecs_t **new_records)
{
	int error = FALSE;
	int add_flag = FALSE;
	struct NewRecs_t *record = malloc(sizeof(struct NewRecs_t));
	initialize_record(&(record->data));
	record->next = NULL;

	while ((*index += 1) && *index < argc && !add_flag && !error) {
		switch (is_cmdline_option(argv[*index])) {
		case TAGS_S:
		case TAGS_L:
			error = get_tags(argv, index, &(record->data.tags), &(record->data.n_tags));
			break;
		case MESSAGE_S:
		case MESSAGE_L:
			error = get_message(argv, index, record->data.message);
			break;
		case DATE_S:
		case DATE_L:
			error = get_date(argv, index, date_frmt, &(record->data.date), FALSE);
			break;
		case -1:
			error = get_amount(argv, index, &(record->data.amount), TRUE, FALSE);
			if (error) 
				return TRUE;

			*index -= 1; /* decrement by one because get_amount sets two forward */
			break;
		default:
			*index -= 2; /* decrement by one so caller reads this argument */
			/* add flag is set to escape the loop and process input */
			add_flag = TRUE;
			break;
		}
	}

	add_flag = FALSE;
	/* an amount has not been defined */
	if (isnan(record->data.amount)) {
		error = TRUE;
	/* if an amount is defined and no errors are present */
	} else if (!error) {
		add2front(new_records, record);
		add_flag = TRUE;

		if (record->data.date == 0)
			record->data.date = get_current_date();
	}

	if (error || !add_flag) {
		if (isnan(record->data.amount)) {
			fprintf(stderr, "Error: amount not given for add option.\n");
		}

		free_list(record, TRUE);
		return TRUE;
	}

	return FALSE;
}

int get_print_commands(int argc, char *argv[], int *index, int date_frmt, struct search_param_t *params)
{
	int error = FALSE;

	while ((*index += 1) && *index < argc && !error) {
		switch (is_cmdline_option(argv[*index])) {
		case QUERY_S:
		case QUERY_L:
			error = get_tags(argv, index, &(params->tags), &(params->n_tags));
			break;
		case INTERVAL_S:
		case INTERVAL_L:
			error = get_date(argv, index, date_frmt, &(params->date1), TRUE);
			/* if no error, within argv still, and the next option is not another command read BOUND2 */
			if (!error && *index < argc-1 && is_cmdline_option(argv[*index+1]) == -1) {
				error = get_date(argv, index, date_frmt, &(params->date2), TRUE);
				if (error)
					*index -= 1;
			}

			/* must be ordered date1 < date2 for search function */
			if (params->date1 > params->date2) {
				int tmp = params->date2;
				params->date2 = params->date1;
				params->date1 = tmp;
			}

			break;
		case RANGE_S:
		case RANGE_L:
			*index += 1;
			if (!isnan(params->amnt_bound2)) {
				error = TRUE;
				fprintf(stderr, "Error: multiple definitions of amount range.\n");
				break;
			}

			/* get first bound */
			error = get_amount(argv, index, &(params->amnt_bound1), FALSE, TRUE);
			/* if the next argument is a number, read that as bound2 */
			if (!error && *index < argc && is_cmdline_option(argv[*index]) == -1)
				error = get_amount(argv, index, &(params->amnt_bound2), FALSE, TRUE);

			/* if bound2 goes unitialized, make it ==bound1 so we search for that amount only */
			if (isnan(params->amnt_bound2)) {
				params->amnt_bound2 = params->amnt_bound1;
			/* reorder bounds if necessary so bound1 < bound2 (for search function */
			} else if (params->amnt_bound1 > params->amnt_bound2) {
				float tmp = params->amnt_bound2;
				params->amnt_bound2 = params->amnt_bound1;
				params->amnt_bound1 = tmp;
			}

			/* accounts for get_amount incrementing index, and then the loop
			 * incrementing index */
			*index -= 1;
			break;
		case SORT_S:
		case SORT_L:
			if (params->sort_flag != FALSE) {
				fprintf(stderr, "Error: multiple invocations of \"sort\" sub-option.\n");
				error = 1;
				break;
			}

			/* sorting the records will be handled in the print function */
			params->sort_flag = TRUE;
			break;
		case -1:
			*index -= 1; /* decrement by 1 because of increment at function caller */
			return FALSE;
		case TAGS_S:
		case TAGS_L:
		case DATE_S:
		case DATE_L:
		case MESSAGE_S:
		case MESSAGE_L:
			fprintf(stderr, "Error: option \"%s\" is not a sub-option for print.\n", argv[*index]);
			error = TRUE;
			break;
		default:
			*index -= 1;
			return FALSE;
		}
	}

	return error;
}

int parse_command_line(char *argv[], int argc, char filename[], struct NewRecs_t **new_records, struct search_param_t *print_params)
{
	strcpy(filename, "records.txt");

	int in_date_frmt = DATE_ISO;
	int error = FALSE;

	if (argc == 1) {
		printf("Usage:\n  ttybudget -a|--add [DATE_FORMAT] amount [ADD_OPTIONS]\n  ttybudget -p|--print [DATE_FORMAT] [PRINT_OPTIONS]\n  ttybudget -e|--edit [DATE_FORMAT] [PRINT_OPTIONS]\n  ttybudget -h|--help\n  ttybudget -v|--version\n  ttybudget CONFIG_OPTIONS [MAIN_OPTIONS]\n");
		error = 2; 
	}

	for (int i = 1; i < argc; i++) {
		switch (is_cmdline_option(argv[i])) {
		case HELP_S:
		case HELP_L:
			printf("Usage:\n  ttybudget -a|--add [DATE_FORMAT] amount [ADD_OPTIONS]\n  ttybudget -p|--print [DATE_FORMAT] [PRINT_OPTIONS]\n  ttybudget -e|--edit [DATE_FORMAT] [PRINT_OPTIONS]\n  ttybudget -h|--help\n  ttybudget -v|--version\n  ttybudget CONFIG_OPTIONS [MAIN_OPTIONS]\n");
			error = 2; 
			break;
		case VERSION_S:
		case VERSION_L:
			printf("ttybudget v1.0 - 01 Dec 21\n");
			error = 2;
			break;
		case ADD_S: 
		case ADD_L:
			error = get_new_records(argc, argv, &i, in_date_frmt, new_records);
			break;
		case PRINT_S: 
		case PRINT_L:
			print_params->print_flag = TRUE;
			error = get_print_commands(argc, argv, &i, in_date_frmt, print_params);
			break;
		// TODO: consider ditching these flags in exchange for using those under CONFIG OPTIONS to alter date formats
		case DATE_ISO: 
		case DATE_US: 
		case DATE_LONG:
		case DATE_ABBR:
			in_date_frmt = is_cmdline_option(argv[i]);
			break;
		// TODO: create a config file with syntax, write functions to read/write defaults to it, and use those defaults in the program
		case BASE_DIR: 
		case CURRENCY: 
		case OUT_DATE: 
		case IN_DATE: 
			printf("Changing config\n");
			break;
		case USE_FILE:
			if ( is_cmdline_option(argv[i+1]) >= ADD_S) 
				printf("Error: no argument given for \"%s\".\n", argv[i]);
			else
				strncpy(filename, argv[++i], 256);
			break;
		case -1:
			printf("Error: invalid option \"%s\".\n", argv[i]);
			error = TRUE;
			break;
		default:
			printf("Error: sub-option \"%s\" lacks parent.\n", argv[i]);
			error = TRUE;
			break;
		}

		if (error)
			break;
	}

	return error;
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

void write_to_file(struct record_t *records, int n_recs, float start_amnt, char *rec_filename)
{
	FILE *rec_file = fopen(rec_filename, "w");

	fprintf(rec_file, "%.2f\n", start_amnt);

	for (int i = 0; i < n_recs; i++)
		print_rec_to_file(rec_file, records[i]);

	fclose(rec_file);	
	return;
}

void print_table_footer(struct record_t *records, int *matches, float start_amnt)
{
	/* get funds at start of period */
	for (int i = 0; i < matches[1]; i++) 
		start_amnt += records[i].amount;	
	/* get funds at end of period */
	float end_amnt = start_amnt;
	for (int i = matches[1]; i <= matches[matches[0]]; i++) 
		end_amnt += records[i].amount;
	/* get selected records total */
	float recs_tot = 0;
	for (int i = matches[1]; i <= matches[matches[0]]; i++)
		recs_tot += records[i].amount;
	/* change in funds */
	float delta = end_amnt - start_amnt;

	/* display output */
	char signs[] = {'-', '+'};
	printf("=========================================================================\n");
	printf(" Funds at start of period: %c$%-10.2f | Change in funds:     %c$%-10.2f\n", signs[start_amnt >= 0], fabs(start_amnt), signs[delta >= 0], fabs(delta));
	printf(" Funds at end of period:   %c$%-10.2f |\n", signs[end_amnt >= 0], fabs(end_amnt));
	printf(" Selected records total:   %c$%-10.2f | Cash flow in period: %s\n\n", signs[recs_tot >= 0], fabs(recs_tot), (delta >= 0 ? "POSITIVE":"NEGATIVE"));

	return;
}

void print_date_ISO(struct record_t record)
{
	int yr = record.date/10000;
	int mo = (record.date - yr*10000)/100;
	int dy = record.date - yr*10000 - mo*100;
	printf(" %i-%02i-%02i ", yr, mo, dy);
	return;
}
void print_date_US(struct record_t record)
{
	int yr = record.date/10000;
	int mo = (record.date - yr*10000)/100;
	int dy = record.date - yr*10000 - mo*100;
	printf(" %02i-%02i-%i ", mo, dy, yr);
	return;
}
void print_date_LONG(struct record_t record, int abbreviated)
{
	int yr = record.date/10000;
	int mo = (record.date - yr*10000)/100;
	int dy = record.date - yr*10000 - mo*100;

	if (abbreviated) {
		char *s_mos[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
		printf(" %02i %s %i ", dy, s_mos[mo-1], yr);
	} else {
		char *l_mos[]  = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};
		printf(" %02i %-9s %i ", dy, l_mos[mo-1], yr);
	}

	return;
}

void print_records(struct record_t *records, int n_recs, float start_amnt, struct search_param_t params, int date_frmt)
{

	int *prints = search_records(records, n_recs, params);

	switch (*prints) {
	case -1:
		fprintf(stderr, "No records in date range %i, %i found.\n", params.date1, params.date2);
		free(prints);
		return;
	case -2:
		fprintf(stderr, "No records with date %i found.\n", params.date2);
		free(prints);
		return;
	case -3:
		fprintf(stderr, "No records in amount range %.2f, %.2f found.\n", params.amnt_bound1, params.amnt_bound2);
		free(prints);
		return;
	case -4:
		fprintf(stderr, "No records with given tags found.\n");
		free(prints);
		return;
	case -5:
		fprintf(stderr, "No records in amount range %.2f, %.2f found.\nError: no records with given tags found.\n", params.amnt_bound1, params.amnt_bound2);
		free(prints);
		return;
	default:
		break;
	}
	
	/* search_records relies upon binary searching, meaning that the array must be searched before sorting. 
	 * To avoid having reorder the prints array, an array of all matches is constructed and sorted as necessary */
	struct record_t *to_print = malloc(prints[0]*sizeof(struct record_t));
	for (int i = 1; i <= prints[0]; i++)
		to_print[i-1] = records[prints[i]];
	if (params.sort_flag) {
		struct record_t *tmp = malloc(prints[0]*sizeof(struct record_t));
		sort_recs_amounts(to_print, tmp, prints[0], 0);
		if (tmp != to_print)
			free_recs_array(tmp, n_recs, FALSE);
	}

	// TODO figure out function pointers, assign a print date function that is used for each individual line
	//if (date_frmt == DATE_ISO)
	//	print_date_ISO(records, prints);
	//else if (date_frmt == DATE_US)
	//	print_date_US(records, prints);
	//else if (date_frmt == DATE_LONG)  
	//	print_date_LONG(records, prints, FALSE);
	//else if (date_frmt == DATE_ABBR)
	//	print_date_LONG(records, prints, TRUE);
	
	char signs[] = {'-', '+'};
	for (int i = 0; i < prints[0]; i++) {
		// TODO once function pointers are worked out, this function should be changed to the generic function pointer assigned above
		print_date_LONG(to_print[i], TRUE);
		printf("  %c$%-10.2f ", signs[to_print[i].amount >= 0], fabs(to_print[i].amount));
		if (to_print[i].message[0] != '\0')
			printf("\"%s\" ", to_print[i].message);
		if (to_print[i].n_tags > 0) {
			printf("[%s", to_print[i].tags[0]);
			for (int j = 1; j < to_print[i].n_tags; j++)
				printf(",%s", to_print[i].tags[j]);
			putchar(']');
		}
		putchar('\n');
	}
		
        print_table_footer(records, prints, start_amnt);

	free_recs_array(to_print, prints[0], FALSE);
	free(prints);
	return;
}
