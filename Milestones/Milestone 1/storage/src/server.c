/**
 * @file
 * @brief This file implements the storage server.
 *
 * The storage server should be named "server" and should take a single
 * command line argument that refers to the configuration file.
 * 
 * The storage server should be able to communicate with the client
 * library functions declared in storage.h and implemented in storage.c.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <assert.h>
#include <signal.h>
#include "utils.h"

#define MAX_LISTENQUEUELEN 20	///< The maximum number of queued connections.


FILE *serverlogger;
char stringmake[MAX_CMD_LEN];
/**
 * @brief Process a command from the client.
 *
 * @param sock The socket connected to the client.
 * @param cmd The command received from the client.
 * @return Returns 0 on success, -1 otherwise.
 */
int handle_command(int sock, char *cmd)
{
    if(LOGGING == 1)
    {
        snprintf(stringmake, sizeof stringmake, "Processing command '%s'\n", cmd);//making a string
        logger(stdout,stringmake); // replace LOG commands with logger() calls
    }
    else if(LOGGING == 2)
    {
        snprintf(stringmake, sizeof stringmake, "Processing command '%s'\n", cmd);//making a string
        logger(serverlogger,stringmake); // replace LOG commands with logger() calls
    }
	// For now, just send back the command to the client.
	sendall(sock, cmd, strlen(cmd));
	sendall(sock, "\n", 1);
    
	return 0;
}

/**
 * @brief Start the storage server.
 *
 * This is the main entry point for the storage server.  It reads the
 * configuration file, starts listening on a port, and proccesses
 * commands from clients.
 */
int main(int argc, char *argv[])
{
    time_t Time = time(NULL);
    struct tm *currentTime = localtime(&Time);
    char ServerFileName[MAX_CONFIG_LINE_LEN];
    
    
    int day = currentTime->tm_mday;
    int month = currentTime->tm_mon + 1; // Month is 0 - 11, add 1 to get a jan-dec 1-12 concept
    int year = currentTime->tm_year + 1900;
    int hour = currentTime->tm_hour;
    int minute = currentTime->tm_min;
    int second = currentTime->tm_sec;
    snprintf(ServerFileName, sizeof ServerFileName, "Server-%d-%02d-%02d-%02d-%02d-%02d.log",year,month,day,hour,minute,second);//making a string

    serverlogger = fopen(ServerFileName,"a");
    // Process command line arguments.
	// This program expects exactly one argument: the config file name.
	assert(argc > 0);
	if (argc != 2)
    {
		printf("Usage %s <config_file>\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	char *config_file = argv[1];

	// Read the config file.
	struct config_params params;
	int status = read_config(config_file, &params);
	if (status != 0)
    {
		printf("Error processing config file.\n");
		exit(EXIT_FAILURE);
	}
    
    if(LOGGING == 1)
    {
        snprintf(stringmake, sizeof stringmake, "Server on %s:%d\n", params.server_host, params.server_port);
        logger(stdout,stringmake);
    }
    else if(LOGGING == 2)
    {
        snprintf(stringmake, sizeof stringmake, "Server on %s:%d\n", params.server_host, params.server_port);
        logger(serverlogger,stringmake); // replace LOG commands with logger() calls
    }
    
    // Create a socket.
	int listensock = socket(PF_INET, SOCK_STREAM, 0);
	if (listensock < 0)
    {
		printf("Error creating socket.\n");
		exit(EXIT_FAILURE);
	}

	// Allow listening port to be reused if defunct.
	int yes = 1;
	status = setsockopt(listensock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);
	if (status != 0)
    {
		printf("Error configuring socket.\n");
		exit(EXIT_FAILURE);
	}

	// Bind it to the listening port.
	struct sockaddr_in listenaddr;
	memset(&listenaddr, 0, sizeof listenaddr);
	listenaddr.sin_family = AF_INET;
	listenaddr.sin_port = htons(params.server_port);
	inet_pton(AF_INET, params.server_host, &(listenaddr.sin_addr)); // bind to local IP address
	status = bind(listensock, (struct sockaddr*) &listenaddr, sizeof listenaddr);
	if (status != 0)
    {
		printf("Error binding socket.\n");
		exit(EXIT_FAILURE);
	}

	// Listen for connections.
	status = listen(listensock, MAX_LISTENQUEUELEN);
	if (status != 0)
    {
		printf("Error listening on socket.\n");
		exit(EXIT_FAILURE);
	}

	// Listen loop.
	int wait_for_connections = 1;
	while (wait_for_connections) {
		// Wait for a connection.
		struct sockaddr_in clientaddr;
		socklen_t clientaddrlen = sizeof clientaddr;
		int clientsock = accept(listensock, (struct sockaddr*)&clientaddr, &clientaddrlen);
		if (clientsock < 0)
        {
			printf("Error accepting a connection.\n");
			exit(EXIT_FAILURE);
		}
        
        
        
        if(LOGGING == 1)
        {
            snprintf(stringmake, sizeof stringmake, "Got a connection from %s:%d.\n", inet_ntoa(clientaddr.sin_addr), clientaddr.sin_port);
            logger(stdout,stringmake);
        }
        else if(LOGGING == 2)
        {
            snprintf(stringmake, sizeof stringmake, "Got a connection from %s:%d.\n", inet_ntoa(clientaddr.sin_addr), clientaddr.sin_port);
            logger(serverlogger,stringmake); // replace LOG commands with logger() calls
        }
        

		// Get commands from client.
		int wait_for_commands = 1;
		do {
			// Read a line from the client.
			char cmd[MAX_CMD_LEN];
			int status = recvline(clientsock, cmd, MAX_CMD_LEN);
			if (status != 0)
            {
				// Either an error occurred or the client closed the connection.
				wait_for_commands = 0;
			} else
            {
				// Handle the command from the client.
				int status = handle_command(clientsock, cmd);
				if (status != 0)
					wait_for_commands = 0; // Oops.  An error occured.
			}
		} while (wait_for_commands);
		
		// Close the connection with the client.
		close(clientsock);
        
        if(LOGGING == 1)
        {
             snprintf(stringmake, sizeof stringmake, "Closed connection from %s:%d.\n", inet_ntoa(clientaddr.sin_addr), clientaddr.sin_port);
            logger(stdout,stringmake);
        }
        else if(LOGGING == 2)
        {
             snprintf(stringmake, sizeof stringmake, "Closed connection from %s:%d.\n", inet_ntoa(clientaddr.sin_addr), clientaddr.sin_port);
            logger(serverlogger,stringmake); // replace LOG commands with logger() calls
        }
        	}

	// Stop listening for connections.
	close(listensock);
    fclose(serverlogger);


	return EXIT_SUCCESS;
}


