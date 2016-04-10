#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <strings.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include <string.h>
#include <dirent.h>

#define MAXLINE 2048
#define LISTENQ 1024

void showMenu(int clifd, char *sendline);
void showMenu_mini(int clifd, char *sendline);
void listDir(int clifd, char *sendline);
void transFileTo(int sockfd, FILE *fp, int fileSize, char *sendline);
void receiveFileFrom(int sockfd, FILE *fp, int fileSize, char *recvline);
ssize_t writen(int fd, const void *vptr, size_t n);
void hw1_service(int clifd);

void handler(int signo)
{
	pid_t pid;
	int stat;

	for(;;){
		pid = waitpid(WAIT_ANY, &stat, WCONTINUED);
		if(pid == -1) break;
		//printf("stat:%d, pid: %d terminated.\n", stat, pid);
	}
	
	/*pid = wait(&stats);
	printf("pid==%d\n", pid);
	*/
}

int main(int argc, char **argv)
{
	if(argc!=2){
		fprintf(stderr, "Usage: ./<executable file> <port>");
		exit(9999);
	}
	int listenfd, connfd;
	struct sockaddr_in cliaddr, servaddr;

	struct stat st = {0};
	if(stat("./Upload", &st) == -1){
		mkdir("./Upload", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	}

	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(atoi(argv[1]));
	memset(servaddr.sin_zero, 0, sizeof(servaddr.sin_zero));

	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr));
	listen(listenfd, LISTENQ);
	signal(SIGCHLD, handler);

	for(;;)
	{
		socklen_t clilen = sizeof(cliaddr);
		connfd = accept(listenfd, (struct sockaddr *) &cliaddr, &clilen);

		if(fork()==0)	/* child process */
		{
			close(listenfd);
			int cliPort = ntohs(cliaddr.sin_port);
			char cliAddrStr[INET_ADDRSTRLEN];
			inet_ntop(AF_INET, &cliaddr.sin_addr, cliAddrStr, INET_ADDRSTRLEN);
			printf("Connection from %s, port: %d established\n", cliAddrStr, cliPort);
			hw1_service(connfd);
			printf("Connection from %s, port: %d terminated\n", cliAddrStr, cliPort);
			exit(0);
		}
	}
}
void hw1_service(int clifd)
{
	int n;
	char recvline[MAXLINE+1];
	char sendline[MAXLINE+1];
	char message[MAXLINE+1];
	char garbage[10];
	showMenu(clifd, sendline);
	while( (n=read(clifd, recvline, MAXLINE)) > 0 )
	{
		//fprintf(stdout, "%s\n", recvline);
		char command[MAXLINE];
		sscanf(recvline, "%s", command);
		//fprintf(stdout, "%s\n", command);
		if(strcmp(command, "cd")==0){
			char changePath[200];
			sscanf(recvline, "cd %s", changePath);
			if( chdir(changePath) < 0 ){
				perror("chdir error");
				sprintf(message, "cd failed");
			}
			else{
				char cwd[200];
				if(getcwd(cwd, sizeof(cwd))==NULL) perror("getcwd error in showMenu()");
				sprintf(message, "successfully cd to:");
				strcat(message, cwd);
				strcat(message, "\n");
			}
			n = strlen(message);
			message[n] = '\0';
			write(clifd, message, n+1 );
		}

		else if (strcmp(command, "ls")==0){
			listDir(clifd, sendline);
		}

		else if (strcmp(command, "upload")==0){
			char fileName[200];
			sscanf(recvline, "upload %s", fileName);
			FILE *fileToUpload;
			if( (fileToUpload = fopen(fileName, "wb")) == NULL){
				perror("fopen file to upload error");
				sprintf(message, "fail to upload\n");
			} else {
				int fileSize;

				write(clifd, " ", 1);
				read(clifd, recvline, MAXLINE);
				sscanf(recvline, "%d", &fileSize);
				//fprintf(stdout, "%s\n", sendline);

				write(clifd, " ", 1);
				receiveFileFrom(clifd, fileToUpload, fileSize, recvline);
				fclose(fileToUpload);
				sprintf(message, "success to upload\n");
			}
			write(clifd, message, strlen(message));
		}

		else if (strcmp(command, "download")==0){
			char fileName[200];
			sscanf(recvline, "download %s", fileName);
			FILE *fileToDownload;
			if( (fileToDownload = fopen(fileName, "rb")) == NULL){
				perror("fopen file to download error");
				sprintf(message, "Fail to open file %s", fileName);
			} else {
				int fileSize;
				fseek(fileToDownload, 0L, SEEK_END);
				fileSize = ftell(fileToDownload);
				rewind(fileToDownload);
				sprintf(sendline, "%d", fileSize);
				write(clifd, sendline, strlen(sendline));
				//fprintf(stdout, "%s\n", sendline);
				read(clifd, garbage, sizeof(garbage));

				transFileTo(clifd, fileToDownload, fileSize, sendline);
				read(clifd, garbage, sizeof(garbage));
				fclose(fileToDownload);

				sprintf(message, "Download Complete!");
			}
			n = strlen(message);
			message[n] = '\0';
			write(clifd, message, n+1);
		}

		else if (strcmp(command, "exit")==0){
			return;
		}

		else {
			fprintf(stdout, "Client entered a invalid command\n");
			// then discard the content of this recvline
		}
		read(clifd, recvline, 17);
		//fprintf(stdout, "%s\n", recvline);
		showMenu_mini(clifd, sendline);
	}
	if(n < 0) perror("read error in hw1_service()");
	//listDir(clifd, sendline);
}

void listDir(int clifd, char *sendline)
{
	DIR *dir;
	struct dirent *entry;
	char tempStr[MAXLINE];
	sendline[0] = '\0';

	if( (dir = opendir("."))==NULL ){
		perror("opendir in listdir()");
		return;
	}
    
	entry = readdir(dir);
	while(entry!=NULL)
	{
		if(entry->d_type == DT_DIR)
			sprintf(tempStr, " - %s/\n", entry->d_name);
		else
			sprintf(tempStr, " - %s\n", entry->d_name);

	 	strcat(sendline, tempStr);
			entry = readdir(dir);
	}

	int n = strlen(sendline);
	sendline[ n ] = '\n';
	sendline[ n+1 ] = '\0';
	write(clifd, sendline, n+1);
	return;
}

void showMenu(int clifd, char *sendline)
{
	sprintf(sendline, "\n------------ Five command for client ------------\n");
	strcat(sendline, " (1) \"cd <dir>\" : to change current directory\n");
	strcat(sendline, " (2) \"ls\"       : to list all dir and files on current dir\n");
	strcat(sendline, " (3) \"upload <file name>\"   : to upload file to current dir\n");
	strcat(sendline, " (4) \"download <file name>\" : to download file from current dir\n");
	strcat(sendline, " (5) \"exit\"     : to terminate connection\n");
	strcat(sendline, "-------------------------------------------------\n");
	
	char cwd[200];
	if(getcwd(cwd, sizeof(cwd))==NULL)
		perror("getcwd error in showMenu()");
	strcat(sendline, "client@server:");
	strcat(sendline, cwd);
	strcat(sendline, "$ ");
	
	int n = strlen(sendline);
	sendline[n] = '\0';
	write(clifd, sendline, n+1);
}

void showMenu_mini(int clifd, char *sendline)
{
	sprintf(sendline, "\n------------ Five command for client ------------\n");
	strcat(sendline, " (1) \"cd\"  ");
	strcat(sendline, " (2) \"ls\"  ");
	strcat(sendline, " (3) \"upload\"  ");
	strcat(sendline, " (4) \"download\"  ");
	strcat(sendline, " (5) \"exit\"\n");
	strcat(sendline, "-------------------------------------------------\n");
	
	char cwd[200];
	if(getcwd(cwd, sizeof(cwd))==NULL)
		perror("getcwd error in showMenu()");
	strcat(sendline, "client@server:");
	strcat(sendline, cwd);
	strcat(sendline, "$ ");
	
	int n = strlen(sendline);
	sendline[n] = '\0';
	write(clifd, sendline, n+1);
}

void transFileTo(int sockfd, FILE *fp, int fileSize, char *sendline)
{
	int numBytes;
	while(fileSize > 0)
	{
		numBytes = fread(sendline, sizeof(char), MAXLINE, fp);
		numBytes = write(sockfd, sendline, numBytes);
		fileSize -= numBytes;

		//fprintf(stdout, "!!!\n%s\n!!!", sendline);
		//fprintf(stdout, "%d\n", numBytes);
	}
	//fprintf(stdout, "transfer finish\n");
}
void receiveFileFrom(int sockfd, FILE *fp, int fileSize, char *recvline)
{
	int numBytes;
	while(fileSize > 0)
	{
		numBytes = read(sockfd, recvline, MAXLINE);
		numBytes = fwrite(recvline, sizeof(char), numBytes, fp);
		fileSize -= numBytes;
		//fprintf(stdout, "!!!\n%s\n!!!", recvline);
		//fprintf(stdout, "%d\n", numBytes);

	}
	//fprintf(stdout, "receive finish\n");
}