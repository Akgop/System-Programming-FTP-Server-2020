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
#include <sys/stat.h>
#include <stdbool.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>

#define CTRL_BUFF_SIZE 128
#define DATA_BUFF_SIZE 1024
#define PROPER_ARG_SIZE 2

/// TODO optimize NLST
int serverNLST(int a_opt_count, char* a_opt_value[], char* a_data_buf) {
    strcpy(a_data_buf, "NLST SUCCESSFULLY DONE\n");
    return 0;
}

/// TODO optimize CWD
int serverCWD(int a_cli_conn_fd, int a_opt_count, char ** a_opt_value) {
    char s_ctrl_res_buf[CTRL_BUFF_SIZE];
    memset(s_ctrl_res_buf, 0, sizeof(s_ctrl_res_buf));
    if(chdir(a_opt_value[1]) == -1){	//change directory
        sprintf(s_ctrl_res_buf, "550 %s: Can’t find such file or directory.\n", a_opt_value[1]);
        write(a_cli_conn_fd, s_ctrl_res_buf, CTRL_BUFF_SIZE);	//send msg to client
        return -1;
    }
    else{	//success
        strcpy(s_ctrl_res_buf, "250 CWD command succeeds.\n");
        write(a_cli_conn_fd, s_ctrl_res_buf, CTRL_BUFF_SIZE);	//send msg to client
        return 0;
    }
}

/// TODO optimize CDUP
int serverCDUP(int a_cli_conn_fd) {
    char s_ctrl_res_buf[CTRL_BUFF_SIZE];
    memset(s_ctrl_res_buf, 0, sizeof(s_ctrl_res_buf));
    if(chdir("..") == -1){	//change directory with ".."
        sprintf(s_ctrl_res_buf, "550 '..': Can’t find such file or directory.\n");
        write(a_cli_conn_fd, s_ctrl_res_buf, CTRL_BUFF_SIZE);	//send msg to client
        return -1;
    }
    else{
        strcpy(s_ctrl_res_buf, "250 CDUP command succeeds.\n");
        write(a_cli_conn_fd, s_ctrl_res_buf, CTRL_BUFF_SIZE);	//send msg to client
    }
    return 0;
}

/// Server-side PWD
int serverPWD(int a_cli_conn_fd)
{
    char s_pwd_result[CTRL_BUFF_SIZE] = {0, };
    char s_ctrl_res_buf[CTRL_BUFF_SIZE] = {0, };
    if(getcwd(s_pwd_result, CTRL_BUFF_SIZE)){	//if no error on getcwd
        sprintf(s_ctrl_res_buf, "257 %s is current directory.\n", s_pwd_result);
        write(a_cli_conn_fd, s_ctrl_res_buf, CTRL_BUFF_SIZE);	//send msg to client
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

/// Server-side Response Command from Client
void serverReceiveCommand(int a_cli_conn_fd, char * a_recv_cmd_buf) {
    if (read(a_cli_conn_fd, a_recv_cmd_buf, CTRL_BUFF_SIZE) < 0) {
        write(STDERR_FILENO, "Server: read() error\n", strlen("Server: read() error\n"));
        exit(1);
    }
}

/// Server-side Parse Client's Data Port & IP Address
void serverParsingIpPort(char * a_cmd, char * a_ip, unsigned int * a_port) {
    for (int i = 0; i < 3; i++) {
        strcat(a_ip, strtok(NULL, ","));
        strcat(a_ip, ".");
    }
    strcat(a_ip, strtok(NULL, ","));
    *a_port = strtol(strtok(NULL, ","), (char **)NULL, 10)
            + (strtol(strtok(NULL, ","), (char **)NULL, 10) << 8);
}

/// TODO Error Handling, 변수 이름 제대로
void serverDataConnection(int a_cli_conn_fd, char * a_ip, unsigned int a_port, char * a_recv_cmd_buf) {
    int s_data_fd;
    struct sockaddr_in s_data_addr;
    char * s_cmd = NULL;
    char s_data_buf[DATA_BUFF_SIZE] = {0, };
    int s_opt_count = 1;
    char * s_opt_value[CTRL_BUFF_SIZE];

    if ((s_data_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Server: Can't open stream socket\n");
        exit(-1);
    }

    memset(&s_data_addr, 0, sizeof(s_data_addr));
    s_data_addr.sin_family = AF_INET;
    s_data_addr.sin_addr.s_addr = inet_addr(a_ip);
    s_data_addr.sin_port = htons(a_port);

    /// 6. Data Connection Setup: Socket -> Connect
    if (connect(s_data_fd, (struct sockaddr *)&s_data_addr, sizeof(s_data_addr))){
        write(STDERR_FILENO, "Server: connection failed\n", strlen("Client: connection failed\n"));
        exit(1);
    }
    write(a_cli_conn_fd, "150 Opening data connection\n", CTRL_BUFF_SIZE);

    /// 8. Send Data using Data Port
    s_cmd = strtok(a_recv_cmd_buf, " \n");
    if (!strcmp(s_cmd, "NLST")) {
        serverNLST(s_opt_count, s_opt_value, s_data_buf);
        write(s_data_fd, s_data_buf, DATA_BUFF_SIZE);
    }

    write(a_cli_conn_fd, "226 Complete transmission.\n", CTRL_BUFF_SIZE);

    close(s_data_fd);
}


/// TODO Add More Commands -PORT-, LS, GET, PUT, TYPE
int serverExecuteCommand(int a_cli_conn_fd, char * a_recv_cmd_buf) {
    char * s_cmd = NULL;
    s_cmd = strtok(a_recv_cmd_buf, " \n");

    if (!strcmp(s_cmd, "PORT")) {
        char s_cli_ip[CTRL_BUFF_SIZE] = {0, };
        unsigned int s_cli_port = 0;
        ssize_t n;

        /// 2. Send Response about PORT
        serverParsingIpPort(NULL, s_cli_ip, &s_cli_port);
        if (!s_cli_port) {
            write(a_cli_conn_fd, "550 Failed to access.\n", CTRL_BUFF_SIZE);
            return -1;
        }
        write(a_cli_conn_fd, "200 PORT performed successfully.\n", CTRL_BUFF_SIZE);

        /// 5. Receive FTP Command
        memset(a_recv_cmd_buf, 0, sizeof(*a_recv_cmd_buf));
        n = read(a_cli_conn_fd, a_recv_cmd_buf, CTRL_BUFF_SIZE);
        a_recv_cmd_buf[n] = '\0';
        printf("---> %s\n", a_recv_cmd_buf);
        serverDataConnection(a_cli_conn_fd, s_cli_ip, s_cli_port, a_recv_cmd_buf);
    }
    else if (!strcmp(s_cmd, "PWD")) {
        return serverPWD(a_cli_conn_fd);
    }
    else if (!strcmp(s_cmd, "CWD")) {
        int s_opt_count = 1;
        char * s_opt_value[100];
        s_opt_value[0] = s_cmd;
        s_opt_value[s_opt_count] = strtok(NULL, " \n");
        while (s_opt_value[s_opt_count]) {
            s_opt_count++;
            s_opt_value[s_opt_count] = strtok(NULL, " \n");
        }
        return serverCWD(a_cli_conn_fd, s_opt_count, s_opt_value);
    }
    else if (!strcmp(s_cmd, "CDUP")) {
        return serverCDUP(a_cli_conn_fd);
    }
    else {
        write(STDOUT_FILENO, s_cmd, strlen(s_cmd));
        write(a_cli_conn_fd, s_cmd, CTRL_BUFF_SIZE);
    }
    return 0;
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
        while(1) {
            /// 6. response commands from client
            memset(recv_cmd_buf, 0, sizeof(recv_cmd_buf));
            serverReceiveCommand(cli_conn_fd, recv_cmd_buf);

            /// ** close Connections **
            if (!strncmp(recv_cmd_buf, "QUIT", strlen("QUIT"))) {
                write(STDOUT_FILENO, "Server: QUIT\n", strlen("Server: QUIT\n"));
                serverQuit(cli_conn_fd);
                exit(0);
            }

            /// 7. Execute Commands
            // TODO error handling -> -1: error, 0: success
            printf("---> %s\n", recv_cmd_buf);
            serverExecuteCommand(cli_conn_fd, recv_cmd_buf);
        }

    }
}