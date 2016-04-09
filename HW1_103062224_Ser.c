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

void showMenu(int sockfd, char *sendline);
void listDir(int sockfd, char *sendline);
ssize_t writen(int fd, const void *vptr, size_t n);
void hw1_service(int sockfd);

void handler(int signo)
{
	pid_t pid;
	int stat;

	for(;;){
		pid = waitpid(-1, &stat, WCONTINUED);
		if(pid == -1) break;
		printf("pid: %d terminated.\n", pid);
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

	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(atoi(argv[1]));
	memset(servaddr.sin_zero, 0, sizeof(servaddr.sin_zero));

	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr));
	listen(listenfd, LISTENQ);

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
void hw1_service(int sockfd)
{
	int n;
	char recvline[MAXLINE+1];
	char sendline[MAXLINE+1];
	struct stat st = {0};
	if(stat("./Upload", &st) == -1){
		mkdir("./Upload", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	}
	showMenu(sockfd, sendline);
	while( (n=read(sockfd, recvline, MAXLINE)) > 0 )
	{
		char command[MAXLINE];
		sscanf(recvline, "%s", command);
	}
	if(n < 0) perror("read error in hw1_service()");
	//listDir(sockfd, sendline);
}

void listDir(int sockfd, char *sendline)
{
    DIR *dir;
    struct dirent *entry;
    char tempStr[MAXLINE];

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
    write(sockfd, sendline, n+1);
    return;
}

void showMenu(int sockfd, char *sendline)
{
	sprintf(sendline, "------------ Five command for client ------------\n");
	strcat(sendline, " (1) \"cd <dir>\" tp change current directory\n");
	strcat(sendline, " (2) \"ls\" to list all dir and files on current dir\n");
	strcat(sendline, " (3) \"upload <file name>\" to upload file to current dir\n");
	strcat(sendline, " (4) \"download <file name>\" to download file from current dir\n");
	strcat(sendline, " (5) \"exit\" to terminate connection\n");
	strcat(sendline, "-------------------------------------------------\n\n");
	
	char cwd[200];
	if(getcwd(cwd, sizeof(cwd))==NULL)
		perror("getcwd in showMenu():");
	strcat(sendline, "client@server: ");
	strcat(sendline, cwd);
	strcat(sendline, "$ ");
	
	int n = strlen(sendline);
	sendline[n] = '\0';
	write(sockfd, sendline, n);
}

/*
int main(int argc, char const *argv[])
{
	int sockfd, connfd, listenfd, maxfd;
	int i, maxi;
	int nready, client[FD_SETSIZE];
	ssize_t n;
	fd_set rset, allset;
	char line[MAXLINE];
	socklen_t clilen;
	struct sockaddr_in cliaddr, servaddr;

	if( (listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ){
		printf("socket error\n");
	}

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(SERV_PORT);

	if( (bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr))) < 0){
		printf("bind error\n");
	}

	if( listen(listenfd, 1024) < 0){ // define LISTENQ 1024 (backlog)
		printf("listen error\n");
	}

	maxfd = listenfd;
	maxi = -1;
	for(i=0 ; i < FD_SETSIZE ; i++){
		client[i] = -1;
	}

	FD_ZERO(&allset);
	FD_SET(listenfd, &allset);

	while(1)
	{
		rset = allset;
		nready = select(maxfd+1, &rset, NULL, NULL, NULL);

		if(FD_ISSET(listenfd, &rset)){
			clilen = sizeof(cliaddr);
			if( (connfd = accept(listenfd, (struct sockaddr *) &cliaddr, &clilen)) < 0 ){
				printf("accept error\n");
			} else {
				printf("accept one connection\n");
			}

			for(i=0 ; i<FD_SETSIZE ; i++){
				if(client[i] < 0){
					client[i] = connfd;
					break;
				}
			}

			if(i >= FD_SETSIZE) printf("Too many clients\n");

			FD_SET(connfd, &allset);
			if(connfd > maxfd) maxfd = connfd;
			if(i > maxi) maxi = i;
			nready--;
			if(nready <= 0) continue;
		}

		for(i=0 ; i<maxi ; i++){
			if( client[i] < 0 ) continue;

			sockfd = client[i];

			if(FD_ISSET(sockfd, &rset)){
				n = read(sockfd, line, MAXLINE);
				if(n > 0) printf("read something");
				if(n==0){
					close(sockfd);
					FD_CLR(sockfd, &allset);
					client[i] = -1;
				} else {
					writen(sockfd, line, n);
				}
				nready--;
				if(nready <= 0) break;
			}
		}


	}
	
	return 0;
}
*/

/* Write "n" bytes to a descriptor. */
ssize_t writen(int fd, const void *vptr, size_t n)
{
	size_t nleft;
	ssize_t nwritten;
	const char *ptr;
	ptr = (const char *)vptr;
	nleft = n;
	while (nleft > 0) {
		if ( (nwritten = write(fd, ptr, nleft)) <= 0) {
			if (nwritten < 0 && errno == EINTR)
				nwritten = 0; /* and call write() again */
			else
				return (-1); /* error */
		}
		nleft -= nwritten;
		ptr += nwritten;
 	}
	return (n);
}