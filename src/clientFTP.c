#include "../include/clientFTP.h"

int socket_create(char *host, int port)
{
    // Socket identification
#ifdef DEBUG
    printf("Creating socket at %s and port %d", host, port);
#endif
    int socket_descriptor;

    if ((socket_descriptor = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    {
        // Error creating socket
        perror("Couldn't create socket!\n");
        return -1;
    }

    // Socket created now connect
    struct sockaddr_in svAddr;

    bzero((char *) &svAddr, sizeof(svAddr));
    svAddr.sin_family = AF_INET;
    svAddr.sin_addr.s_addr = inet_addr(host);
    svAddr.sin_port = htons(port);


#ifdef DEBUG
    printf("Socket address: %x", ip);
    printf("Socket port: %x", port);
#endif

    // Try to establish connection to the host
    if ((connect(socket_descriptor, (struct sockaddr *)&svAddr, sizeof(svAddr))) < 0)
    {
        perror("Couldn't connect to host!\n");
        return -1;
    }

    return socket_descriptor;
}

int enter_passive_mode(int socket)
{
    char buffer[MAX_BUFFER];

    // Send PASV command to control socket
    if (send_ftp_command(socket, "PASV\r\n") != 0)
    {
        perror("Passive mode failed!");
        return -1;
    }

    // Check if server responds with Server passive
    if (read_ftp_response(socket, buffer) != SERVER_PASSIVE)
    {
        perror("Couldn't get passive socket ip!\n");
        return -1;
    }

    char *last = strchr(buffer, ')') + 1;
    *last = '\0';

    int ip1, ip2, ip3, ip4, port1, port2;
    if (sscanf(buffer, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)",
               &ip1, &ip2, &ip3, &ip4, &port1, &port2) != 6)
    {
        printf("Failed to parse PASV response.\n");
        return -1;
    }

    char ip[16];
    sprintf(ip, "%d.%d.%d.%d", ip1, ip2, ip3, ip4);
    int port = port1 * 256 + port2;

#ifdef DEBUG
    printf("Passive socket info:\t IP %s\t PORT %d\n", ip, port);
#endif

    int dataSocket = socket_create(ip, port);
    if (dataSocket < 0)
    {
        printf("Failed to create data socket!\n");
        return -1;
    }

    printf("Entered passive mode successfully!\n");
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
        perror("Invalid FTP URL\n");
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
        strcpy(url->user, DEFAULT_USER);
        strcpy(url->password, DEFAULT_PASSWORD);
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
        perror("Couldn't resolve hostname!\n");
        return -1;
    }

    char *ip = inet_ntoa(*((struct in_addr *)he->h_addr_list[0]));
    strcpy(url->ip, ip);

    return 0;
}

int send_ftp_command(int sockfd, const char *command)
{
    if (command == NULL || strlen(command) == 0)
    {
        printf("Error: Command cannot be empty\n");
        return -1;
    }
#ifdef DEBUG
    printf("SENDING COMMAND: %s\n", command);
#endif

    // Send the command to the FTP server
    int n = write(sockfd, command, strlen(command));
    if (n < 0)
    {
        perror("Error: Failed to write to socket");
        return -1;
    }

    return 0;
}

int read_ftp_response(int sockfd, char *buffer)
{
    // Read byte by byte to check

    char byte;
    int index = 0;
    memset(buffer, 0, MAX_BUFFER);

    // First 4 bytes are for line identification if of type (000 content) -> last line
    // Go line by line

    while (1)
    {
        read(sockfd, &byte, 1);
        buffer[index++] = byte;
        if (byte == '\n')
        {
            // end of line (check if line is of type 000 content)
#ifdef DEBUG

            printf("LINE %s\n", buffer);
#endif
            index = 0;

            if (strchr(buffer, ' ') == &buffer[3])
            {
                break;
            }
            memset(buffer, 0, MAX_BUFFER);
        }
    }

    return get_code_response(buffer);
}

int get_code_response(const char *buffer)
{
    if (buffer == NULL || strlen(buffer) < 3)
    {
        perror("Invalid response from server!");
        return -1;
    }

    char code_str[4];
    strncpy(code_str, buffer, 3);
    code_str[3] = '\0';

    return atoi(code_str);
}

int server_login(int socket, char *user, char *password)
{
    char buffer[MAX_BUFFER];
    if (read_ftp_response(socket, buffer) != SERVER_USER_READY)
    {
        printf("Server is not ready for login. \n");
        return -1;
    }

    // Send USER command
    char user_command[MAX_BUFFER];
    snprintf(user_command, sizeof(user_command), "USER %s\r\n", user);
    if (send_ftp_command(socket, user_command) != 0)
    {
        return -1;
    }

    int response = read_ftp_response(socket, buffer);
    if (response == SERVER_LOGGED)
        return 0;

    if (response != SERVER_SENDPASSWORD)
    {
        printf("Username is not recognized by the server!\n");
        return -1;
    }

    // username was recognized
    char password_command[MAX_BUFFER];
    snprintf(password_command, sizeof(password_command), "PASS %s\r\n", password);
    if (send_ftp_command(socket, password_command) != 0)
    {
        return -1;
    }

    if (read_ftp_response(socket, buffer) != SERVER_LOGGED)
    {
        printf("Authentication failed!\n");
        return -1;
    }

    printf("Authentication Successful!\n");
    return 0;
}

int request_resource(int socket, char *resource)
{
    printf("Requesting resource: %s!\n", resource);

    char buffer[MAX_BUFFER];
    char retrieve_cmd[5 + strlen(resource) + 2];
    sprintf(retrieve_cmd, "retr %s\r\n", resource);

    if (send_ftp_command(socket, retrieve_cmd) != 0)
    {
        printf("Resource retrieval failed!\n");
        return -1;
    }

    int response = read_ftp_response(socket, buffer);
    if (response != SERVER_READY && response != SERVER_CONNECTION_ALREADY_OPEN)
    {
        printf("Server is not ready to begin transfer!\n");
        return -1;
    }

    printf("Resource request done!\n");
    return 0;
}

int get_resource(int controlSocket, int dataSocket, char *filename)
{
    // create in bin folder
    FILE *file = fopen(filename, "wb");

    if (file == NULL)
    {
        printf("Error opening or creating file: %s", filename);
        return -1;
    }

    char buffer[MAX_BUFFER];
    int bytes;

    printf("Transfering file: %s!\n", filename);

    while ((bytes = read(dataSocket, buffer, sizeof(buffer))) > 0)
    {
        if (fwrite(buffer, 1, bytes, file) != bytes)
        {
            printf("Error writing to file");
            fclose(file);
            return -1;
        }
        memset(buffer, 0, sizeof(buffer));
        fflush(file);
    }

    fclose(file);
    if (close(dataSocket) != 0)
    {
        printf("Error closing passive socket!\n");
        return -1;
    }

    if (read_ftp_response(controlSocket, buffer) != SERVER_CLOSING_ON_SUCCESS)
    {
        return -1;
    }

    printf("File retrieval successful!\n");
    return 0;
}

int close_conn(int controlSocket)
{
    char buf[MAX_BUFFER];

    if (send_ftp_command(controlSocket, "QUIT\r\n") < 0)
    {
        perror("Error sending QUIT command");
        return -1;
    }

    if (read_ftp_response(controlSocket, buf) != SERVER_CLOSING)
        return -1;

    return close(controlSocket);
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

#ifdef DEBUG
    printf("Host: %s\nResource: %s\nFile: %s\nUser: %s\nPassword: %s\nIP Address: %s\n", url.host, url.resource, url.file, url.user, url.password, url.ip);
#endif

    int controlSocket = socket_create(url.ip, FTP_PORT);
    if (controlSocket < 0)
    {
        printf("Socket to '%s' failed!\n", url.ip);
        exit(-1);
    }

    if (server_login(controlSocket, url.user, url.password) < 0)
    {
        close(controlSocket);
        exit(-1);
    }

    int dataSocket;
    if ((dataSocket = enter_passive_mode(controlSocket)) < 0)
    {
        printf("Error on passive mode!\n");
        close_conn(controlSocket);
        exit(-1);
    }

    if (request_resource(controlSocket, url.resource) < 0)
    {
        printf("Error on requesting resource!\n");
        close(dataSocket);
        close_conn(controlSocket);
        exit(-1);
    }

    if (get_resource(controlSocket, dataSocket, url.file) != 0)
    {
        printf("Error transfering file '%s' from '%s:%d'\n", url.file, url.ip, FTP_PORT);
        close(dataSocket);
        close_conn(controlSocket);
        exit(-1);
    }

    if (close_conn(controlSocket) != 0)
    {
        printf("Sockets close error!\n");
        exit(-1);
    }
    
    return 0;
}