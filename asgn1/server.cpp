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

#define BUFSIZE 4096

using namespace std;

void setServerPort(int argc, char const *argv[], int *portNum){
	for (int i = 0; i < argc; ++i)
	{
		if (i == 2)
		{
			*portNum = atoi(argv[2]);
		}
	}
}

void parser(char header[BUFSIZE], vector<string> *headerVector ){
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


bool asciCheck(string test){
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

void getFunctionality(vector<string> headerVector, int new_socket){
	char header[50];
	char fileBuffer[BUFSIZE];
	struct stat file_stat;
  
    string fileName = headerVector.at(1);

    int file;
    int readFile;

    file = open(fileName.c_str(), O_RDONLY);

    if(file == -1){
    	sprintf(header,"HTTP/1.1 404 Not Found\r\n\r\n");
    	send(new_socket,header,strlen(header),0);
    }
    else if(access(fileName.c_str(), R_OK | W_OK) != 0){
    	sprintf(header,"HTTP/1.1 403 Forbidden\r\n\r\n");
	    send(new_socket,header,strlen(header),0);
    }
    else if(!asciCheck(headerVector.at(1))){
	    sprintf(header,"HTTP/1.1 403 Forbidden\r\n\r\n");
	    send(new_socket,header,strlen(header),0);
    }
    else{
    	if (fstat(file, &file_stat) < 0)
        {
            sprintf(header,"HTTP/1.1 400 Bad Request\r\n\r\n");
	    	send(new_socket,header,strlen(header),0);
        }
        else{

	    	sprintf(header,"HTTP/1.1 200 OK\r\nContent-Length: %ld\r\n\r\n", file_stat.st_size);
	    	send(new_socket,header,strlen(header),0);

	    	while ((readFile = read(file, fileBuffer, BUFSIZE)) > 0){
	    		if(send(new_socket, fileBuffer, readFile,0) != readFile){
	    			perror("Erorr in get");
	    		}
	    	}
    	}

    }
    close(file);

}

void putFunctionality(vector<string> headerVector, int new_socket){

	char buffer[BUFSIZE] = {0};
	char header[50];
	string length = "4096";
	int file;
	int ret;
	int totalBytesRead = 0;

	bool contentLength = false;

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
		sprintf(header,"HTTP/1.1 500 Internal Server Error\r\n100-continue\r\n\r\n");
		send(new_socket,header,strlen(header),0);
	}
	else if(!asciCheck(headerVector.at(1))){
		sprintf(header,"HTTP/1.1 403 Forbidden\r\n100-continue\r\n\r\n");
		send(new_socket,header,strlen(header),0);
	}
	else if(size == 0){
		sprintf(header,"HTTP/1.1 201 Created\r\n100-continue\r\n\r\n");
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
		sprintf(header,"HTTP/1.1 201 Created\r\n100-continue\r\n\r\n");
		send(new_socket,header,strlen(header),0);
		while((ret = recv( new_socket , buffer, BUFSIZE,0)) > 0){
			totalBytesRead += ret;
			if(write(file,buffer,ret) != ret){
				perror("Error in PUT");
				break;
			}
			else if(totalBytesRead == size){
				break;
			}
		}
	}

	close(file);
	
}


int main(int argc, char const *argv[]) 
{ 
	int server_fd, new_socket, valread; 
	struct sockaddr_in address; 
	int opt = 1; 
	int addrlen = sizeof(address); 
	char buffer[BUFSIZE] = {0}; 
	int port = 80;
	struct hostent *hostnm;
	vector<string> headerVector;

	// Creating socket file descriptor 
	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) 
	{ 
		perror("socket failed"); 
		exit(EXIT_FAILURE); 
	} 
	
	//assigns port number
	setServerPort(argc,argv, &port);

	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, 
												&opt, sizeof(opt))) 
	{ 
		perror("setsockopt"); 
		exit(EXIT_FAILURE); 
	}

	address.sin_family = AF_INET;
	if(argc > 1){
		hostnm = gethostbyname(argv[1]);
		if (hostnm == (struct hostent *) 0)
    	{
        	perror("Gethostbyname failed\n");
        	exit(EXIT_FAILURE);
    	}
    	address.sin_addr.s_addr = *((unsigned long *)hostnm->h_addr);
	}
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

	while(1){
		if ((new_socket = accept(server_fd, (struct sockaddr *)&address, 
						(socklen_t*)&addrlen))<0) 
		{ 
			perror("accept");
			continue;
		}

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
	}

	close(server_fd);
	return 0;
} 
