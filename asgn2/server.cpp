// Server side C/C++ program to demonstrate Socket programming 
#include <unistd.h> 
#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h>
#include <arpa/inet.h> 
#include <string.h> 
#include <bits/stdc++.h> 
#include <vector>
#include <netdb.h>
#include <ctype.h>
#include <fcntl.h> 
#include <err.h>
#include <sys/stat.h>
#include <sys/queue.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/param.h>

#define BUFSIZE 4096
#define UNUSED(x) (void)(x)

using namespace std;

char *logfile = {0};
bool logfileExists = false;
int logFd = -1;
pthread_mutex_t logMutex = PTHREAD_MUTEX_INITIALIZER;

sem_t sem_IdleThread;
pthread_mutex_t wQueueLock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t wQueueCond = PTHREAD_COND_INITIALIZER;
vector<int> socketQueue;

//taken from GeeksforGeeks
void parseCommandLine(int argc, char *argv[], int *portNum, char **hostname, int *threadCount)
{
	int opt;
	bool first = true;
	bool second = false;
    while((opt = getopt(argc, argv, "N:l:")) != -1)  
    {  
        switch(opt)  
        {  
            case 'N':
            	*threadCount = atoi(optarg);  
                break;
            case 'l':
            	logfileExists = true;
            	logfile = optarg;
              	break;
            case '?':
            	exit(EXIT_FAILURE);
                break;  
        }  
    }  
      
    // optind is for the extra arguments 
    // which are not parsed 
    for(; optind < argc; optind++){
    	if(first){
    		*hostname = argv[optind];
    		first = false;
    		second = true;
    	}
    	else if(second){
    		*portNum = atoi(argv[optind]);
    		second = false;
    	}
    }    
}

void parser(char header[BUFSIZE], vector<string> *headerVector )
{
	headerVector->clear();
	int counter = 0;
	char * pch;
	pch = strtok(header," \r\n");
	while(pch != NULL){
		headerVector->push_back(pch);
    	pch = strtok (NULL, " \r\n");
    	counter++;
	}
}


bool asciCheck(string test)
{
	if (test.length() != 27)
	{
		return false;
	}
	else{
		for (unsigned int i = 1; i < test.length(); ++i)
		{
			if(  isalpha(test[i]) || isdigit(test[i]) || test[i] == '-' || test[i] == '_' ){
				//Do nothing
			}
			else{
				return false;
			}
		}
		return true;
	}

}

int logHttpOpen(const char* fileName)
{
	logFd = open(fileName, O_WRONLY |   O_TRUNC | O_CREAT, 0644);
	if(logFd < 0){
		perror("Couldn't create file");
		return -errno;
	}

	return 0;
}

void logHttpClose()
{
	close(logFd);
}

void logHttpReq(const char *request, const char *fileName, int fileLength)
{
	char buffer[BUFSIZE] = {0};
	int ret = sprintf(buffer,"%s %s length %d \n", request, fileName, fileLength);
	//pthread_mutex_lock(&logMutex);
	write(logFd,buffer,ret);
	//pthread_mutex_unlock(&logMutex);
}

void logHttpBuffer(char *logBuf, int totalBufferCount)
{
	int count = 0;
	char buffer[BUFSIZE] ={0};
	char lastLineBuffer[BUFSIZE] = {0};
	int ret;
	//pthread_mutex_lock(&logMutex);
	while(count < totalBufferCount){
		ret = sprintf(buffer, "%08d", count);
		write(logFd,buffer,ret);

		char *cp = &logBuf[count];
		char *wp = &buffer[0];

		int i;
		int writeCount = MIN(totalBufferCount-count,20);
		for(i = 0; i< writeCount;i++){
			ret = sprintf(wp," %02x", *cp );
			wp += ret;
			cp++;
			count++;
		}
		sprintf(wp,"\n");
		write(logFd, buffer, strlen(buffer));
	}

	sprintf(lastLineBuffer, "========\n");
	write(logFd, lastLineBuffer, 9);
	//pthread_mutex_unlock(&logMutex);
}

void logFailRequest(const char *request, const char *fileName, const char *response)
{
	char buffer[BUFSIZE] = {0};
	char lastLineBuffer[BUFSIZE] = {0};
	int ret = sprintf(buffer,"FAIL: %s %s %s \n", request, fileName, response);
	pthread_mutex_lock(&logMutex);
	write(logFd,buffer,ret);
	sprintf(lastLineBuffer, "========\n");
	write(logFd, lastLineBuffer, 9);
	pthread_mutex_unlock(&logMutex);
}

void getFunctionality(vector<string> headerVector, int new_socket)
{
	char header[50];
	char fileBuffer[BUFSIZE];
	struct stat file_stat;
	const char *command = "GET";
  
    string fileName = headerVector.at(1);

    int file;
    int readFile;

    file = open(fileName.c_str(), O_RDONLY);

    if(file == -1){
    	const char *response = "HTTP/1.1 --- response 404";
    	if(logfileExists){
    		logFailRequest(command,fileName.c_str(),response);
    	}
    	sprintf(header,"HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n");
    	send(new_socket,header,strlen(header),0);
    }
    else if(access(fileName.c_str(), R_OK | W_OK) != 0){
    	const char *response = "HTTP/1.1 --- response 403";
    	if(logfileExists){
    		logFailRequest(command,fileName.c_str(),response);
    	}
    	sprintf(header,"HTTP/1.1 403 Forbidden\r\nContent-Length: 0\r\n\r\n");
	    send(new_socket,header,strlen(header),0);
    }
    else if(!asciCheck(headerVector.at(1))){
    	const char *response = "HTTP/1.1 --- response 403";
    	if(logfileExists){
    		logFailRequest(command,fileName.c_str(),response);
    	}
	    sprintf(header,"HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\n\r\n");
	    send(new_socket,header,strlen(header),0);
    }
    else{
    	if (fstat(file, &file_stat) < 0)
        {
        	const char *response = "HTTP/1.1 --- response 400";
    		if(logfileExists){
    			logFailRequest(command,fileName.c_str(),response);
    		}
            sprintf(header,"HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n");
	    	send(new_socket,header,strlen(header),0);
        }
        else{

        	if(logfileExists){
        		pthread_mutex_lock(&logMutex);
	    		logHttpReq(command, fileName.c_str(),file_stat.st_size);
	    	}

	    	sprintf(header,"HTTP/1.1 200 OK\r\nContent-Length: %ld\r\n\r\n", file_stat.st_size);
	    	send(new_socket,header,strlen(header),0);

	    	while ((readFile = read(file, fileBuffer, BUFSIZE)) > 0){
	    		if(logfileExists){
	    			logHttpBuffer(fileBuffer,readFile);
	    		}
	    		if(send(new_socket, fileBuffer, readFile,0) != readFile){
	    			perror("Erorr in get");
	    		}
	    	}

	    	if(file_stat.st_size == 0){
	    		logHttpBuffer(fileBuffer,0);
	    		pthread_mutex_unlock(&logMutex);
	    	}

	    	if(logfileExists){
	    		pthread_mutex_unlock(&logMutex);
	    	}

    	}

    }
    close(file);
}

void putFunctionality(vector<string> headerVector, int new_socket)
{


	char buffer[BUFSIZE] = {0};
	char header[50];
	string length = "4096";
	int file;
	int ret;
	int totalBytesRead = 0;
	bool contentLength = false;
	const char *command = "PUT";

	string fileName = headerVector.at(1);

    for (unsigned n=0; n<headerVector.size(); ++n) {
	        if(headerVector.at(n).compare("Content-Length:") == 0)
	        {
	        	contentLength = true;
	        	length = headerVector.at(n+1);
	        }
	}



	int size = stoi(length);
	file = open(fileName.c_str(), O_WRONLY |  O_TRUNC | O_CREAT, 0644);

	if(file < 0){
		const char *response = "HTTP/1.1 --- response 403";
    	if(logfileExists){
    		logFailRequest(command,fileName.c_str(),response);
    	}
    	sprintf(header,"HTTP/1.1 403 Forbidden\r\nContent-Length: 0\r\n\r\n");
	    send(new_socket,header,strlen(header),0);
	}
	else if(access(fileName.c_str(), R_OK | W_OK) != 0){
    	const char *response = "HTTP/1.1 --- response 403";
    	if(logfileExists){
    		logFailRequest(command,fileName.c_str(),response);
    	}
    	sprintf(header,"HTTP/1.1 403 Forbidden\r\nContent-Length: 0\r\n\r\n");
	    send(new_socket,header,strlen(header),0);
    }
	else if(!asciCheck(headerVector.at(1))){
		const char *response = "HTTP/1.1 --- response 500";
    	if(logfileExists){
    		logFailRequest(command,fileName.c_str(),response);
    	}

		sprintf(header,"HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\n\r\n");
		send(new_socket,header,strlen(header),0);
	}
	else if(size == 0){
		sprintf(header,"HTTP/1.1 201 Created\r\nContent-Length: 0\r\n\r\n");
		send(new_socket,header,strlen(header),0);
	}
	else if(contentLength == false){
		sprintf(header,"HTTP/1.1 201 Created\r\n100-continue\r\n\r\n");
		send(new_socket,header,strlen(header),0);

		while((ret = recv( new_socket , buffer, BUFSIZE,0)) > 0){
			if(write(file,buffer,ret) != ret){
				perror("Error in PUT");
				break;
			}
		}
	}
	else{

		if(logfileExists){
			pthread_mutex_lock(&logMutex);
			logHttpReq(command, fileName.c_str(),size);
		}

		sprintf(header,"HTTP/1.1 201 Created\r\n100-continue\r\n\r\n");
		send(new_socket,header,strlen(header),0);


		while((ret = recv( new_socket , buffer, BUFSIZE,0)) > 0){
			totalBytesRead += ret;
			if(logfileExists){
	    		logHttpBuffer(buffer,ret);
	    	}
			if(write(file,buffer,ret) != ret){
				perror("Error in PUT");
				break;
			}
			else if(totalBytesRead == size){
				break;
			}
			
		}
		if(logfileExists){
			pthread_mutex_unlock(&logMutex);
		}

	}

	close(file);	
}

void* worker(void *arg)
{
	UNUSED(arg);
	vector<string> headerVector;
	char buffer[BUFSIZE] = {0}; 
	int valread;

	while(1){
		
		//wait for a new connection to be added the vector 
		pthread_mutex_lock(&wQueueLock);
		while(socketQueue.empty() == true){
			pthread_cond_wait(&wQueueCond, &wQueueLock); //waiting for worker condition and release lock
		}

		int new_socket = socketQueue.front();
		socketQueue.erase(socketQueue.begin());
		pthread_mutex_unlock(&wQueueLock);

		//Now serve this http client
		valread = read( new_socket , buffer, BUFSIZE);
		parser(buffer,&headerVector);

			for (unsigned n=0; n<headerVector.size(); ++n) {
		        if( n == 0 && headerVector.at(n).compare("PUT") == 0)
		        {
		        	putFunctionality(headerVector, new_socket);
		        }
		        else if(n == 0 && headerVector.at(n).compare("GET") == 0)
		        {
		        	getFunctionality(headerVector, new_socket);
		        }
		        else if(n ==0 && (headerVector.at(n).compare("GET") != 0 || headerVector.at(n).compare("PUT") == 0)){
		        	char header[50];
		        	sprintf(header,"HTTP/1.1 400 Bad Request\r\n\r\n");
		    		send(new_socket,header,strlen(header),0);
		        }

		    }

		    close(new_socket);
		    //declare yourself to be idle, increment semaphore
		    sem_post(&sem_IdleThread);
	}
	    return NULL;
}


int main(int argc, char *argv[]) 
{ 
	int server_fd, new_socket; 
	struct sockaddr_in address; 
	int opt = 1; 
	int addrlen = sizeof(address); 
	int port = 80;
	char *hostname = {0};
	int threadCount = 4;
	struct hostent *hostnm;
	
	if(argc < 2){
		perror("Not enough arguments");
		exit(EXIT_FAILURE);
	}
	parseCommandLine(argc,argv, &port, &hostname, &threadCount);
	/*printf("log file: %s\n", logfile);
	printf("threadCount: %d\n", threadCount);
	printf("port: %d\n", port);
	printf("hostname: %s\n", hostname);*/

	// Creating socket file descriptor 
	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) 
	{ 
		perror("socket failed"); 
		exit(EXIT_FAILURE); 
	} 
	

	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, 
												&opt, sizeof(opt))) 
	{ 
		perror("setsockopt"); 
		exit(EXIT_FAILURE); 
	}

	address.sin_family = AF_INET;
	hostnm = gethostbyname(hostname);
	if (hostnm == (struct hostent *) 0)
    {
        perror("Gethostbyname failed");
        exit(EXIT_FAILURE);
    }
    address.sin_addr.s_addr = *((unsigned long *)hostnm->h_addr);
	address.sin_port = htons( port ); 
	
	// Forcefully attaching socket to the port 
	if (bind(server_fd, (struct sockaddr *)&address, 
								sizeof(address))<0) 
	{ 
		perror("bind failed"); 
		exit(EXIT_FAILURE); 
	} 
	if (listen(server_fd, 3) < 0) 
	{ 
		perror("listen"); 
		exit(EXIT_FAILURE); 
	}

	sem_init(&sem_IdleThread,0,threadCount); //init to all threads idle
	
	if(logfileExists){
		logHttpOpen(logfile);//init log file
	}

	//create 4 threads with entry called worker	
	for(int i=0; i<threadCount; i++){
		pthread_t threadID;
		pthread_create(&threadID,NULL,&worker,NULL);
	}


	while(1){

		if ((new_socket = accept(server_fd, (struct sockaddr *)&address, 
						(socklen_t*)&addrlen))<0) 
		{ 
			perror("accept");
			continue;
		}

		sem_wait(&sem_IdleThread);//wait for free thread/hangs until thread released/sleeps 
		pthread_mutex_lock(&wQueueLock);
		socketQueue.push_back(new_socket);
		pthread_cond_signal(&wQueueCond);
		pthread_mutex_unlock(&wQueueLock);

	}
	if(logfileExists){
		logHttpClose();
	}
	close(server_fd);
	return 0;
} 
