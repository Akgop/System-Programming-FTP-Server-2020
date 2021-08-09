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

int serverClientInfo(struct sockaddr_in * a_client_address) {
    char * ip_address = inet_ntoa(a_client_address->sin_addr);
    char port_number[5] = {0, };

    sprintf(port_number, "%d", a_client_address->sin_port);
    write(STDOUT_FILENO, "client IP: ", strlen("client IP: "));
    write(STDOUT_FILENO, ip_address, strlen(ip_address));

    write(STDOUT_FILENO, "\nclient port: ", strlen("\nclient port: "));
    write(STDOUT_FILENO, port_number,  strlen(port_number));
    printf("\n");
    return 0;
}

void serverQuit(int a_connection_fd) {
    write(a_connection_fd, "221 Goodbye.\n", CTRL_BUFF_SIZE);
    close(a_connection_fd);
}

int main(int argc, char *argv[]) {
    int serv_socket_fd, cli_conn_fd;
    socklen_t cli_addr_size;
    ssize_t recv_cmd_len;
    struct sockaddr_in server_address, client_address;
    char receive_cmd_buf[DATA_BUFF_SIZE];

    // arguments size has to be 2
    if(argc != PROPER_ARG_SIZE) {
        write(STDERR_FILENO, "Server: arguments size has to be 2\n", strlen("Server: arguments size has to be 2\n"));
        exit(1);
    }

    /// 0. Control Connection Setup - Socket -> Bind -> Listen
    if ((serv_socket_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        write(STDERR_FILENO, "Server: Can't open stream socket\n", strlen("Server: Can't open stream socket\n"));
        exit(1);
    }

    // 0.1 socket
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons((int)strtol(argv[1], (char **)NULL, 10));

    // 0.2 bind
    if(bind(serv_socket_fd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        write(STDERR_FILENO, "Server: Cant bind local address.\n", strlen("Server: Cant bind local address.\n"));
        exit(1);
    }

    // 0.3 listen: MAX concurrent client
    listen(serv_socket_fd, 1);

    write(STDOUT_FILENO, "Server: Socket Created, Listening...\n", strlen("Server: Socket Created, Listening...\n"));

    /// loop - COMMANDS
    while(1) {
        cli_addr_size = sizeof(client_address);

        /// 2. client: Connect -> server: Accept
        cli_conn_fd = accept(serv_socket_fd, (struct sockaddr *)&client_address, &cli_addr_size);
        if(cli_conn_fd < 0) {
            write(STDERR_FILENO, "Server: accept() failed.\n", strlen("Server: accept() failed.\n"));
            exit(1);
        }
//        if(serverClientInfo(&client_address) < 0){
//            write(STDERR_FILENO, "Server: client_info() err!!\n", strlen("Server: client_info() err!!\n"));
//        }
        /// Multi-Threaded

        /// 3. Authentication: IP Address
        // success
        write(STDOUT_FILENO, "Server: ** Client is connected **\n", strlen("Server: ** Client is connected **\n"));
        write(cli_conn_fd, "220 Akgop.github.io Connected\n", strlen("220 Akgop.github.io Connected\n"));

        /// Command Process
        while(1) {
            memset(receive_cmd_buf, 0, sizeof(receive_cmd_buf));

            /// 6. read command
            if (read(cli_conn_fd, receive_cmd_buf, CTRL_BUFF_SIZE) < 0) {
                write(STDERR_FILENO, "Server: read() error\n", strlen("Server: read() error\n"));
                break;
            }
            //receive_cmd_buf[recv_cmd_len] = '\0';

            /// 7. close Connections
            if (!strncmp(receive_cmd_buf, "QUIT", strlen("QUIT"))) {
                write(STDOUT_FILENO, "Server: QUIT\n", strlen("Server: QUIT\n"));
                serverQuit(cli_conn_fd);
                exit(0);
            }
            else {
                write(STDOUT_FILENO, receive_cmd_buf, CTRL_BUFF_SIZE);
            }
        }

    }
}