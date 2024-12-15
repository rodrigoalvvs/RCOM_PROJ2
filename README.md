# FTP Client Project

## Overview
This project is a simple FTP client implemented in C, allowing users to download files from FTP servers using the command-line interface. It supports standard FTP features such as:

- Connecting to an FTP server.
- Logging in with or without credentials.
- Navigating to passive mode.
- Downloading files from the server.

The program is structured around creating and managing socket connections, sending FTP commands, and handling server responses.

## Features
1. **Connect to an FTP Server**: Establish a socket connection to a given FTP server.
2. **Passive Mode Support**: Enables data transfer using passive mode.
3. **User Authentication**: Handles both anonymous and authenticated access.
4. **File Transfer**: Downloads files from the server and saves them locally.
5. **Error Handling**: Validates user input, server responses, and socket connections.

## Prerequisites
- A C compiler (e.g., GCC)
- A Unix-based environment (Linux/MacOS) for running the code
- A basic understanding of FTP protocols

## File Structure
```
project/
├── src/
│   ├── clientFTP.c        # Main implementation file
├── include/
│   ├── clientFTP.h        # Header file for declarations
├── bin/                   # Directory for compiled binaries
├── Makefile               # Build instructions
└── README.md              # Project documentation
```

## Usage

### Compilation
This project uses a `Makefile` for compilation. To compile the code, simply run:

```bash
make
```
This will compile the source code and create an executable named `download` in the `bin/` directory.

### Running the Program
Use the following syntax to run the program:

```bash
./bin/download ftp://[<user>:<password>@]<host>/<url-path>
```

Examples:

- Anonymous login:
  ```bash
  ./bin/download ftp://ftp.example.com/file.txt
  ```
- Authenticated login:
  ```bash
  ./bin/download ftp://username:password@ftp.example.com/file.txt
  ```

### Debug Mode
Enable debug mode to see detailed logs by appending `DEBUG=1` during compilation:

```bash
make DEBUG=1
```

### Cleaning Up
To remove compiled binaries, run:

```bash
make clean
```

## Key Functions

### **`socket_create`**
Establishes a socket connection to the FTP server. Handles socket creation, address resolution, and connection.

### **`parse`**
Parses the input FTP URL, extracting components such as host, resource path, and user credentials. Resolves the hostname to an IP address.

### **`server_login`**
Handles user authentication by sending `USER` and `PASS` commands to the server.

### **`enter_passive_mode`**
Switches the server to passive mode and establishes a data socket for file transfer.

### **`request_resource`**
Sends a command to the server to request the desired file for download.

### **`get_resource`**
Receives the file data over the data socket and writes it to the local filesystem.

### **`close_conn`**
Closes the control socket and gracefully ends the session with the server.

## Error Handling
- **Socket Creation**: Ensures that the socket is successfully created and connected.
- **FTP Commands**: Validates the server's response to ensure commands are executed correctly.
- **File Transfer**: Checks for write errors during file transfer.
- **URL Parsing**: Ensures the input URL conforms to the expected FTP format.

## Debugging Tips
- Use the debug mode to see detailed logs for each step.
- Verify the FTP URL format before running the program.
- Ensure the FTP server allows passive mode and file downloads.

## Limitations
- The client only supports passive mode for data transfer.
- Does not support advanced FTP commands like directory navigation (`CWD`) or file listing (`LIST`).
- Assumes the server supports the `PASV` command.

## Future Improvements
- Add support for active mode connections.
- Implement additional FTP commands such as `LIST` and `CWD`.
- Enhance error messages for better debugging.
