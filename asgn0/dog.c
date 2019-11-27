#include <fcntl.h>  
#include <stdio.h>   
#include <unistd.h> 
#include <err.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#define BUFSIZE 32

int main( int argc, char *argv[] ){

    int n;
    char buffer[BUFSIZE];

    if(argc == 1){
        while((n=read(0,buffer,BUFSIZE)) > 0){
            if(write(1,buffer,n) != n){
                perror("dog");
            }
        }
    }
    else{
        
        int fileCounter = 1;
        int file;
        int readNumber; 
        char *str = "dog: ";
        struct stat st;
        while(argc > 1){

            if(strcmp("-",argv[fileCounter]) == 0){
                while((n=read(0,buffer,BUFSIZE)) > 0){
                    if(write(1,buffer,n) != n){
                        perror("dog");
                    }
                }
            }
            else{
                file = open(argv[fileCounter], O_RDONLY);
                if (stat( argv[fileCounter], &st ) != 0){
                    continue;
                }

                if(file == -1){
                    char dest[12];
                    strcpy( dest, str );
                    strcat( dest, argv[fileCounter]);
                    perror(dest);
                }
                else if(S_ISDIR( st.st_mode )){
                    warnx("%s: Is a directory",
                            argv[fileCounter]);
                }
                else{
                    while ((readNumber = read(file, buffer, BUFSIZE)) > 0)
                        if(write(1, &buffer, readNumber) != readNumber){
                            perror("dog");
                        }
                }
                close(file);
            }
            argc--;
            fileCounter++;
        }
    }
}

