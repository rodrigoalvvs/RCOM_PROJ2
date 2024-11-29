#include "../include/clientFTP.h"

int socket_create(char *host, int port)
{
    int socket_descriptor;

    if ((socket_descriptor = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
        // Error creating socket
        perror("Couldn't create socket!\n");
        return -1;
    }

    // Socket created now connect
    struct sockaddr_in svAddr;
    uint32_t ip = inet_addr(host);

    bzero(&svAddr, sizeof(svAddr));
    svAddr.sin_family = AF_INET;
    svAddr.sin_addr.s_addr = ip;
    svAddr.sin_port = htons(port);

    if ((connect(socket_descriptor, (struct sockaddr *)&svAddr, sizeof(struct sockaddr_in))) < 0)
    {
        perror("Couldn't connect to host!\n");
        return -1;
    }
    return socket_descriptor;
}

int enter_passive_mode(int socket)
{

    if (send_ftp_command(socket, "PASV\r\n") != SERVER_PASSIVE)
    {
        printf("Couldn't enter passive mode!\n");
        return -1;
    }

    char response[MAX_BUFFER];
    int bytes_received = recv(socket, response, sizeof(response) - 1, 0);
    if (bytes_received <= 0)
    {
        printf("Failed to receive response from server.\n");
        return -1;
    }
    response[bytes_received] = '\0';

    int ip1, ip2, ip3, ip4, port1, port2;
    if (sscanf(response, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)",
               &ip1, &ip2, &ip3, &ip4, &port1, &port2) != 6)
    {
        printf("Failed to parse PASV response.\n");
        return -1;
    }

    char ip[16];
    sprintf(ip, "%d.%d.%d.%d", ip1, ip2, ip3, ip4);
    int port = port1 * 256 + port2;

    int dataSocket = socket_create(ip, port);
    if (dataSocket < 0)
    {
        printf("Failed to create data socket!\n");
        return -1;
    }
    return dataSocket;
}

int parse(char *input, struct URL *url)
{
    char temp[strlen(input) + 1];
    strcpy(temp, input); // Copying to temp to avoid modifying the input

    if (strncmp(temp, "ftp://", 6) == 0)
    {
        // Shift the string left by 6 to remove prefix
        memmove(temp, temp + 6, strlen(temp) - 5);
    }
    else
    {
        printf("Invalid FTP URL\n");
        return -1;
    }

    // Check if user credentials are provided
    char *atSymbol = strchr(temp, '@');
    if (atSymbol != NULL)
    {
        *atSymbol = '\0';
        char *colon = strchr(temp, ':');
        if (colon != NULL)
        {
            *colon = '\0';
            strcpy(url->user, temp);
            strcpy(url->password, colon + 1);
        }
        else
        {
            strcpy(url->user, temp);
            strcpy(url->password, "");
        }
        strcpy(temp, atSymbol + 1); // Get host/resource
    }
    else
    {
        // No credentials provided
    }

    // Extract host/resource
    char *slash = strchr(temp, '/');
    if (slash != NULL)
    {
        // Extract host
        *slash = '\0';
        strcpy(url->host, temp);
        strcpy(url->resource, slash + 1);
    }
    else
    {
        strcpy(url->host, temp);
        strcpy(url->resource, "");
    }

    // Extract file from resource
    char *lastSlash = strrchr(url->resource, '/');
    if (lastSlash != NULL)
    {
        strcpy(url->file, lastSlash + 1);
    }
    else
    {
        strcpy(url->file, url->resource);
    }

    
    // Resolve hostname
    struct hostent *he = gethostbyname(url->host);
    if (he == NULL)
    {
        printf("Couldn't resolve hostname!");
        return -1;
    }

    char *ip = inet_ntoa(*((struct in_addr *)he->h_addr_list[0]));
    strcpy(url->ip, ip);
    
    return 0;
}

int send_ftp_command(int sockfd, const char *command)
{
    char buffer[MAX_BUFFER];
    int n;

    // Send the command to the FTP server
    n = write(sockfd, command, strlen(command));
    if (n < 0)
    {
        perror("Error: Failed to write to socket");
        exit(1);
    }

    // Read the response from the FTP server
    bzero(buffer, MAX_BUFFER);
    n = read(sockfd, buffer, MAX_BUFFER - 1);
    if (n < 0)
    {
        perror("Error: Failed to read from socket");
        exit(1);
    }

    // Extract the response code (first 3 digits)
    int response_code = atoi(buffer); // The first 3 characters should be the response code
    return response_code;
}

int server_login(int socket, char *user, char *password)
{
    // Send USER command
    char user_command[MAX_BUFFER];
    snprintf(user_command, MAX_BUFFER, "USER %s\r\n", user);

    if (send_ftp_command(socket, user_command) != SERVER_SENDPASSWORD)
    {
        printf("Username is not recognized by the server!\n");
        return -1;
    }

    // username was recognized
    char password_command[MAX_BUFFER];
    snprintf(password_command, MAX_BUFFER, "PASS %s\r\n", password);
    if (send_ftp_command(socket, password_command) != SERVER_LOGGED)
    {
        printf("Authentication failed!\n");
        return -1;
    }

    printf("Authentication Successful!\n");
    return 0;
}

int request_resource(int socket, char *resource)
{
    char retrieve_cmd[5 + strlen(resource) + 1];
    sprintf(retrieve_cmd, "retr %s\n", resource);
    if (send_ftp_command(socket, retrieve_cmd) != SERVER_READY)
    {
        printf("Couldn't request the resource");
        return -1;
    }
    return 0;
}

int get_resource(int controlSocket, int dataSocket, char *filename)
{
    FILE *file = fopen(filename, "wb");

    if (file == NULL)
    {
        printf("Error opening or creating file: %s", filename);
        return -1;
    }

    char buffer[MAX_BUFFER];
    int bytes = 1;
    while (bytes > 0)
    {
        bytes = read(dataSocket, buffer, MAX_BUFFER);
        if (fwrite(buffer, bytes, 1, file) < 0)
            return -1;
    }
    fclose(file);

    // Read response
    bzero(buffer, MAX_BUFFER);
    int n = read(controlSocket, buffer, MAX_BUFFER - 1);
    if (n < 0)
    {
        perror("Error: Failed to read from socket");
        exit(1);
    }

    // Extract the response code (first 3 digits)
    int response_code = atoi(buffer); // The first 3 characters should be the response code
    return response_code;
}

int close_conn(int controlSocket, int dataSocket){
    if(send_ftp_command(controlSocket, "quit\n") != SERVER_CLOSING){
        return -1;
    }
    return close(controlSocket) || close(dataSocket);
}

int main(int argc, char *argv[])
{

    if (argc != 2)
    {
        // Invalid arguments
        printf("Usage: ./download ftp://[<user>:<password>@]<host>/<url-path>\n");
        exit(-1);
    }

    struct URL url;
    memset(&url, 0, sizeof(url));
    if (parse(argv[1], &url) < 0)
    {
        // Error on parsing
        exit(-1);
    }

    printf("Host: %s\nResource: %s\nFile: %s\nUser: %s\nPassword: %s\nIP Address: %s\n", url.host, url.resource, url.file, url.user, url.password, url.ip);

    int controlSocket = socket_create(url.ip, FTP_PORT);
    if (controlSocket < 0)
    {
        printf("Socket to '%s' failed!\n", url.ip);
        exit(-1);
    }

    if (server_login(controlSocket, url.user, url.password) < 0)
    {
        exit(-1);
    }

    int dataSocket;
    if ((dataSocket = enter_passive_mode(controlSocket)) < 0)
    {
        exit(-1);
    }

    if (request_resource(controlSocket, url.resource) < 0)
    {
        exit(-1);
    }

    if (get_resource(controlSocket, dataSocket, url.file) != SERVER_CLOSING_ON_SUCCESS)
    {
        printf("Error transfering file '%s' from '%s:%d'\n", url.file, url.ip, FTP_PORT);
        exit(-1);
    }

    if(close_conn(controlSocket, dataSocket) !=0){
        printf("Sockets close error!\n");
        exit(-1);
    }
    return 0;
}