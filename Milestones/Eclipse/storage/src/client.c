/**
 * @file
 * @brief This file implements an "improved" client shell.
 *
 * The client connects to the server, running at SERVERHOST:SERVERPORT
 * and performs a number of storage_* operations. If there are certain errors,
 * the client exists.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "storage.h"
#include "utils.h"


 int command_parser(struct storage_record *r);
 int connect_server();
 int authenticate_client();
 int get_record(struct storage_record *r);
 int store_record(struct storage_record *r);
 int disconnect();
/**
 * @brief File pointer to open 'stdout' or a .txt file.
 */
 FILE *client_log = NULL;
/**
 * @brief String to store entire input line from input.
 */
 char input_buffer[MAX_CONFIG_LINE_LEN];
/**
 * @brief Junk variable to store user mistakes while parsing the input.
 */
 char trash[MAX_CONFIG_LINE_LEN];
 extern bool connected;
 void *conn = NULL;


/**
 * @brief Start a client to interact with the storage server.
 *
 * Decides where to start logging based on the LOGGING constant.
 * Calls the command_parser function. 
 */
 int main(int argc, char *argv[])
 {

    char client_file[MAX_LOG_NAME] = "Client";
    int status = 0;
    struct storage_record r;

    switch (LOGGING)
    {
        case 0:
        client_log = NULL;
        break;

        case 1:
        client_log = stdout;
        break;

        case 2:
        client_log = fopen(generate_logfile(client_file), "w");
        break;
    }


    while(status != -1)
        status = command_parser(&r);


    if(LOGGING == 2)
        fclose(client_log);

    return status;
}


/**
 * @brief Displays shell and parses input from user.
 *
 * Only allows integer input.
 * Call a function based on user selection (1-6).
 * If connected to a server, it disconnects before exiting.
 * @param r A pointer to a record structure.
 * @return Returns 0 on success, -1 otherwise.
 */
 int command_parser(struct storage_record *r)
 {

    printf("\n\n------------------------------------------\n"
     "\t1) Connect\n"
    "\t2) Authenticate\n"
    "\t3) Get\n"
    "\t4) Set\n"
    "\t5) Disconnect\n"
    "\t6) Exit\n"
    "------------------------------------------\n\n\n");

    int option, status;

    bool read_success = false;
    while(read_success == false)
    {
        printf("Please enter your selection: ");
        char *l = fgets(input_buffer, sizeof input_buffer, stdin);
        if(l != input_buffer || (sscanf(input_buffer, "%d %s\n", &option, trash) != 1) || option > 6 || option < 1)
            printf("Invalid selection. Please enter a valid option number (1-6).\n");
        else
            read_success = true;
    }

    switch (option)
    {
    case 1: // Connect to server
    status = connect_server();
    break;

    case 2: // Authenticate the client.
    status = authenticate_client();
    break;

    case 3: // Issue storage_get
    get_record(r);
    break;

    case 4: // Issue storage_set
    store_record(r);
    break;

    case 5: // Disconnect from server
    status = disconnect();
    break;

    case 6: //Exit
    if (connected)
        status = disconnect();
        status = -1; // To stop main loop
        printf("Goodbye!\n");
        break;

        default:
        status = -1;
        printf("Invalid selection.\n");
        break;
    }

    return status;
}


/**
 * @brief Attempts to establish a connection with the server.
 *
 * Exits if the server connection fails due to any reason. 
 * If already connected to a server, it asks the user if he/she wants to disconnect first. 
 * @return Returns 0 on success, -1 otherwise.
 */
 int connect_server()
 {

    char host[MAX_HOST_LEN], port[MAX_PORT_LEN];
    int status;

    if(connected == true)
    {
        char c;
        bool read_success = false;

        while(read_success == false)
        {
            printf("Already connected to a server. Would you like to disconnect from it? (Y/N): ");
            char *l = fgets(input_buffer, sizeof input_buffer, stdin);
            if(l != input_buffer || (sscanf(input_buffer, " %1[YyNn] %s\n", &c, trash) != 1))
                printf("Incorrect selection.\n");
            else
            {
                read_success = true;
                if(c == 'Y' || c == 'y')
                {
                    status = disconnect();
                    if(status == 0)
                        status = connect_server();  
                }
                else
                    status = 0;
            }

        }

        return status;
    }

    bool read_success = false;
    while(read_success == false)
    {
        printf("Please enter the hostname: ");
        char *l = fgets(input_buffer, sizeof input_buffer, stdin);
        if(l != input_buffer || (sscanf(input_buffer, "%s %s\n", host, trash) != 1))
            printf("Please enter a valid hostname (no spaces).\n");
        else
            read_success = true;
    }

    read_success = false;
    while(read_success == false)
    {
        printf("Please enter the port: ");
        char *l = fgets(input_buffer, sizeof input_buffer, stdin);
        if(l != input_buffer || (sscanf(input_buffer, "%[0-9] %s", port, trash) != 1))
            printf("Invalid entry. Please enter a valid TCP port number (1024 - 65535).\n");
        else
            read_success = true;      
    }

    conn = storage_connect(host, atoi(port));
    if(!conn)
    {
        printf("Cannot connect to server @ %s:%d. Error code: %d.\n", host, atoi(port), errno);
        status = -1;
    }
    else
        printf("Connection to %s:%s successful\n", host, port);

    return status;
}


/**
 * @brief Attempts to authenticate the user with the server.
 *
 * Closes server connection and exits if the user fails to authenticate. 
 * @return Returns 0 on success, -1 otherwise.
 */
 int authenticate_client()
 {

    char username[MAX_USERNAME_LEN], password[MAX_ENC_PASSWORD_LEN];

    bool read_success = false;
    while(read_success == false)
    {
        printf("Please enter the username: ");
        char *l = fgets(input_buffer, sizeof input_buffer, stdin);
        if(l != input_buffer || (sscanf(input_buffer, "%s %s", username, trash) != 1))
            printf("Please enter a valid username (no spaces).\n");
        else
            read_success = true;
    }
    
    read_success = false;
    while(read_success == false)
    {
        printf("Please enter the password: ");
        char *l = fgets(input_buffer, sizeof input_buffer, stdin);
        if(l != input_buffer || (sscanf(input_buffer, "%s %s", password, trash) != 1))
            printf("Please enter a valid password (no spaces).\n");
        else
            read_success = true;
    }

    int i;
    char pass_asterik[strlen(password) + 1];
    for(i = 0; i < strlen(password); i++)
        pass_asterik[i] = '*';
    pass_asterik[i] = '\0';    

    int status = storage_auth(username, password, conn);
    if(status != 0)
    {
        printf("storage_auth failed with username '%s' and password '%s'. " \
         "Error code: %d.\n", username, pass_asterik, errno);
        storage_disconnect(conn);
    }
    else
        printf("storage_auth: successful.\nWelcome %s!\n", username);

    return status;
}


/**
 * @brief Attempts to get a value given table and key names from server.
 *
 * Reports the error number set by the storage server. 
 * Does not exit if an error occurs. 
 * @param r A pointer to a record structure.
 * @return Returns 0 on success, -1 otherwise.
 */
 int get_record(struct storage_record *r)
 {

    char table[MAX_TABLE_LEN], key[MAX_KEY_LEN];

    bool read_success = false;
    while(read_success == false)
    {
        printf("Please enter the table name: ");
        char *l = fgets(input_buffer, sizeof input_buffer, stdin);
        if(l != input_buffer || (sscanf(input_buffer, "%[a-zA-Z0-9] %s", table, trash) != 1))
            printf("Please enter a valid table name (only alphanumeric characters).\n");
        else
            read_success = true;
    }

    read_success = false;
    while(read_success == false)
    {
        printf("Please enter the key: ");
        char *l = fgets(input_buffer, sizeof input_buffer, stdin);
        if(l != input_buffer || (sscanf(input_buffer, "%[a-zA-Z0-9] %s", key, trash) != 1))
            printf("Please enter a valid key (only alphanumeric characters).\n");
        else
            read_success = true;
    }


    int status = storage_get(table, key, r, conn);
    if(status != 0)
    {
        printf("storage_get failed. Error code: %d.\n", errno);
        //storage_disconnect(conn);
    }
    else
        printf("storage_get: the value returned for key '%s' is '%s'.\n", key, r->value);

    return status;
}


/**
 * @brief Attempts to set a value given table and key names and the new value in the server.
 *
 * Reports the error number set by the storage server. 
 * Does not exit if an error occurs. 
 * @param r A pointer to a record structure.
 * @return Returns 0 on success, -1 otherwise.
 */
 int store_record(struct storage_record *r)
 {

    char table[MAX_TABLE_LEN], key[MAX_KEY_LEN], value[MAX_VALUE_LEN];

    bool read_success = false;
    while(read_success == false)
    {
        printf("Please enter the table name: ");
        char *l = fgets(input_buffer, sizeof input_buffer, stdin);
        if(l != input_buffer || (sscanf(input_buffer, "%[a-zA-Z0-9] %s", table, trash) != 1))
            printf("Please enter a valid table name (only alphanumeric characters).\n");
        else
            read_success = true;
    }

    read_success = false;
    while(read_success == false)
    {
        printf("Please enter the key: ");
        char *l = fgets(input_buffer, sizeof input_buffer, stdin);
        if(l != input_buffer || (sscanf(input_buffer, "%[a-zA-Z0-9] %s", key, trash) != 1))
            printf("Please enter a valid key (only alphanumeric characters).\n");
        else
            read_success = true;
    }

    read_success = false;
    while(read_success == false)
    {
        printf("Please enter the value: ");
        char *l = fgets(value, sizeof value, stdin);
        if(l == value)
        {
            read_success = check_special(value);
            if(read_success == false)
                printf("Please enter a valid value (only alphanumeric characters).\n");
        }
    }
    
    strncpy(r->value, value, sizeof r->value);

    int status = storage_set(table, key, r, conn);
    if(status != 0)
    {
        printf("storage_set failed. Error code: %d.\n", errno);
        //storage_disconnect(conn);
    }
    else
        printf("storage_set: value changed for key '%s' in table '%s' to '%s'.\n", key, table, value);

    return status;
}


/**
 * @brief Attempts to disconnect from the server.
 * @return Returns 0 on success, -1 otherwise.
 */
 int disconnect()
 {
    int status = storage_disconnect(conn);

    if(status != 0)
        printf("storage_disconnect failed. Error code: %d.\n", errno);
    else
        printf("Server connection closed.\n");

    return status;
}

