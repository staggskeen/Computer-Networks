/*
 Echo Server: Accepts TCP client connections and echoes received text.
 Usage: echos <Port Number>
 Written by Dymond Allen
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <netinet/in.h>

/*
 Write exactly length bytes from buffer to the socket.
 Returns the total number of bytes written, or -1 on error.
*/
ssize_t writen(int client_socket, const char *buffer, size_t length) {
    ssize_t total_written = 0;

    while (total_written < (ssize_t)length) {
        ssize_t bytes_written = write(client_socket,
                                      buffer + total_written,
                                      length - total_written);

        if (bytes_written < 0) {
            if (errno == EINTR) {
                continue;
            }

            return -1;
        }

        total_written += bytes_written;
    }

    return total_written;
}

/*
 Read one line from the socket.
 Stops at a newline, EOF, or when the buffer is full.
*/
ssize_t readline(int client_socket, char *buffer, size_t max_length) {
    ssize_t total_read = 0;

    while (total_read < (ssize_t)max_length - 1) {
        unsigned char c;

        ssize_t bytes_read = read(client_socket, &c, 1);

        if (bytes_read < 0) {
            if (errno == EINTR) {
                continue;
            }

            return -1;
        }

        if (bytes_read == 0) {
            break;
        }

        buffer[total_read++] = c;

        if (c == '\n') {
            break;
        }
    }

    buffer[total_read] = '\0';

    return total_read;
}

int main(int argc, char *argv[]) {

    int server_socket;
    int port;
    struct sockaddr_in server_addr;

    // Check for correct number of command line arguments
    if (argc != 2) {
        fprintf(stderr, "Usage: echos <Port Number>\n");
        return -1;
    }

    // Convert port number from string to integer
    port = atoi(argv[1]);

    if (port <= 0 || port > 65535) {
        fprintf(stderr, "Invalid port number\n");
        return -1;
    }

    // Create TCP socket
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        return -1;
    }

    // Initialize server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);

    // Bind socket to the requested port
    if (bind(server_socket,
             (struct sockaddr *)&server_addr,
             sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_socket);
        return -1;
    }

    // Listen for incoming client connections
    if (listen(server_socket, 10) < 0) {
        perror("Listen failed");
        close(server_socket);
        return -1;
    }

    printf("Server listening on port %d\n", port);

    while (1) {
        int client_socket;
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        // Wait for a client to connect
        client_socket = accept(server_socket,
                               (struct sockaddr *)&client_addr,
                               &client_len);

        if (client_socket < 0) {
            if (errno == EINTR) {
                continue;
            }

            perror("Accept failed");
            continue;
        }

        // Create a child process to handle this client
        pid_t pid = fork();

        if (pid < 0) {
            perror("Fork failed");
            close(client_socket);
            continue;
        }

        if (pid == 0) {
            // CHILD PROCESS

            // Child does not need the listening socket
            close(server_socket);

            printf("Client connected\n");

            char buffer[BUFSIZ];

            while (1) {
                // Read a line sent by the client
                ssize_t n = readline(client_socket,
                                     buffer,
                                     sizeof(buffer));

                if (n < 0) {
                    perror("Read from client failed");
                    break;
                }

                // A return value of 0 means the client reached EOF
                if (n == 0) {
                    printf("Client disconnected\n");
                    break;
                }

                // Send exactly the same data back to the client
                if (writen(client_socket, buffer, n) < 0) {
                    perror("Write to client failed");
                    break;
                }
            }

            close(client_socket);
            exit(0);
        }
        else {
            // PARENT PROCESS

            // Parent does not communicate with this client
            close(client_socket);
        }
    }

    close(server_socket);

    return 0;
}
