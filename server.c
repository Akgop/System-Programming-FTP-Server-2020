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


/// Server-side PWD
// TODO Optimize serverPWD
int serverPWD(int a_cli_conn_fd)
{
    char result[CTRL_BUFF_SIZE] = {0, };
    char msg_buff[CTRL_BUFF_SIZE] = {0, };
    if(getcwd(result, CTRL_BUFF_SIZE)){	//if no error on getcwd
        sprintf(msg_buff, "257 %s is current directory.\n", result);
        write(a_cli_conn_fd, msg_buff, CTRL_BUFF_SIZE);	//send msg to client
        return 0;
    }
    else return -1;
}

/// Server-side Print Client Information: IP & Port
int serverPrintClientInfo(struct sockaddr_in * a_cli_addr) {
    char * ip_address = inet_ntoa(a_cli_addr->sin_addr);
    in_port_t port_number = ntohs(a_cli_addr->sin_port);

    printf("%s\n", "Server: ** Client is connected **");
    printf("%s\n", "==== Client Info ====");
    printf("%s: %s\n", "IP Address", ip_address);
    printf("%s: %d\n", "Port Number", port_number);
    printf("%s\n\n", "=====================");

    return 0;
}

/// Server-side QUIT Connection
void serverQuit(int a_connection_fd) {
    write(a_connection_fd, "221 Goodbye.\n", CTRL_BUFF_SIZE);
    close(a_connection_fd);
}

/// Server-Side Create Socket: Socket -> Bind -> Listen
int serverCreateSocket(struct sockaddr_in * a_serv_addr, char ** a_argv) {
    int s_serv_socket_fd;
    if ((s_serv_socket_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        write(STDERR_FILENO, "Server: Can't open stream socket\n", strlen("Server: Can't open stream socket\n"));
        exit(1);
    }
    a_serv_addr->sin_family = AF_INET;
    a_serv_addr->sin_addr.s_addr = htonl(INADDR_ANY);
    a_serv_addr->sin_port = htons((int)strtol(a_argv[1], (char **)NULL, 10));

    if(bind(s_serv_socket_fd, (struct sockaddr *)a_serv_addr, sizeof(*a_serv_addr)) < 0) {
        write(STDERR_FILENO, "Server: Cant bind local address.\n", strlen("Server: Cant bind local address.\n"));
        exit(1);
    }

    listen(s_serv_socket_fd, 1);
    write(STDOUT_FILENO, "Server: Socket Created, Listening...\n", strlen("Server: Socket Created, Listening...\n"));
    return s_serv_socket_fd;
}

/// Server-Side Accept Control Connection
int serverAcceptConnection(int a_serv_socket_fd, struct sockaddr_in * a_cli_addr) {
    socklen_t s_cli_addr_size;
    int s_cli_conn_fd;
    s_cli_addr_size = sizeof(*a_cli_addr);
    s_cli_conn_fd = accept(a_serv_socket_fd, (struct sockaddr *)a_cli_addr, &s_cli_addr_size);
    if(s_cli_conn_fd < 0) {
        write(STDERR_FILENO, "Server: accept() failed.\n", strlen("Server: accept() failed.\n"));
        exit(1);
    }
    serverPrintClientInfo(a_cli_addr);
    return s_cli_conn_fd;
}

void serverReceiveCommand(int a_cli_conn_fd, char * a_recv_cmd_buf) {
    if (read(a_cli_conn_fd, a_recv_cmd_buf, CTRL_BUFF_SIZE) < 0) {
        write(STDERR_FILENO, "Server: read() error\n", strlen("Server: read() error\n"));
        exit(1);
    }
}

/// main function
int main(int argc, char *argv[]) {
    int serv_socket_fd, cli_conn_fd;
    ssize_t recv_cmd_len;
    struct sockaddr_in serv_addr, cli_addr;
    char recv_cmd_buf[DATA_BUFF_SIZE];

    // arguments size has to be 2
    if(argc != PROPER_ARG_SIZE) {
        write(STDERR_FILENO, "Server: arguments size has to be 2\n", strlen("Server: arguments size has to be 2\n"));
        exit(1);
    }

    /// 0. Control Connection Setup - Socket -> Bind -> Listen
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_socket_fd = serverCreateSocket(&serv_addr, argv);

    /// loop - COMMANDS
    while(1) {
        /// 2. client: Connect -> server: Accept
        cli_conn_fd = serverAcceptConnection(serv_socket_fd, &cli_addr);

        /// Multi-Threaded

        /// 3. Authentication: IP Address
        write(cli_conn_fd, "220 Akgop.github.io Connected\n", strlen("220 Akgop.github.io Connected\n"));

        /// Command Process
        // TODO optimze command process
        while(1) {
            /// 6. read command
            memset(recv_cmd_buf, 0, sizeof(recv_cmd_buf));
            serverReceiveCommand(cli_conn_fd, recv_cmd_buf);

            /// 7. close Connections
            if (!strncmp(recv_cmd_buf, "QUIT", strlen("QUIT"))) {
                write(STDOUT_FILENO, "Server: QUIT\n", strlen("Server: QUIT\n"));
                serverQuit(cli_conn_fd);
                exit(0);
            }
            else if (!strncmp(recv_cmd_buf, "PWD", strlen("PWD"))) {
                serverPWD(cli_conn_fd);
            }
            else {
                write(STDOUT_FILENO, recv_cmd_buf, strlen(recv_cmd_buf));
                write(cli_conn_fd, recv_cmd_buf, CTRL_BUFF_SIZE);
            }
        }

    }
}