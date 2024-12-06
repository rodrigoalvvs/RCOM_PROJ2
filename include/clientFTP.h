#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/stat.h> 



/* FTP Reply codes */

/*  Ready for user */
#define SERVER_USER_READY 220

#define SERVER_SENDPASSWORD 331
#define SERVER_LOGGED 230

#define SERVER_READY 150
#define SERVER_CONNECTION_ALREADY_OPEN 125
#define SERVER_STARTING_TRANSFER 125

#define SERVER_CLOSING_ON_SUCCESS 226
#define SERVER_CLOSING 221


#define SERVER_ERROR 500
#define SERVER_AUTH_FAILED 530
#define SERVER_NOT_FOUND 550

#define SERVER_PASSIVE 227


#define FTP_PORT 21
#define MAX_BUFFER 8192

#define DEFAULT_USER        "anonymous"
#define DEFAULT_PASSWORD    "password"

struct URL {
    char host[MAX_BUFFER];
    char resource[MAX_BUFFER];
    char file[MAX_BUFFER];
    char user[MAX_BUFFER];
    char password[MAX_BUFFER];
    char ip[MAX_BUFFER];
};

typedef enum {
    START,
    SINGLE,
    MULTIPLE,
    END
} ResponseState;

/** 
 * Creates socket and connects
 * Returns -1 on error, socket fd on success
 */
int socket_create(char* host, int port);


/**
 * Function that parses the given argument to valid URL struct
 * Returns -1 on error, 0 on success
 */
int parse(char* input, struct URL* url);


int server_login(int socket, char* user, char* password);

int enter_passive_mode(int socket);

int request_resource(int socket, char* resource);

int get_resource(int controlSocket, int dataSocket, char* filename);

int close_conn(int controlSocket, int dataSocket);

int send_ftp_command(int sockfd, const char* command);

/**
 * Function that translates the response to a response code
 * returns the code of the response
 */
int get_code_response(const char* buffer);

/**
 * Function that gets ftp responses
 * return the numbers of bytes received
 */
int read_ftp_response(int sockfd, char* buffer);