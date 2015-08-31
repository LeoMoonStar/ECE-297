/**
 * @file
 * @brief This file contains the implementation of the storage server
 * interface as specified in storage.h.
 */

/* In this document I made 4 logger calls based on my research. Logger calls will help in segmentation fault by giving them printf statements.
 * In future milestones during log protocol when sending information to server and receiving from server logger calls can help in debugging any faults.
 * The location where I have made logger calls I have written numbers over there.
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include "storage.h"
#include "utils.h"

/**
 * @brief This is just a minimal stub implementation.  You should modify it 
 * according to your design.
 */
extern FILE *ClientFile;

void* storage_connect(const char *hostname, const int port)
{
	// Create a socket.
	int sock = socket(PF_INET, SOCK_STREAM, 0);
	if (sock < 0)
		return NULL;

	// Get info about the server.
	struct addrinfo serveraddr, *res;
	memset(&serveraddr, 0, sizeof serveraddr);
	serveraddr.ai_family = AF_UNSPEC;
	serveraddr.ai_socktype = SOCK_STREAM;
	char portstr[MAX_PORT_LEN];
    
    //First place where I made logger call.
    if(LOGGING == 1)
    {
        snprintf(portstr, sizeof portstr, "%d", port);
        logger(stdout,portstr);
    }
    else if(LOGGING == 2)
    {
        snprintf(portstr, sizeof portstr, "%d", port);
        logger(ClientFile,portstr);
    }
    
	int status = getaddrinfo(hostname, portstr, &serveraddr, &res);
	if (status != 0)
		return NULL;

	// Connect to the server.
	status = connect(sock, res->ai_addr, res->ai_addrlen);
	if (status != 0)
		return NULL;

	return (void*) sock;
}


/**
 * @brief This is just a minimal stub implementation.  You should modify it 
 * according to your design.
 */
int storage_auth(const char *username, const char *passwd, void *conn)
{

	// Connection is really just a socket file descriptor.
	int sock = (int)conn;

	// Send some data.
	char buf[MAX_CMD_LEN];
	memset(buf, 0, sizeof buf);
	char *encrypted_passwd = generate_encrypted_password(passwd, NULL);
	
    // Second place where I made logger call
    if(LOGGING == 1)
    {
        snprintf(buf, sizeof buf, "AUTH %s %s\n", username, encrypted_passwd);
        logger(stdout,buf);
    }
    else if(LOGGING == 2)
    {
        snprintf(buf, sizeof buf, "AUTH %s %s\n", username, encrypted_passwd);
        logger(ClientFile,buf);
    }
    
	if (sendall(sock, buf, strlen(buf)) == 0 && recvline(sock, buf, sizeof buf) == 0) {
		return 0;
	}

	return -1;
}

/**
 * @brief This is just a minimal stub implementation.  You should modify it 
 * according to your design.
 */
int storage_get(const char *table, const char *key, struct storage_record *record, void *conn)
{

	// Connection is really just a socket file descriptor.
	int sock = (int)conn;

	// Send some data.
	char buf[MAX_CMD_LEN];
	memset(buf, 0, sizeof buf);
    
    //Third place where I made logger call
    if(LOGGING == 1)
    {
        snprintf(buf, sizeof buf, "GET %s %s\n", table, key);
        logger(stdout,buf);
    }
    else if(LOGGING == 2)
    {
        snprintf(buf, sizeof buf, "GET %s %s\n", table, key);
        logger(ClientFile,buf);
    }
	if (sendall(sock, buf, strlen(buf)) == 0 && recvline(sock, buf, sizeof buf) == 0) {
		strncpy(record->value, buf, sizeof record->value);
		return 0;
	}

	return -1;
}


/**
 * @brief This is just a minimal stub implementation.  You should modify it 
 * according to your design.
 */
int storage_set(const char *table, const char *key, struct storage_record *record, void *conn)
{
    // Connection is really just a socket file descriptor.
	int sock = (int)conn;

	// Send some data.
	char buf[MAX_CMD_LEN];
	memset(buf, 0, sizeof buf);
    
    //Fourth place where I made logger call
    if(LOGGING == 1)
    {
        snprintf(buf, sizeof buf, "SET %s %s %s\n", table, key, record->value);
        logger(stdout,buf);
    }
    else if(LOGGING == 2)
    {
       snprintf(buf, sizeof buf, "SET %s %s %s\n", table, key, record->value);
        logger(ClientFile,buf);
    }
	if (sendall(sock, buf, strlen(buf)) == 0 && recvline(sock, buf, sizeof buf) == 0) {
		return 0;
	}

	return -1;
}


/**
 * @brief This is just a minimal stub implementation.  You should modify it 
 * according to your design.
 */
int storage_disconnect(void *conn)
{
	// Cleanup
	int sock = (int)conn;
	close(sock);

	return 0;
}

