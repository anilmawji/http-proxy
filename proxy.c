/**
 * HTTP proxy
 * 
 * Name: Anil Mawji
 * UCID: 30099809
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <arpa/inet.h>
#include <string.h>
#include <fcntl.h>

#define TRUE 1
#define FALSE 0

#define DEBUG FALSE
#define CONFIG_PORT 21356
#define CONFIG_TIMEOUT 10
#define PROXY_SERVER_PORT 21355
#define WEB_SERVER_ADDR "136.159.2.17"
#define WEB_SERVER_PORT 80

#define MAX_BUFFER_SIZE 2048
#define MAX_KEYWORD_SIZE 50
#define MAX_BLOCKED_KEYWORDS 5
#define ERROR_URL "http://pages.cpsc.ucalgary.ca/~carey/CPSC441/ass1/error.html"

/**
 * Check whether a function has returned an error code and exit the program if necessary
 * Prints the relevant error to the console
 */
int check(int status, char* function_name, int should_exit) {
    if (status < 0) {
        fprintf(stderr, "[ERROR]: %s() call has failed!\n", function_name);
        perror(function_name);
        if (should_exit) exit(1);
    }
    return status;
}

/**
 * Creates and returns a new client socket for interacting with a server socket
 */
int initClient(const char* server_addr, const int server_port) {
    // Specify server info
    struct sockaddr_in server;
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(server_port);
    server.sin_addr.s_addr = inet_addr(server_addr);

    int clientsockfd;

    // Create client socket
    check((clientsockfd = socket(PF_INET, SOCK_STREAM, 0)), "socket", TRUE);
    // Connect to server socket
    check(connect(clientsockfd, (struct sockaddr*) &server, sizeof(server)), "connect", TRUE);

    return clientsockfd;
}

/**
 * Creates and returns a new server socket
 */
int initServer(int server_port) {
    // Specify server info
    struct sockaddr_in server;
    memset(&server, 0, sizeof(server));
    // The socket will communicate over TCP
    server.sin_family = AF_INET;
    server.sin_port = htons(server_port);
    // Server will take on any local address
    server.sin_addr.s_addr = htonl(INADDR_ANY);

    int serversockfd;

    // Create server socket
    check((serversockfd = socket(PF_INET, SOCK_STREAM, 0)), "socket", TRUE);
    // Enable address reuse
    check(setsockopt(serversockfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)), "setsockopt", TRUE);
    // Bind the socket to the server struct
    check(bind(serversockfd, (struct sockaddr*) &server, sizeof(struct sockaddr_in)), "bind", TRUE);
    // Start listening for incoming connections
    check(listen(serversockfd, 5), "listen", TRUE);

    return serversockfd;
}

/**
 * Creates and return the server socket meant for configuring the program over telnet
 */
int initConfigServer() {
    int serversockfd = initServer(CONFIG_PORT);

    // Create time struct and specify a time in seconds
    struct timeval timeout;      
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;

    // Set the socket to close after 10 seconds if there are no incoming connections
    check(setsockopt(serversockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)), "setsockopt", TRUE);
    
    return serversockfd;
}

/**
 * Block a given keyword by adding it to the array
 * Returns TRUE if the keyword was added, FALSE if the array is full
 */
int block(const char* keyword, char blockedKeywords[MAX_BLOCKED_KEYWORDS][MAX_KEYWORD_SIZE]) {
	for (int i = 0; i < MAX_BLOCKED_KEYWORDS; i++) {
        // Empty position in the array was found
        if (strlen(blockedKeywords[i]) == 0) {
            // Add the keyword to the array
		    strcpy(blockedKeywords[i], keyword);
            return TRUE;
        }
	}
    return FALSE;
}

/**
 * Remove all blocked keywords from the array
 */
void unblockAll(char blockedKeywords[MAX_BLOCKED_KEYWORDS][MAX_KEYWORD_SIZE]) {
    for (int i = 0; i < MAX_BLOCKED_KEYWORDS; i++) {
        // Clear the memory of each element in the array
        memset(blockedKeywords[i], 0, MAX_KEYWORD_SIZE);
	}
}

/**
 * Check if a string contains a blocked keyword
 * Returns the first keyword found in content if it has blocked keyword in it, FALSE otherwise
 */
char* isBlocked(const char* content, char blockedKeywords[MAX_BLOCKED_KEYWORDS][MAX_KEYWORD_SIZE]) {
	for (int i = 0; i < MAX_BLOCKED_KEYWORDS; i++) {
        if (strlen(blockedKeywords[i]) > 0 && strstr(content, blockedKeywords[i])) {
            // Found a blocked keyword in content
		    return blockedKeywords[i];
        }
	}
    return NULL;
}

/**
 * Modifies an HTTP GET request by removing the URL in the request and replacing it with a new one
 */
char* httpRedirect(char* request, const char* newURL) {
	char content[MAX_BUFFER_SIZE];
	memset(&content, 0, MAX_BUFFER_SIZE);

    // Remove the URL from the GET request and store it in content
	strncpy(content, strstr(request, "HTTP")-1, strlen(content));
    //Prepend the new URL to the request
	snprintf(request, MAX_BUFFER_SIZE, "GET %s%s", newURL, content);
}

int main() {
    printf("[PROXY]: Welcome! The HTTP proxy is now running.\n");
    printf("[PROXY]: You have *%d* seconds to connect to configuration port %d!\n", CONFIG_TIMEOUT, CONFIG_PORT);

    // Set up sockets for the configuration window
    int configserversockfd = initConfigServer();
    int configclientsockfd;
    struct sockaddr_in configclient;
    int structsize = sizeof(struct sockaddr_in);
    int configConnected = FALSE;

    char configbuffer[MAX_BUFFER_SIZE];
    memset(configbuffer, 0, MAX_BUFFER_SIZE);

    // Keep listening for connections to the configuration port
    while (!configConnected) {
        if ((configclientsockfd = accept(configserversockfd, (struct sockaddr*) &configclient, (socklen_t*) &structsize)) < 0) {
            // Time ran out, so stop accepting connections to the configuration port
            printf("[PROXY]: Time's up. Proceeding without configuration...\n");
            break;
        } else {
            // A connection to the configuration port has been established
            snprintf(configbuffer, MAX_BUFFER_SIZE, "[PROXY]: Successfully connected to the configuration port.\n", CONFIG_PORT);
            printf(configbuffer);
            check(send(configclientsockfd, configbuffer, strlen(configbuffer), 0), "send", FALSE);
            // CRITICAL: Set the client socket to non blocking mode
            fcntl(configclientsockfd, F_SETFL, O_NONBLOCK);
            // Break out of the loop
            configConnected = TRUE;
        }
    }
    // Close the configuration server socket since we no longer need to listen for connections
    close(configserversockfd);
    memset(configbuffer, 0, MAX_BUFFER_SIZE);

    char buffer[MAX_BUFFER_SIZE];
    memset(buffer, 0, MAX_BUFFER_SIZE);

    char configresponse[MAX_BUFFER_SIZE];
    memset(configresponse, 0, MAX_BUFFER_SIZE);

    char blockedKeywords[MAX_BLOCKED_KEYWORDS][MAX_KEYWORD_SIZE];
	unblockAll(blockedKeywords);

    // Set up sockets for the proxy server and client
    int proxyserversockfd = initServer(PROXY_SERVER_PORT);
    int clientsockfd;
    struct sockaddr_in client;
    int bytes;
    int done = FALSE;

    while (!done) {
        // A web request has been made, accept the incoming connection
        check((clientsockfd = accept(proxyserversockfd, (struct sockaddr*) &client, (socklen_t*) &structsize)), "accept", TRUE);

        if ((bytes = recv(configclientsockfd, configbuffer, MAX_BUFFER_SIZE, 0)) > 0) {
            // A command has been entered into the config window
            if (strncmp(configbuffer, "BLOCK ", 6) == 0) {
                // Remove the trailing newline from the entered command
                configbuffer[strlen(configbuffer)-2] = '\0';
                // Remove "BLOCK " from the command to extract the keyword and store it in configbuffer
                memmove(configbuffer, configbuffer+6, strlen(configbuffer));
                // Block the requested keyword
                block(configbuffer, blockedKeywords);
                snprintf(configresponse, MAX_BUFFER_SIZE, "[PROXY]: '%s' is now a blocked keyword.\n", configbuffer);
            } else if (strstr(configbuffer, "UNBLOCK")) {
                // Unblock all keywords
                unblockAll(blockedKeywords);
                strcpy(configresponse, "[PROXY]: All keywords have been unblocked.\n");
            }
            printf(configresponse);
            memset(configresponse, 0, MAX_BUFFER_SIZE);
        }

        // User has sent outgoing network data
        if ((bytes = recv(clientsockfd, buffer, MAX_BUFFER_SIZE, 0)) > 0) {
            #if DEBUG
                printf("Received %d bytes from client: \n\n%s\n\n", bytes, buffer);
            #endif
            // Set up a connection to the web server that the user requested access to
            int proxyclientsockfd = initClient(WEB_SERVER_ADDR, WEB_SERVER_PORT);

            if (strstr(buffer, "GET") && isBlocked(buffer, blockedKeywords)) {
                // The user request is an HTTP GET request and the requested URL contains blocked content
                // Redirect the user to an error page by modifying the URL in the GET request
                httpRedirect(buffer, ERROR_URL);
            }
            // Forward the user request through the proxy and over to the server
            check(send(proxyclientsockfd, buffer, bytes, 0), "send", FALSE);
            // Clear the buffer to prepare for receiving data from the web server
            memset(buffer, 0, MAX_KEYWORD_SIZE);

            // Keep receiving data from the web server while there is still data to receive
            while ((bytes = recv(proxyclientsockfd, buffer, MAX_BUFFER_SIZE, 0)) > 0) {
                #if DEBUG
                    printf("Received %d bytes from server:\n\n%s\n\n", bytes, buffer);
                #endif
                // Forward the web page back to the user's browser
                check(send(clientsockfd, buffer, bytes, 0), "send", FALSE);
            }
            // Close the connection with the web server
            close(proxyclientsockfd);
        } else {
            // The user stopped sending data, so close the remaining sockets and clear the data buffer
            #if DEBUG
                printf("Closing connection...\n\n");
            #endif
            close(clientsockfd);
            memset(buffer, 0, MAX_KEYWORD_SIZE);
        }
    }
    // Proxy server stopped running, close the socket
    close(proxyserversockfd);

    return 0;
}
