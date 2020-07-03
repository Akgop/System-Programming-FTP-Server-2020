#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/stat.h>
#include <signal.h>
#include <pwd.h>
#include <fcntl.h>

#define DATA_BUF 1024   //for rcv buff
#define CONT_BUF 200     //for FTP_CMD and reply_MSG from server
#define DATA_PORT 20001 //for data connection
#define MAX_BUF 9999

#define FLAGS       (O_RDWR | O_CREAT | O_TRUNC)
#define ASCII_MODE  (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH) 
#define BIN_MODE    (S_IXUSR | S_IXGRP | S_IXOTH)

int ascii_mode_set = 1;

int sockfd, connfd;

// conv_cmd
//=====================================================
// Input	: char * buff, char * cmd_buff
//			-> USER COMMAND, FTP COMMAND 
// Output	: if < 0 -> err occurs
// Purpose	: convert USER COMAND to FTP COMMAND
int conv_cmd(char* buff, char* cmd_buff)
{
    int optc = 0;
    char * optv[100];
    int index;
    optv[optc] = strtok(buff, " \n");
    while(optv[optc] != 0){	//loop while no more argument
        optc++;	//count up
        optv[optc] = strtok(NULL, " \n");	//make ls, count option
    }
    if(optc < 1)
    {
        return -1;
    }
    if(strcmp(optv[0], "ls") == 0)
    {
        int aflag = 0, lflag = 0;	//a,l option flag
        int index, opt = 0;
        opterr = 0;
        optind = 0;
        while ( (opt = getopt(optc, optv, "al")) != -1)	//parse a, l option
        {
            switch (opt)
            {
                case 'a':	//option == -a
                    aflag = 1;	//aflag set
                    break;
                case 'l':	//option == -b
                    lflag = 1;	//bflag set
                    break;
                case '?':	//unknown option
                    return -1;
                default:	//else
                    break;
            }
        }
        strcpy(cmd_buff, "NLST");
        if(aflag == 1 && lflag == 1)	strcat(cmd_buff, " -al");	// [-al]
        else if(aflag == 1 && lflag == 0) strcat(cmd_buff, " -a");	// [-a]
        else if(aflag == 0 && lflag == 1) strcat(cmd_buff, " -l");	// [-l]
        for(index = optind; index < optc; index++){	//non-option argument
            strcat(cmd_buff, " ");
            strcat(cmd_buff, optv[index]);	//concatenate
        }
        return 2;
    }
    else if(strcmp(optv[0], "dir") == 0)
    {
        int index, opt = 0;
        opterr = 0;
        optind = 0;
        while ( (opt = getopt(optc, optv, "")) != -1)	//parse a, l option
        {
            switch (opt)
            {
                case '?':	//unknown option
                    return -1;
                default:	//else
                    break;
            }
        }
        strcpy(cmd_buff, "LIST");
        for(index = optind; index < optc; index++){	//non-option argument
            strcat(cmd_buff, " ");
            strcat(cmd_buff, optv[index]);	//concatenate
        }
        return 2;
    }
    else if(strcmp(optv[0], "pwd") == 0)
    {
        int index, opt = 0;
        opterr = 0;
        optind = 0;
        if(optc > 1)
        {
            return -1;
        }
        while ( (opt = getopt(optc, optv, "")) != -1)	//parse a, l option
        {
            switch (opt)
            {
                case '?':	//unknown option
                    return -1;
                default:	//else
                    break;
            }
        }
        strcpy(cmd_buff, "PWD");
        for(index = optind; index < optc; index++){	//non-option argument
            strcat(cmd_buff, " ");
            strcat(cmd_buff, optv[index]);	//concatenate
        }
    }
    else if(strcmp(optv[0], "cd") == 0)
    {
        int index, opt = 0;
        opterr = 0;
        optind = 0;
        if(optc < 2)
        {
            return -1;
        }
        while ( (opt = getopt(optc, optv, "")) != -1)	//parse a, l option
        {
            switch (opt)
            {
                case '?':	//unknown option
                    return -1;
                default:	//else
                    break;
            }
        }
        if(strcmp(optv[optind], "..") == 0){
			if(optc > 2){
                return -1;
			}
			strcat(cmd_buff, "CDUP");	//convert cd .. -> CDUP
		}
        else{
            strcat(cmd_buff, "CWD");
            for(index = optind; index < optc; index++){	//non-option argument
                strcat(cmd_buff, " ");
                strcat(cmd_buff, optv[index]);	//concatenate
            }
        }
    }
    else if(strcmp(optv[0], "mkdir") == 0)
    {
        int index, opt = 0;
        opterr = 0;
        optind = 0;
        if(optc < 2)
        {
            return -1;
        }
        while ( (opt = getopt(optc, optv, "")) != -1)	//parse a, l option
        {
            switch (opt)
            {
                case '?':	//unknown option
                    return -1;
                default:	//else
                    break;
            }
        }
        strcat(cmd_buff, "MKD");
        for(index = optind; index < optc; index++){	//non-option argument
            strcat(cmd_buff, " ");
            strcat(cmd_buff, optv[index]);	//concatenate
        }
    }
    else if(strcmp(optv[0], "delete") == 0)
    {
        int index, opt = 0;
        opterr = 0;
        optind = 0;
        if(optc < 2)
        {
            return -1;
        }
        while ( (opt = getopt(optc, optv, "")) != -1)	//parse a, l option
        {
            switch (opt)
            {
                case '?':	//unknown option
                    return -1;
                default:	//else
                    break;
            }
        }
        strcat(cmd_buff, "DELE");
        for(index = optind; index < optc; index++){	//non-option argument
            strcat(cmd_buff, " ");
            strcat(cmd_buff, optv[index]);	//concatenate
        }
    }
    else if(strcmp(optv[0], "rmdir") == 0)
    {
        int index, opt = 0;
        opterr = 0;
        optind = 0;
        if(optc < 2)
        {
            return -1;
        }
        while ( (opt = getopt(optc, optv, "")) != -1)	//parse a, l option
        {
            switch (opt)
            {
                case '?':	//unknown option
                    return -1;
                default:	//else
                    break;
            }
        }
        strcat(cmd_buff, "RMD");
        for(index = optind; index < optc; index++){	//non-option argument
            strcat(cmd_buff, " ");
            strcat(cmd_buff, optv[index]);	//concatenate
        }
    }
    else if(strcmp(optv[0], "rename") == 0)
    {
        char temp[CONT_BUF];
        bzero(temp, sizeof(temp));
        int index, opt = 0;
        opterr = 0;
        optind = 0;
        if(optc != 3)
        {
            return -1;
        }
        while ( (opt = getopt(optc, optv, "")) != -1)	//parse a, l option
        {
            switch (opt)
            {
                case '?':	//unknown option
                    return -1;
                default:	//else
                    break;
            }
        }
        //convert rename -> RNFR
        strcat(cmd_buff, "RNFR ");
        strcat(cmd_buff, optv[optind]);
        write(STDOUT_FILENO, "---> ", 6);
        write(STDOUT_FILENO, cmd_buff, strlen(cmd_buff));
        write(STDOUT_FILENO, "\n", 2);
        write(sockfd, cmd_buff, CONT_BUF);  //send RNFR

        read(sockfd, temp, CONT_BUF);   //rcv msg from server
        write(STDOUT_FILENO, temp, strlen(temp));
        if(strncmp(temp, "550", 3) == 0){    //FAIL
            return 1;
        }
        bzero(cmd_buff, sizeof(cmd_buff));
		strcat(cmd_buff, "RNTO "); // RNTO
        strcat(cmd_buff, optv[optind + 1]);
    }
    else if(strcmp(optv[0], "type") == 0)
    {
        int index, opt = 0;
        opterr = 0;
        optind = 0;
        if(optc != 2)
        {
            return -1;
        }
        while ( (opt = getopt(optc, optv, "")) != -1)	//parse option
        {
            switch (opt)
            {
                case '?':	//unknown option
                    return -1;
                default:	//else
                    break;
            }
        }
        strcat(cmd_buff, "TYPE");
        if(strcmp(optv[optind], "ascii") == 0){
            strcat(cmd_buff, " A");
        }
        else if(strcmp(optv[optind], "binary") == 0){
            strcat(cmd_buff, " I");
        }
        else return -1;
    }
    else if(strcmp(optv[0], "ascii") == 0)
    {
        strcpy(cmd_buff, "TYPE A");
    }
    else if(strcmp(optv[0], "bin") == 0)
    {
        strcpy(cmd_buff, "TYPE I");
    }
    else if(strcmp(optv[0], "get") == 0)
    {
        int index, opt = 0;
        opterr = 0;
        optind = 0;
        if(optc != 2)
        {
            return -1;
        }
        while ( (opt = getopt(optc, optv, "")) != -1)	//parse option
        {
            switch (opt)
            {
                case '?':	//unknown option
                    return -1;
                default:	//else
                    break;
            }
        }
        strcpy(cmd_buff, "RETR ");
        strcat(cmd_buff, optv[optind]);
        return 3;
    }
    else if(strcmp(optv[0], "put") == 0)
    {
        int index, opt = 0;
        opterr = 0;
        optind = 0;
        if(optc != 2)
        {
            return -1;
        }
        while ( (opt = getopt(optc, optv, "")) != -1)	//parse option
        {
            switch (opt)
            {
                case '?':	//unknown option
                    return -1;
                default:	//else
                    break;
            }
        }
        strcpy(cmd_buff, "STOR ");  //STOR
        strcat(cmd_buff, optv[optind]);
        return 4;
    }
    else if(strcmp(optv[0], "quit") == 0)   //QUIT
    {
        strcat(cmd_buff, "QUIT");
    }
    else {
        return -1;
    }
    
    return 0;
}

// convert_addr_to_str
//=====================================================
// Input	: unsigned long ip_addr, unsigned int port
// Output	: char * ip addr
// Purpose	: FTP client
char* convert_addr_to_str(unsigned long ip_addr, unsigned int port)
{
    char *addr;
    char buff[50];
    int ipmod1, ipmod2, ipmod3, ipmod4, pomod1, pomod2; //ip addr & port no

    //convert big Endian to little endian ip address with comma
    ipmod1 = ip_addr & 0xff;
    ipmod2 = ip_addr & 0xff00;
    ipmod2 = ipmod2 >> 8;
    ipmod3 = ip_addr & 0xff0000;
    ipmod3 = ipmod3 >> 16;
    ipmod4 = ip_addr & 0xff000000;
    ipmod4 = ipmod4 >> 24;
    sprintf(buff, "%d,%d,%d,%d", ipmod1, ipmod2, ipmod3, ipmod4);
    strcpy(addr, buff);
    bzero(buff, sizeof(buff));

    //convert portnumber with comma
    pomod2 = port & 0xff;
    pomod1 = port & 0xff00;
    pomod1 = pomod1 >> 8;
    sprintf(buff, ",%d,%d", pomod1, pomod2);
    strcat(addr, buff); //concatenate ipaddress and portnumber
    return addr;
}


/* when SIGINT occurs */
void sh_int(int signum)
{
    char cmd_buff[CONT_BUF];
    bzero(cmd_buff, sizeof(cmd_buff));
    strcpy(cmd_buff, "QUIT");
    write(sockfd, cmd_buff, CONT_BUF);   //send QUIT to server
    write(STDOUT_FILENO, "\n", 2);
    exit(1);    //client quit
}

// main
//=====================================================
// Input	: int argc, char **argv
// Output	: none
// Purpose	: FTP client
void main(int argc, char **argv)
{
    int datafd, recvlen, n;
    char * hostport, * passwd;
    struct sockaddr_in cliaddr, recvaddr;
    char total_buff[999999] = {0,};
    char ascii_buff[2000] = {0,};
    char msg_buff[CONT_BUF], cmd_buff[CONT_BUF], rcv_buff[DATA_BUF], input_buff[CONT_BUF], port_buff[CONT_BUF], temp_buff[MAX_BUF];

    if(argc != 3){  //not enough input
        write(STDERR_FILENO, "Client: wrong input format\n", 28);
		exit(1);
    }

    // create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&cliaddr, 0, sizeof(cliaddr));
    cliaddr.sin_family = AF_INET; //set client info
    cliaddr.sin_addr.s_addr = inet_addr(argv[1]); //IP ADDR
    cliaddr.sin_port = htons(atoi(argv[2]));  //PORTNO

    // control connection request to server
    connect(sockfd, (struct sockaddr*)&cliaddr, sizeof(cliaddr));
    // ready for data connection 
    hostport = convert_addr_to_str(cliaddr.sin_addr.s_addr, DATA_PORT);    //convert function

    // Check if the IP is acceptable
    read(sockfd, msg_buff, CONT_BUF); //Accepted or Rejection
    if(strncmp(msg_buff, "220", 3) == 0){   //if ACCEPTED -> connected
        write(STDOUT_FILENO, msg_buff, strlen(msg_buff));
    }
    else{   //Else REJECTION -> not connected
        write(STDOUT_FILENO, msg_buff, strlen(msg_buff));
        return; //terminate
    }
    write(STDOUT_FILENO, "\n", 2);
    signal(SIGINT, sh_int);    //SIGINT occurs
    // Authentication
    while(1){
        // send username to server
        bzero(input_buff, sizeof(input_buff));
        bzero(cmd_buff, sizeof(cmd_buff));
        bzero(msg_buff, sizeof(msg_buff));
        strcpy(cmd_buff, "USER ");
        write(STDOUT_FILENO, "Input ID : ", 12);
        read(STDIN_FILENO, input_buff, CONT_BUF);  //username
        strcat(cmd_buff, input_buff);
        write(STDOUT_FILENO, "---> ", 6);
        write(STDOUT_FILENO, cmd_buff, strlen(cmd_buff));
        write(sockfd, cmd_buff, CONT_BUF);   //send username to server

        // recv msg for username
        read(sockfd, msg_buff, CONT_BUF);
        if(strncmp(msg_buff, "331", 3) == 0){  //username success
            write(STDOUT_FILENO, msg_buff, strlen(msg_buff));
            write(STDOUT_FILENO, "\n", 2);
        }
        else if(strncmp(msg_buff, "430", 3) == 0){ //username fail
            write(STDOUT_FILENO, msg_buff, strlen(msg_buff));
            write(STDOUT_FILENO, "\n", 2);
            continue;
        }
        else{   //disconnection : 530
            write(STDOUT_FILENO, msg_buff, strlen(msg_buff));
            write(STDOUT_FILENO, "\n", 2);
            close(sockfd);  //close connection
            exit(0);
        }

        // send password to server
        bzero(cmd_buff, sizeof(cmd_buff));
        bzero(msg_buff, sizeof(msg_buff));
        strcpy(cmd_buff, "PASS ");
        passwd = getpass("Input Password : ");  //password
        strcat(cmd_buff, passwd);
        strcat(cmd_buff, "\n");
        write(STDOUT_FILENO, "---> ", 6);
        write(STDOUT_FILENO, cmd_buff, strlen(cmd_buff));
        write(sockfd, cmd_buff, CONT_BUF);   //send password to server
        read(sockfd, msg_buff, CONT_BUF); 
        if(strncmp(msg_buff, "230", 3) == 0){  //username success
            write(STDOUT_FILENO, msg_buff, strlen(msg_buff));   //login successful
            write(STDOUT_FILENO, "\n", 2);
            break;
        }
        else if(strncmp(msg_buff, "430", 3) == 0){ //password fail
            write(STDOUT_FILENO, msg_buff, strlen(msg_buff));
            write(STDOUT_FILENO, "\n", 2);
            continue;
        }
        else{    //disconnection : 530
            write(STDOUT_FILENO, msg_buff, strlen(msg_buff));
            write(STDOUT_FILENO, "\n", 2);
            close(sockfd);  //close connection
            exit(0);
        }
    }

    while(1){
        bzero(msg_buff, sizeof(msg_buff));  //clear buffer
        bzero(cmd_buff, sizeof(cmd_buff));
        bzero(rcv_buff, sizeof(rcv_buff));
        write(STDOUT_FILENO, "ftp> ", 6);
        n = read(STDIN_FILENO, input_buff, CONT_BUF); //command line input
        input_buff[n] = '\0';
        // convert USER_CMD to FTP_CMD
        if((n = conv_cmd(input_buff, cmd_buff)) < 0)    //convert User command to FTP command
        {
            write(STDERR_FILENO, "conv_cmd() error!!\n", 20);
            continue;
        }
        else if(n == 1){
            continue;
        }
        else if(n >= 2){    // ls, dir, get, put           
            // Send PORT ~
            bzero(port_buff, sizeof(port_buff));
            strcpy(port_buff, "PORT ");
            strcat(port_buff, hostport);
            write(STDOUT_FILENO, "---> ", 6);
            write(STDOUT_FILENO, port_buff, strlen(port_buff));
            write(STDOUT_FILENO, "\n", 2);
            write(sockfd, port_buff, CONT_BUF); //send PORT command

            //recv ACK msg from server
            read(sockfd, msg_buff, CONT_BUF);
            write(STDOUT_FILENO, msg_buff, strlen(msg_buff));

            // send command
            write(STDOUT_FILENO, "---> ", 6);
            write(STDOUT_FILENO, cmd_buff, strlen(cmd_buff));
            write(STDOUT_FILENO, "\n", 2);
            write(sockfd, cmd_buff, CONT_BUF);

            // Data connection setup 
            if((datafd = socket(PF_INET, SOCK_STREAM, 0)) < 0){	//create socket IPv4, Portnum
                write(STDERR_FILENO, "Server: Can't open stream socket\n", 34); //socket error
                exit(-1);	//exit
            }

            //set server socket info
            memset(&recvaddr, 0, sizeof(recvaddr));
            recvaddr.sin_family = AF_INET;
            recvaddr.sin_addr.s_addr = htonl(INADDR_ANY);    //ipaddr
            recvaddr.sin_port = htons(DATA_PORT);    //portno

            //bind
            if(bind(datafd, (struct sockaddr *)&recvaddr, sizeof(recvaddr)) < 0)	//bind
            {
                write(STDERR_FILENO, "Can't bind data connection.\n", 29);	//bind error
                close(datafd);
                continue;
            }

            //listen: MAX concurrent client = 1
            listen(datafd, 1);

            // data connection accept
            recvlen = sizeof(recvaddr);
            connfd = accept(datafd, (struct sockaddr *) &recvaddr, &recvlen); //accept

            // recv msg from server
            bzero(msg_buff, CONT_BUF);
            read(sockfd, msg_buff, CONT_BUF);
            write(STDOUT_FILENO, msg_buff, strlen(msg_buff));

            if(n == 2){ // ls, dir
                // recv DATA from server
                bzero(temp_buff, sizeof(temp_buff));
                read(connfd, temp_buff, MAX_BUF);
                temp_buff[MAX_BUF] = '\0';
                write(STDOUT_FILENO, temp_buff, strlen(temp_buff));

                // recv msg from server
                bzero(msg_buff, sizeof(msg_buff));
                read(sockfd, msg_buff, CONT_BUF);
                write(STDOUT_FILENO, msg_buff, strlen(msg_buff));

                // print bytes
                bzero(msg_buff, sizeof(msg_buff));
                sprintf(msg_buff, "OK. %ld bytes is received.\n", strlen(temp_buff));
                write(STDOUT_FILENO, msg_buff, strlen(msg_buff));
            }
            else if(n == 3){    //big size data -> get
                int len = 0;
                int fd;
                bzero(msg_buff, sizeof(msg_buff));
                read(sockfd, msg_buff, CONT_BUF);
                msg_buff[strlen(msg_buff)] = '\0';
                
                char fullpath[512] = {0, };
                int n;

                // file path
                getcwd(fullpath, 512);
                strcat(fullpath, "/");
                strcat(fullpath, msg_buff);

                // open file
                if(ascii_mode_set == 1){	//ascii mode
                    int temp_cnt = 0;
                    int i;
                    fd = open(fullpath, FLAGS, ASCII_MODE);
                    while(1){
                        bzero(ascii_buff, sizeof(ascii_buff));
                        bzero(rcv_buff, sizeof(rcv_buff));  //BIG SIZE data Transfer
                        read(connfd, rcv_buff, DATA_BUF);
                        rcv_buff[DATA_BUF] = '\0';
                      
                        //ASCII MODE CONVERTING
                        temp_cnt = 0;
                        for(i=0; i < DATA_BUF; i++)
                        {
                            if(rcv_buff[i] == '\n'){    //binary CR
                                ascii_buff[temp_cnt] = rcv_buff[i];
                                temp_cnt++;
                                ascii_buff[temp_cnt] = '\r';    //ADD \r
                            }
                            else{
                                ascii_buff[temp_cnt] = rcv_buff[i];
                            }
                            temp_cnt++;
                        }

                        strcat(total_buff, ascii_buff);
                        len += strlen(ascii_buff);  //length
                        if(strlen(rcv_buff) < DATA_BUF){
                            break;
                        }
                        write(connfd, "MORE", 5);   //ACK
                    }
                    write(fd, total_buff, strlen(total_buff));
                    close(fd);
                }
                else {	//binary mode
                    fd = open(fullpath, FLAGS, BIN_MODE);
                    while(1){
                        bzero(rcv_buff, sizeof(rcv_buff));  //BIG SIZE data Transfer
                        read(connfd, rcv_buff, DATA_BUF);
                        rcv_buff[DATA_BUF] = '\0';
                        write(fd, rcv_buff, DATA_BUF);
                        len += strlen(rcv_buff);
                        if(strlen(rcv_buff) < DATA_BUF){
                            break;
                        }
                        write(connfd, "MORE", 5);   //ACK
                    }
                    close(fd);
                }

                // recv msg from server
                bzero(msg_buff, sizeof(msg_buff));
                read(sockfd, msg_buff, CONT_BUF);
                write(STDOUT_FILENO, msg_buff, strlen(msg_buff));

                // print bytes
                bzero(msg_buff, sizeof(msg_buff));
                sprintf(msg_buff, "OK. %d bytes is received.\n", len);
                write(STDOUT_FILENO, msg_buff, strlen(msg_buff));

                write(sockfd, "DONE", 5);
            }
            else if(n == 4){    //put
                int len = 0;
                int fd;
                int temp_cnt = 0;
                int i;
                bzero(msg_buff, sizeof(msg_buff));
                read(sockfd, msg_buff, CONT_BUF);   //read filename
                msg_buff[strlen(msg_buff)] = '\0';
                
                char fullpath[512] = {0, };
                int n;

                // file path
                getcwd(fullpath, 512);
                strcat(fullpath, "/");
                strcat(fullpath, msg_buff);

                if(ascii_mode_set == 1){
                    fd = open(fullpath, O_RDONLY, ASCII_MODE);
                    while(1){
                        bzero(rcv_buff, sizeof(rcv_buff));  //BIG SIZE data Transfer
                        read(fd, rcv_buff, DATA_BUF);
                        rcv_buff[DATA_BUF] = '\0';
                        write(connfd, rcv_buff, DATA_BUF);
                        len += strlen(rcv_buff);

                        bzero(msg_buff, sizeof(msg_buff));
                        read(connfd, msg_buff, 5);   //ACK
                        if(strlen(rcv_buff) < DATA_BUF){
                            close(fd);
                            break;
                        }
                    }
                }
                else{
                    fd = open(fullpath, O_RDONLY, BIN_MODE);
                    while(1){
                        bzero(rcv_buff, sizeof(rcv_buff));  //BIG SIZE data Transfer
                        read(fd, rcv_buff, DATA_BUF);
                        rcv_buff[DATA_BUF] = '\0';
                        write(connfd, rcv_buff, DATA_BUF);
                        len += strlen(rcv_buff);

                        bzero(msg_buff, sizeof(msg_buff));
                        read(connfd, msg_buff, 5);   //ACK
                        if(strlen(rcv_buff) < DATA_BUF){
                            close(fd);
                            break;
                        }
                    }
                }
                // recv msg from server
                bzero(msg_buff, sizeof(msg_buff));
                read(sockfd, msg_buff, CONT_BUF);
                write(STDOUT_FILENO, msg_buff, strlen(msg_buff));

                // print bytes
                bzero(msg_buff, sizeof(msg_buff));
                sprintf(msg_buff, "OK. %d bytes is sent.\n", len);
                write(STDOUT_FILENO, msg_buff, strlen(msg_buff));
            }

            // close data connection
            close(datafd);
            close(connfd);
        }
        else{
            // send command
            write(STDOUT_FILENO, "---> ", 6);
            write(STDOUT_FILENO, cmd_buff, strlen(cmd_buff));
            write(STDOUT_FILENO, "\n", 2);
            write(sockfd, cmd_buff, CONT_BUF);

            // recv msg from server
            read(sockfd, msg_buff, CONT_BUF);
            write(STDOUT_FILENO, msg_buff, strlen(msg_buff));
            if(strcmp(msg_buff, "201 Type set to A.\n") == 0){
                ascii_mode_set = 1;
            }
            else if(strcmp(msg_buff, "201 Type set to I.\n") == 0){
                ascii_mode_set = 0;
            }
            else if(strncmp(msg_buff, "221", 3) == 0){  //QUIT;
                break;
            }
        }
    }
    close(sockfd);
}