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
#define SIMPLETABLES_CONF		"conf-simpletables.conf"	// Server configuration file with simple tables.

#define BADTABLE	"spaced $table"	// A bad table name.
#define BADKEY		"spaced {key"	// A bad key name.

#define INTTABLE		"inttbl"	// The table to use.
#define STRTABLE		"strtbl"	// The table to use.

#define MISSINGTABLE	"missingtable"	// A non-existing table.
#define MISSINGKEY	"missingkey"	// A non-existing key.

#define KEY		"somekey"	// A key used in the test cases.
#define KEY1		"somekey1"	// A key used in the test cases.
#define KEY2		"somekey2"	// A key used in the test cases.
#define KEY3		"somekey3"	// A key used in the test cases.
#define KEY4		"somekey4"	// A key used in the test cases.

// These settings should correspond to what's in the config file.
#define SERVERHOST	"localhost"	// The hostname where the server is running.
#define SERVERPORT	4848		// The port where the server is running.
#define SERVERUSERNAME	"admin"		// The server username
#define SERVERPASSWORD	"dog4sale"	// The server password


/* Server port used by test */
int server_port;


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


void* start_connect_not_authenticated(char *config_file, char *serverout_file, int *serverpid)
{
	// Start the server.
	int pid = start_server(config_file, NULL, serverout_file);
	fail_unless(pid > 0, "Server didn't run properly.");
	if (serverpid != NULL)
		*serverpid = pid;

	// Connect to the server.
	void *conn = storage_connect(SERVERHOST, server_port);
	fail_unless(conn != NULL, "Couldn't connect to server.");

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


void test_setup_not_authenticated()
{
	test_conn = start_connect_not_authenticated(SIMPLETABLES_CONF, "simpleempty.serverout", NULL);
	fail_unless(test_conn != NULL, "Couldn't start or connect to server.");
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


START_TEST (test_null_conn)
{
	struct storage_record record;
	int status = storage_get(INTTABLE, KEY, &record, NULL);
	fail_unless(status == -1, "storage_get with null connection should fail.");
	fail_unless(errno == ERR_INVALID_PARAM, "storage_get with null connection not setting errno properly.");
}
END_TEST


START_TEST (test_null_record)
{
	int status = storage_get(INTTABLE, KEY, NULL, test_conn);
	fail_unless(status == -1, "storage_get with null storage record should fail.");
	fail_unless(errno == ERR_INVALID_PARAM, "storage_get with null storage record not setting errno properly.");
}
END_TEST


START_TEST (test_null_table)
{
	struct storage_record record;
	int status = storage_get(NULL, KEY, &record, test_conn);
	fail_unless(status == -1, "storage_get with no table name provided (null) should fail.");
	fail_unless(errno == ERR_INVALID_PARAM, "storage_get with no table name provided (null) setting errno properly.");
}
END_TEST


START_TEST (test_null_key)
{
	struct storage_record record;
	int status = storage_get(INTTABLE, NULL, &record, test_conn);
	fail_unless(status == -1, "storage_get with no key name provided (null) should fail.");
	fail_unless(errno == ERR_INVALID_PARAM, "storage_get with no key name provided (null) setting errno properly.");
}
END_TEST


START_TEST (test_invalid_table)
{
	struct storage_record record;
	int status = storage_get(BADTABLE, KEY, &record, test_conn);
	fail_unless(status == -1, "storage_get with bad table name should fail.");
	fail_unless(errno == ERR_INVALID_PARAM, "storage_get with bad table name not setting errno properly.");
}
END_TEST


START_TEST (test_invalid_key)
{
	struct storage_record record;
	int status = storage_get(INTTABLE, BADKEY, &record, test_conn);
	fail_unless(status == -1, "storage_get with bad key name should fail.");
	fail_unless(errno == ERR_INVALID_PARAM, "storage_get with bad key name not setting errno properly.");
}
END_TEST


START_TEST (test_not_authenticated)
{
	struct storage_record record;
	int status = storage_get(INTTABLE, KEY, &record, test_conn);
	fail_unless(status == -1, "storage_get without authenticating should fail.");
	fail_unless(errno == ERR_NOT_AUTHENTICATED, "storage_get without authenticating not setting errno properly.");
}
END_TEST


START_TEST (test_missing_table)
{
	struct storage_record record;
	int status = storage_get(MISSINGTABLE, KEY, &record, test_conn);
	fail_unless(status == -1, "storage_get with missing table should fail.");
	fail_unless(errno == ERR_TABLE_NOT_FOUND, "storage_get with missing table not setting errno properly.");
}
END_TEST


START_TEST (test_missing_key1)
{
	struct storage_record record;
	int status = storage_get(INTTABLE, MISSINGKEY, &record, test_conn);
	fail_unless(status == -1, "storage_get with missing key should fail.");
	fail_unless(errno == ERR_KEY_NOT_FOUND, "storage_get with missing key not setting errno properly.");
}
END_TEST


START_TEST (test_missing_key2)
{
	struct storage_record record;
	int status = storage_get(STRTABLE, MISSINGKEY, &record, test_conn);
	fail_unless(status == -1, "storage_get with missing key should fail.");
	fail_unless(errno == ERR_KEY_NOT_FOUND, "storage_get with missing key not setting errno properly.");
}
END_TEST


START_TEST (test_valid_parameters1)
{
	struct storage_record record;
	int status = storage_get(INTTABLE, KEY1, &record, test_conn);
	fail_unless(status == 0, "storage_get with valid parameters should not fail.");
}
END_TEST


START_TEST (test_valid_parameters2)
{
	struct storage_record record;
	int status = storage_get(STRTABLE, KEY2, &record, test_conn);
	fail_unless(status == 0, "storage_get with valid parameters should not fail.");
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
	Suite *s = suite_create("a3-get");
	TCase *tc;

	// Get tests with invalid parameters
	tc = tcase_create("get_invalid_parameters");
	tcase_set_timeout(tc, TESTTIMEOUT);
	tcase_add_checked_fixture(tc, test_setup_simple, test_teardown);
	tcase_add_test(tc, test_null_conn);
	tcase_add_test(tc, test_null_record);
	tcase_add_test(tc, test_null_table);
	tcase_add_test(tc, test_null_key);
	tcase_add_test(tc, test_invalid_table);
	tcase_add_test(tc, test_invalid_key);
	suite_add_tcase(s, tc);

	// Get tests without authentication
	tc = tcase_create("get_table_without_authentication");
	tcase_set_timeout(tc, TESTTIMEOUT);
	tcase_add_checked_fixture(tc, test_setup_not_authenticated, test_teardown);
	tcase_add_test(tc, test_not_authenticated);
	suite_add_tcase(s, tc);

	// Get tests with missing key/table
	tc = tcase_create("get_missing_key/table");
	tcase_set_timeout(tc, TESTTIMEOUT);
	tcase_add_checked_fixture(tc, test_setup_simple, test_teardown);
	tcase_add_test(tc, test_missing_table);
	tcase_add_test(tc, test_missing_key1);
	tcase_add_test(tc, test_missing_key2);
	suite_add_tcase(s, tc);

	// Get tests with valid parameters
	tc = tcase_create("get_valid_parameters");
	tcase_set_timeout(tc, TESTTIMEOUT);
	tcase_add_checked_fixture(tc, test_setup_simple_populate, test_teardown);
	tcase_add_test(tc, test_valid_parameters1);
	tcase_add_test(tc, test_valid_parameters2);
	suite_add_tcase(s, tc);

	SRunner *sr = srunner_create(s);
	srunner_set_log(sr, "results.log");
	srunner_run_all(sr, CK_ENV);
	srunner_ntests_failed(sr);
	srunner_free(sr);

	return EXIT_SUCCESS;
}

