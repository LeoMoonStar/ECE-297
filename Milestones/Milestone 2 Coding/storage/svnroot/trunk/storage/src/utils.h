/**
 * @file
 * @brief This file declares various utility functions that are
 * can be used by the storage server and client library.
 */

#ifndef	UTILS_H
#define UTILS_H

#include <stdio.h>
#include "storage.h"
#include <stdbool.h>
#include <errno.h>
#include <ctype.h>
#include <time.h>
#include <sys/time.h>


/**
 * @brief Any lines in the config file that start with this character
 * are treated as comments.
 */
static const char CONFIG_COMMENT_CHAR = '#';


/**
 * @brief The max length in bytes of a command from the client to the server.
 */
#define MAX_CMD_LEN (1024 * 8)


/**
 * @brief A macro to log some information.
 *
 * Use it like this:  LOG(("Hello %s", "world\n"))
 *
 * Don't forget the double parentheses, or you'll get weird errors!
 */
#define LOG(x)  {printf x; fflush(stdout);}


/**
 * @brief A macro to output debug information.
 *
 * It is only enabled in debug builds.
 */
#ifdef NDEBUG
#define DBG(x)  {}
#else
#define DBG(x)  {printf x; fflush(stdout);}
#endif

#define MAX_LOG_NAME 27 ///< Maximum characters in the log file name.
#define BUFFER_SIZE (2 * MAX_CMD_LEN) ///< Buffer size to send commands to logger.
#define LOGGING 0 ///< Client-side Logging output stream config, 0 = Disable, 1 = STDOUT, 2 = Defined File.

extern FILE *client_log;


/**
 * @brief A struct to store config parameters.
 */
struct config_params
{
    /// The hostname of the server.
    char server_host[MAX_HOST_LEN];

    /// The listening port of the server.
    int server_port;

    /// The storage server's username.
    char username[MAX_USERNAME_LEN];

    /// The storage server's encrypted password.
    char password[MAX_ENC_PASSWORD_LEN];

    /// The storage server's list of table names.
    char table_names[MAX_TABLES][MAX_TABLE_LEN];

    /// The number of valid tables.
    int num_tables;

    // The directory where tables are stored.
//	char data_directory[MAX_PATH_LEN];
};


/**
 * @brief Exit the program because a fatal error occured.
 *
 * @param msg The error message to print.
 * @param code The program exit return value.
 */
static inline void die(char *msg, int code)
{
    printf("%s\n", msg);
    exit(code);
}


/**
 * @brief Keep sending the contents of the buffer until complete.
 * @return Return 0 on success, -1 otherwise.
 *
 * The parameters mimic the send() function.
 */
int sendall(const int sock, const char *buf, const size_t len);


/**
 * @brief Receive an entire line from a socket.
 * @return Return 0 on success, -1 otherwise.
 */
int recvline(const int sock, char *buf, const size_t buflen);


/**
 * @brief Read and load configuration parameters.
 *
 * @param config_file The name of the configuration file.
 * @param params The structure where config parameters are loaded.
 * @return Return 0 on success, -1 otherwise.
 */
int read_config(const char *config_file, struct config_params *params);


/**
 * @brief Generates a log message.
 *
 * @param file The output stream
 * @param message Message to log.
 */
void logger(FILE *file, char *message);


/**
 * @brief Default two character salt used for password encryption.
 */
#define DEFAULT_CRYPT_SALT "xx"


/**
 * @brief Generates an encrypted password string using salt CRYPT_SALT.
 *
 * @param passwd Password before encryption.
 * @param salt Salt used to encrypt the password. If NULL default value
 * DEFAULT_CRYPT_SALT is used.
 * @return Returns encrypted password.
 */
char *generate_encrypted_password(const char *passwd, const char *salt);


/**
 * @brief Creates file name including time and date stamps.
 * 
 * @param file_name String containing either "Client" or "Server" based on call origin 
 * @return Returns string name for text file
 */
char *generate_logfile(char *file_name);


/**
 * @brief Checks for special characters in a string.
 * 
 * @param buf String being checked 
 * @return Returns true on success, false otherwise.
 */
bool check_special(char* buf);



/**
 * @brief Removes everything including special characters and spaces in a string.
 * 
 * @param buf String being checked 
 * @return Returns 0 on success.
 */
int make_key(char* buf);


/**
 * @brief Removes special characters in a string.
 * 
 * @param buf String being checked 
 * @return Returns 0 on success.
 */
int make_value(char* buf);

#endif

