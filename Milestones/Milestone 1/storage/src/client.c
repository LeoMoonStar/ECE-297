/**
 * @file
 * @brief This file implements a "very" simple sample client.
 *
 * The client connects to the server, running at SERVERHOST:SERVERPORT
 * and performs a number of storage_* operations. If there are errors,
 * the client exists.
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "storage.h"
#include "utils.h"

FILE *ClientFile;
/**
 * @brief Start a client to interact with the storage server.
 *
 * If connect is successful, the client performs a storage_set/get() on
 * TABLE and KEY and outputs the results on stdout. Finally, it exists
 * after disconnecting from the server.
 */
int main(int argc, char *argv[])
{
    if(LOGGING == 1)
        ClientFile = stdout;
    else if(LOGGING == 2)
    {
        time_t Time = time(NULL);
        struct tm *currentTime = localtime(&Time);
        char ClientFileName[MAX_CONFIG_LINE_LEN];
        int day = currentTime->tm_mday;
        int month = currentTime->tm_mon + 1; // Month is 0 - 11, add 1 to get a jan-dec 1-12 concept
        int year = currentTime->tm_year + 1900;
        int hour = currentTime->tm_hour;
        int minute = currentTime->tm_min;
        int second = currentTime->tm_sec;
        snprintf(ClientFileName, sizeof ClientFileName, "Client-%d-%02d-%02d-%02d-%02d-%02d.log",year,month,day,hour,minute,second);//making a string
        ClientFile = fopen(ClientFileName,"a");
    }
        int selection = 0, port, status;
        char hostname[MAX_HOST_LEN+1], username[MAX_USERNAME_LEN], password[MAX_ENC_PASSWORD_LEN],table[MAX_TABLE_LEN],key[MAX_KEY_LEN], strc[MAX_CONFIG_LINE_LEN];
        struct storage_record r;
        void *conn;
    // Client Interaction with the server
    while(selection!=6)
    {
        printf("> ----------------------\n");
        printf("> 1) Connect\n");
        printf("> 2) Authenticate\n");
        printf("> 3) Get\n");
        printf("> 4) Set\n");
        printf("> 5) Disconnect\n");
        printf("> 6) Exit\n");
        printf("> ----------------------\n");
        printf("> Please enter your selection: ");
        scanf("%d",&selection);
        
        // Connect to server
        if(selection == 1)
        {
            printf("> Please input the hostname: ");
            scanf("%s",hostname);
            printf("> Please input the port: ");
            scanf("%d",&port);
            conn = storage_connect(hostname, port);
            if(!conn)
            {
                snprintf(strc, sizeof strc, "> Cannot connect to server @ %s:%d. Error code: %d.\n",hostname, port, errno);
                printf("%s",strc);
                logger(ClientFile,"a");
                return -1;
            }
            snprintf(strc, sizeof strc, "> Connecting to %s:%d ...\n",hostname,port);
            printf("%s",strc);
            logger(ClientFile,"a");
        }
        
        // Authenticate the client.
        else if (selection == 2)
        {
            printf("> Please enter your username: ");
            scanf("%s",username);
            printf("> Please enter your password: ");
            scanf("%s",password);
            status = storage_auth(username, password, conn);
            if(status != 0)
            {
                snprintf(strc, sizeof strc, "> storage_auth failed with username '%s' and password '%s'. " \
                       "Error code: %d.\n", username, password, errno);
                printf("%s",strc);
                logger(ClientFile,strc);
                storage_disconnect(conn);
                return status;
            }
            snprintf(strc, sizeof strc, "> storage_auth: successful.\n");
            printf("%s",strc);
            logger(ClientFile,strc);
        }
        
        // Issue storage_get
        else if (selection == 3)
        {
            printf("> Please enter table name: ");
            scanf("%s",table);
            printf("> Please enter key: ");
            scanf("%s",key);
            status = storage_get(table, key, &r, conn);
            if(status != 0)
            {
                snprintf(strc, sizeof strc, "> storage_get failed. Error code: %d.\n", errno);
                printf("%s",strc);
                logger(ClientFile,strc);
                storage_disconnect(conn);
                return status;
            }
            snprintf(strc, sizeof strc, "> storage_get: the value returned for key '%s' is '%s'.\n",
                   key, r.value);
            printf("%s",strc);
            logger(ClientFile,strc);
        }
        // Issue storage_set
        else if(selection == 4)
        {
            printf("> Please enter table name: ");
            scanf("%s",table);
            printf("> Please enter key: ");
            scanf("%s",key);
            
            strncpy(r.value, "some_value", sizeof r.value);
            status = storage_set(table, key, &r, conn);
            if(status != 0)
            {
                snprintf(strc, sizeof strc, "> storage_set failed. Error code: %d.\n", errno);
                printf("%s",strc);
                logger(ClientFile,strc);
                storage_disconnect(conn);
                return status;
            }
            snprintf(strc, sizeof strc, "> storage_set: successful.\n");
            printf("%s",strc);
            logger(ClientFile,strc);
        }
        // Disconnect from server
        else if (selection == 5)
        {
            status = storage_disconnect(conn);
            if(status != 0)
            {
                snprintf(strc, sizeof strc, "> storage_disconnect failed. Error code: %d.\n", errno);
                printf("%s",strc);
                logger(ClientFile,strc);
                return status;
            }
            snprintf(strc, sizeof strc, "> Disconnecting from %s:%d ...\n",hostname,port);
            printf("%s",strc);
            logger(ClientFile,strc);
        }
        //Exit
        else if (selection == 6)
            printf("> Exiting server...\n");
        else
            printf("> Enter your selection again\n");
        
    }
    fclose(ClientFile);
    return 0;
}
