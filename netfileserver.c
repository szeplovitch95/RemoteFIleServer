#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<errno.h>
#include<fcntl.h>
#include<sys/stat.h>
#include<unistd.h>

struct FList { //this struct keeps track of all the files opened in the server

	FILE * fd;
	int setFlag;
	int mode;   //ext A. sets the mode for the file descriptor
	int writer; //ext A. is set if there is one writer and the file is in exclusive mode
	char * file;

};


struct thread_data { //a thread struct for multi-threading
	int id;
	int sock;
};

typedef struct FList FileListT; //An array of the previously mentioned struct
pthread_mutex_t file_mutex;
void *dostuff(void *);

FileListT FileList[sizeof(FileListT) * 200]; //set so the array is large enough
int currFile = 2;	//for file descriptor purposes the the index starts at 2

int is_reg_file(const char * path) { //a method to check for errors

	struct stat path_stat;
	stat(path , &path_stat);
	return S_ISREG(path_stat.st_mode);

}

int doesFileExist(const char * filename) { //a method to check if a file exists

	struct stat st;
	int result = stat(filename , &st);
	return result == 0;

}

int opener(int sock , char * arguments) { //a method for if the client want to open a file

	//printf("opening\n");

	char * path = (char *)malloc((sizeof(char) * strlen(arguments)));
	int j = 0;
	int i;
	for(i = 2 ; arguments[i] != ',' ; i++) {

		path[j] = arguments[i];
		j++;

	}
	path[j] = '\0';

	//printf("path found\n");

	int ret = access(path , F_OK);
	if(ret != 0) {

		if(errno == ENOENT) {

			write(sock , "-1,enoent" , 256);
			return -1;

		} else if(errno == EACCES) {

			write(sock , "-1,eacces" , 256);
                        return -1;


		}

	}

	ret = access(path , W_OK);
	if(ret != 0) {

		if(errno == EROFS) {

                        write(sock , "-1,erofs" , 256);
                        return -1;

                } else if(errno == EACCES) {

                        write(sock , "-1,eacces" , 256);
                        return -1;


                }
		

	}

	if(is_reg_file(path) == 0) {

		write(sock , "-1,eisdir" , 256);
		return -1;

	}

	//printf("errors checked\n");

	i++;
	char * theFlag;
	theFlag = &arguments[i];
	int inFlag = atoi(theFlag);

	//printf("flag found\n");

	i = i + 2;
	char * theMode;
	theMode = &arguments[i];
	int theModeN = atoi(theMode);

	//printf("mode found\n");

	FILE * iFile;

	pthread_mutex_lock(&file_mutex);	
	FileList[currFile].writer = 0; 
	pthread_mutex_unlock(&file_mutex);

	//printf("writer set\n");
	
	int k;  //this loop works as extention A, checking if the mode and permission are appropriate for the client
	for(k = 2 ; k < currFile ; k++) {

		//printf("looping\n");

		if(FileList[k].file == NULL) {

			continue;
		
		}

		if(strcmp(path , FileList[k].file) == 0) {

			//printf("other found\n");
			if(FileList[k].mode == 0) {

				if(theModeN == 0) {

					continue;

				} else if(theModeN == 1){

					if(FileList[k].writer == 1) {

						if(inFlag == 1 || inFlag == 2) {

							write(sock , "-1" , 256);

						} 

					}

				} else {

					write(sock , "-1" , 256);
					return -1;

				}

			} else if(FileList[k].mode == 1) {

				if(theModeN == 1) {

					if(FileList[k].writer == 1 && (inFlag == 1 || inFlag == 2)) {

						//printf("The File must be opened with read permission\n");
						write(sock , "-1" , 256);
						return -1;

					} else {

						continue;

					}

				} else {

					//printf("The File must be opened in exclusive mode\n");
					write(sock , "-1" , 256);
					return -1;
				}

			} else if(FileList[k].mode == 2) {

				//printf("The File cannot be opened will another has it in transaction mode\n");
				write(sock , "-1" , 256);
				return -1;

			}

		}
		

	} 

	//printf("mode checked\n");

	if(inFlag == 0) {  			//this opens the file with the right flag depending on how the client asked
                iFile = fopen(path , "r");
        } else if(inFlag == 1) {
                iFile = fopen(path , "w");
		pthread_mutex_lock(&file_mutex);				
		FileList[currFile].writer = 1;
		pthread_mutex_unlock(&file_mutex);				
        } else if(inFlag == 2) {
                iFile = fopen(path , "r+");
		FileList[currFile].writer = 1;
        } else {
                write(sock , "-1" , 256);
                return -1;
        }

	pthread_mutex_lock(&file_mutex);	//the data is set for the specific file descriptor
	FileList[currFile].fd = iFile;
	FileList[currFile].mode = theModeN; 
	FileList[currFile].file = (char*)malloc((sizeof(char) * strlen(path)) + 1); 
	strcpy(FileList[currFile].file , path); 
	FileList[currFile].setFlag = inFlag;
	pthread_mutex_unlock(&file_mutex);	

	char * retAd = (char *)malloc(10);
	sprintf(retAd , "%d" , currFile * -1);		//file descriptors are all negative numbers
	currFile++;					//they correspnd to the positive index on the struct array

	write(sock, retAd , 256);

	free(path);
	free(retAd);

	return 0;
}

int reader(int sock , char * arguments) { //a method for if a client wants to read from a file on the server

	//printf("reading\n");

	char * filedest = (char *)malloc((sizeof(char) * strlen(arguments))); //the arguments are obtained
	int i;
	int j = 0;
	for(i = 2 ; arguments[i] != ',' ; i++) {

		filedest[j] = arguments[i];
		j++;

	}
	filedest[j] = '\0';

	char * bytes = (char*)malloc((sizeof(char) * strlen(arguments)));
	i++;
	j = 0;
	for(i = i ; arguments[i] != '\0' ; i++) {

		bytes[j] = arguments[i];
		j++;

	}
	bytes[j] = '\0';

	int filedestN = atoi(filedest);

	if((filedestN * -1) < 2 || (filedestN * -1 > 199)) { //we check if they have a valid file descriptor

                write(sock , "-1,ebadf" , 256);
                return -1;

        }

        if(FileList[filedestN * -1].fd == NULL) {

                write(sock , "-1,ebadf" , 256);
                return -1;

        }


	if(FileList[filedestN * -1].setFlag != 0 && FileList[filedestN * -1].setFlag != 2) { //we check if the flag is appropriate

		//printf("ERROR : Client does not have read permission.\n");
		write(sock , "-1" , 256);
		return -1;

	}
	
	int bytesN = atoi(bytes);
	char * buffer = (char *)malloc(sizeof(char) * (bytesN + 11));
	size_t n = fread(buffer  , 1 , bytesN , FileList[filedestN * -1].fd);

	write(sock , buffer , 256); //the file is read from and what was read is sent back to the client as is the number of bytes read

	char * nRead = (char *)malloc(11);
	sprintf(nRead , "%d" , n);
	write(sock , nRead , 256);

	free(filedest);
	free(bytes);
	free(buffer);
	free(nRead);

}

int writer(int sock , char * arguments) { //a method for if a client wants to write to a file on the server

	//printf("writing\n");

	char * filedest = (char *)malloc((sizeof(char) * strlen(arguments)));
        int i;
        int j = 0;
        for(i = 2 ; arguments[i] != ',' ; i++) {

                filedest[j] = arguments[i];
                j++;

        }
        filedest[j] = '\0';

	char * input = (char*)malloc((sizeof(char) * strlen(arguments)));
        i++;
        j = 0;
        for(i = i ; arguments[i] != ',' ; i++) {

                input[j] = arguments[i];
                j++;

        }
        input[j] = '\0';

	char * bytes = (char*)malloc((sizeof(char) * strlen(arguments)));
        i++;
        j = 0;
        for(i = i ; arguments[i] != '\0' ; i++) {

                bytes[j] = arguments[i];
                j++;

        }
        bytes[j] = '\0';

	int filedestN = atoi(filedest);

	if((filedestN * -1) < 2 || (filedestN * -1 > 199)) {

                write(sock , "-1,ebadf" , 256);
                return -1;

        }

        if(FileList[filedestN * -1].fd == NULL) {

                write(sock , "-1,ebadf" , 256);
                return -1;

        }


	if(FileList[filedestN * -1].setFlag != 1 && FileList[filedestN * -1].setFlag != 2) {

		//printf("ERROR : Client does not have write permission.\n");
		write(sock , "-1" , 256);
                return -1;

        }

	int bytesN = atoi(bytes);
        size_t n = fwrite(input  , 1 , bytesN , FileList[filedestN * -1].fd);

	pthread_mutex_lock(&file_mutex);	
	fflush(FileList[filedestN * -1].fd);
	pthread_mutex_unlock(&file_mutex);

	char * nWrite = (char *)malloc(11);
        sprintf(nWrite , "%d" , n);
        write(sock , nWrite , 256); //the number of bytes written is sent back to the client

	free(filedest);
	free(input);
	free(bytes);

	return 0;
}

int closer(int sock , char * arguments) { //a method for if a client wants to close a file

	//printf("closing\n");

	char * filedest = (char *)malloc((sizeof(char) * strlen(arguments)));
        int i;
        int j = 0;
        for(i = 2 ; arguments[i] != '\0' ; i++) {

                filedest[j] = arguments[i];
                j++;

        }
        filedest[j] = '\0';

	int filedestN = atoi(filedest);

	if((filedestN * -1) < 2 || (filedestN * -1 > 199)) {

		write(sock , "-1,ebadf" , 256);
		return -1;

	}

	if(FileList[filedestN * -1].fd == NULL) {

		write(sock , "-1,ebadf" , 256);
		return -1;

	}

	fclose(FileList[filedestN * -1].fd); //the file is closed

	pthread_mutex_lock(&file_mutex);
	FileList[filedestN * -1].fd = NULL;
	FileList[filedestN * -1].setFlag = -1;
	FileList[filedestN * -1].file = NULL; 
	FileList[filedestN * -1].mode = -1; 
	FileList[filedestN * -1].writer = 0;
	pthread_mutex_unlock(&file_mutex);	

	write(sock , "0" , 20); //whether or not the close was successful is ent back to the client

	free(filedest);
	return 1;
	
}


int main(int argc, char *argv[]) {

     int sockfd, newsockfd, portno, pid;	//the server listens for any clients trying to communicate
     socklen_t clilen;
     struct sockaddr_in serv_addr, cli_addr;

     sockfd = socket(AF_INET, SOCK_STREAM, 0);

     if (sockfd < 0) {

     	printf("ERROR opening socket\n");

     }

     bzero((char *) &serv_addr, sizeof(serv_addr));
     portno = 7659;
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;
     serv_addr.sin_port = htons(portno);

     if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {

              printf("ERROR on binding\n");

     }

     listen(sockfd,5);
     clilen = sizeof(cli_addr);
	 pthread_t thread_id;

 		while (1) {		//the server always stays on once executed to get any clients at any time

     	 clilen = sizeof(cli_addr);
         newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
		 		 
		 if (newsockfd < 0) {
             printf("ERROR : Client not accepted.\n");
	   	 }


		 if(pthread_create(&thread_id, NULL, dostuff, (void *)newsockfd) < 0) { //do stuff is run if a client communicates with the server
			 printf("could not create thread\n");

		 }

		 sleep(2);
         close(newsockfd);
     } 

     close(sockfd);

     pthread_exit(NULL);


}

void *dostuff(void *arg) { //this method checks the input from the client to tell what to do to the files

	int newsockfd = (int)arg;
	//printf("sock id: %d\n", newsockfd);


		int n;
		char buffer[256];

		bzero(buffer,256);
		n = read(newsockfd,buffer,255);


		if (n < 0) {

		printf("ERROR : Socket could not be read.\n");

		}

		printf("Here is the message: %s\n",buffer);

	 if(buffer[0] == '1') {			//runs read, write, open, or close depending on what the client sent

		opener(newsockfd , buffer);

	 } else if(buffer[0] == '2') {

		reader(newsockfd , buffer);

	 } else if(buffer[0] == '3') {

		writer(newsockfd , buffer);

	 } else if(buffer[0] == '4') {

		closer(newsockfd , buffer);

	 } else {

		printf("No argument given\n");

	 }
	
	close(newsockfd);
	pthread_exit(NULL);
}






