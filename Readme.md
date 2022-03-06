# ttybudget
### A command line ledger and budgeting tool
-----------------
## Table of Contents
1. [About](https://github.com/uglifruit3/ttybudget#about)
2. [Installation](https://github.com/uglifruit3/ttybudget#installation)
3. [Overview](https://github.com/uglifruit3/ttybudget#overview)
4. [Usage](https://github.com/uglifruit3/ttybudget#usage)
5. [Program files](https://github.com/uglifruit3/ttybudget#program-files)
6. [Examples](https://github.com/uglifruit3/ttybudget#examples)
7. [Bugs and feedback](https://github.com/uglifruit3/ttybudget#bugs-and-feedback)

## About
-----------------
ttybudget is a command line tool for storing, organizing, and usefully displaying personal financial transactions.

It allows for various pieces of amplifying information to be included with the entry of a new transaction, and is equipped with search tools that allow for detailed queries as to existing records.

This is an amateur project; having undertaken it with little notable prior experience in programming, I gratefully welcome any constructive criticism or suggestions.

## Installation
-----------------
The included makefile performs all actions necessary for installation. Clone the repository to your machine, navigate to it, and as root, run 
```bash
# make install USR='username'
```
where 'username' is name of the user in whose home directory ttybudget files will be stored.

## Overview
-----------------
ttybudget is a command line tool for tracking personal finances. It includes commands for adding transaction records to a ledger and printing them to the standard output device. Supplemental information for records can also be specified, and that information can be searched for with matches being selectively displayed, along with additional optional information about the ledger as a whole.

A record will always contain, at minimum, a currency amount and date. If a date is not specified when the record is added, the current date will be used. Additionally, a record may contain an unlimited number of searchable tags and a message. Using the print options provided, records can be searched for on a basis of matching tag(s), date/date ranges, and currency amounts/amount ranges. Records will be displayed in chronological order (oldest-to-newest), but can also be sorted by greatest-to-least currency amounts. Regardless of how they are displayed, the order of records can be reversed as well. ttybudget also displays additional cash flow information in the form of a footer, which is enabled by default, and can optionally display all tags belonging to displayed records. 

ttybudget accesses a records file, containing the ledger which will be read from, written to, and displayed if instructed. If a records file is being accessed for the first time, the user will be prompted to specify the initial amount of money they wish for ttybudget to add/subract transactions to or from. A records file must be specified either in the configuration file, as is typical, or by the user via command line option. If not specified, the program will exit with an error.

Amplifying information regarding usage, configuration file syntax, return values, and entry/output formats for all types of data can be found in ttybudget's manpage.

## Usage
-----------------
```
Usage: ttybudget [MAIN OPTIONS ...] RECORDS OPERATIONS ...

 Main options:
   -h, --help                Display this message and exit
   -v, --version             Display version info and exit
   --date-in-<iso,us>        Explicitly set the date input format
   --date-out-<iso,us,long,abbr>
                             Explicitly set the date input format
   -c CURR_CHAR, --currency-char
                             Explicitly define the character prepended to currency
                               amounts
   -u RECS_FILE, --use-file  Use the records file given by the path RECS_FILE

 Records operations:
   -a AMOUNT [ADD OPTIONS ...], --add
                             Add a record to the ledger stored in the records file
   -p [PRINT OPTIONS ...], --print
                             Print the records stored in the ledger

 Add options:
   -t TAGS, --tags           Assign tags to the added record
   -m "MESSAGE", --messsage  Assign a message to the added record
   -d DATE, --date           Assign a date to the added record

 Print options:
   -i DATE1 [DATE2], --interval
                             Search for records matching DATE1, or occuring inclusively
                               between DATE1 and DATE2
   -f VALUE1 [VALUE2], --find-range
                             Search for records matching VALUE1, or occuring inclusively
                               between VALUE1 and VALUE2
   -q TAGS, --query-tags     Search for records with tags matching those specified
   -s, --sort                Sort displayed records in greatest-to-least currency amount
   -r, --reverse             Reverse the order in which records are displayed
   -n, --no-footer           Omit the footer
   -l, --list-tags           Display a list of all tags associated with displayed records

Mandatory or optional arguments for short options are also mandatory or optional for any
  corresponding long options.
```
### First use
When first used, the program will prompt the user to enter a starting amount of money. This is the amount of money the user has at the moment, and it is this amount that ttybudget will add to or subtract from when calculating totals.

### Formats
* Dates - A date can be input and output in 4 distinct formats.
	* iso refers to ISO 6801 (YYYYMMDD)
	* us refers to the date format commonly used in the United States (MMDDYYYY)
		* When being input, both us and iso formats accept dates with slashes or hyphens as delimeters (e.g. YYYY-MM-DD or YYYY/MM/DD in iso) or with no delimeters at all (YYYYMMDD).
	* long is the most verbose date format, consisting of DD Month YYYY
	* abbr is an abbreviated form of long, consisting of DD MON YYYY where MON is a month's three-letter abbreviation.
		* Both of these formats are accepted as inputs without the need to specify such. Months are not case sensitive. Spaces must be used as delimeters when inputting these formats.
* Tags - A tag is a string consisting of no more than 32 of the characters '{a..z}', '{A..Z}', '\_', and '-'. Multiple tags are delimited by commas (',') without spaces between them. An arbitrary number of tags may be assigned to a record.
* Messages - A message should be enclosed in double quotes and be no more than 256 characters in length.
* Dates - see Date formats above.

## Program files
-----------------
ttybudget uses two files for operation. The first is a records file, in which all transactions are stored. ttybudget reads from this file for printing, and stores new transactions here. Its defaults location is /home/$USER/.local/share/ttybudget/records, but can be specified as being elsewhere by command line option (affecting only that invocation of ttybudget) or editing a configuration file (permanently changing the path, explained below). A records file must be specified in one of these manners, or else ttybudget will not operate.

When the program starts, it attempts to collect information from the config file /home/$USER/.config/ttybudget/defaults.conf. Here, the default input and output date formats, currency character, and records file path are stored. If ttybudget is unable to gather any of these behaviors from the configuration file, but detects no syntax errors or invalid options, it will continue operation. Depending on the default left unspecified, it will either initialize it automatically, or await specification from the user elsewhere. 

## Examples
-----------------
* `ttybudget -u localrecs -a +25.30` Instruct ttybudget to use the records file localrecs, and add a new record to the ledger consisting of amount +25.30 occuring on the current day.
* `ttybudget --date-in-iso -a 595 -d 1982-09-01 -t credit_card,computer,personal -m "bought the Commodore 64"` Explicitly specify the input date to be iso, then; add a record of amount -595 to the ledger, occuring on 01 September 1982; and include the tags credit_card, computer, and personal, as well as the message "bought the Commodore 64" with the record.
* `ttybudget -p -f -INFINITY 0 -q credit_card,vacation -s -r` Print the ledger, displaying records with currency amounts between -INFINITY and 0; with tags credit\_card and vacation; and in sorted, reversed order (least-to-greatest amount).
* `ttybudget -a +25 -a -45.99 -c "" -p -i 20010401 -n` Add two records to the ledger, then print only records that occured on 01 April 2001, with no currency characters and omitting the footer.

## Bugs and feedback
-----------------
Submit bugs and feedback to mszembruski@tutanota.com
