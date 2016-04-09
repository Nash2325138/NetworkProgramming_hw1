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

#include <string.h>
#include <sys/stat.h>

#define MAXLINE 2048


void read_print(int servfd, char* recvline);
void transFileTo(int sockfd, FILE *fp, int fileSize, char *sendline);
void receiveFileFrom(int sockfd, FILE *fp, int fileSize, char *recvline);

int main (int argc, char **argv)
{
	
	int servfd, n;
	char recvline[MAXLINE+1];
	char sendline[MAXLINE+1];
	char garbage[10];
	struct sockaddr_in servaddr;
	if(argc!=3){
		fprintf(stderr, "Usage: ./<executable file> <server IP> <server port>\n");
		exit(9999);
	}

	if( (servfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ){
		perror("socket error");
		exit(9999);
	}

	struct stat st = {0};
	if(stat("./Download", &st) == -1){
		mkdir("./Download", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	}

	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(atoi(argv[2]));
	if( (inet_pton(AF_INET, argv[1], &servaddr.sin_addr)) <= 0 ){
		fprintf(stderr, "inet_pton error\n");
		exit(9999);
	}
	memset(servaddr.sin_zero, 0, sizeof(servaddr.sin_zero));

	if( connect(servfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0 ){
		perror("connect error");
		exit(9999);
	}

	/*while( (n=read(servfd, recvline, MAXLINE)) > 0){
		recvline[n] = '\0';
		if(fputs(recvline, stdout)==EOF){
			perror("fputs error");
			exit(9999);
		}
	}*/

	read_print(servfd, recvline);

	// A terminating null character is automatically appended to sendline after fgets().
	while( fgets(sendline, MAXLINE, stdin) != NULL)
	{
		write(servfd, sendline, strlen(sendline));
		
		char command[MAXLINE];
		sscanf(sendline, "%s", command);
		if(strcmp(command, "cd")==0){
			read_print(servfd, recvline);
		}

		else if(strcmp(command, "ls")==0){
			read_print(servfd, recvline);
		}

		else if(strcmp(command, "upload")==0){

		}

		else if(strcmp(command, "download")==0){
			char fileName[200];
			char fullPath[210];
			sscanf(sendline, "download %s", fileName);
			strcpy(fullPath, "./Download/");
			strcat(fullPath, fileName);
			FILE *fileToDownload;
			if( (fileToDownload = fopen(fullPath, "wb")) == NULL){
				perror("fopen file to download error");
			} else {
				int fileSize;
				n = read(servfd, recvline, MAXLINE);
				recvline[n] = '\0';
				sscanf(recvline, "%d", &fileSize);
				fprintf(stdout, "%s\n", recvline);
				write(servfd, " ", 1);

				receiveFileFrom(servfd, fileToDownload, fileSize, recvline);
				write(servfd, " ", 1);
				fclose(fileToDownload);

				read_print(servfd, recvline);
			}
		}

		else if(strcmp(command, "exit")==0){
			return 0;
		}

		else {
			fprintf(stdout, "Please enter a valid command\n");
		}
		write(servfd, "message received\0", 17);
		read_print(servfd, recvline);
		//if(strncmp(recvline, "\n------", 7)!=0) read_print(servfd, recvline);
	}

	return 0;
}

void read_print(int servfd, char* recvline)
{
	int n = read(servfd, recvline, MAXLINE);
	if(n > 0){
		recvline[n] = 0;
		if(fputs(recvline, stdout)==EOF){
			perror("fputs error");
			//exit(9999);
		}
	} else {
		perror("read_print error");
		//exit(9999);
	}
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
	fprintf(stdout, "transfer finish\n");
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
	fprintf(stdout, "receive finish\n");
}
