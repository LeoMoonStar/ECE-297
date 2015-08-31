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
#define BADTABLE	"bad_table"	// A bad table name.
#define BADKEY		"bad_key"	// A bad key name.
#define BADVALUE "col 22!?" // An invalid value with special characters
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

#define FLOATTOLERANCE  0.0001		// How much a float value can be off by (due to type conversions).

/* Server port used by test */
int server_port;

/**
 * @brief Compare whether two floating point numbers are almost the same.
 * @return 0 if the numbers are almost the same, -1 otherwise.
 */
int floatcmp(float a, float b)
{
	if (fabs(a - b) < FLOATTOLERANCE)
		return 0;
	else
		return -1;
}

/**
 * @brief Remove trailing spaces from a string.
 * @param str The string to trim.
 * @return The modified string.
 */
char* trimtrailingspc(char *str)
{
	// Make sure string isn't null.
	if (str == NULL)
		return NULL;

	// Trim spaces from the end.
	int i = 0;
	for (i = strlen(str) - 1; i >= 0; i--) {
		if (str[i] == ' ')
			str[i] = '\0';
		else
			break; // stop if it's not a space.
	}
	return str;
}

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
 * @brief Create an empty data directory, start the server, and connect to it.
 * @return A connection to the server if successful.
 */
void* init_start_connect(char *config_file, char *serverout_file, int *serverpid)
{
	// Delete the data directory.
//	system("rm -rf " DATADIR);
	
	// Create the data directory.
//	mkdir(DATADIR, 0777);

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


/// Connection used by test fixture.
void *test_conn = NULL;


// Keys array used by test fixture.
char* test_keys[MAX_RECORDS_PER_TABLE];

/**
 * @brief Text fixture setup.  Start the server.
 */
void test_setup_simple()
{
	test_conn = init_start_connect(SIMPLETABLES_CONF, "simpleempty.serverout", NULL);
	fail_unless(test_conn != NULL, "Couldn't start or connect to server.");
}

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

/**
 * @brief Text fixture setup.  Start the server with complex tables.
 */
void test_setup_complex()
{
	test_conn = init_start_connect(COMPLEXTABLES_CONF, "complexempty.serverout", NULL);
	fail_unless(test_conn != NULL, "Couldn't start or connect to server.");
}

/**
 * @brief Text fixture setup.  Start the server with complex tables and populate the tables.
 */
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
/**
 * @brief Text fixture teardown.  Disconnect from the server.
 */
void test_teardown()
{
	// Disconnect from the server.
	storage_disconnect(test_conn);
	//fail_unless(status == 0, "Error disconnecting from the server.");
}



/*
 * Set failure tests:
 * 	set with invalid table/key/conn (fail)
 * 	set with bad table/key/values (fail)
 * 	set with non-existent table/key (fail)
 */

// Invalid param tests
START_TEST (test_setinvalid_invalidconn)
{
	struct storage_record record;
	strncpy(record.value, INTCOLVAL, sizeof record.value);
	int status = storage_set(INTTABLE, KEY, &record, NULL);
	fail_unless(status == -1, "storage_set with invalid param should fail.");
	fail_unless(errno == ERR_INVALID_PARAM, "storage_set with invalid param not setting errno properly.");
}
END_TEST


START_TEST (test_setinvalid_invalidtable)
{
	struct storage_record record;
	strncpy(record.value, INTCOLVAL, sizeof record.value);
	int status = storage_set(NULL, KEY, &record, test_conn);
	fail_unless(status == -1, "storage_set with invalid param should fail.");
	fail_unless(errno == ERR_INVALID_PARAM, "storage_set with invalid param not setting errno properly.");
}
END_TEST


START_TEST (test_setinvalid_invalidkey)
{
	struct storage_record record;
	strncpy(record.value, INTCOLVAL, sizeof record.value);
	int status = storage_set(INTTABLE, NULL, &record, test_conn);
	strncpy(record.value, BADVALUE, sizeof record.value);
	fail_unless(status == -1, "storage_set with invalid param should fail.");
	fail_unless(errno == ERR_INVALID_PARAM, "storage_set with invalid param not setting errno properly.");
}
END_TEST


START_TEST (test_setinvalid_badvalue)
{
	struct storage_record record;
	strncpy(record.value, BADVALUE, sizeof record.value);
	int status = storage_set(INTTABLE, KEY, &record, test_conn);
	fail_unless(status == -1, "storage_set with bad value should fail.");
	fail_unless(errno == ERR_INVALID_PARAM, "storage_set with bad value not setting errno properly.");
}
END_TEST


START_TEST (test_setinvalid_badkey)
{
	struct storage_record record;
	strncpy(record.value, INTCOLVAL, sizeof record.value);
	int status = storage_set(INTTABLE, BADKEY, &record, test_conn);
	fail_unless(status == -1, "storage_set with bad key name should fail.");
	fail_unless(errno == ERR_INVALID_PARAM, "storage_set with bad key name not setting errno properly.");
}
END_TEST


START_TEST (test_setinvalid_badtable)
{
	struct storage_record record;
	strncpy(record.value, INTCOLVAL, sizeof record.value);
	int status = storage_set(BADTABLE, KEY, &record, test_conn);
	fail_unless(status == -1, "storage_set with bad table name should fail.");
	fail_unless(errno == ERR_INVALID_PARAM, "storage_set with bad table name not setting errno properly.");
}
END_TEST


//Failed connection test, should do at end of test case
START_TEST (test_setinvalid_disconnected)
{
	struct storage_record record;
	strncpy(record.value, INTCOLVAL, sizeof record.value);
	storage_disconnect(test_conn);
	int status = storage_set(INTTABLE, KEY, &record, test_conn);
	fail_unless(status == -1, "storage_set with closed connection should fail.");
	fail_unless(errno == ERR_CONNECTION_FAIL, "storage_set with closed connection not setting errno properly.");
}
END_TEST


//Missing table test
START_TEST (test_setmissing_missingtable)
{
	struct storage_record record;
	strncpy(record.value, INTCOLVAL, sizeof record.value);
	int status = storage_set(MISSINGTABLE, KEY1, &record, test_conn);
	fail_unless(status == -1, "storage_set with missing table should fail.");
	fail_unless(errno == ERR_TABLE_NOT_FOUND, "storage_set with missing table not setting errno properly.");
}
END_TEST


START_TEST (test_setmissing_deletemissingkey)
{
	int status = storage_set(INTTABLE, MISSINGKEY, NULL, test_conn);
	fail_unless(status == -1, "storage_set delete with missing key should fail.");
	fail_unless(errno == ERR_KEY_NOT_FOUND, "storage_set delete with missing key not setting errno properly.");
}
END_TEST


//Testing complex value invalid params
START_TEST (test_setinvalidcomplex_missingcolumn)
{
        struct storage_record record;
        strncpy(record.value, "col1 22,col2 22", sizeof record.value);
        int status = storage_set(THREECOLSTABLE, KEY, &record, test_conn);
        fail_unless(status == -1, "storage_set with missing column should fail.");
        fail_unless(errno == ERR_INVALID_PARAM, "storage_set with missing column not setting errno properly.");
}
END_TEST


START_TEST (test_setinvalidcomplex_unorderedcolumns)
{
        struct storage_record record;
        strncpy(record.value, "col1 22,col3 abc,col2 22", sizeof record.value);
        int status = storage_set(THREECOLSTABLE, KEY, &record, test_conn);
        fail_unless(status == -1, "storage_set with out of order columns should fail.");
        fail_unless(errno == ERR_INVALID_PARAM, "storage_set with out of order columns not setting errno properly.");
}
END_TEST


START_TEST (test_setinvalidcomplex_extracolumn)
{
        struct storage_record record;
        strncpy(record.value, "col1 22,col2 2,col3 abc,col4 abc", sizeof record.value);
        int status = storage_set(THREECOLSTABLE, KEY, &record, test_conn);
        fail_unless(status == -1, "storage_set with extra column should fail.");
        fail_unless(errno == ERR_INVALID_PARAM, "storage_set with extra column not setting errno properly.");
}
END_TEST


START_TEST (test_setinvalidcomplex_mismatchedcolumndata)
{
        struct storage_record record;
        strncpy(record.value, "col1 22,col2 abc,col3 abc", sizeof record.value);
        int status = storage_set(THREECOLSTABLE, KEY, &record, test_conn);
        fail_unless(status == -1, "storage_set with mismatched column data type should fail.");
        fail_unless(errno == ERR_INVALID_PARAM, "storage_set with mismatched column data type not setting errno properly.");
}
END_TEST


START_TEST (test_setinvalidcomplex_invalidcolumn)
{
        struct storage_record record;
        strncpy(record.value, "col10 22,col2 2,col3 abc", sizeof record.value);
        int status = storage_set(THREECOLSTABLE, KEY, &record, test_conn);
        fail_unless(status == -1, "storage_set with undefined column should fail.");
        fail_unless(errno == ERR_INVALID_PARAM, "storage_set with undefined column not setting errno properly.");
}
END_TEST


// Set operations with populated tables.
/*
 * Set operations with simple tables.
 * 	update an existing record (pass).
 * 	delete an existing record (pass).
 */


START_TEST (test_set_modifyint)
{
	struct storage_record record;
	strncpy(record.value, "col 221", sizeof record.value);
	int status = storage_set(INTTABLE, KEY1, &record, test_conn);
	fail_unless(status == 0, "Error setting the key/value pair.");

	//Check modified value
	struct storage_record new_record;
	strncpy(new_record.value, "", sizeof new_record.value);
	status = storage_get(INTTABLE, KEY1, &new_record, test_conn);
	fail_unless(status == 0, "Error getting the modified key/value pair.");
	fail_unless(strcmp("col 221", new_record.value) == 0, "storage_get returned incorrect value for modified key.");
}
END_TEST


START_TEST (test_set_modifystr)
{
	struct storage_record record;
	strncpy(record.value, "col newval", sizeof record.value);
	int status = storage_set(STRTABLE, KEY1, &record, test_conn);
	fail_unless(status == 0, "Error setting the key/value pair.");

	//Check modified value
	struct storage_record new_record;
	strncpy(new_record.value, "", sizeof new_record.value);
	status = storage_get(STRTABLE, KEY1, &new_record, test_conn);
	fail_unless(status == 0, "Error getting the modified key/value pair.");
	fail_unless(strcmp("col newval", new_record.value) == 0, "storage_get returned incorrect value for modified key.");
}
END_TEST


START_TEST (test_set_deleteint)
{
	// Delete a key/value pair.
	int status = storage_set(INTTABLE, KEY1, NULL, test_conn);
	fail_unless(status == 0, "Error deleting the key/value pair.");

	// Try to get the deleted value.
	struct storage_record record;
	strncpy(record.value, "", sizeof record.value);
	status = storage_get(INTTABLE, KEY1, &record, test_conn);
	fail_unless(status == -1, "storage_get for deleted key should fail.");
	fail_unless(errno == ERR_KEY_NOT_FOUND, "storage_get for deleted key not setting errno properly.");
}
END_TEST


START_TEST (test_set_deletestr)
{
	// Delete a key/value pair.
	int status = storage_set(STRTABLE, KEY1, NULL, test_conn);
	fail_unless(status == 0, "Error deleting the key/value pair.");

	// Try to get the deleted value.
	struct storage_record record;
	strncpy(record.value, "", sizeof record.value);
	status = storage_get(STRTABLE, KEY1, &record, test_conn);
	fail_unless(status == -1, "storage_get for deleted key should fail.");
	fail_unless(errno == ERR_KEY_NOT_FOUND, "storage_get for deleted key not setting errno properly.");
}
END_TEST


START_TEST (test_set_createkey)
{
	struct storage_record record;
	strncpy(record.value, "col 221", sizeof record.value);
	int status = storage_set(INTTABLE, MISSINGKEY, &record, test_conn);
	fail_unless(status == 0, "Error creating new key/value pair.");

	//Check modified value
	struct storage_record new_record;
	strncpy(new_record.value, "", sizeof new_record.value);
	status = storage_get(INTTABLE, MISSINGKEY, &new_record, test_conn);
	fail_unless(status == 0, "Error getting the created key/value pair.");
	fail_unless(strcmp("col 221", new_record.value) == 0, "storage_get returned incorrect value for new key/value.");
}
END_TEST

/*
 * Set operations with complex tables.
 * 	update an existing record (pass).
 * 	delete an existing record (pass).
 */

START_TEST (test_setcomplex_createthreecols)
{
	struct storage_record record;
	int fields = 0;
	int intval = 0;
	float floatval = 0;
	char strval[MAX_VALUE_LEN];
	strncpy(strval, "", sizeof strval);

	// Update the value
	strncpy(record.value, "col1 -8,col2 -8,col3 ABC", sizeof record.value);
	int status = storage_set(THREECOLSTABLE, MISSINGKEY, &record, test_conn);
	fail_unless(status == 0, "Error creating a value.");

	// Get the new value.
	strncpy(record.value, "", sizeof record.value);
	status = storage_get(THREECOLSTABLE, MISSINGKEY, &record, test_conn);
	fail_unless(status == 0, "Error getting a value.");
	fields = sscanf(record.value, "col1 %d , col2 %f , col3 %[a-zA-Z0-9 ]", &intval, &floatval, strval);
	fail_unless(fields == 3, "Got wrong number of fields.");
	fail_unless(intval == -8, "Got wrong value.");
	fail_unless(floatval == -8, "Got wrong value.");
	fail_unless(strcmp(trimtrailingspc(strval), "ABC") == 0, "Got wrong value.");
}
END_TEST


START_TEST (test_setcomplex_deletethreecols)
{
	// Delete a key/value pair.
	int status = storage_set(THREECOLSTABLE, KEY1, NULL, test_conn);
	fail_unless(status == 0, "Error deleting the key/value pair.");

	// Try to get the deleted value.
	struct storage_record record;
	strncpy(record.value, "", sizeof record.value);
	status = storage_get(THREECOLSTABLE, KEY1, &record, test_conn);
	fail_unless(status == -1, "storage_get for deleted key should fail.");
	fail_unless(errno == ERR_KEY_NOT_FOUND, "storage_get for deleted key not setting errno properly.");
}
END_TEST


START_TEST (test_setcomplex_updatethreecols) 
{
	struct storage_record record;
	int fields = 0;
	int intval = 0;
	float floatval = 0;
	char strval[MAX_VALUE_LEN];
	strncpy(strval, "", sizeof strval);

	// Update the value
	strncpy(record.value, "col1 -8,col2 -8,col3 ABC", sizeof record.value);
	int status = storage_set(THREECOLSTABLE, KEY1, &record, test_conn);
	fail_unless(status == 0, "Error updating a value.");

	// Get the new value.
	strncpy(record.value, "", sizeof record.value);
	status = storage_get(THREECOLSTABLE, KEY1, &record, test_conn);
	fail_unless(status == 0, "Error getting a value.");
	fields = sscanf(record.value, "col1 %d , col2 %f , col3 %[a-zA-Z0-9 ]", &intval, &floatval, strval);
	fail_unless(fields == 3, "Got wrong number of fields.");
	fail_unless(intval == -8, "Got wrong value.");
	fail_unless(floatval == -8, "Got wrong value.");
	fail_unless(strcmp(trimtrailingspc(strval), "ABC") == 0, "Got wrong value.");
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
	Suite *s = suite_create("a3-set");
	TCase *tc;

	// Set tests with invalid parameters and set failed on simple tables
	tc = tcase_create("set_invalid_simple");
	tcase_set_timeout(tc, TESTTIMEOUT);
	tcase_add_checked_fixture(tc, test_setup_simple, test_teardown);
	tcase_add_test(tc, test_setinvalid_invalidconn);
	tcase_add_test(tc, test_setinvalid_invalidtable);
	tcase_add_test(tc, test_setinvalid_invalidkey);
	tcase_add_test(tc, test_setinvalid_badvalue);
	tcase_add_test(tc, test_setinvalid_badkey);
	tcase_add_test(tc, test_setinvalid_badtable);
	tcase_add_test(tc, test_setmissing_missingtable);
	tcase_add_test(tc, test_setmissing_deletemissingkey);
	tcase_add_test(tc, test_setinvalid_disconnected);
	suite_add_tcase(s, tc);

	// Set tests with invalid parameters on complex tables
	tc = tcase_create("set_invalid_complex");
	tcase_set_timeout(tc, TESTTIMEOUT);
	tcase_add_checked_fixture(tc, test_setup_complex, test_teardown);
	tcase_add_test(tc, test_setinvalidcomplex_missingcolumn);
	tcase_add_test(tc, test_setinvalidcomplex_unorderedcolumns);
	tcase_add_test(tc, test_setinvalidcomplex_extracolumn);
	tcase_add_test(tc, test_setinvalidcomplex_mismatchedcolumndata);
	tcase_add_test(tc, test_setinvalidcomplex_invalidcolumn);
	suite_add_tcase(s, tc);

	
	// Set tests on simple tables
	tc = tcase_create("set_simple");
	tcase_set_timeout(tc, TESTTIMEOUT);
	tcase_add_checked_fixture(tc, test_setup_simple_populate, test_teardown);
	tcase_add_test(tc, test_set_modifyint);
	tcase_add_test(tc, test_set_modifystr);
	tcase_add_test(tc, test_set_createkey);
	tcase_add_test(tc, test_set_deleteint);
	tcase_add_test(tc, test_set_deletestr);
	suite_add_tcase(s, tc);

	// Set tests on complex tables
	tc = tcase_create("set_complex");
	tcase_set_timeout(tc, TESTTIMEOUT);
	tcase_add_checked_fixture(tc, test_setup_complex_populate, test_teardown);
	tcase_add_test(tc, test_setcomplex_createthreecols);
	tcase_add_test(tc, test_setcomplex_deletethreecols);
	tcase_add_test(tc, test_setcomplex_updatethreecols);
	suite_add_tcase(s, tc);

	SRunner *sr = srunner_create(s);
	srunner_set_log(sr, "results.log");
	srunner_run_all(sr, CK_ENV);
	srunner_ntests_failed(sr);
	srunner_free(sr);

	return EXIT_SUCCESS;
}

