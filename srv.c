#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <dirent.h>
#include <pwd.h>
#include <time.h>
#include <grp.h>
#include <fcntl.h>

#define DATA_BUF 1024   //for rcv buff
#define CONT_BUF 200     //for FTP_CMD and reply_MSG from server
#define MAX_BUF 9999

#define FLAGS		(O_RDWR | O_CREAT | O_TRUNC)
#define ASCII_MODE	(S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#define BIN_MODE	(S_IXUSR | S_IXGRP | S_IXOTH)

// child_info struct
struct child_info {
	int num; // connection index
	int port;	//port number
	int time;	//timer
	int pid; // process id for client
};
struct child_info info[5];	//struct array
int child_count = 0;	//total child amount
int connfd, sockfd;   //socket descriptor
int ascii_mode_set = 1;	//if 0 -> binary mode

// bubble_sort
//=====================================================
// Input	: int cnt, char **arr
// Output	: None
// Purpose	: 2D arr bubble sorting
void bubble_sort(char (*arr)[100], int cnt)
{
	char temp[100] = {0, };
	int i=0, j=0;
	for(i=0; i<cnt; i++){
		for(j=0; j<cnt-1; j++){
			if(strcmp(arr[j], arr[j+1]) > 0){	// A > B -> swap
				strcpy(temp, arr[j]);
				memset(arr[j], 0, sizeof(arr[j]));
				strcpy(arr[j], arr[j+1]);
				memset(arr[j+1], 0, sizeof(arr[j+1]));
				strcpy(arr[j+1], temp);
			}
		}
	}
}

// myls_l
//=====================================================
// Input	: char * pathname, int len, int hidden
//			-> pathname, length, [-a]option flag
// Output	: None
// Purpose	: NLST [-l], [-al], [filename], [dirname]
void myls_l(char * pathname, int len, int hidden, char *result_buff)
{
	DIR * dir = opendir(pathname);	//open directory
	struct dirent * dp;	//dirent
	char temp[100][100] = {0, };
	char fullpath[100] = {0, }; 
	char result[512] = {0, };
	int cnt = 0, i = 0;
	char buff[512] = {0, };
	struct stat buf;
	struct passwd * user_info;	//passwd struct
	struct group * group_info;	//group struct
	struct tm * ltp;
	if(NULL != dir)		//if dir exists == directory
	{
		if(len >= 1){	//directory name exist
			char colon[10] = ":\n";
			strcat(result_buff, pathname);
			strcat(result_buff, colon);
		}
		while(dp = readdir(dir))	//print every files & dir in working directory
		{
			if((hidden == 0) && (dp->d_name[0] == '.')){
				continue;	//if aflag 0, skip hidden file
			}
			strcpy(temp[cnt], dp->d_name);	//start here
			cnt++;
		}
		bubble_sort(temp, cnt);
		for(i=0; i<cnt; i++){
			memset(fullpath, 0, sizeof(fullpath));	
			memset(result, 0, sizeof(result));
			if(pathname[0] == '/'){	//absolute path
				strcpy(fullpath, pathname);
				strcat(fullpath, "/");
				strcat(fullpath, temp[i]);
			}
			else{	//relative path
				getcwd(fullpath, 100);
				strcat(fullpath, "/");
				strcat(fullpath, pathname);
				strcat(fullpath, "/");
				strcat(fullpath, temp[i]);
			}
			// file permission -> r for read, w for write, x for execute
			if(lstat(fullpath, &buf) == 0){
				if(S_ISDIR(buf.st_mode)){	//if directory, add "/"
					strcat(temp[i], "/");
					result[0] = 'd';
				}
				else if(S_ISFIFO(buf.st_mode)) result[0] = 'p';	//p for FIFO
				else if(S_ISLNK(buf.st_mode)) result[0] = 'l';	//l for sym link
				else result[0] = '-';	//else(regular file)
				if((buf.st_mode)& S_IRUSR) result[1] = 'r';
				else result[1] = '-';
				if((buf.st_mode)& S_IWUSR) result[2] = 'w';
				else result[2] = '-';
				if((buf.st_mode)& S_IXUSR) result[3] = 'x';
				else result[3] = '-';
				if((buf.st_mode)& S_IRGRP) result[4] = 'r';
				else result[4] = '-';
				if((buf.st_mode)& S_IWGRP) result[5] = 'w';
				else result[5] = '-';
				if((buf.st_mode)& S_IXGRP) result[6] = 'x';
				else result[6] = '-';
				if((buf.st_mode)& S_IROTH) result[7] = 'r';
				else result[7] = '-';
				if((buf.st_mode)& S_IWOTH) result[8] = 'w';
				else result[8] = '-';
				if((buf.st_mode)& S_IXOTH) result[9] = 'x';
				else result[9] = '-';
				strcat(result, " ");
				sprintf(buff, "%2lu ", buf.st_nlink);	//link count
				strcat(result, buff);
				user_info = getpwuid(buf.st_uid);	//user uid
				if(user_info != NULL) strcat(result, user_info->pw_name);
				strcat(result, " ");
				group_info = getgrgid(buf.st_gid);	//group uid
				if(group_info != NULL) strcat(result, group_info->gr_name);
				strcat(result, " ");
				sprintf(buff, "%6ld", buf.st_size);
				strcat(result, buff);
				strcat(result, " ");
				ltp = localtime(&buf.st_mtime);	//modified time
				sprintf(buff, "%2d월 %02d일 %02d:%02d", (ltp->tm_mon)+1, ltp->tm_mday, ltp->tm_hour, ltp->tm_min);
				strcat(result, buff);
				strcat(result, " ");
			}
			strcat(result, temp[i]);
			strcat(result_buff, result);
			strcat(result_buff, "\n");
		}
	}
	else	//dir not exist == file
	{
		getcwd(fullpath, 100);
		strcat(fullpath, "/");	//get pathname
		strcat(fullpath, pathname);
		// file permission -> r for read, w for write, x for execute
		if(lstat(fullpath, &buf) == 0){
			if(S_ISDIR(buf.st_mode)) result[0] = 'd';	//d for directory
			else if(S_ISFIFO(buf.st_mode)) result[0] = 'p';	//p for FIFO
			else if(S_ISLNK(buf.st_mode)) result[0] = 'l';	//l for symlink
			else result[0] = '-';
			if((buf.st_mode)& S_IRUSR) result[1] = 'r';
			else result[1] = '-';
			if((buf.st_mode)& S_IWUSR) result[2] = 'w';
			else result[2] = '-';
			if((buf.st_mode)& S_IXUSR) result[3] = 'x';
			else result[3] = '-';
			if((buf.st_mode)& S_IRGRP) result[4] = 'r';
			else result[4] = '-';
			if((buf.st_mode)& S_IWGRP) result[5] = 'w';
			else result[5] = '-';
			if((buf.st_mode)& S_IXGRP) result[6] = 'x';
			else result[6] = '-';
			if((buf.st_mode)& S_IROTH) result[7] = 'r';
			else result[7] = '-';
			if((buf.st_mode)& S_IWOTH) result[8] = 'w';
			else result[8] = '-';
			if((buf.st_mode)& S_IXOTH) result[9] = 'x';
			else result[9] = '-';
			strcat(result, " ");
			sprintf(buff, "%2lu ", buf.st_nlink);	//link count
			strcat(result, buff);
			user_info = getpwuid(buf.st_uid);	//user uid
			if(user_info != NULL) strcat(result, user_info->pw_name);
			strcat(result, " ");
			group_info = getgrgid(buf.st_gid);	//group uid
			if(group_info != NULL) strcat(result, group_info->gr_name);
			strcat(result, " ");
			sprintf(buff, "%6ld", buf.st_size);
			strcat(result, buff);
			strcat(result, " ");
			ltp = localtime(&buf.st_mtime);	//modified time
			sprintf(buff, "%2d월 %02d일 %02d:%02d", (ltp->tm_mon)+1, ltp->tm_mday, ltp->tm_hour, ltp->tm_min);
			strcat(result, buff);
			strcat(result, " ");
			strcat(result, pathname);
			strcat(result_buff, result);
			strcat(result_buff, "\n");
		}
	}
}

// myls_default
//=====================================================
// Input	: char * pathname, int len, int hidden
//			-> pathname, length, [-a]option flag
// Output	: None
// Purpose	: NLST [-a], [filename], [dirname]
void myls_default(char * pathname, int len, int hidden, char *result_buff)
{
	DIR * dir = opendir(pathname);	//open directory
	struct dirent * dp;	//dirent
	char temp[100][100] = {0, };
	char fullpath[100] = {0, }; 
	int cnt = 0, i = 0;
	struct stat buf;
	if(NULL != dir)		//if dir exists
	{
		if(len >= 1){	//directory name exist
			char colon[10] = ":\n";
            strcat(result_buff, pathname);
            strcat(result_buff, colon);
		}
		while(dp = readdir(dir))	//print every files & dir in working directory
		{
			if((hidden == 0) && (dp->d_name[0] == '.')){
				continue;	//if aflag 0, skip hidden file
			}
			strcpy(temp[cnt], dp->d_name);	//start here
			cnt++;
		}
		bubble_sort(temp, cnt);
		for(i=0; i<cnt; i++){
			memset(fullpath, 0, sizeof(fullpath));
			if(pathname[0] == '/'){	//absolute path
				strcpy(fullpath, pathname);
				strcat(fullpath, "/");
				strcat(fullpath, temp[i]);
			}
			else{	//relative path
				getcwd(fullpath, 100);
				strcat(fullpath, "/");
				strcat(fullpath, pathname);
				strcat(fullpath, "/");
				strcat(fullpath, temp[i]);
			}
			if(lstat(fullpath, &buf) == 0){
				if(S_ISDIR(buf.st_mode)){	//if directory, add "/"
					strcat(temp[i], "/");
				}
			}
			if((i == cnt-1) || (i % 5 == 4)){	//one row: MAX 5
				strcat(result_buff, temp[i]);
				strcat(result_buff, "\n");
			}
			else{
				strcat(result_buff, temp[i]);
				strcat(result_buff, "\t");
			}
		}
		closedir(dir);	//close
	}
	else	//file
	{
        strcat(result_buff, pathname);
	}
}

// myls
//=====================================================
// Input	: int optc, char ** optv, char *result_buff
//			-> argc, argv, result
// Output	: -1 : error
//             0 : done
// Purpose	: NLST control function
int myls(int optc, char ** optv, char *result_buff)
{
	int aflag = 0, lflag = 0;	//each flags
	int opt, index;
	opterr = 0;	//don't show error option
	optind = 0;
	while( (opt = getopt(optc, optv, "al")) != -1)
	{
		switch(opt)
		{
			case 'a':
				aflag = 1;	//option a set
				break;
			case 'l':
				lflag = 1;	//option l set
				break;
			case '?':		//invalid option
				strcpy(result_buff, "ls: invalid option -- \n");
				return -1;
			default:
				break;
		}
	}
	//check non-option argument whether filename or directory name
	struct stat buf;
	int file_cnt = 0, dir_cnt = 0;
	char filename[100][100] = {0, };
	char dirname[100][100] = {0, };
	int errflag = 0;
	for(index = optind; index < optc; index++){	//non-option argument
		if(!lstat(optv[index], &buf)){
			if(S_ISREG(buf.st_mode)) {			//if argument is reqular file
				strcpy(filename[file_cnt], optv[index]);	//file
				file_cnt++;
			}
			else if(S_ISDIR(buf.st_mode)){	//if argument is directory
				strcpy(dirname[dir_cnt], optv[index]);	//directory
				dir_cnt++;
			}
			else{
				strcat(result_buff, "ls: cannot access '");	//not a file or directory
				strcat(result_buff, optv[index]);
				strcat(result_buff, "': No such file or directory\n");
				errflag++;	//flag set
			}
		}
		else{
			strcat(result_buff, "ls: cannot access '");	//not a file or directory
			strcat(result_buff, optv[index]);
			strcat(result_buff, "': No such file or directory\n");
			errflag++;	//flag set
		}
	}
	bubble_sort(filename, file_cnt);
	bubble_sort(dirname, dir_cnt);
	if(aflag == 1 && lflag == 1){	// -a, -l option
		for(index = 0; index < file_cnt; index++){	//print file first
			myls_l(filename[index], file_cnt+dir_cnt+errflag, aflag, result_buff);
		}
		if(file_cnt!= 0 && dir_cnt != 0){	//when file ends
            strcat(result_buff, "\n");
		}
		for(index = 0; index < dir_cnt; index++){	//print directory
			myls_l(dirname[index], file_cnt+dir_cnt+errflag, aflag, result_buff);
			if(index != dir_cnt - 1){
				strcat(result_buff, "\n");
			}
		}
		// no file or dir for arguments
		if(file_cnt == 0 && dir_cnt == 0 && errflag == 0){
			myls_l(".", 0, aflag, result_buff);
		}
	}
	else if(aflag == 1 && lflag == 0){	// -a option
		for(index = 0; index < file_cnt; index++){	//print file first
			myls_default(filename[index], file_cnt+dir_cnt+errflag, aflag, result_buff);
			if(index % 5 == 4) strcat(result_buff, "\n");	//one row: MAX 5
			else if(index != file_cnt - 1) strcat(result_buff, "\t");
		}
		if(file_cnt!= 0 && dir_cnt != 0){	//if file done
			strcat(result_buff, "\n");
            strcat(result_buff, "\n");
		}
		for(index = 0; index < dir_cnt; index++){	//print directory
			myls_default(dirname[index], file_cnt+dir_cnt+errflag, aflag, result_buff);
			if(index != dir_cnt - 1){
				strcat(result_buff, "\n");
			}
		}
		// no file or dir for arguments
		if(file_cnt == 0 && dir_cnt == 0 && errflag == 0){
			myls_default(".", 0, aflag, result_buff);
		}
	}
	else if(aflag == 0 && lflag == 1){	// -l option
		for(index = 0; index < file_cnt; index++){	//print file first
			myls_l(filename[index], file_cnt+dir_cnt+errflag, aflag, result_buff);
		}
		if(file_cnt!= 0 && dir_cnt != 0){	//when file ends
			strcat(result_buff, "\n");
		}
		for(index = 0; index < dir_cnt; index++){	//print directory
			myls_l(dirname[index], file_cnt+dir_cnt+errflag, aflag, result_buff);
			if(index != dir_cnt - 1){
				strcat(result_buff, "\n");
			}
		}
		// no file or dir for arguments
		if(file_cnt == 0 && dir_cnt == 0 && errflag == 0){
			myls_l(".", 0, aflag, result_buff);
		}
	}
	else{	//no option
		for(index = 0; index < file_cnt; index++){	//print file first
			myls_default(filename[index], file_cnt+dir_cnt+errflag, aflag, result_buff);
			if(index % 5 == 4) strcat(result_buff, "\n");	//one row: MAX 5
			else if(index != file_cnt - 1) strcat(result_buff, "\t");
		}
		if(file_cnt!= 0 && dir_cnt != 0){	//if file done
			strcat(result_buff, "\n");
            strcat(result_buff, "\n");
		}
		for(index = 0; index < dir_cnt; index++){	//print directory
			myls_default(dirname[index], file_cnt+dir_cnt+errflag, aflag, result_buff);
			if(index != dir_cnt - 1){
				strcat(result_buff, "\n");
			}
		}	
		// no file or dir for arguments
		if(file_cnt == 0 && dir_cnt == 0 && errflag == 0){
			myls_default(".", 0, aflag, result_buff);
		}
	}
    return 0;
}

// mydir
//=====================================================
// Input	: int optc, char ** optv
//			-> argc, argv
// Output	: -1 : error
//              0 : done
// Purpose	: LIST, [filename], [dirname]
int mydir(int optc, char ** optv, char *result_buff)
{
	int opt, index;
	optind = 0;
	opterr = 0;	//don't show error option
	while( (opt = getopt(optc, optv, "")) != -1)
	{
		switch(opt)
		{
			case '?':		//invalid option
				return -1;
			default:
				break;
		}
	}
	struct stat buf;
	int file_cnt = 0, dir_cnt = 0;
	char filename[512][512] = {0, };
	char dirname[512][512] = {0, };
	int errflag = 0;
	for(index = optind; index < optc; index++){	//non-option argument
		if(!lstat(optv[index], &buf)){
			if(S_ISREG(buf.st_mode)) {			//if argument is reqular file
				strcpy(filename[file_cnt], optv[index]);	//file
				file_cnt++;
			}
			else if(S_ISDIR(buf.st_mode)){	//if argument is directory
				strcpy(dirname[dir_cnt], optv[index]);	//directory
				dir_cnt++;
			}
			else{
                strcat(result_buff, "dir: cannot access '");	//not a file or directory
				strcat(result_buff, optv[index]);
				strcat(result_buff, "': No such file or directory\n");
				errflag++;	//flag set
			}
		}
		else{
			strcat(result_buff, "dir: cannot access '");	//not a file or directory
            strcat(result_buff, optv[index]);
            strcat(result_buff, "': No such file or directory\n");
			errflag++;	//flag set
		}
	}
	// dir == ls -al, so call myls_l function with aflag 1
	for(index = 0; index < file_cnt; index++){	//print file first
		myls_l(filename[index], file_cnt+dir_cnt+errflag, 1, result_buff);
	}
	if(file_cnt!= 0 && dir_cnt != 0){	//when file done
        strcat(result_buff, "\n");
	}
	for(index = 0; index < dir_cnt; index++){	//print directory
		myls_l(dirname[index], file_cnt+dir_cnt+errflag, 1, result_buff);
		if(index != dir_cnt - 1){
			strcat(result_buff, "\n");
		}
	}
	//if no file or directory argument
	if(file_cnt == 0 && dir_cnt == 0 && errflag == 0){
		myls_l(".", 0, 1, result_buff);
	}
    return 0;
}

// mypwd
//=====================================================
// Input	: none
// Output	: -1 : error
//              0 : done
// Purpose	: PWD
int mypwd()
{
	char result[CONT_BUF] = {0, };
	char msg_buff[CONT_BUF];
	bzero(msg_buff, sizeof(msg_buff));
	if(getcwd(result, CONT_BUF) != NULL){	//if no error on getcwd
        sprintf(msg_buff, "257 %s is current directory.\n", result);
		write(connfd, msg_buff, CONT_BUF);	//send msg to client
		return 0;
	}
	else return -1;
}

// mycwd
//=====================================================
// Input	: int optc, char ** optv
//			-> argc, argv
// Output	: -1 : error
//              0 : done
// Purpose	: CWD [pathname]
int mycwd(int optc, char ** optv)
{
	int file_cnt = 0, dir_cnt = 0;
	char msg_buff[CONT_BUF];
	bzero(msg_buff, sizeof(msg_buff));
	if(chdir(optv[1]) == -1){	//change directory
		sprintf(msg_buff, "550 %s: Can’t find such file or directory.\n", optv[1]);
		write(connfd, msg_buff, CONT_BUF);	//send msg to client
		return -1;
	}
	else{	//success
		strcpy(msg_buff, "250 CWD command succeeds.\n");
		write(connfd, msg_buff, CONT_BUF);	//send msg to client
		return 0;
	}
}

// mycdup
//=====================================================
// Input	: int optc, char ** optv
//			-> argc, argv
// Output	: -1 : error
//              0 : done
// Purpose	: CDUP(cd ..)
int mycdup()
{
	char msg_buff[CONT_BUF];
	bzero(msg_buff, sizeof(msg_buff));
	if(chdir("..") == -1){	//change directory with ".."
		sprintf(msg_buff, "550 '..': Can’t find such file or directory.\n");
		write(connfd, msg_buff, CONT_BUF);	//send msg to client
		return -1;
	}
	else{
		strcpy(msg_buff, "250 CDUP command succeeds.\n");
		write(connfd, msg_buff, CONT_BUF);	//send msg to client
	}
	return 0;
}

// mymkdir
//=====================================================
// Input	: int optc, char ** optv
//			-> argc, argv
// Output	: -1 : error
//              0 : done
// Purpose	: MKD [dirname1], [dirname2] ...
int mymkdir(int optc, char ** optv)
{
	int index;
	int errflag = 0;
	char temp[100] = {0,};
	char msg_buff[CONT_BUF];
	bzero(msg_buff, sizeof(msg_buff));
	if(optc == 1)	//not enough args
	{
		return -1;
	}
	for(index = 1; index < optc; index++){	//non-option argument
		if(mkdir(optv[index], 0775) != 0)	//make directory
		{
			sprintf(temp, "550 %s: Can’t create directory.\n", optv[index]);
			strcat(msg_buff, temp);
			errflag = -1;
		}
	}
	if(errflag == 0){
		strcpy(msg_buff, "250 MKD command performed successfully.\n");
	}
	write(connfd, msg_buff, CONT_BUF);	//send msg to client
	return 0;
}

// mydelete
//=====================================================
// Input	: int optc, char ** optv
//			-> argc, argv
// Output	: -1 : error
//              0 : done
// Purpose	: DELE [filename1], [filename2] ...
int mydelete(int optc, char ** optv)
{
	int index;
	int errflag = 0;
	char temp[100] = {0,};
	char msg_buff[CONT_BUF];
	bzero(msg_buff, sizeof(msg_buff));
	if(optc == 1)
	{
		return -1;
	}
	for(index = 1; index < optc; index++){	//non-option argument
		if(unlink(optv[index]) == -1)	//unlink(delete) file
		{
			sprintf(temp, "550 %s: Can’t find such file or directory.\n", optv[index]);
			strcat(msg_buff, temp);
			errflag = -1;
		}
	}
	if(errflag == 0){
		strcpy(msg_buff, "250 DELE command performed successfully.\n");
	}
	write(connfd, msg_buff, CONT_BUF);	//send msg to client
	return 0;
}

// myrmdir
//=====================================================
// Input	: int optc, char ** optv
//			-> argc, argv
// Output	: -1 : error
//              0 : done
// Purpose	: RMD [dirname1], [dirname2] ...(empty directory)
int myrmdir(int optc, char ** optv)
{
	int index;
	int errflag = 0;
	char temp[100] = {0,};
	char msg_buff[CONT_BUF];
	bzero(msg_buff, sizeof(msg_buff));
	for(index = 1; index < optc; index++){	//non-option argument
		if(rmdir(optv[index]) == -1)	//remove empty directory
		{
			sprintf(temp, "550 %s: Can’t remove directory.\n", optv[index]);
			strcat(msg_buff, temp);
			errflag = -1;
		}
	}
	if(errflag == 0){
		strcpy(msg_buff, "250 RMD command performed successfully.\n");
	}
	write(connfd, msg_buff, CONT_BUF);	//send msg to client
	return 0;
}

// myrnto
//=====================================================
// Input	: int optc, char **optv
// Output	: 0, -1
// Purpose	: rename 
int myrnto(char *cmd_buff, char * from)
{
	char * cmd = NULL;
	cmd = strtok(cmd_buff, " \n");

	int optc = 1;
	char * optv[100];	//similar as argv
	optv[0] = "cmd";		//fill first argument
	optv[optc] = strtok(NULL, " \n");	//tokenize
	while(optv[optc] != 0){	//loop while no more argument
		optc++;	//count up
		optv[optc] = strtok(NULL, " \n");	//make ls, count option
	}

	int index;
	char msg_buff[CONT_BUF];
	bzero(msg_buff, sizeof(msg_buff));
	char to[512] = {0, };	//<from>
	strcpy(to, optv[1]);
	struct stat buf;
	char fullpath[512] = {0, };
	memset(fullpath, 0, sizeof(fullpath));
	if(to[0] != '/'){	//relative path
		getcwd(fullpath, 512);
		strcat(fullpath, "/");
		strcat(fullpath, to);
	}
	if(lstat(fullpath, &buf) == 0){	//check whether <to> is exists
		if(S_ISDIR(buf.st_mode)){
			sprintf(msg_buff, "550 %s: can’t be renamed.\n", to);
			write(connfd, msg_buff, CONT_BUF);
			return -1;
		}
	}
	if(rename(from, to) == -1){	//rename directory or file
		sprintf(msg_buff, "550 %s: can’t be renamed.\n", to);
		write(connfd, msg_buff, CONT_BUF);
		return -1;
	}
	strcpy(msg_buff, "250 RNTO command succeeds.\n");
	write(connfd, msg_buff, CONT_BUF);
	return 0;
}

// myrnfr
//=====================================================
// Input	: int optc, char **optv
// Output	: from value
// Purpose	: rename 
int myrnfr(int optc, char **optv)
{
	int index, n;
	char msg_buff[CONT_BUF], cmd_buff[CONT_BUF], from[512] = {0, };
	char temp[CONT_BUF];
	bzero(msg_buff, sizeof(msg_buff));
	bzero(cmd_buff, sizeof(cmd_buff));
	bzero(temp, sizeof(temp));
	strcpy(from, optv[1]);
	struct stat buf;
	char fullpath[512] = {0, };
	memset(fullpath, 0, sizeof(fullpath));
	if(from[0] != '/'){	//relative path
		getcwd(fullpath, 512);
		strcat(fullpath, "/");
		strcat(fullpath, from);
	}
	if(lstat(fullpath, &buf) == 0){	//check whether <from> is exists
		if(S_ISDIR(buf.st_mode) || S_ISREG(buf.st_mode)){
			strcpy(msg_buff, "350 File exists, ready to rename.\n");	//send msg
			write(connfd, msg_buff, CONT_BUF);

			read(connfd, cmd_buff, CONT_BUF);
			write(STDOUT_FILENO, cmd_buff, strlen(cmd_buff));
			sprintf(temp, "\t\t[%d]", getpid());
			write(STDOUT_FILENO, temp, strlen(temp));
			write(STDOUT_FILENO, "\n", 2);
			return myrnto(cmd_buff, from);	//call rnto function
		}
	}
	sprintf(msg_buff, "550 %s: Can’t find such file or directory.\n", from);
	write(connfd, msg_buff, CONT_BUF);
	return -1;
}

// myquit
//=====================================================
// Input	: none
// Output	: none
// Purpose	: quit 
void myquit()
{
	char msg_buff[CONT_BUF];
	bzero(msg_buff, sizeof(msg_buff));
	strcpy(msg_buff, "221 Goodbye.\n");
	write(connfd, msg_buff, CONT_BUF);	//send msg to client
}

// convert_str_to_addr
//=====================================================
// Input	: char * temp, unsigned int * port
// Output	: char * ipaddr
// Purpose	: convert string to ipaddr and portnum
char * convert_str_to_addr(char *temp, unsigned int * port)
{
    char * ipaddr;
    char * iptok;
    int ip1, ip2, ip3, ip4, po1, po2;

    //tokenize ipaddr
    iptok = strtok(NULL, ",");
    ip1 = atoi(iptok);  //ip1 = 127
    iptok = strtok(NULL, ",");
    ip2 = atoi(iptok);  //ip2 = 0
    iptok = strtok(NULL, ",");
    ip3 = atoi(iptok);  //ip3 = 0
    iptok = strtok(NULL, ",");
    ip4 = atoi(iptok);  //ip4 = 1

    ipaddr = (char * ) malloc (CONT_BUF);    //malloc
    sprintf(ipaddr, "%d.%d.%d.%d", ip1, ip2, ip3, ip4); //strcat

    iptok = strtok(NULL, ",");  //tokenize portnum
    po1 = atoi(iptok);
    po1 = po1 << 8;
    iptok = strtok(NULL, ",");
    po2 = atoi(iptok);
    *port = po1 + po2;

    return ipaddr;  //return ipaddr
}

// data_connection
//=====================================================
// Input	: char * host_ip, unsigned int port_num, char * cmd_buff
// Output	: char * ipaddr
// Purpose	: convert string to ipaddr and portnum
void data_connection(char * host_ip, unsigned int port_num, char * cmd_buff)
{
	int datafd;
	char * cmd = NULL;
	struct sockaddr_in data_addr;
	char msg_buff[CONT_BUF], result_buff[DATA_BUF], temp_buff[MAX_BUF];
	char total_buff[999999] = {0, };
	if((datafd = socket(PF_INET, SOCK_STREAM, 0)) < 0){	//create socket IPv4, Portnum
        write(STDERR_FILENO, "Server: Can't open stream socket\n", 34); //socket error
        exit(-1);	//exit
    }

    memset(&data_addr, 0, sizeof(data_addr));
    data_addr.sin_family = AF_INET;       //set server socket info
    data_addr.sin_addr.s_addr = inet_addr(host_ip);    //ipaddr
    data_addr.sin_port = htons(port_num);    //portno

    connect(datafd, (struct sockaddr*)&data_addr, sizeof(data_addr)); //data connection setup

	cmd = strtok(cmd_buff, " ");
	if(strcmp(cmd, "NLST") == 0)
	{
		// send msg
		bzero(msg_buff, sizeof(msg_buff));
		strcpy(msg_buff, "150 Opening data connection for directory list.\n");
		write(connfd, msg_buff, CONT_BUF);

		int optc = 1;
		char * optv[100];	//similar as argv
		optv[0] = "cmd";		//fill first argument
		optv[optc] = strtok(NULL, " \n");	//tokenize
		while(optv[optc] != 0){	//loop while no more argument
			optc++;	//count up
			optv[optc] = strtok(NULL, " \n");	//make ls, count option
		}
		bzero(temp_buff, sizeof(temp_buff));
		myls(optc, optv, temp_buff);	//ls function call

		// send result to client using data connection
		write(datafd, temp_buff, MAX_BUF);

		// send msg to client
		bzero(msg_buff, sizeof(msg_buff));
		strcpy(msg_buff, "226 Complete transmission.\n");
		write(connfd, msg_buff, CONT_BUF);

		// close data connection
		close(datafd);
	}
	else if(strcmp(cmd, "LIST") == 0)
	{
		// send msg
		bzero(msg_buff, sizeof(msg_buff));
		strcpy(msg_buff, "150 Opening data connection for directory list.\n");
		write(connfd, msg_buff, CONT_BUF);

		int optc = 1;
		char * optv[100];	//similar as argv
		optv[0] = "cmd";		//fill first argument
		optv[optc] = strtok(NULL, " \n");	//tokenize
		while(optv[optc] != 0){	//loop while no more argument
			optc++;	//count up
			optv[optc] = strtok(NULL, " \n");	//make ls, count option
		}
		bzero(temp_buff, sizeof(temp_buff));
		mydir(optc, optv, temp_buff);

		// send result to client using data connection
		write(datafd, temp_buff, MAX_BUF);

		// send msg to client
		bzero(msg_buff, sizeof(msg_buff));
		strcpy(msg_buff, "226 Complete transmission.\n");
		write(connfd, msg_buff, CONT_BUF);

		// close data connection
		close(datafd);
	}
	else if(strcmp(cmd, "RETR") == 0)	//get
	{
		// send msg
		bzero(msg_buff, sizeof(msg_buff));
		if(ascii_mode_set == 1){
			strcpy(msg_buff, "150 Opening ASCII mode data connection for directory list.\n");
		}
		else{
			strcpy(msg_buff, "150 Opening BINARY mode data connection for directory list.\n");
		}
		write(connfd, msg_buff, CONT_BUF);

		char *filename = NULL;
		filename = strtok(NULL, " ");
		char fullpath[512] = {0, };
		int n;

		// file path
		getcwd(fullpath, 512);
		strcat(fullpath, "/");
		strcat(fullpath, filename);
		write(connfd, filename, CONT_BUF);

		// open file
		int fd;
		if(ascii_mode_set == 1){
			fd = open(fullpath, O_RDONLY, ASCII_MODE);
		}
		else fd = open(fullpath, O_RDONLY, BIN_MODE);
		while(1){
			bzero(result_buff, sizeof(result_buff));
			read(fd, result_buff, DATA_BUF);
			result_buff[DATA_BUF] = '\0';
			write(datafd, result_buff, DATA_BUF);
			if(strlen(result_buff) < DATA_BUF){
				close(fd);
				break;
			}
			read(datafd, "MORE", 5);
		}

		// send msg to client
		bzero(msg_buff, sizeof(msg_buff));
		strcpy(msg_buff, "226 Complete transmission.\n");
		write(connfd, msg_buff, CONT_BUF);
		
		read(connfd, msg_buff, 5);
		
		close(datafd);
	}
	else if(strcmp(cmd, "STOR") == 0)
	{
		// send msg
		bzero(msg_buff, sizeof(msg_buff));
		if(ascii_mode_set == 1){
			strcpy(msg_buff, "150 Opening ASCII mode data connection for directory list.\n");
		}
		else{
			strcpy(msg_buff, "150 Opening BINARY mode data connection for directory list.\n");
		}
		write(connfd, msg_buff, CONT_BUF);

		char *filename = NULL;
		filename = strtok(NULL, " ");
		char fullpath[512] = {0, };
		int n;

		// file path
		getcwd(fullpath, 512);
		strcat(fullpath, "/");
		strcat(fullpath, filename);
		write(connfd, filename, CONT_BUF);

		// open file
		if(ascii_mode_set == 1){	//ascii mode
			char ascii_buff[2000] = {0,}; 
			int fd, len = 0;
			int i, temp_cnt = 0;
			fd = open(fullpath, FLAGS, ASCII_MODE);
			while(1){
				bzero(ascii_buff, sizeof(ascii_buff));
				bzero(result_buff, sizeof(result_buff));
				read(datafd, result_buff, DATA_BUF);
				result_buff[DATA_BUF] = '\0';

				//ASCII CONVERTING
				temp_cnt = 0;
				for(i=0; i<DATA_BUF; i++){
					if(result_buff[i] == '\r'){
						continue;
					}
					else{
						ascii_buff[temp_cnt] = result_buff[i];
						temp_cnt++;
					}
				}

				strcat(total_buff, ascii_buff);
				if(strlen(result_buff) < DATA_BUF){
					break;
				}
				write(datafd, "MORE", 5);   //ACK
			}
			write(fd, total_buff, strlen(total_buff));
			close(fd);
		}
		else {	//binary mode
			int fd, len = 0;
			fd = open(fullpath, FLAGS, BIN_MODE);
			while(1){
				bzero(result_buff, sizeof(result_buff));
				read(datafd, result_buff, DATA_BUF);
				result_buff[DATA_BUF] = '\0';
				write(fd, result_buff, DATA_BUF);
				if(strlen(result_buff) < DATA_BUF){
					break;
				}
				write(datafd, "MORE", 5);   //ACK
			}
			close(fd);
		}
		// send msg to client
		bzero(msg_buff, sizeof(msg_buff));
		strcpy(msg_buff, "226 Complete transmission.\n");
		write(connfd, msg_buff, CONT_BUF);

		close(datafd);
	}
}

// cmd_process
//=====================================================
// Input	: char * buff, char * result_buff
//			-> FTP CMD, process result
// Output	: if < 0 -> err occurs
// Purpose	: process about FTP COMMAND input -> ls, quit
int cmd_process(char *buff)
{
    char * cmd = NULL;
    cmd = strtok(buff, " \n");
    // command control
	if(strcmp(cmd, "PORT") == 0)	//PORT
	{
		int datafd, n;
		char *host_ip;
		unsigned int port_num;
		char cmd_buff[CONT_BUF], temp[CONT_BUF], msg_buff[CONT_BUF];
		bzero(cmd_buff, sizeof(cmd_buff));
		bzero(msg_buff, sizeof(msg_buff));
		
		// convert string to addr and portnumber
		host_ip = convert_str_to_addr(NULL, (unsigned int *) &port_num);

		// Send ACK
		if(host_ip == NULL || port_num < 0){
			strcpy(msg_buff, "550 Failed to access.\n");
			write(connfd, msg_buff, CONT_BUF);
			return -1;
		}
		strcpy(msg_buff, "200 PORT command performed successfully.\n");
		write(connfd, msg_buff, CONT_BUF);

		// recv FTP_CMD from client
		n = read(connfd, cmd_buff, CONT_BUF);
		cmd_buff[n] = '\0';
		write(STDOUT_FILENO, cmd_buff, strlen(cmd_buff));
		sprintf(temp, "\t\t[%d]", getpid());
		write(STDOUT_FILENO, temp, strlen(temp));
		write(STDOUT_FILENO, "\n", 2);

		// convert string to addr and portnumber
		data_connection(host_ip, port_num, cmd_buff);
	}
	else if(strcmp(cmd, "TYPE") == 0)	//TYPE
	{
		char *type = NULL;
		type = strtok(NULL, " ");
		char msg_buff[CONT_BUF];
		bzero(msg_buff, sizeof(msg_buff));
		if(strcmp(type, "A") == 0){		//ascii mode
			ascii_mode_set = 1;
			strcpy(msg_buff, "201 Type set to A.\n");
			write(connfd, msg_buff, CONT_BUF);
		}
		else if(strcmp(type, "I") == 0) {	//bin mode
			ascii_mode_set = 0;
			strcpy(msg_buff, "201 Type set to I.\n");
			write(connfd, msg_buff, CONT_BUF);
		}
		else{
			strcpy(msg_buff, "502 Type doesn't set.\n");
			write(connfd, msg_buff, CONT_BUF);
		}
	}
	else if(strcmp(cmd, "PWD") == 0)	//PWD
	{
		return mypwd();
	}
	else if(strcmp(cmd, "CWD") == 0)
	{
		int optc = 1;
		char * optv[100];	//similar as argv
		optv[0] = "cmd";		//fill first argument
		optv[optc] = strtok(NULL, " \n");	//tokenize
		while(optv[optc] != 0){	//loop while no more argument
			optc++;	//count up
			optv[optc] = strtok(NULL, " \n");	//make ls, count option
		}
		return mycwd(optc, optv);
	}
	else if(strcmp(cmd, "CDUP") == 0)
	{
		return mycdup();
	}
	else if(strcmp(cmd, "MKD") == 0)
	{
		int optc = 1;
		char * optv[100];	//similar as argv
		optv[0] = "cmd";		//fill first argument
		optv[optc] = strtok(NULL, " \n");	//tokenize
		while(optv[optc] != 0){	//loop while no more argument
			optc++;	//count up
			optv[optc] = strtok(NULL, " \n");	//make ls, count option
		}
		return mymkdir(optc, optv);
	}
	else if(strcmp(cmd, "DELE") == 0)
	{
		int optc = 1;
		char * optv[100];	//similar as argv
		optv[0] = "cmd";		//fill first argument
		optv[optc] = strtok(NULL, " \n");	//tokenize
		while(optv[optc] != 0){	//loop while no more argument
			optc++;	//count up
			optv[optc] = strtok(NULL, " \n");	//make ls, count option
		}
		return mydelete(optc, optv);
	}
	else if(strcmp(cmd, "RMD") == 0)
	{
		int optc = 1;
		char * optv[100];	//similar as argv
		optv[0] = "cmd";		//fill first argument
		optv[optc] = strtok(NULL, " \n");	//tokenize
		while(optv[optc] != 0){	//loop while no more argument
			optc++;	//count up
			optv[optc] = strtok(NULL, " \n");	//make ls, count option
		}
		return myrmdir(optc, optv);
	}
	else if(strcmp(cmd, "RNFR") == 0)
	{
		int optc = 1;
		char * optv[100];	//similar as argv
		optv[0] = "cmd";		//fill first argument
		optv[optc] = strtok(NULL, " \n");	//tokenize
		while(optv[optc] != 0){	//loop while no more argument
			optc++;	//count up
			optv[optc] = strtok(NULL, " \n");	//make ls, count option
		}
		myrnfr(optc, optv);
		return 0;
	}
	else{	//if wrong command
		return -1;
	}
}

// user_match
//=====================================================
// Input	: char * user, char * passwd
// Output	: 1 : matched
//            0 : not matched
// Purpose	: user match from passwd file
int user_match(char *user)
{
    FILE *fp;
    struct passwd *pw;
    int matched = 1;    //matched flag

    fp = fopen("passwd", "r");  //open passwd file
    while((pw = fgetpwent(fp)) != NULL) {   //get element from passwd file
        matched = 1;
        if(strcmp(pw->pw_name, user) != 0){ //user name equals?
            matched = 0;    //if not -> 0
        }
        else {   //if user, pwd matched
            fclose(fp);
            return matched; //return matched
        }
    }
    fclose(fp);
    return matched;   //return 0
}

// passwd_match
//=====================================================
// Input	: char * passwd
// Output	: 1 : matched
//            0 : not matched
// Purpose	: passwd match from passwd file
int passwd_match(char *passwd)
{
    FILE *fp;
    struct passwd *pw;
    int matched = 1;    //matched flag

    fp = fopen("passwd", "r");  //open passwd file
    while((pw = fgetpwent(fp)) != NULL) {   //get element from passwd file
        matched = 1;
        if(strcmp(pw->pw_passwd, passwd) != 0){ //pwd equals?
            matched = 0;    //if not -> 0
        }
        else {   //if user, pwd matched
            fclose(fp);
            return matched; //return matched
        }
    }
    fclose(fp);
    return matched;   //return 0
}

// log_auth
//=====================================================
// Input	: int client_fd
// Output	: 1 : login-success
//            0 : disconnection
// Purpose	: login
int log_auth(int connfd)
{
    char user[CONT_BUF], passwd[CONT_BUF];
    int n=0, count=1;
    char printbuf[CONT_BUF], *temp_user;
    char * tok;
    while(1){
        // Authentication for USERNAME
        bzero(printbuf, sizeof(printbuf));
        read(connfd, user, CONT_BUF);    //username
        write(STDOUT_FILENO, user, strlen(user));
        tok = strtok(user, " ");
        temp_user = strtok(NULL, " ");
        temp_user[strlen(temp_user) - 1] = '\0';
        if((n = user_match(temp_user)) == 1){
            sprintf(printbuf, "331 Password is required for %s.", temp_user);
            write(connfd, printbuf, CONT_BUF);
        }
        else{
            sprintf(printbuf, "** User is trying to log-in (%d/3) **\n", count);
            write(STDOUT_FILENO, printbuf, CONT_BUF);
            if(count >= 3){
                //3 times fail -> DISCONNECTION
                write(connfd, "530 Failed to log-in", CONT_BUF);
                return 0;
            }
            write(connfd, "430 Invalid username or password.", CONT_BUF);
            count++;
            continue;
        }

        // Authentication for PASSWORD
        bzero(printbuf, sizeof(printbuf));
        read(connfd, passwd, CONT_BUF);  //password
        write(STDOUT_FILENO, passwd, strlen(passwd));
        tok = strtok(passwd, " ");
        tok = strtok(NULL, " ");
        tok[strlen(tok) - 1] = '\0';
        if((n = passwd_match(tok)) == 1){
            sprintf(printbuf, "230 User %s logged in.", temp_user);
            write(connfd, printbuf, CONT_BUF);
            break;
        }
        else{
            sprintf(printbuf, "** User is trying to log-in (%d/3) **\n", count);
            write(STDOUT_FILENO, printbuf, strlen(printbuf));
            if(count >= 3){
                //3 times fail -> DISCONNECTION
                write(connfd, "530 Failed to log-in.", CONT_BUF);
                return 0;
            }
            write(connfd, "430 Invalid username or password", CONT_BUF);
            count++;
            continue;
        }
    }
    return 1;
}

// my_fnmatch
//=====================================================
// Input	: FILE * fp_checkIP, char * IP address
// Output	: -1 : not found
//              0 : matched
// Purpose	: check ipaddress whether it is authenticated
int my_fnmatch(FILE * fp_checkIP, char * ipaddr)
{
    char temp_ip[20], ipbuf[10];
    char * dot_token;
    int i=0;
    int diff_flag = 0;
    while(fgets(temp_ip, sizeof(temp_ip), fp_checkIP)){
        temp_ip[strlen(temp_ip) - 1] = '\0';    //delete lf

        //token by dot
        for(int j=0; '.' != ipaddr[i]; j++){
            ipbuf[j] = ipaddr[i++];
        }
        dot_token = strtok(temp_ip, ".");   //first part
        if(strcmp(dot_token, "*") != 0){    //wild card -> skip
            if(strcmp(dot_token, ipbuf) != 0){  //else compare
                diff_flag += 1;
            }
        }
        bzero(ipbuf, sizeof(ipbuf));

        i++;
        for(int j=0; '.' != ipaddr[i]; j++){
            ipbuf[j] = ipaddr[i++];
        }
        dot_token = strtok(NULL, ".");   //second part
        if(strcmp(dot_token, "*") != 0){    //wild card -> skip
            if(strcmp(dot_token, ipbuf) != 0){  //else compare
                diff_flag += 2;
            }
        }
        bzero(ipbuf, sizeof(ipbuf));

        i++;
        for(int j=0; '.' != ipaddr[i]; j++){
            ipbuf[j] = ipaddr[i++];
        }
        dot_token = strtok(NULL, ".");   //third part
        if(strcmp(dot_token, "*") != 0){    //wild card -> skip
            if(strcmp(dot_token, ipbuf) != 0){  //else compare
                diff_flag += 3;
            }
        }
        bzero(ipbuf, sizeof(ipbuf));

        i++;
        for(int j=0; i<strlen(ipaddr); j++){ //fourth part
            ipbuf[j] = ipaddr[i++];
        }
        dot_token = strtok(NULL, ".\n");
        if(strcmp(dot_token, "*") != 0){    //wild card -> skip
            if(strcmp(dot_token, ipbuf) != 0){  //else compare
                diff_flag += 4;
            }
        }
        bzero(ipbuf, sizeof(ipbuf));

        if(diff_flag == 0){ //matched
            return 0;
        }

        i = 0;
        diff_flag = 0;
        dot_token = NULL;
        bzero(temp_ip, sizeof(temp_ip));    //initiate buffers
    }
    return -1;  //not matched
}

// client_info
//=====================================================
// Input	: struct sockaddr_in *client_addr
// Output	: -1 : error
//              0 : done
// Purpose	: print client information, IP ADDR, PORTNO
int client_info(struct sockaddr_in *client_addr)
{
    char *ip_addr = inet_ntoa(client_addr->sin_addr);	
	char portnum[5] = {0, };
	
	sprintf(portnum, "%d", client_addr->sin_port);
	write(STDOUT_FILENO, "==========Client info==========\n", 33);
    write(STDOUT_FILENO, "client IP: ", 12);
    write(STDOUT_FILENO, ip_addr, strlen(ip_addr));

    write(STDOUT_FILENO, "\n\nclient port: ", 16);
    write(STDOUT_FILENO, portnum,  strlen(portnum));
    write(STDOUT_FILENO, "\n===============================\n", 34);
	return 0;
}

// child_rearrange
//=====================================================
// Input	: int die_pid -> deleted pid num
// Output	: none
// Purpose	: if child process deleted, rearrange struct array
void child_rearrange(int die_pid)
{
	int index = 0;
	for(int i=0; i<child_count; i++){
		if(die_pid == info[i].pid){	//search deleted pid and it's target
			index = i;
		}
	}
	for(int i=index; i<child_count; i++){	//pull forward one by one
		info[i].num = info[i+1].num;
		info[i].pid = info[i+1].pid;
		info[i].port = info[i+1].port;
		info[i].time = info[i+1].time;
	}
	info[child_count -1].num = 0;	//and set zero : last index
	info[child_count -1].pid = 0;
	info[child_count -1].port = 0;
	info[child_count -1].time = 0;
}

/* when SIGCHILD occurs */
void sh_chld(int signum)
{
    char printbuf[CONT_BUF] = {0,};
	int die_pid;
	die_pid = wait(NULL); //wait till child process terminates
	child_rearrange(die_pid);	//rearrange
	child_count--;	//count down
    sprintf(printbuf, "Client [%d]'s Release\n", die_pid);
    write(STDOUT_FILENO, printbuf, strlen(printbuf));
}

/* when SIGINT occurs */
void sh_int(int signum)
{
	char msg_buff[CONT_BUF];
	bzero(msg_buff, sizeof(msg_buff));
	strcpy(msg_buff, "221 Goodbye.\n");
	write(connfd, msg_buff, CONT_BUF);
	close(connfd); //close all the child process
	exit(0);
}

/* when SIGALRM occurs */
void sh_alrm(int signum)
{
	alarm(10);	//re-alarm
    if(child_count > 0){
        printf("Current Number of Client : %d\n", child_count);	//prints child amount
        write(STDOUT_FILENO, "PID\tPORT\tTIME\n", 15);
        for(int i=0; i<child_count; i++)	//print info
        {
            printf("%d\t", info[i].pid);
            printf("%d\t", info[i].port);
            printf("%ld\n", time(NULL) - info[i].time);
        }
    }
}

// main
//=====================================================
// Input	: int argc, char *argv[]
// Output	: none
// Purpose	: server -> ip, id/pw authentication
int main(int argc, char *argv[])
{
    int clilen, pid, n, status;
    struct sockaddr_in servaddr, cliaddr;
    FILE *fp_checkIP;
    char msg_buff[CONT_BUF], cmd_buff[CONT_BUF], temp[CONT_BUF];

    if(argc != 2){	//wrong input format
		write(STDERR_FILENO, "Server: wrong input format\n", 28);
		exit(1);	//exit
	}

    if((sockfd = socket(PF_INET, SOCK_STREAM, 0)) < 0){	//create socket IPv4, Portnum
        write(STDERR_FILENO, "Server: Can't open stream socket\n", 34); //socket error
        exit(-1);	//exit
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;       //set server socket info
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);    //ipaddr
    servaddr.sin_port = htons(atoi(argv[1]));    //portno
    
    //bind
    if(bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)	//bind
    {
		write(STDERR_FILENO, "Server: Cant bind local address.\n", 34);	//bind error
        exit(0);	//exit
    }

    //listen: MAX concurrent client = 5
    listen(sockfd, 5);

    for(;;){    //child
        clilen = sizeof(cliaddr);
        connfd = accept(sockfd, (struct sockaddr *) &cliaddr, &clilen); //accept
        alarm(1);   //alarm start
        //error occurs with accept
        if(connfd < 0){
			write(STDERR_FILENO, "Server: accept failed.\n", 24);	//client accept error
       		exit(0);
        }
        if(client_info(&cliaddr) < 0)	//print client information
            write(STDERR_FILENO, "client_info() err!!\n", 21);

        pid = fork();
        child_count++;
        if(pid == 0){
            // Authentication : IP ADDRESS
            if(fp_checkIP = fopen("access.txt", "r")){  //file open
                if(my_fnmatch(fp_checkIP, inet_ntoa(cliaddr.sin_addr)) == -1){   //fail
                    write(STDOUT_FILENO, "** It is NOT authenticated client **\n", 38);
                    write(connfd, "431 This client can’t access. Close the session.", CONT_BUF);    //send rejection signal
                    close(connfd);
                    fclose(fp_checkIP); //close file access.txt
                    continue;
                }
                else{   //success
                    write(STDOUT_FILENO, "** Client is connected **\n", 27);
                    write(connfd, "220 sswlab.kw.ac.kr FTP server (version myftp [1.0] Fri May 30 14:40:36 KST 2014) ready.", CONT_BUF); //send accepted signal
                }
                fclose(fp_checkIP); //close file access.txt
            }
            else{   //file open failed
                write(connfd, "431 This client can’t access. Close the session.", CONT_BUF); //File Open Fail
                continue;
            }

            // Authentication : Login
            if(log_auth(connfd) == 0){  //log authentication function call
                write(STDERR_FILENO, "** Fail to log-in **\n", 22);
                exit(0);
            }
            write(STDOUT_FILENO, "** Success to log-in **\n", 25);

            // CMD Process
			signal(SIGINT, sh_int);	//SIGINT occurs
            while(1){
                bzero(cmd_buff, sizeof(cmd_buff));  //clear buffer
				bzero(temp, sizeof(temp));
                if((n = read(connfd, cmd_buff, CONT_BUF)) < 0){  //read command from client
                    write(STDERR_FILENO, "read() error\n", 14); //if read error
					//write(connfd, "read() error\n", 14); //if read error
					continue;
                }

				// if command is QUIT -> close connection, terminate child process
                if(strcmp(cmd_buff, "QUIT") == 0){
					cmd_buff[n] = '\0';
                    write(STDOUT_FILENO, cmd_buff, strlen(cmd_buff));
					sprintf(temp, "\t\t[%d]", getpid());
					write(STDOUT_FILENO, temp, strlen(temp));
                    write(STDOUT_FILENO, "\n", 2);
					myquit();
					exit(0);	//terminate
                }

                // FTP command prcessing
				cmd_buff[n] = '\0';
				write(STDOUT_FILENO, cmd_buff, strlen(cmd_buff));
				sprintf(temp, "\t\t[%d]", getpid());
				write(STDOUT_FILENO, temp, strlen(temp));
				write(STDOUT_FILENO, "\n", 2);
				cmd_process(cmd_buff);	//call processing function
				
            }
        }
        else{   //parent
            info[child_count - 1].num = child_count - 1;
			info[child_count - 1].pid = pid;
			info[child_count - 1].port = servaddr.sin_port;
			info[child_count - 1].time = time(NULL);
			signal(SIGALRM, sh_alrm);
            signal(SIGCHLD, sh_chld);   //when child signal occurs
        }
    
        
    }
}