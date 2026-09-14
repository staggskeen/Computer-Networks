/*
 Echo Client: Sends a line of text to the server and prints the echoed response.
 Usage: echo <IP Address> <Port Number>
 Written by Keenan Staggs
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>

/*
Write a number of bytes equal to length from the buffer to the socket. Returns the total number of bytes written, or -1 on error.
*/
ssize_t writen(int client_socket, const char *buffer, size_t length) {
    ssize_t total_written = 0;
    while (total_written < length) {                                                                    // loop until all bytes are written
        ssize_t bytes_written = write(client_socket, buffer + total_written, length - total_written);
        if (bytes_written < 0) {
            if (errno == EINTR) {
                continue; // Interrupted, try again
            }
            else {
                return -1; // Error occurred
            }
        }
        total_written += bytes_written;
    }
    return total_written;
}

/*
Read a line of text from the socket into the buffer. Returns the number of bytes read, or -1 on error.
*/
ssize_t readline(int client_socket, char *buffer, size_t max_length) {
    ssize_t total_read = 0;
    while (total_read < max_length - 1) {                        // Leave space for null terminator '\0' to avoid buffer overrun
        unsigned char c;
        ssize_t bytes_read = read(client_socket, &c, 1);
        if (bytes_read < 0) {
            if (errno == EINTR) {
                continue; // Interrupted, try again
            }
            else {
                return -1; // Error occurred
            }
        }
        else if (bytes_read == 0) {
            break; // EOF
        }
        buffer[total_read++] = c;         // store the character (including '\n', the line delimiter, when c == '\n')
        if (c == '\n') {
            break; // Newline found
        }
    }
    buffer[total_read] = '\0'; // Null-terminate the string
    return total_read;
}

int main(int argc, char *argv[]) {  // argv[1] the IP string, argv[2] the port string
    
    int client_socket;
    char buffer[BUFSIZ];
    struct sockaddr_in server_addr;

    // Initialize server address structure
    server_addr.sin_port = htons(atoi(argv[2]));            // convert port number from string to integer and then to network byte order
    server_addr.sin_family = AF_INET;
    if((inet_aton(argv[1], &server_addr.sin_addr) == 0)){   // convert IP address from string to binary form and check for validity
        fprintf(stderr, "Invalid IP address\n");
        return -1;
    }

    // Check for correct number of command line arguments
    if (argc != 3) {   
        fprintf(stderr, "Usage: echo <IP Address> <Port Number>\n");
        return -1;
    }

    // Create socket
    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        return -1;
    }

    // Connect to the server
    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        return -1;
    }

    // Main loop: read from stdin, send to server, read echo from server, print to stdout
    while (1){
        if(fgets(buffer, sizeof(buffer), stdin) == NULL){ // check for EOF
            break;
        }
        else{
            // write to the socket
            if (writen(client_socket, buffer, strlen(buffer)) < 0) { // strlen(buffer) so only relevant bytes are sent, not the entire buffer 
                perror("Write to socket failed");
                return -1;
            }
            // read echo from the socket
            ssize_t n = readline(client_socket, buffer, sizeof(buffer)); // sizeof(buffer) to ensure we don't read more than the buffer can hold
            if (n < 0) {
                perror("Read from socket failed");
                return -1; // error occurred
            }
            else if (n == 0) {
                fprintf(stderr, "Server closed the connection\n");
                break;
            }
            else {
            // print the echo to stdout
            fputs(buffer, stdout);
            }
        }
    }
    close(client_socket); // close the socket due to EOF or server closing the connection
    return 0;
}