//
// Created by David Nam on 2021/08/07.
//

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define CTRL_BUFF_SIZE 128
#define DATA_BUFF_SIZE 1024
#define PROPER_ARG_SIZE 2

/// TODO modify client_info function
int serverClientInfo(struct sockaddr_in * a_client_address) {
    char * ip_address = inet_ntoa(a_client_address->sin_addr);
    char port_number[5] = {0, };

    sprintf(port_number, "%d", a_client_address->sin_port);
    write(STDOUT_FILENO, "==========Client info==========\n", 33);
    write(STDOUT_FILENO, "client IP: ", 12);
    write(STDOUT_FILENO, ip_address, strlen(ip_address));

    write(STDOUT_FILENO, "\n\nclient port: ", 16);
    write(STDOUT_FILENO, port_number,  strlen(port_number));
    write(STDOUT_FILENO, "\n===============================\n", 34);
    return 0;
}

void serverQuit(int a_connection_fd) {
    char s_message_buf[CTRL_BUFF_SIZE];
    memset(s_message_buf, 0, sizeof(s_message_buf));
    write(a_connection_fd, s_message_buf, CTRL_BUFF_SIZE);
    close(a_connection_fd);
}

int main(int argc, char *argv[]) {
    int socket_fd, connection_fd;
    long command_length;
    unsigned int client_address_length;
    struct sockaddr_in server_address, client_address;
    char command_buf[DATA_BUFF_SIZE];
    FILE *fp_checkIP;

    // arguments size has to be 2
    if(argc != PROPER_ARG_SIZE) {
        write(STDERR_FILENO, "Server: arguments size has to be 2\n", strlen("Server: arguments size has to be 2\n"));
        exit(1);
    }

    /// Control Connection Setup - Socket -> Bind -> Listen
    if ((socket_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        write(STDERR_FILENO, "Server: Can't open stream socket\n", strlen("Server: Can't open stream socket\n"));
        exit(1);
    }

    // socket
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons((int)strtol(argv[1], (char **)NULL, 10));

    // bind
    if(bind(socket_fd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        write(STDERR_FILENO, "Server: Cant bind local address.\n", strlen("Server: Cant bind local address.\n"));
        exit(1);
    }

    // listen: MAX concurrent client
    listen(socket_fd, 1);

    /// loop - COMMANDS
    while(1) {
        client_address_length = sizeof(client_address);

        /// client: Connect -> server: Accept
        connection_fd = accept(socket_fd, (struct sockaddr *)&client_address, &client_address_length);
        if(connection_fd < 0) {
            write(STDERR_FILENO, "Server: accept() failed.\n", strlen("Server: accept() failed.\n"));
            exit(1);
        }
        if(serverClientInfo(&client_address) < 0){
            write(STDERR_FILENO, "Server: client_info() err!!\n", strlen("Server: client_info() err!!\n"));
        }
        /// Multi-Threaded

        /// Authentication: IP Address
        // success
        write(STDOUT_FILENO, "Server: ** Client is connected **\n", strlen("Server: ** Client is connected **\n"));
        write(connection_fd, "220 Akgop.github.io Connected\n", strlen("220 Akgop.github.io Connected\n"));

        /// Command Process
        while(1) {
            memset(command_buf, 0, sizeof(command_buf));

            // read command
            if (read(connection_fd, command_buf, CTRL_BUFF_SIZE) < 0) {
                write(STDERR_FILENO, "Server: read() error\n", strlen("Server: read() error\n"));
                continue;
            }

            /// close Connections
            if (!strncmp(command_buf, "QUIT", strlen("QUIT"))) {
                write(STDOUT_FILENO, "Server: QUIT\n", strlen("Server: QUIT\n"));
                serverQuit(connection_fd);
                exit(0);
            }
            else {
                write(STDOUT_FILENO, command_buf, CTRL_BUFF_SIZE);
            }
        }
    }
}