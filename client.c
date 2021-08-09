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

#define PROPER_ARG_SIZE 3
#define COMMAND_OPTION_SIZE 64


/// HTTP Status Code
#define ACCEPTED "220"
#define QUIT "221"
#define REJECTED "530"


// TODO Optimize Convert Function
int clientConvertCommand(char * a_user_cmd, char * a_ftp_cmd) {
    int opt_count = 0;
    char * opt_value[COMMAND_OPTION_SIZE];
    opt_value[opt_count] = strtok(a_user_cmd, " \n");
    while(opt_value[opt_count]) {
        opt_count++;
        opt_value[opt_count] = strtok(NULL, " \n");
    }
    if (opt_count < 1) return -1;
    else if(!strcmp(opt_value[0], "pwd")) {
        int opt, idx;
        opterr = 0;
        optind = 1;
        if(opt_count > 1) return -1;
        while ((opt = getopt(opt_count, opt_value, "")) != -1) {
            if (opt == '?') return -1;
            else            break;
        }
        strcpy(a_ftp_cmd, "PWD");
        for(idx = optind; idx < opt_count; idx++) {
            strcat(a_ftp_cmd, " ");
            strcat(a_ftp_cmd, opt_value[idx]);
        }
    }
    else if(!strcmp(opt_value[0], "quit")) {
        strcat(a_ftp_cmd, "QUIT");
    }
    return 0;
}

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

/// Client-Side Create Socket
// return client socket file descriptor
int clientCreateSocket(struct sockaddr_in * a_cli_addr, char ** a_argv) {
    int s_cli_socket_fd;
    if ((s_cli_socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
        write(STDERR_FILENO, "Client: Can't open stream socket\n", strlen("Client: Can't open stream socket\n"));
        exit(1);
    }
    a_cli_addr->sin_family = AF_INET;    // set client info
    a_cli_addr->sin_addr.s_addr = inet_addr(a_argv[1]); // IP ADDRESS
    a_cli_addr->sin_port = htons((int)strtol(a_argv[2], (char **)NULL, 10)); // PORT NUMBER
    return s_cli_socket_fd;
}

/// Send Control Connection to Server
void clientCreateConnection(int a_cli_socket_fd, struct sockaddr_in a_cli_addr) {
    if (connect(a_cli_socket_fd, (struct sockaddr *)&a_cli_addr, sizeof(a_cli_addr))){
        write(STDERR_FILENO, "Client: connection failed\n", strlen("Client: connection failed\n"));
        exit(1);
    }
}

/// Send Control Commands to Server
void clientSendCommand(int a_cli_socket_fd) {
    // Get user input
    char s_user_input_buf[CTRL_BUFF_SIZE] = {0, };
    char s_cli_cmd_buf[CTRL_BUFF_SIZE] = {0, };
    ssize_t input_len;
    while(1){
        memset(s_user_input_buf, 0, CTRL_BUFF_SIZE);
        memset(s_cli_cmd_buf, 0, CTRL_BUFF_SIZE);
        write(STDOUT_FILENO, "ftp> ", strlen("ftp> "));
        input_len = read(STDIN_FILENO, s_user_input_buf, CTRL_BUFF_SIZE);
        s_user_input_buf[input_len] = '\0';

        // Convert User Command to FTP Command
        if (!clientConvertCommand(s_user_input_buf, s_cli_cmd_buf)) {
            break;
        }
        printf("Client: illegal command\n\n");
    }

    printf("---> %s\n\n", s_cli_cmd_buf);
    write(a_cli_socket_fd, s_cli_cmd_buf, CTRL_BUFF_SIZE);
}


/// main function
int main(int argc, char * argv[]) {
    struct sockaddr_in cli_addr;
    char ip_address_and_port_number[DATA_BUFF_SIZE];
    char ctrl_receive_buf[CTRL_BUFF_SIZE];
    int cli_socket_fd;
    long input_length;

    if (argc != PROPER_ARG_SIZE) {
        write(STDERR_FILENO, "Client: arguments size has to be 3\n", strlen("Client: arguments size has to be 3\n"));
        exit(1);
    }

    /// 0. control connection setup: Socket
    memset(&cli_addr, 0, sizeof(cli_addr));
    cli_socket_fd = clientCreateSocket(&cli_addr, argv);

    /// 1. client: Connect -> server: Accept
    clientCreateConnection(cli_socket_fd, cli_addr);

    /// data connection pre-processing
    //    clientAddressToString(ip_address_and_port_number, cli_addr.sin_addr.s_addr, DATA_PORT_NUM);
    //    write(STDOUT_FILENO, "\n", strlen("\n"));

    /// Authentication
    /// whether IP ADDRESS acceptable
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

    /// login to server - MAX 3 TIMES

    /// Command Process
    while(1) {
        memset(ctrl_receive_buf, 0, sizeof(ctrl_receive_buf));

        /// 5. send Command
        clientSendCommand(cli_socket_fd);

        /// 8. receive response
        read(cli_socket_fd, ctrl_receive_buf, CTRL_BUFF_SIZE);
        write(STDOUT_FILENO, ctrl_receive_buf, strlen(ctrl_receive_buf));
        if (!strncmp(ctrl_receive_buf, QUIT, 3)) {
            break;
        }
    }
    close(cli_socket_fd);
}
