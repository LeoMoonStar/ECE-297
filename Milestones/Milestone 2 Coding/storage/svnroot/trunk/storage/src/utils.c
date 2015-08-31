/**
 * @file
 * @brief This file implements various utility functions that are
 * can be used by the storage server and client library.
 */

#define _XOPEN_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include "utils.h"


int sendall(const int sock, const char *buf, const size_t len)
{
    size_t tosend = len;
    while (tosend > 0)
    {
        ssize_t bytes = send(sock, buf, tosend, 0);
        if (bytes <= 0)
            break; // send() was not successful, so stop.
        tosend -= (size_t) bytes;
        buf += bytes;
    };

    return tosend == 0 ? 0 : -1;
}


/**
 * @brief In order to avoid reading more than a line from the stream,
 * this function only reads one byte at a time.  This is very
 * inefficient, and you are free to optimize it or implement your
 * own function.
 */
int recvline(const int sock, char *buf, const size_t buflen)
{
    int status = 0; // Return status.
    size_t bufleft = buflen;

    while (bufleft > 1)
    {
        // Read one byte from scoket.
        ssize_t bytes = recv(sock, buf, 1, 0);
        if (bytes <= 0)
        {
            // recv() was not successful, so stop.
            status = -1;
            break;
        }
        else if (*buf == '\n')
        {
            // Found end of line, so stop.
            *buf = 0; // Replace end of line with a null terminator.
            status = 0;
            break;
        }
        else
        {
            // Keep going.
            bufleft -= 1;
            buf += 1;
        }
    }
    *buf = 0; // add null terminator in case it's not already there.

    return status;
}


/**
 * @brief Parse and process a line in the config file.
 */
int process_config_line(char *line, struct config_params *params)
{
    // Ignore comments.
    if (line[0] == CONFIG_COMMENT_CHAR)
        return 0;

    // Extract config parameter name and value.
    char name[MAX_CONFIG_LINE_LEN] = {0};
    char value[MAX_CONFIG_LINE_LEN] = {0};
    char trash[MAX_CONFIG_LINE_LEN] = {0};

    int items = sscanf(line, "%s %s %s\n", name, value, trash);

    // Line wasn't as expected.
    if (items != 2)
        return 1;

    // Process this line.
    if (strcmp(name, "server_host") == 0)
    {
        // Checking if server_host already entered, then invalid config file
        if(params->server_host[0] == '\0')
            strncpy(params->server_host, value, sizeof params->server_host);
        else
            return 1;
    }
    else if (strcmp(name, "server_port") == 0)
    {
        
        // Checking if server_port already entered, then invalid config file
        // Checking port range
        if(params->server_port == -1 && atoi(value) > 1023 && atoi(value) < 65536)
            params->server_port = atoi(value);
        else
            return 1;
    }
    else if (strcmp(name, "username") == 0)
    {
        // Checking if server_username already entered, then invalid config file
        if(params->username[0] == '\0')
            strncpy(params->username, value, sizeof params->username);
        else
            return 1;
    }
    else if (strcmp(name, "password") == 0)
    {
        // Checking if server_password already entered, then invalid config file
        if(params->password[0] == '\0')
            strncpy(params->password, value, sizeof params->password);
        else
            return 1;
    }// else if (strcmp(name, "data_directory") == 0) {
    //	strncpy(params->data_directory, value, sizeof params->data_directory);
    //}
    else if (strcmp(name, "table") == 0)
    {
        if(check_special(value) == false)
            return 1;
        // Check if table name already in table list
        int i;
        for(i = 0; i < params->num_tables; i++)
            if(strcmp(params->table_names[i], value) == 0)
                return 1;

        // Add to list of of tables names and increment number of tables
        strncpy(params->table_names[params->num_tables], value, sizeof params->table_names[params->num_tables]);
        params->num_tables++;
    }

    return 0;
}


int read_config(const char *config_file, struct config_params *params)
{
    int error_occurred = 0;

    // Open file for reading.
    FILE *file = fopen(config_file, "r");
    if (file == NULL)
        error_occurred = 1;

    // Process the config file.
    while (!error_occurred && !feof(file))
    {
        // Read a line from the file.
        char line[MAX_CONFIG_LINE_LEN] = {0};
        char *l = fgets(line, sizeof line, file);

        // Process the line.
        if (l == line)
            error_occurred = process_config_line(line, params);
        else if (!feof(file))
            error_occurred = 1;
    }

    // Checking if no tables specified in config file
    if(params->num_tables == 0)
        error_occurred = 1;

    return error_occurred ? -1 : 0;
}

void logger(FILE *file, char *message)
{

    if(file == NULL)
        return;

    fprintf(file, "%s", message);
    fflush(file);
}


char *generate_encrypted_password(const char *passwd, const char *salt)
{
    if(salt != NULL)
        return crypt(passwd, salt);
    else
        return crypt(passwd, DEFAULT_CRYPT_SALT);
}


char *generate_logfile(char *file_name)
{

    char curr_time[MAX_LOG_NAME] = {0};
    time_t rawtime;
    struct tm * timeinfo;

    time (&rawtime);
    timeinfo = localtime (&rawtime);

    strftime(curr_time, MAX_LOG_NAME, "-%F-%H-%M-%S.log", timeinfo);
    return strcat(file_name, curr_time);
}


 bool check_special(char* buf)
 {
    int i;
    for(i = 0; i < strlen(buf); i++)
        if(!isalnum(buf[i]) && !isspace(buf[i]))
            return false;
    return true;
}


int make_key(char* buf)
{
    int i, j = 0, length = strlen(buf);
    char new_buf[MAX_VALUE_LEN] = {0};

    for(i = 0; i < length; i++)
        if(isalnum(buf[i]))
            new_buf[j++] = buf[i];

    strcpy(buf, new_buf);

    return 0;
}


int make_value(char* buf)
{

    int i, j = 0, length = strlen(buf);
    char new_buf[MAX_VALUE_LEN] = {0};

    for(i = 0; i < length; i++)
        if(isalnum(buf[i]) || isspace(buf[i]))
            new_buf[j++] = buf[i];

    strcpy(buf, new_buf);

    return 0;
}




