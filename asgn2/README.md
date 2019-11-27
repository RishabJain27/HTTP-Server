# HttpServer



## Compiling Server

To compile the server just run make.

```bash
make
```
This will create the executable httpserver.

## Running the server

To run the server you MUST provide a hostname/ip address for the server to run on. Optionally, you can also provide a port number as a second argument, but if you don't the server will run on port 80. You can also set flags -N and -l which both require a value or the server won't run. Program defaults to 4 threads if no -N.
```bash
./httpserver 127.0.0.1 8080
./httpserver localhost
./httpserver localhost -N 5 -l logfile 8080
```

## Limitations

I have only tested the program using curl so testing the server with a different client might lead to issues I couldn't foresee. The server can only handle one request at a time, so if a client sends a request but inside has a GET, PUT and GET it would not be able to respond to all the requests. It would only respond to the first request. The server also only can respond with 5 different http headers. The client is also only able to handle PUT and GET requests and the file names must be 27 letters long and only contain letters,numbers, dashes, and underscores. If the file is not formatted correctly the server will respond with a 403 error. The server is assuming the first element in the header is either a GET or PUT and is followed by the filename. The server will not work if the request header is not formatted like this.

## Assumptions Client Curl Commands

Assuming the client runs this command for PUT:
```bash
curl -T localfile http://localhost:8080 --request-target filename_27_character
```
and this command for GET:
```bash
curl http://localhost:8080 --request-target filename_27_character
```
## General Assumptions

For the assignment I am assuming that the filename that is passed in the header from the client doesn't contain the '/' character. Also assuming that the response header are sent to the client and that no messages are printed to the server. Also if a file exists but it isn’t a correct 27 character ascii name that the program will respond with a Internal Server error. Assume for PUT case where there is no content length that the client must close the connection and the server will purposely keep reading from client until the connection is closed.

## Grading

I got my assignment grade for asgn1 a day before this assignment was due so the professor told me that any points that I lost on asgn1 won't count against me in this assignment. For example if a test case didn;t pass for asgn1 and the same issue exists for this assignment I won't be penalized twice. 

 
