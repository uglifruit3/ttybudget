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

#define VER_INFO "ttybudget 1.1 - 03 April 2022"

#define HELP_MSG "Usage: ttybudget [MAIN OPTIONS ...] RECORDS OPERATIONS ...\n \n Main options:\n   -h, --help                Display this message and exit\n   -v, --version             Display version info and exit\n   --date-in-<iso,us>        Explicitly set the date input format\n   --date-out-<iso,us,long,abbr>\n                             Explicitly set the date input format\n   -c CURR_CHAR, --currency-char \n                             Explicitly define the character prepended to currency \n                               amounts\n   -u RECS_FILE, --use-file  Use the records file given by the path RECS_FILE\n\n Records operations:\n   -a AMOUNT [ADD OPTIONS ...], --add\n                             Add a record to the ledger stored in the records file\n   -p [PRINT OPTIONS ...], --print\n                             Print the records stored in the ledger\n\n Add options:\n   -t TAGS, --tags           Assign tags to the added record\n   -m \"MESSAGE\", --messsage  Assign a message to the added record\n   -d DATE, --date           Assign a date to the added record\n\n Print options:\n   -i [!] DATE1 [DATE2], --interval\n                             Search for records matching DATE1, or occuring inclusively \n                               between DATE1 and DATE2. '!' inverts results. \n   -f [!] VALUE1 [VALUE2], --find-range\n                             Search for records matching VALUE1, or occuring inclusively \n                               between VALUE1 and VALUE2. '!' inverts results.\n   -q [!] TAGS, --query-tags\n                             Search for records with tags matching those specified\n                               '!' inverts results.\n   -s, --sort                Sort displayed records in greatest-to-least currency amount\n   -r, --reverse             Reverse the order in which records are displayed\n   -n, --no-footer           Omit the footer \n   -l, --list-tags           Display a list of all tags associated with displayed records\n\nMandatory or optional arguments for short options are also mandatory or optional for any\n  corresponding long options.\n\nReport any bugs to mszembruski@tutanota.com.\n"

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

void free_list(struct NewRecs_t *list, bool del_mode) 
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
		if (del_mode == true && to_free->data.n_tags > 0) {
			for (int i = 0; i < to_free->data.n_tags; i++)
				free(to_free->data.tags[i]);
			free(to_free->data.tags);
		}
		free(to_free);
	}	
}

signed int string_in_list(char *string, char **list, int num_list_items)
{
	if (!string || string[0] == '\0') return -1;

	for (int i = 0; i < num_list_items; i++) {
		if(!strcmp(string, list[i]))
			return i;
	}

	return -1;
}

signed int is_cmdline_option(char *str1)
{
	/* this static array must match, element for element, the options listed in the Commandline_Options 
	 * enumerator in io.h */
	const char *cmdline_opts[] = {
		"-a", "--add",                              /* 0-1 add */
			"-t", "--tags",                     /* 2-7 add options */
			"-m", "--message",
			"-d", "--date",
		"-p", "--print",                            /* 8-9 print */
			"-q", "--query-tags",               /* 10-19 print options */
			"-i", "--interval",
			"-f", "--find-range",
			"-s", "--sort",
			"-r", "--reverse",
			"-n", "--no-footer",
			"-l", "--list-tags",
		"-h", "--help",                             /* 23-27 help and version info */
		"-v", "--version",
		"--date-in-iso", "--date-in-us",            /* 28-33 date formats */
		"--date-out-iso", "--date-out-us", "--date-out-long", "--date-out-abbr",
		"-c", "--currency",                         /* 34-35 currency config */
		"-u", "--use-file"                          /* 36-37 use file config */
	};
	const int num_opts = 38;

	signed int opt = string_in_list(str1, (char **)cmdline_opts, num_opts);
			
	return opt;
}

long get_filesize(FILE *fp)
{
	long file_size;
	if (fseek(fp, 0L, SEEK_END) == 0)
		file_size = ftell(fp);
	else
		file_size = -1;
	
	rewind(fp);
	return file_size;
}

FILE *open_records_file(char file_name[256], int *err)
{
	/* checks if the file name has been specified */
	if (file_name[0] == '\0') {
		fprintf(stderr, "Error: a records file has not been specified--exiting.\n");
		*err = USR_ERR;
		return NULL;
	}

	FILE *records_file = fopen(file_name, "r");

	/* initializes records file to a starting amount of money */
	if ((!records_file && errno == ENOENT) || get_filesize(records_file) == 0) {
		errno = 0;
		float amount;
		char line[256];
		char *end;

		records_file = fopen(file_name, "w");
		if (records_file && !errno) {
			printf("Records file \"%s\" has not been initialized.\nEnter a starting amount of money: ", file_name);

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
	} 
	if (!records_file || errno != 0) {
		errno = 0;
		fprintf(stderr, "Error opening records file \"%s\"--exiting.\n", file_name);
		*err = SYS_ERR;
		return NULL;
	} else if (dir_exists(file_name, err)) {
		if (*err == SYS_ERR)
			return NULL;

		fprintf(stderr, "Error: path \"%s\" to records file is a directory--exiting.\n", file_name);
		*err = USR_ERR;
		return NULL;
	}

	return records_file;
}

int get_tags(char *argv[], int *index, char ***tags, int *n_tags)
{
	*index += 1;

	if (*n_tags != 0) {
		fprintf(stderr, "Error: multiple definitions of tags.\n");
		return USR_ERR;
	}
	if (!argv[*index]) {
		fprintf(stderr, "Error: no argument given for tags option.\n");
		return USR_ERR;
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
			return USR_ERR;
		} else if (k == MAX_TAG_LEN) {
			fprintf(stderr, "Error: tag name too long (max 32 chars).\n");
			free_array(tmptags, *n_tags);
			*n_tags = 0;
			return USR_ERR;
		}

		temp[k] = tag_list[i];	
		k++; i++;
	}
	strncpy(tmptags[j], temp, MAX_TAG_LEN);
	*tags = tmptags;

	return NO_ERR;
}

int get_message(char *argv[], int *index, char message[256])
{
	*index+=1;

	if (!argv[*index]) {
		fprintf(stderr, "Error: no argument given for message option.\n");
		return USR_ERR;
	}

	char *msg = argv[*index];
	
	if (strlen(msg) > MAX_MSG_LEN) {
		fprintf(stderr, "Error: message length exceeds character limit (256).\n");
		return USR_ERR;
	} else if (message[0] != '\0') {
		fprintf(stderr, "Error: multiple definitions of message in new record.\n");
		return USR_ERR;
	}

	strncpy(message, msg, MAX_MSG_LEN);
	return NO_ERR;
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

	const char *months_long[]  = {"january", "february", "march", "april", "may", "june", "july", "august", "september", "october", "november", "december"};
	const char *months_short[] = {"jan", "feb", "mar", "apr", "may", "jun", "jul", "aug", "sep", "oct", "nov", "dec"};
	int num_mo = string_in_list(str_mo, (char **) months_long, 12) + 1;
	if (num_mo == 0) {
		num_mo = string_in_list(str_mo, (char **) months_short, 12) + 1;
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
	return NO_ERR;
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
	return NO_ERR;
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
	return NO_ERR;
}
int get_date(char *argv[], int *index, int format, int *date, bool accept_inf)
{
	*index += 1;

	if (*date != 0) {
		fprintf(stderr, "Error: multiple definitions of date.\n");
		return USR_ERR;
	}
	if (!argv[*index]) {
		fprintf(stderr, "Error: no argument given for date option.\n");
		return USR_ERR;
	}

	float inf_arg = strtof(argv[*index], NULL);
	if (fabs(inf_arg) == INFINITY) {
		if (accept_inf) {
			if (inf_arg < 0)
				*date = INT_MIN;
			else
				*date = INT_MAX;
			return NO_ERR;
		} 
		fprintf(stderr, "Error: passing infinity for a date is not permitted here.\n");
		return USR_ERR;
	}

	/* final day of each month, Jan to Dec */
	const int max_days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

	/* needs to be an int, since get_date_* returns several different error codes */
	int error;
	if (format == IN_DATE_ISO) 
		error = get_date_ISO(argv[*index], (int *) max_days, date, argv, index);
	else if (format == IN_DATE_US)
		error = get_date_US(argv[*index], (int *) max_days, date, argv, index);

	/* table of return values for get_date functions:
	 * 0/NO_ERR: no error
	 * 1:        bad delimeters
	 * 2:        bad format
	 * 3:        invalid date expression
	 * 4:        invalid month value
	 * 5:        invalid day value 
	 * 6:        get_date_LONG returned an error (handles own output) */

	switch (error) {
	case 0:
		return NO_ERR;
	case 1:
		fprintf(stderr, "Error: date expression \"%s\" is not consistently delimited.\n", argv[*index]);
		return USR_ERR;
	case 2:
		fprintf(stderr, "Error: date expression \"%s\" is not correctly formatted.\n", argv[*index]);
		return USR_ERR;
	case 3:
		fprintf(stderr, "Error: date expression \"%s\" is invalid.\n", argv[*index]);
		return USR_ERR;
	case 4:
		fprintf(stderr, "Error: month in date expression \"%s\" outside valid range (1-12).\n", argv[*index]);
		return USR_ERR;
	case 5:
		fprintf(stderr, "Error: day in date expression \"%s\" is not a calendar day.\n", argv[*index]);
		return USR_ERR;
	case 6: 
		return USR_ERR;
	}
	return USR_ERR;
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

bool get_negator(int argc, char *argv[], int *index)
{
	*index += 1;
	if (*index >= argc || strcmp(argv[*index], "!")) {
		*index -= 1;
		return false;
	} else {
		return true;
	}
}


int get_amount(char *argv[], int *index, float *amount, bool assume_negative, bool accept_inf)
{
	errno = 0;
	char *end;
	float tmp = strtof(argv[*index], &end);

	if (errno || *end) {
		fprintf(stderr, "Error: \"%s\" not a valid expression of amount.\n",
				argv[*index]);
		return USR_ERR;
	}
	if (fabs(tmp) == INFINITY && !accept_inf) {
		fprintf(stderr, "Error: passing infinity for an amount is not permitted here.\n");
		return USR_ERR;
	}
	if (assume_negative && argv[*index][0] != '+' && argv[*index][0] != '-')
		tmp *= -1;

	*index += 1;
	*amount = tmp;
	return NO_ERR;
}

int get_new_records(int argc, char *argv[], int *index, int date_frmt, struct NewRecs_t **new_records)
{
	bool error = false;
	bool add_flag = false;
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
			error = get_date(argv, index, date_frmt, &(record->data.date), false);
			break;
		case -1:
			/* check if amount has already been defined */
			if (!isnan(record->data.amount)) {
				*index -= 2;
				/* add flag is set to escape the loop and process input */
				add_flag = true;
				break;
			}
			/* get the amount */
			error = get_amount(argv, index, &(record->data.amount), true, false);
			if (error) 
				return USR_ERR;

			*index -= 1; /* decrement by one because get_amount sets two forward */
			break;
		default:
			*index -= 2; /* decrement by one so caller reads this argument */
			/* add flag is set to escape the loop and process input */
			add_flag = true;
			break;
		}
	}

	add_flag = false;
	/* an amount has not been defined */
	if (isnan(record->data.amount)) {
		error = true;
	/* if an amount is defined and no errors are present */
	} else if (!error) {
		add2front(new_records, record);
		add_flag = true;

		if (record->data.date == 0)
			record->data.date = get_current_date();
	}

	if (error || !add_flag) {
		if (isnan(record->data.amount)) {
			fprintf(stderr, "Error: amount not given for add option.\n");
		}

		free_list(record, true);
		return USR_ERR;
	}

	return NO_ERR;
}

int get_print_commands(int argc, char *argv[], int *index, int date_frmt, struct search_param_t *params)
{
	int error = NO_ERR;

	while ((*index += 1) && *index < argc && !error) {
		switch (is_cmdline_option(argv[*index])) {
		case QUERY_S:
		case QUERY_L:
			params->negate_tags = get_negator(argc, argv, index);
			error = get_tags(argv, index, &(params->tags), &(params->n_tags));
			break;
		case INTERVAL_S:
		case INTERVAL_L:
			params->negate_date = get_negator(argc, argv, index);
			error = get_date(argv, index, date_frmt, &(params->date1), true);
			/* if no error, within argv still, and the next option is not another command read BOUND2 */
			if (!error && *index < argc-1 && is_cmdline_option(argv[*index+1]) == -1) {
				error = get_date(argv, index, date_frmt, &(params->date2), true);
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
		case FIND_RANGE_S:
		case FIND_RANGE_L:
			params->negate_range = get_negator(argc, argv, index);
			*index += 1;
			if (!isnan(params->amnt_bound2)) {
				error = USR_ERR;
				fprintf(stderr, "Error: multiple definitions of amount range.\n");
				break;
			}

			/* check for end of argv buffer */
			if (*index >= argc) {
				error = USR_ERR;
				fprintf(stderr, "Error: amount not given for find range option.\n");
				break;
			}
			
			/* get first bound */
			error = get_amount(argv, index, &(params->amnt_bound1), false, true);
			/* if the next argument is a number, read that as bound2 */
			if (!error && *index < argc && is_cmdline_option(argv[*index]) == -1)
				error = get_amount(argv, index, &(params->amnt_bound2), false, true);

			/* if bound2 goes unitialized, make it ==bound1 so we search for that amount only */
			if (isnan(params->amnt_bound2)) {
				params->amnt_bound2 = params->amnt_bound1;
			/* reorder bounds if necessary so bound1 < bound2 (for search function) */
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
			if (params->sort_flag != false) {
				fprintf(stderr, "Error: multiple invocations of \"sort\" sub-option.\n");
				error = USR_ERR;
				break;
			}

			/* sorting the records will be handled in the print function */
			params->sort_flag = true;
			break;
		case REVERSE_S:
		case REVERSE_L:
			params->reverse_flag = true;
			break;
		case NOFOOTER_S:
		case NOFOOTER_L:
			params->show_footer = false;
			break;
		case LIST_TAGS_S:
		case LIST_TAGS_L:
			params->list_tags = true;
			break;
		case -1:
			*index -= 1; /* decrement by 1 because of increment at function caller */
			return NO_ERR;
		case TAGS_S:
		case TAGS_L:
		case DATE_S:
		case DATE_L:
		case MESSAGE_S:
		case MESSAGE_L:
			fprintf(stderr, "Error: option \"%s\" is not a sub-option for print.\n", argv[*index]);
			error = USR_ERR;
			break;
		default:
			*index -= 1;
			return NO_ERR;
		}
	}

	return error;
}

int parse_command_line(char *argv[], int argc, char filename[], struct NewRecs_t **new_records, struct search_param_t *print_params, struct defaults_t *defaults)
{
	strcpy(filename, defaults->recs_file);

	int error = NO_ERR;

	if (argc == 1) {
		printf("Usage:\n  ttybudget MAIN_OPTIONS [RECORDS_OPERATIONS]\n  ttybudget -a|--add AMOUNT [ADD_OPTIONS]\n  ttybudget -p|--print [PRINT_OPTIONS]\n  ttybudget -h|--help\n  ttybudget -v|--version\n");
		error = EXIT_NOW; 
	}

	for (int i = 1; i < argc; i++) {
		switch (is_cmdline_option(argv[i])) {
		case HELP_S:
		case HELP_L:
			printf(HELP_MSG);
			error = EXIT_NOW; 
			break;
		case VERSION_S:
		case VERSION_L:
			printf(VER_INFO "\n");
			error = EXIT_NOW;
			break;
		case ADD_S: 
		case ADD_L:
			error = get_new_records(argc, argv, &i, defaults->in_date_frmt, new_records);
			break;
		case PRINT_S: 
		case PRINT_L:
			print_params->print_flag = true;
			error = get_print_commands(argc, argv, &i, defaults->in_date_frmt, print_params);
			break;
		case IN_DATE_ISO:
		case IN_DATE_US:
			defaults->in_date_frmt = is_cmdline_option(argv[i]);
			defaults->change_flag = true;
			break;
		case OUT_DATE_ISO:
		case OUT_DATE_US:
		case OUT_DATE_LONG:
		case OUT_DATE_ABBR:
			defaults->out_date_frmt = is_cmdline_option(argv[i]);
			defaults->change_flag = true;
			break;
		case CURRENCY_S: 
		case CURRENCY_L: 
			i++;
			if (!strcmp(argv[i], "\0")) {
				defaults->currency_char = 0;
				defaults->change_flag = true;
			} else if (strlen(argv[i]) != 1) {
				fprintf(stderr, "Error: argument for option \"%s\" must be a single character.\n", argv[i-1]);
				error = USR_ERR;
			} else {
				defaults->currency_char = argv[i][0];
				defaults->change_flag = true;
			}
			break;
		case USE_FILE_S:
		case USE_FILE_L:
			if ( is_cmdline_option(argv[i+1]) >= ADD_S) {
				fprintf(stderr, "Error: no argument given for \"%s\".\n", argv[i]);
				error = USR_ERR;
			} else {
				strncpy(filename, argv[++i], 256);
			}
			break;
		/* the argument is not a command line argument */
		case -1:
			fprintf(stderr, "Error: invalid option \"%s\".\n", argv[i]);
			error = USR_ERR;
			break;
		/* the argument is not a main option (e.g. it lacks a parent) */
		default:
			fprintf(stderr, "Error: sub-option \"%s\" lacks parent.\n", argv[i]);
			error = USR_ERR;
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

void write_to_file(struct record_t *records, int n_recs, float start_amnt, char *rec_filename, int *err)
{
	FILE *rec_file = fopen(rec_filename, "w");
	if (!rec_file || errno) {
		fprintf(stderr, "Error opening records file \"%s\" for writing--exiting.\n", rec_filename);
		*err = SYS_ERR;
		return;
	}

	fprintf(rec_file, "%.2f\n", start_amnt);

	for (int i = 0; i < n_recs; i++)
		print_rec_to_file(rec_file, records[i]);

	fclose(rec_file);	
	return;
}

void print_date_ISO(int date, FILE *device)
{
	if (date == INT_MIN) {
		fprintf(device, " -inf ");
		return;
	} else if (date == INT_MAX) {
		fprintf(device, " inf ");
		return;
	}

	int yr = date/10000;
	int mo = (date - yr*10000)/100;
	int dy = date - yr*10000 - mo*100;
	fprintf(device, " %i-%02i-%02i ", yr, mo, dy);
	return;
}
void print_date_US(int date, FILE *device)
{
	if (date == INT_MIN) {
		fprintf(device, " -inf ");
		return;
	} else if (date == INT_MAX) {
		fprintf(device, " inf ");
		return;
	}

	int yr = date/10000;
	int mo = (date - yr*10000)/100;
	int dy = date - yr*10000 - mo*100;
	fprintf(device, " %02i-%02i-%i ", mo, dy, yr);
	return;
}
void print_date_LONG(int date, FILE *device)
{
	if (date == INT_MIN) {
		fprintf(device, " -inf ");
		return;
	} else if (date == INT_MAX) {
		fprintf(device, " inf ");
		return;
	}

	int yr = date/10000;
	int mo = (date - yr*10000)/100;
	int dy = date - yr*10000 - mo*100;

	char *l_mos[]  = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};
	fprintf(device, " %02i %-9s %i ", dy, l_mos[mo-1], yr);
	return;
}
void print_date_ABBR(int date, FILE *device)
{
	if (date == INT_MIN) {
		fprintf(device, " -inf ");
		return;
	} else if (date == INT_MAX) {
		fprintf(device, " inf ");
		return;
	}

	int yr = date/10000;
	int mo = (date - yr*10000)/100;
	int dy = date - yr*10000 - mo*100;

	char *s_mos[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
	fprintf(device, " %02i %s %i ", dy, s_mos[mo-1], yr);
	return;
}

void print_amnt_no_cc(char sign, char curr_char, float amnt, int field_width)
{
	char str[16];
	char frmt[16];
	sprintf(frmt, "    %%%i.2f    ", field_width);
        sprintf(str, frmt, fabs(amnt));

	int i;
	for (i = 0; str[i] == ' '; i++);
	str[i-1] = sign;

	printf(str);
	return;
}
void print_amnt_cc(char sign, char curr_char, float amnt, int field_width)
{
	char str[16];
	char frmt[16];
	sprintf(frmt, "    %%%i.2f    ", field_width);
        sprintf(str, frmt, fabs(amnt));

	int i;
	for (i = 0; str[i] == ' '; i++);
	str[i-2] = sign;
	str[i-1] = curr_char;

	printf(str);
	return;
}

void print_table_footer(struct record_t *records, int n_recs, int *matches, float start_amnt, char cur_char)
{
	/* get funds at start of period */
	int i = 0;
	while (records[i].date < records[matches[1]].date) {
		start_amnt += records[i].amount;
		i++;
	}
	/* get funds at end of period */
	float end_amnt = start_amnt;

	/* get selected records total */
	while (i < n_recs && records[i].date <= records[matches[matches[0]]].date) {
		end_amnt += records[i].amount;
		i++;
	}
	float recs_tot = 0;
	for (int i = 1; i <= matches[0]; i++)
		recs_tot += records[matches[i]].amount;

	/* change in funds */
	float delta = end_amnt - start_amnt;

	/* display output */
	char signs[] = {'-', '+'};
	printf("=========================================================================\n");
	printf(" Funds at start of period: %c%c%-10.2f | Selected records total: %c%c%-10.2f\n", signs[start_amnt >= 0], cur_char, fabs(start_amnt), signs[recs_tot >= 0], cur_char, fabs(recs_tot));
	printf(" Funds at end of period:   %c%c%-10.2f | Total records displayed: %i\n", signs[end_amnt >= 0], cur_char, fabs(end_amnt), n_recs);
	printf(" Change in funds:          %c%c%-10.2f | Cash flow in period: %s\n\n", signs[delta >= 0], cur_char, fabs(delta), (delta >= 0 ? "POSITIVE":"NEGATIVE"));

	return;
}

void print_tags(struct record_t *records, int n_recs)
{
	int n_tags;
	struct tagnode_t *tags = get_tag_list(records, n_recs, &n_tags);	
	struct tagnode_t *tmp = malloc(n_tags * sizeof(struct tagnode_t));
	sort_taglist(tags, tmp, n_tags, 0);
	if (tmp != tags)
		free(tmp);

	printf("=========================================================================\n");
	if (tags != NULL) {
		printf(" Tags in displayed records:");
		int fw = 73; /* field width is 73 characters */
		/* cols counts the number of columns a printed line occupies */
		int cols;
		char tmp[64];
		for (int i = 0; i < n_tags; i++) {
			if (cols == 0)
				putchar(' ');

			memset(tmp, '\0', 64);
			sprintf(tmp, " %s (%i)", tags[i].tag, tags[i].times);

			cols += strlen(tmp);
			if (cols <= fw - 1) {
				printf("%s", tmp);
			} else {
				cols = 0;
				i--;
				putchar('\n');
				continue;
			}

			if (i < n_tags - 1) {
				putchar(',');
				cols++;
			} else {
				putchar('\n');
			}
		}
		free(tags);
	} else {
		printf(" No tags in displayed records.\n");
	}

	return;
}

void print_records(struct record_t *records, int n_recs, float start_amnt, struct search_param_t params, struct defaults_t defs)
{
	int *prints = search_records(records, n_recs, params);

	/* decide which date format to employ */
	void (*print_date)(int date, FILE *device);
	switch (defs.out_date_frmt) {
	case OUT_DATE_ISO:
		print_date = &print_date_ISO;	
		break;
	case OUT_DATE_US:
		print_date = &print_date_US;	
		break;
	case OUT_DATE_LONG:
		print_date = &print_date_LONG;	
		break;
	case OUT_DATE_ABBR:
		print_date = &print_date_ABBR;	
		break;
	}


	/* display output and exit if the search returns no results */
	switch (*prints) {
	case -1:
		fprintf(stderr, "No records between dates");
		print_date(params.date1, stderr);
		fprintf(stderr, "and");
		print_date(params.date2, stderr);
		fprintf(stderr, "found.\n");
		free(prints);
		return;
	case -2:
		fprintf(stderr, "No records with date"); 
		print_date(params.date2, stderr);
		fprintf(stderr, "found.\n");
		free(prints);
		return;
	case -3:
		if (n_recs != 0)
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
	float max_amnt = 0;
	for (int i = 1; i <= prints[0]; i++) {
		to_print[i-1] = records[prints[i]];
		/* also finding largest records amount for getting field width of numbers */
		if (fabs(records[prints[i]].amount) > max_amnt)
			max_amnt = fabs(records[prints[i]].amount);
	}

	if (params.sort_flag) {
		struct record_t *tmp = malloc(prints[0]*sizeof(struct record_t));
		sort_recs_amounts(to_print, tmp, prints[0], 0);
		if (tmp != to_print)
			free_recs_array(tmp, n_recs, false);
	}

	/* decide whether to print with a currency character */
	void (*print_amnt)(char sign, char curr_char, float amnt, int field_width);
	if (defs.currency_char == 0)
		print_amnt = &print_amnt_no_cc;
	else
		print_amnt = &print_amnt_cc;

	/* set the display order for records (regular or reversed) */
	int start, end, inc;
	if (params.reverse_flag == true) {
		start = prints[0]-1;
		end = -1;
		inc = -1;
	} else {
		start = 0;
		end = prints[0];
		inc = 1;
	}
	
	/* get max number field width */
	int fw = (int) log10(max_amnt) + 4;
	/* display output */
	char signs[] = {'-', '+'};
	int i = start;
	/* index starts at start, moves by inc each iteration, until it is equal to end */
	while (i != end) {
		/* print the date */
		(*print_date)(to_print[i].date, stdout);
		/* print amounts */
		print_amnt(signs[to_print[i].amount >= 0], defs.currency_char, to_print[i].amount, fw);
		/* print message */
		if (to_print[i].message[0] != '\0')
			printf("\"%s\" ", to_print[i].message);
		/* prints tags */
		if (to_print[i].n_tags > 0) {
			printf("[%s", to_print[i].tags[0]);
			for (int j = 1; j < to_print[i].n_tags; j++)
				printf(",%s", to_print[i].tags[j]);
			putchar(']');
		}

		putchar('\n');

		i += inc;
	}
		
	/* display footer and tag list if specified */
	if (params.list_tags)
		print_tags(to_print, prints[0]);
	if (params.show_footer)
		print_table_footer(records, n_recs, prints, start_amnt, defs.currency_char);

	free_recs_array(to_print, prints[0], false);
	free(prints);
	return;
}
