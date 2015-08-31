#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <check.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <math.h>
#include "storage.h"

#define TESTTIMEOUT	10		// How long to wait for each test to run.
#define SERVEREXEC	"./server"	// Server executable file.
#define SERVEROUT	"default.serverout"	// File where the server's output is stored.
#define SERVEROUT_MODE	0666		// Permissions of the server ouptut file.
#define ONETABLE_CONF			"conf-onetable.conf"	// Server configuration file with one table.
#define SIMPLETABLES_CONF		"conf-simpletables.conf"	// Server configuration file with simple tables.
#define COMPLEXTABLES_CONF		"conf-complextables.conf"	// Server configuration file with complex tables.
#define DUPLICATE_COLUMN_TYPES_CONF     "conf-duplicatetablecoltype.conf"        // Server configuration file with duplicate column types.
#define BADTABLE	"bad table"	// A bad table name.
#define BADKEY		"bad key"	// A bad key name.
#define KEY		"somekey"	// A key used in the test cases.
#define KEY1		"somekey1"	// A key used in the test cases.
#define KEY2		"somekey2"	// A key used in the test cases.
#define KEY3		"somekey3"	// A key used in the test cases.
#define KEY4		"somekey4"	// A key used in the test cases.
#define VALUESPC	"someval 4"	// A value used in the test cases.
#define INTCOL		"col"		// An integer column
#define INTVALUE	"22"		// An integer value
#define INTCOLVAL	"col 22"	// An integer column name and value

// These settings should correspond to what's in the config file.
#define SERVERHOST	"localhost"	// The hostname where the server is running.
#define SERVERPORT	4848		// The port where the server is running.
#define SERVERUSERNAME	"admin"		// The server username
#define SERVERPASSWORD	"dog4sale"	// The server password
//#define SERVERPUBLICKEY	"keys/public.pem"	// The server public key
// #define DATADIR		"./mydata/"	// The data directory.
#define TABLE		"inttbl"	// The table to use.
#define INTTABLE	"inttbl"	// The first simple table.
//#define FLOATTABLE	"floattbl"	// The second simple table.
#define STRTABLE	"strtbl"	// The third simple table.
#define THREECOLSTABLE	"threecols"	// The first complex table.
#define FOURCOLSTABLE	"fourcols"	// The second complex table.
#define SIXCOLSTABLE	"sixcols"	// The third complex table.
#define MISSINGTABLE	"missingtable"	// A non-existing table.
#define MISSINGKEY	"missingkey"	// A non-existing key.
#define NEGATIVE_MAX_KEYS -1
#define FLOATTOLERANCE  0.0001		// How much a float value can be off by (due to type conversions).
#define ZERO 0
#define PREDICATE_SPACES "c o l > 2"
#define PREDICATE_SPECIAL "c#@#ol > 2"
/* Server port used by test */
int server_port;
void *test_conn = NULL;



/**
 * @brief Start the storage server.
 *
 * @param config_file The configuration file the server should use.
 * @param status Status info about the server (from waitpid).
 * @param serverout_file File where server output is stored.
 * @return Return server process id on success, or -1 otherwise.
 */
int start_server(char *config_file, int *status, const char *serverout_file)
{
	sleep(1);       // Give the OS enough time to kill previous process

	pid_t childpid = fork();
	if (childpid < 0) {
		// Failed to create child.
		return -1;
	} else if (childpid == 0) {
		// The child.

		// Redirect stdout and stderr to a file.
		const char *outfile = serverout_file == NULL ? SERVEROUT : serverout_file;
		//int outfd = creat(outfile, SERVEROUT_MODE);
		int outfd = open(outfile, O_CREAT|O_WRONLY, SERVEROUT_MODE);
		close(STDOUT_FILENO);
		close(STDERR_FILENO);
		if (dup2(outfd, STDOUT_FILENO) < 0 || dup2(outfd, STDERR_FILENO) < 0) {
			perror("dup2 error");
			return -1;
		}

		// Start the server
		execl(SERVEREXEC, SERVEREXEC, config_file, NULL);

		// Should never get here.
		perror("Couldn't start server");
		exit(EXIT_FAILURE);
	} else {
		// The parent.

		// If the child terminates quickly, then there was probably a
		// problem running the server (e.g., config file not found).
		sleep(1);
		int pid = waitpid(childpid, status, WNOHANG);
		//printf("Parent returned %d with child status %d\n", pid, WEXITSTATUS(*status));
		if (pid == childpid)
			return -1; // Probably a problem starting the server.
		else
			return childpid; // Probably ok.
	}
}

/**
 * @brief Start the server, and connect to it.
 * @return A connection to the server if successful.
 */
void* start_connect(char *config_file, char *serverout_file, int *serverpid)
{
	// Start the server.
	int pid = start_server(config_file, NULL, serverout_file);
	fail_unless(pid > 0, "Server didn't run properly.");
	if (serverpid != NULL)
		*serverpid = pid;

	// Connect to the server.
	void *conn = storage_connect(SERVERHOST, server_port);
	fail_unless(conn != NULL, "Couldn't connect to server.");

	// Authenticate with the server.
	int status = storage_auth(SERVERUSERNAME,
				  SERVERPASSWORD,
				  //SERVERPUBLICKEY,
				      conn);
	fail_unless(status == 0, "Authentication failed.");

	return conn;
}

void* init_start_connect(char *config_file, char *serverout_file, int *serverpid)
{
	// Delete the data directory.
//	system("rm -rf " DATADIR);
	
	// Create the data directory.
//	mkdir(DATADIR, 0777);

	return start_connect(config_file, serverout_file, serverpid);
}


/**
 * @brief Delete the data directory, start the server, and connect to it.
 * @return A connection to the server if successful.
 */
void* clean_start_connect(char *config_file, char *serverout_file, int *serverpid)
{
	// Delete the data directory.
//	system("rm -rf " DATADIR);

	return start_connect(config_file, serverout_file, serverpid);
}

/**
 * @brief Kill the server with given pid.
 * @return 0 on success, -1 on error.
 */
int kill_server(int pid)
{
	int status = kill(pid, SIGKILL);
	fail_unless(status == 0, "Couldn't kill server.");
	return status;
}

/**
 * @brief Text fixture teardown.  Disconnect from the server.
 */
void test_teardown()
{
	// Disconnect from the server.
	storage_disconnect(test_conn);
	//fail_unless(status == 0, "Error disconnecting from the server.");
}


// Keys array used by test fixture.
char* test_keys[MAX_RECORDS_PER_TABLE];

/**
 * @brief Text fixture setup.  Start the server and populate the tables.
 */
void test_setup_simple_populate()
{
	test_conn = init_start_connect(SIMPLETABLES_CONF, "simpledata.serverout", NULL);
	fail_unless(test_conn != NULL, "Couldn't start or connect to server.");

	struct storage_record record;
	int status = 0;
	int i = 0;

	// Create an empty keys array.
	// No need to free this memory since Check will clean it up anyway.
	for (i = 0; i < MAX_RECORDS_PER_TABLE; i++) {
		test_keys[i] = (char*)malloc(MAX_KEY_LEN); 
		strncpy(test_keys[i], "", sizeof(test_keys[i]));
	}

	// Do a bunch of sets (don't bother checking for error).

	strncpy(record.value, "col -2", sizeof record.value);
	status = storage_set(INTTABLE, KEY1, &record, test_conn);
	strncpy(record.value, "col 2", sizeof record.value);
	status = storage_set(INTTABLE, KEY2, &record, test_conn);
	strncpy(record.value, "col 4", sizeof record.value);
	status = storage_set(INTTABLE, KEY3, &record, test_conn);

	strncpy(record.value, "col abc", sizeof record.value);
	status = storage_set(STRTABLE, KEY1, &record, test_conn);
	strncpy(record.value, "col def", sizeof record.value);
	status = storage_set(STRTABLE, KEY2, &record, test_conn);
	strncpy(record.value, "col abc def", sizeof record.value);
	status = storage_set(STRTABLE, KEY3, &record, test_conn);
}


void test_setup_complex_populate()
{
	test_conn = init_start_connect(COMPLEXTABLES_CONF, "complexdata.serverout", NULL);
	fail_unless(test_conn != NULL, "Couldn't start or connect to server.");

	struct storage_record record;
	int status = 0;
	int i = 0;

	// Create an empty keys array.
	// No need to free this memory since Check will clean it up anyway.
	for (i = 0; i < MAX_RECORDS_PER_TABLE; i++) {
		test_keys[i] = (char*)malloc(MAX_KEY_LEN); 
		strncpy(test_keys[i], "", sizeof(test_keys[i]));
	}

	// Do a bunch of sets (don't bother checking for error).

	strncpy(record.value, "col1 -2,col2 -2,col3 abc", sizeof record.value);
	status = storage_set(THREECOLSTABLE, KEY1, &record, test_conn);
	strncpy(record.value, "col1 2,col2 2,col3 def", sizeof record.value);
	status = storage_set(THREECOLSTABLE, KEY2, &record, test_conn);
	strncpy(record.value, "col1 4,col2 4,col3 abc def", sizeof record.value);
	status = storage_set(THREECOLSTABLE, KEY3, &record, test_conn);

	strncpy(record.value, "col1 abc,col2 -2,col3 -2,col4 ABC", sizeof record.value);
	status = storage_set(FOURCOLSTABLE, KEY1, &record, test_conn);
	strncpy(record.value, "col1 def,col2 2,col3 2,col4 DEF", sizeof record.value);
	status = storage_set(FOURCOLSTABLE, KEY2, &record, test_conn);
	strncpy(record.value, "col1 abc def,col2 4,col3 4,col4 ABC DEF", sizeof record.value);
	status = storage_set(FOURCOLSTABLE, KEY3, &record, test_conn);

	strncpy(record.value, "col1 abc,col2 ABC,col3 -2,col4 2,col5 -2,col6 2", sizeof record.value);
	status = storage_set(SIXCOLSTABLE, KEY1, &record, test_conn);
	strncpy(record.value, "col1 abc,col2 ABC,col3 2,col4 -2,col5 2,col6 -2", sizeof record.value);
	status = storage_set(SIXCOLSTABLE, KEY2, &record, test_conn);
	strncpy(record.value, "col1 def,col2 DEF,col3 4,col4 -4,col5 4,col6 -4", sizeof record.value);
	status = storage_set(SIXCOLSTABLE, KEY3, &record, test_conn);

}

START_TEST (test_query_max_keys1)
{
	// Do a query.  Expect no matches.
	int foundkeys = storage_query(INTTABLE, "col > 9", test_keys, NEGATIVE_MAX_KEYS, test_conn);
	fail_unless(foundkeys == -1, "Query didn't check for max_keys being negative");
}
END_TEST

START_TEST (test_query_max_keys2)
{
	// Do a query.  Expect no matches.
	int foundkeys = storage_query(INTTABLE, "col > 9", NULL, MAX_RECORDS_PER_TABLE, test_conn);
	fail_unless(foundkeys == -1, "Query didn't check for max_keys being positive but test keys being NULL");
}
END_TEST

START_TEST (test_query_max_keys3)
{
	// Do a query.  Expect no matches.
	int foundkeys = storage_query(INTTABLE, "col > 9", NULL, ZERO, test_conn);
	fail_unless(foundkeys == 0, "Query didn't check for max_keys being 0 and test keys being NULL");

	fail_unless(strcmp(test_keys[0], "") == 0, "No extra keys should be modified.");
}
END_TEST

START_TEST (test_query_predicates1)
{
	// Do a query.  Expect no matches.
	int foundkeys = storage_query(INTTABLE, NULL, test_keys, MAX_RECORDS_PER_TABLE, test_conn);
	fail_unless(foundkeys == -1, "Query didn't check for predicates being NULL");
}
END_TEST

START_TEST (test_query_predicates2)
{
	// Do a query.  Expect no matches.
	int foundkeys = storage_query(INTTABLE, PREDICATE_SPACES, test_keys, MAX_RECORDS_PER_TABLE, test_conn);
	fail_unless(foundkeys == -1, "Query didn't check for predicates having spaces in column name");
}
END_TEST

START_TEST (test_query_predicates3)
{
	// Do a query.  Expect no matches.
	int foundkeys = storage_query(INTTABLE, PREDICATE_SPECIAL, test_keys, MAX_RECORDS_PER_TABLE, test_conn);
	fail_unless(foundkeys == -1, "Query didn't check for predicates having special characters");
}
END_TEST

/*
 * One server instance tests:
 * 	query from simple tables.
 */

START_TEST (test_query_int_comparison1)
{
	// Do a query.  Expect no matches.
	int foundkeys = storage_query(INTTABLE, "col > 9", test_keys, MAX_RECORDS_PER_TABLE, test_conn);
	fail_unless(foundkeys == 0, "Query didn't find the correct number of keys.");

	// Make sure next key is not set to anything.
	fail_unless(strcmp(test_keys[0], "") == 0, "No extra keys should be modified.");
}
END_TEST


START_TEST (test_query_int_comparison2)
{
	// Do a query.  Expect one match.
	int foundkeys = storage_query(INTTABLE, "col<-1", test_keys, MAX_RECORDS_PER_TABLE, test_conn);
	fail_unless(foundkeys == 1, "Query didn't find the correct number of keys.");

	// Check the matching keys.
	fail_unless(
		( strcmp(test_keys[0], KEY1) == 0 ),
		"The returned keys don't match the query.");

	// Make sure next key is not set to anything.
	fail_unless(strcmp(test_keys[1], "") == 0, "No extra keys should be modified.");
}
END_TEST

START_TEST (test_query_int_comparison3)
{	
	// Do a query. Expect one match
	int foundkeys = storage_query(INTTABLE, "col = 2", test_keys, MAX_RECORDS_PER_TABLE, test_conn);
	fail_unless(foundkeys == 1, "Query didn't find the correct number of keys");

	//Check the matching keys
	fail_unless(
		( strcmp(test_keys[0], KEY2) == 0 ), 
		"The returned keys don't match the query.");

	// Make sure next key is not set to anything.
	fail_unless(strcmp(test_keys[1], "") == 0, "No extra keys should be modified.");
}
END_TEST

START_TEST (test_query_int_comparison4)
{
	// Do a query.  Expect no matches.
	int foundkeys = storage_query(INTTABLE, "col > abc", test_keys, MAX_RECORDS_PER_TABLE, test_conn);
	fail_unless(foundkeys == -1, "Query didn't check for predicates data type");
}
END_TEST

START_TEST (test_query_str_comparison1)
{
	// Do a query. Expect one match
	int foundkeys = storage_query(STRTABLE, "col = def", test_keys, MAX_RECORDS_PER_TABLE, test_conn);
	fail_unless(foundkeys == 1, "Query didn't find the correct number of keys");

	//Check the matching keys
	fail_unless(
		( strcmp(test_keys[0], KEY2) == 0 ), 
		"The returned keys don't match the query.");

	// Make sure next key is not set to anything.
	fail_unless(strcmp(test_keys[1], "") == 0, "No extra keys should be modified.");
}
END_TEST

START_TEST (test_query_str_comparison2)
{
	// Do a query. Expect one match
	int foundkeys = storage_query(STRTABLE, "col > abc def", test_keys, MAX_RECORDS_PER_TABLE, test_conn);
	fail_unless(foundkeys == -1, "Query didn't check for predicates operator.");
}
END_TEST


START_TEST (test_query_int_str_comparison_complex3)
{
	// Do a query.  Expect one match.
	int foundkeys = storage_query(THREECOLSTABLE, "col1  = -2, col1 = 2, col3 = abc", test_keys, MAX_RECORDS_PER_TABLE, test_conn);
	fail_unless(foundkeys == -1, "Query didn't check for multiple predicates for a column.");
}
END_TEST

START_TEST (test_query_int_str_comparison_complex2)
{
	// Do a query.  Expect one match.
	int foundkeys = storage_query(SIXCOLSTABLE, "col1  >= abc, col2  <= ABC, col3 = -2, col4 = 2, col5 = -2, col6 = 2", test_keys, MAX_RECORDS_PER_TABLE, test_conn);
	fail_unless(foundkeys == -1, "Query didn't check for the number of operators in a predicate");
}
END_TEST

START_TEST (test_query_int_str_comparison_complex1)
{
	// Do a query.  Expect one match.
	int foundkeys = storage_query(SIXCOLSTABLE, "col1  = abc, col2  = ABC, col3 = -2, col4 = 2, col5 = -2, col6 = 2", test_keys, MAX_RECORDS_PER_TABLE, test_conn);
	fail_unless(foundkeys == 1, "Query didn't find the correct number of keys.");

	// Check the matching keys.
	fail_unless(
		( strcmp(test_keys[0], KEY1) == 0 ),
		"The returned keys don't match the query.");

	// Make sure next key is not set to anything.
	fail_unless(strcmp(test_keys[1], "") == 0, "No extra keys should be modified.");
}
END_TEST

START_TEST (test_query_int_comparison_complex2)
{
	// Do a query.  Expect one match.
	int foundkeys = storage_query(THREECOLSTABLE, "col1  > 0, col2 > 2", test_keys, MAX_RECORDS_PER_TABLE, test_conn);
	fail_unless(foundkeys == 1, "Query didn't find the correct number of keys.");

	// Check the matching keys.
	fail_unless(
		( strcmp(test_keys[0], KEY3) == 0 ),
		"The returned keys don't match the query.\n");

	// Make sure next key is not set to anything.
	fail_unless(strcmp(test_keys[1], "") == 0, "No extra keys should be modified.");
}
END_TEST

START_TEST (test_query_int_comparison_complex1)
{
	// Do a query.  Expect one match.
	int foundkeys = storage_query(THREECOLSTABLE, "col1  = -2, col2 = -2", test_keys, MAX_RECORDS_PER_TABLE, test_conn);
	fail_unless(foundkeys == 1, "Query didn't find the correct number of keys.");

	// Check the matching keys.
	fail_unless(
		( strcmp(test_keys[0], KEY1) == 0 ),
		"The returned keys don't match the query.\n");

	// Make sure next key is not set to anything.
	fail_unless(strcmp(test_keys[1], "") == 0, "No extra keys should be modified.");
}
END_TEST

/**
 * @brief This runs the marking tests for Assignment 3.
 */
int main(int argc, char *argv[])
{
	if(argc == 2)
		server_port = atoi(argv[1]);
	else
		server_port = SERVERPORT;
	printf("Using server port: %d.\n", server_port);
	Suite *s = suite_create("a3-query");
	TCase *tc;

	// Query tests on simple tables
	tc = tcase_create("query_max_keys");
	tcase_set_timeout(tc, TESTTIMEOUT);
	tcase_add_checked_fixture(tc, test_setup_simple_populate, test_teardown);
	tcase_add_test(tc, test_query_max_keys1);
	tcase_add_test(tc, test_query_max_keys2);
	tcase_add_test(tc, test_query_max_keys3);
	suite_add_tcase(s, tc); 

	tc = tcase_create("query_predicates");
	tcase_set_timeout(tc, TESTTIMEOUT);
	tcase_add_checked_fixture(tc, test_setup_simple_populate, test_teardown);
	tcase_add_test(tc, test_query_predicates1);
	tcase_add_test(tc, test_query_predicates2);
	tcase_add_test(tc, test_query_predicates3);
	suite_add_tcase(s, tc); 

	tc = tcase_create("query_comparison");
	tcase_set_timeout(tc, TESTTIMEOUT);
	tcase_add_checked_fixture(tc, test_setup_simple_populate, test_teardown);
	tcase_add_test(tc, test_query_int_comparison1);
	tcase_add_test(tc, test_query_int_comparison2);
	tcase_add_test(tc, test_query_int_comparison3);
	tcase_add_test(tc, test_query_int_comparison4);
	tcase_add_test(tc, test_query_str_comparison1);
	tcase_add_test(tc, test_query_str_comparison2);
	suite_add_tcase(s, tc); 

	tc = tcase_create("query_complex_comparison");
	tcase_set_timeout(tc, TESTTIMEOUT);
	tcase_add_checked_fixture(tc, test_setup_complex_populate, test_teardown);
	tcase_add_test(tc, test_query_int_comparison_complex1);
	tcase_add_test(tc, test_query_int_comparison_complex2);
	tcase_add_test(tc, test_query_int_str_comparison_complex1);
	tcase_add_test(tc, test_query_int_str_comparison_complex2);
	tcase_add_test(tc, test_query_int_str_comparison_complex3);
	suite_add_tcase(s, tc); 

	SRunner *sr = srunner_create(s);
	srunner_set_log(sr, "results.log");
	srunner_run_all(sr, CK_ENV);
	srunner_ntests_failed(sr);
	srunner_free(sr);

	return EXIT_SUCCESS;
}