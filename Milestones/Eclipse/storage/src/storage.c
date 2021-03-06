/**
 * @file
 * @brief This file contains the implementation of the storage server
 * interface as specified in storage.h.
 */


#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include "storage.h"
#include "utils.h"


extern FILE *client_log;
/**
 * @brief String to pass to the logger function.
 */
char client_buffer[BUFFER_SIZE];
/**
 * @brief Boolean variable to check if the user has been authenticated.
 */
bool authenticated = false;
/**
 * @brief Boolean variable to check if a connection is established with a server.
 */
bool connected = false;


/**
 * @brief This is just a minimal stub implementation.  You should modify it
 * according to your design.
 */
void* storage_connect(const char *hostname, const int port)
{
    if(port == 0 || port < 1024 || port > 65535)
    {
        errno = ERR_INVALID_PARAM;
        // Log failed port number.        
        sprintf(client_buffer, "Incorrect port number entered: %d.\n", port);
        logger(client_log, client_buffer);
        return NULL;
    }
    
    // Create a socket.
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        errno = ERR_CONNECTION_FAIL; // Error occured when not able to connect to server
        return NULL;
    }

    // Get info about the server.
    struct addrinfo serveraddr, *res;
    memset(&serveraddr, 0, sizeof serveraddr);
    serveraddr.ai_family = AF_UNSPEC;
    serveraddr.ai_socktype = SOCK_STREAM;
    char portstr[MAX_PORT_LEN];
    snprintf(portstr, sizeof portstr, "%d", port);
    int status = getaddrinfo(hostname, portstr, &serveraddr, &res);
    if (status != 0)
    {
        errno = ERR_CONNECTION_FAIL; // Error occured when not able to connect to server
        // Log failed address info retrieval from server.
        sprintf(client_buffer, "Unable to retrieve address info with hostname %s and port %s.\n", hostname, portstr);
        logger(client_log, client_buffer);
        return NULL;
    }

    // Log successful address info retrieval from server.
    sprintf(client_buffer, "Address info retrieved with hostname %s and port %s.\n", hostname, portstr);
    logger(client_log, client_buffer);

    // Connect to the server.
    status = connect(sock, res->ai_addr, res->ai_addrlen);
    if (status != 0)
    {
        errno = ERR_CONNECTION_FAIL; // Error occured when not able to connect to server
        // Log failed connection between client and the server.
        sprintf(client_buffer, "Unable to connect to server through socket %d.\n", sock);
        logger(client_log, client_buffer);
        return NULL;
    }

    connected = true;

    // Log successful connection between client and the server.
    sprintf(client_buffer, "Connected to server through socket %d.\n", sock);
    logger(client_log, client_buffer);

    return (void*) sock;
}


/**
 * @brief This is just a minimal stub implementation.  You should modify it
 * according to your design.
 */
int storage_auth(const char *username, const char *passwd, void *conn)
{

    if(conn == NULL)
    {
        errno = ERR_INVALID_PARAM;
        return -1;
    }
    else if(connected == false)
    {
        errno = ERR_CONNECTION_FAIL;
        sprintf(client_buffer, "Not connected to a server.\n");
        logger(client_log, client_buffer);
        return -1;
    }

    // Connection is really just a socket file descriptor.
    int sock = (int)conn;

    int i;
    char pass_asterik[strlen(passwd) + 1];
    for(i = 0; i < strlen(passwd); i++)
        pass_asterik[i] = '*';
    pass_asterik[i] = '\0';

    
    // Send some data.
    char buf[MAX_CMD_LEN];
    memset(buf, 0, sizeof buf);
    char *encrypted_passwd = generate_encrypted_password(passwd, NULL);
    snprintf(buf, sizeof buf, "AUTH #%s #%s\n", username, encrypted_passwd);
    if (sendall(sock, buf, strlen(buf)) == 0 && recvline(sock, buf, sizeof buf) == 0)
    {
        if(strcmp(buf, "AUTH #pass") == 0)
        {
            authenticated = true;
            // Log successful client authorization.
            sprintf(client_buffer, "Client authorization successful. Username: %s and Password: %s.\n", username, pass_asterik);
            logger(client_log, client_buffer);
            return 0;
        }
        else
            errno = ERR_AUTHENTICATION_FAILED;
    }

    // Log failed client authorization.
    sprintf(client_buffer, "Client authorization failure. Username: %s and Password: %s.\n", username, pass_asterik);
    logger(client_log, client_buffer);

    return -1;
}


/**
 * @brief This is just a minimal stub implementation.  You should modify it
 * according to your design.
 */
int storage_get(const char *table, const char *key, struct storage_record *record, void *conn)
{
    if(conn == NULL)
    {
        errno = ERR_INVALID_PARAM;
        return -1;
    }

    // Connection is really just a socket file descriptor.
    int sock = (int)conn;
    char temp_table[MAX_TABLE_LEN] = "\0", temp_key[MAX_KEY_LEN], temp_value[MAX_VALUE_LEN];

    // Send some data.
    char buf[MAX_CMD_LEN];
    memset(buf, 0, sizeof buf);
    snprintf(buf, sizeof buf, "GET #%s #%s\n", table, key);
    if(connected == false)
    {
        errno = ERR_CONNECTION_FAIL;
        sprintf(client_buffer, "Not connected to a server.\n");
        logger(client_log, client_buffer);
    }
    else if(authenticated == false)
    {
        errno = ERR_AUTHENTICATION_FAILED; //Error to check if the user is authenticated 
        sprintf(client_buffer, "Connected to a server, but not yet authenticated.\n");
        logger(client_log, client_buffer);
    }
    else if (sendall(sock, buf, strlen(buf)) == 0 && recvline(sock, buf, sizeof buf) == 0)
    {
        if(sscanf(buf, "GET #%s #%s #%s\n", temp_table, temp_key, temp_value) == 3)
        {
            strncpy(record->value, temp_value, sizeof temp_value);
            return 0;
        }
        else if(strcmp(temp_table, table) != 0)
            errno = ERR_TABLE_NOT_FOUND;
        else
            errno = ERR_KEY_NOT_FOUND;
    }

    // Log failed record retrieval due to problem in data transaction with server.
    sprintf(client_buffer, "Record retrieval failed. Table: %s and Key: %s.\n", table, key);
    logger(client_log, client_buffer);

    return -1;
}


/**
 * @brief This is just a minimal stub implementation.  You should modify it
 * according to your design.
 */
int storage_set(const char *table, const char *key, struct storage_record *record, void *conn)
{
    if(conn == NULL)
    {
        errno = ERR_INVALID_PARAM;
        return -1;
    }

    // Connection is really just a socket file descriptor.
    int sock = (int)conn;
    char temp_table[MAX_TABLE_LEN] = "\0", temp_key[MAX_KEY_LEN], temp_value[MAX_VALUE_LEN];

    // Send some data.
    char buf[MAX_CMD_LEN];
    memset(buf, 0, sizeof buf);
    snprintf(buf, sizeof buf, "SET #%s #%s #%s\n", table, key, record->value);

    if(connected == false)
    {
        errno = ERR_CONNECTION_FAIL;
        sprintf(client_buffer, "Not connected to a server.\n");
        logger(client_log, client_buffer);
    }
    else if(authenticated == false)
    {
        errno = ERR_AUTHENTICATION_FAILED; //Error to check if the user is authenticated 
        sprintf(client_buffer, "Connected to a server, but not yet authenticated.\n");
        logger(client_log, client_buffer);
    }
    else if (sendall(sock, buf, strlen(buf)) == 0 && recvline(sock, buf, sizeof buf) == 0)
    {
        if(sscanf(buf, "SET #%s #%s #%s\n", temp_table, temp_key, temp_value) == 3)
        {
            strncpy(record->value, temp_value, sizeof temp_value);
            return 0;
        }
        else if(strcmp(temp_table, table) != 0)
            errno = ERR_TABLE_NOT_FOUND;
        else
            errno = ERR_KEY_NOT_FOUND;
    }

    // Log failed record modification due to problem in data transaction with server.
    sprintf(client_buffer, "Record modification failed. Table: %s, Key: %s and Value: %s.\n", table, key, record->value);
    logger(client_log, client_buffer);

    return -1;
}


/**
 * @brief This is just a minimal stub implementation.  You should modify it
 * according to your design.
 */
int storage_disconnect(void *conn)
{
    if(conn == NULL)
    {
        errno = ERR_INVALID_PARAM;
        return -1;
    }

    // Cleanup
    int sock = (int)conn;
    close(sock);

    // Log successful address info retrieval from server.
    sprintf(client_buffer, "Server connection closed.\n");
    logger(client_log, client_buffer);

    connected = false;

    return 0;
}
