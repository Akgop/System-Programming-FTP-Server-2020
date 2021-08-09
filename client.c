//
// Created by Akgop on 2021/08/06.
//
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>

/// Buffer Sizes
#define CTRL_BUFF_SIZE 128      // control connection port number
#define DATA_BUFF_SIZE 1024   // for receive buffer from server
#define DATA_PORT_NUM 5000     // data connection port number
#define PROPER_ARG_SIZE 3
// TODO define MAX BUFFER SIZE
#define MAX_BUFF_SIZE 9999    // packet size?

/// HTTP Status Code
#define ACCEPTED "220"
#define REJECTED "530"

void clientAddressToString(char* a_ip_port, unsigned long a_ip_address, unsigned int a_port_number)
{
    unsigned int s_ip1, s_ip2, s_ip3, s_ip4;
    unsigned int s_port1, s_port2;

    // convert big Endian to little endian ip address with comma
    s_ip1 = a_ip_address & 0x000000ff;
    s_ip2 = (a_ip_address & 0x0000ff00) >> 8;
    s_ip3 = (a_ip_address & 0x00ff0000) >> 16;
    s_ip4 = (a_ip_address & 0xff000000) >> 24;
    sprintf(a_ip_port, "%d,%d,%d,%d", s_ip1, s_ip2, s_ip3, s_ip4);

    // convert port number with comma
    s_port1 = a_port_number & 0x00ff;
    s_port2 = (a_port_number & 0xff00) >> 8;
    sprintf(a_ip_port, "%s,%d,%d", a_ip_port, s_port1, s_port2);
}

int clientIsIPAcceptable(char* msg) {
    if(strncmp(msg, ACCEPTED, 3) == 0) {
        write(STDOUT_FILENO, msg, strlen(msg));
        return 1;
    }
    write(STDOUT_FILENO, msg, strlen(msg));
    return 0;
}

/// main function
// Purpose	: FTP client driver
int main(int argc, char * argv[]) {
    struct sockaddr_in cli_addr;
    char ip_address_and_port_number[DATA_BUFF_SIZE];
    char cli_cmd_buf[CTRL_BUFF_SIZE];
    char user_input_buf[CTRL_BUFF_SIZE];
    char ctrl_receive_buf[CTRL_BUFF_SIZE];
    int cli_socket_fd;
    long input_length;

    if (argc != PROPER_ARG_SIZE) {
        write(STDERR_FILENO, "Client: arguments size has to be 3\n", strlen("Client: arguments size has to be 3\n"));
        exit(1);
    }

    /// 0. control connection setup: Socket
    // create socket
    if ((cli_socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
        write(STDERR_FILENO, "Client: Can't open stream socket\n", strlen("Client: Can't open stream socket\n"));
        exit(1);
    }
    memset(&cli_addr, 0, sizeof(cli_addr));
    cli_addr.sin_family = AF_INET;    // set client info
    cli_addr.sin_addr.s_addr = inet_addr(argv[1]); // IP ADDRESS
    cli_addr.sin_port = htons((int)strtol(argv[2], (char **)NULL, 10)); // PORT NUMBER

    /// 1. client: Connect -> server: Accept
    if (connect(cli_socket_fd, (struct sockaddr *)&cli_addr, sizeof(cli_addr))){
        write(STDERR_FILENO, "Client: connection failed\n", strlen("Client: connection failed\n"));
        exit(1);
    }
    /// 4. receive response
    memset(ctrl_receive_buf, 0, sizeof(ctrl_receive_buf));
    if (read(cli_socket_fd, ctrl_receive_buf, CTRL_BUFF_SIZE) < 0 ){
        write(STDERR_FILENO, "Client: Connection Failed\n", strlen("Client: Connection Failed\n"));
        exit(1);
    }
    if (!clientIsIPAcceptable(ctrl_receive_buf)) {
        write(STDERR_FILENO, "Client: IP is not Acceptable\n", strlen("Client: IP is not Acceptable\n"));
        exit(1);
    }

    /// data connection setup
//    clientAddressToString(ip_address_and_port_number, cli_addr.sin_addr.s_addr, DATA_PORT_NUM);
//    write(STDOUT_FILENO, "\n", strlen("\n"));

    /// Authentication
    /// whether IP ADDRESS acceptable

    /// login to server - MAX 3 TIMES

    /// Command Process
    while(1) {
        memset(cli_cmd_buf, 0, sizeof(cli_cmd_buf));
        memset(ctrl_receive_buf, 0, sizeof(ctrl_receive_buf));
        memset(user_input_buf, 0, sizeof(user_input_buf));

        /// 5. read user command
        write(STDOUT_FILENO, "ftp> ", strlen("ftp> "));
        input_length = read(STDIN_FILENO, user_input_buf, CTRL_BUFF_SIZE);
        user_input_buf[input_length] = '\0';

        /// 5.1 convert USER COMMAND to FTP COMMAND

        /// 5.2 send command
        strcpy(cli_cmd_buf, user_input_buf);
        write(STDOUT_FILENO, "---> ", strlen("---> "));
        write(STDOUT_FILENO, cli_cmd_buf, strlen(cli_cmd_buf));
        write(STDOUT_FILENO, "\n", strlen("\n"));
        write(cli_socket_fd, cli_cmd_buf, CTRL_BUFF_SIZE);

        /// 8. receive response
        read(cli_socket_fd, ctrl_receive_buf, CTRL_BUFF_SIZE);
        write(STDOUT_FILENO, ctrl_receive_buf, strlen(ctrl_receive_buf));
        if (!strncmp(ctrl_receive_buf, "221", 3)) {
            break;
        }
    }
    close(cli_socket_fd);
}
