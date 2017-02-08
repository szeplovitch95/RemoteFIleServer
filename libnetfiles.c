#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<netdb.h>
#include<fcntl.h>
#include<sys/stat.h>
#include<errno.h>

int inited = 0;
char * host = NULL;
int portno = 7659;
int mode = 0; //ext A. sets mode

int netserverinit(char * hostname , int filemode) {

	struct hostent *server;
	server = gethostbyname(hostname);

	if (server->h_name == NULL) {

		fprintf(stderr, "ERROR : No such host can be found.\n");
		errno = HOST_NOT_FOUND;
		return -1;

	}

	host = (char *)malloc((strlen(hostname) * sizeof(char)) + 1);

	host = strcpy(host , hostname);

	inited = 1;

	mode = filemode; 

	//printf("Initialized\n");

        return 0;

}

//all functions except netserverinit make a connection with the server and send their arguments over as a string

int netopen(const char* pathname , int flags) {

	if(inited == 0) { //they won't run unless the server was initiated

		return -1;

	}

	int sockfd;
        int n;
        struct sockaddr_in serv_addr;
        struct hostent *server;

        char buffer[256];

	sockfd = socket(AF_INET, SOCK_STREAM, 0);

        if (sockfd < 0) {

                printf("ERROR : Unable to open socket\n");
		return -1;

        }

        server = gethostbyname(host);
	
	bzero((char *) &serv_addr, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
        serv_addr.sin_port = htons(portno);

        if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {

                printf("ERROR : Unable to connect\n");
		errno = EINTR;
		return -1;

        }
	
	bzero(buffer,256);

	buffer[0] = '1';
	buffer[1] = ',';
	strcat(buffer , pathname);
	buffer[strlen(buffer)] = ',';
	char buff2[3];
	sprintf(buff2 , "%d" , flags);
	strcat(buffer , buff2);         
	buffer[strlen(buffer)] = ',';
	char mBuff[3];
	sprintf(mBuff , "%d" , mode); //ext. A the mode is sent over in open
	//printf("%s\n" , mBuff);
	strcat(buffer , mBuff);         

	write(sockfd , buffer , strlen(buffer));

	read(sockfd , buffer , 256); //all functions also read back what the server sent them to see if there were any errors and get output 
	
	close(sockfd);

	if(strcmp(buffer, "-1,enoent") == 0) {

		errno = ENOENT;
		return -1;

	} else if(strcmp(buffer , "-1,eacces") == 0) {

		errno = EACCES;
		return -1;

	} else if(strcmp(buffer , "-1,erofs") == 0) {

		errno = EROFS;
		return -1;

        } else if(strcmp(buffer , "-1,eisdir") == 0) {

		errno = EISDIR;
		return -1;

        }



	if(strcmp(buffer , "-1") == 0) {

		return -1;

	}		

	//printf("Opened\n");

	int fileAd = atoi(buffer);

	return fileAd;

}

ssize_t netread(int fildes , void * buf , size_t nbyte) {

	if(inited == 0) { //i forgot to add these error checks

		return -1;

	}

	int sockfd;
        int n;
        struct sockaddr_in serv_addr;
        struct hostent *server;

        char buffer[256];

        sockfd = socket(AF_INET, SOCK_STREAM, 0);

        if (sockfd < 0) {

                printf("ERROR : Unable to open socket\n");
                return -1;

        }

        server = gethostbyname(host);

        bzero((char *) &serv_addr, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
        serv_addr.sin_port = htons(portno);

        if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {

                printf("ERROR : Unable to connect\n");
                return -1;

        }

        bzero(buffer,256);

	buffer[0] = '2';
        buffer[1] = ',';
	char dest[10];
	sprintf(dest , "%d" , fildes);
        strcat(buffer , dest);
	buffer[strlen(buffer)] = ',';
        char buff2[10];
        sprintf(buff2 , "%d" , nbyte);
        strcat(buffer , buff2);

        write(sockfd , buffer , strlen(buffer));

        read(sockfd , buf , 256); //what was read from the file was sent from the server to the client

	if(strcmp(buf , "-1,ebadf") == 0) {

		errno = EBADF;
		buf = NULL;
		return -1;

	}

	if(strcmp(buf , "-1") == 0) {

		buf = NULL;
		return -1;

	}

	char * byteBuf = (char *)malloc(11);

	read(sockfd , byteBuf , 10);
	int nRead = atoi(byteBuf); //as was the number of bytes read

        close(sockfd);

        //printf("Read\n");

	return nRead;

}

ssize_t netwrite(int fildes , const void* buf , size_t nbyte) {

	if(inited == 0) {  //insert me

		return -1;

	}

	int sockfd;
        int n;
        struct sockaddr_in serv_addr;
        struct hostent *server;

        char buffer[256];

        sockfd = socket(AF_INET, SOCK_STREAM, 0);

        if (sockfd < 0) {

                printf("ERROR : Unable to open socket\n");
                return -1;

        }

        server = gethostbyname(host);

        bzero((char *) &serv_addr, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
        serv_addr.sin_port = htons(portno);

        if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {

                printf("ERROR : Unable to connect\n");
                return -1;

        }

        bzero(buffer,256);

	buffer[0] = '3';
        buffer[1] = ',';
        char dest[10];
        sprintf(dest , "%d" , fildes);
        strcat(buffer , dest);
        buffer[strlen(buffer)] = ',';
	strcat(buffer , buf);
	buffer[strlen(buffer)] = ',';
        char buff2[10];
        sprintf(buff2 , "%d" , nbyte);
        strcat(buffer , buff2);
        
	write(sockfd , buffer , strlen(buffer));

        char * byteBuf = (char *)malloc(11);

        read(sockfd , byteBuf , 10);

	if(strcmp(byteBuf , "-1,ebadf") == 0) {

                errno = EBADF;
                return -1;

        }

	if(strcmp(byteBuf , "-1") == 0) {

		return -1;

	}

        int nWrite = atoi(byteBuf); //the number of bytes written was sent back

        close(sockfd);

	return nWrite;

}

int netclose(int fd) {

	if(inited == 0) { //insert me

		return -1;

	}

	int sockfd;
        int n;
        struct sockaddr_in serv_addr;
        struct hostent *server;

        char buffer[256];

        sockfd = socket(AF_INET, SOCK_STREAM, 0);

        if (sockfd < 0) {

                printf("ERROR : Unable to open socket\n");
                return -1;

        }

        server = gethostbyname(host);

        bzero((char *) &serv_addr, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
        serv_addr.sin_port = htons(portno);

        if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {

                printf("ERROR : Unable to connect\n");
                return -1;

        }

        bzero(buffer,256);

	buffer[0] = '4';
        buffer[1] = ',';
        char dest[10];
        sprintf(dest , "%d" , fd);
        strcat(buffer , dest);

	write(sockfd , buffer , strlen(buffer));

	char returnVal[3];
	read(sockfd , returnVal , 20);

	if(strcmp(returnVal , "-1,ebadf") == 0) {

		errno = EBADF;
		return -1;

	}

	int returnValN = atoi(returnVal); //basically whether or not the file was closed is sent back

        close(sockfd);

	return returnValN;

}

