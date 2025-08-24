#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

void handle_input(int n){
    if(n == SIGHUP){
        printf("Ouch!\n");
    }
    else if(n == SIGINT){
        printf("Yeah!\n");
    }
}
//commant line arguments passed into main
int main(int argc, char *argv[]){

//need to convert string to int
int convertN = atoi(argv[1]);

//override default behaviour
signal(SIGHUP, handle_input);
signal(SIGINT, handle_input);

for (int i = 0; i < convertN; i++){
    if(i % 2 == 0){
        printf("%d\n", i);
        fflush(stdout);
        sleep(5);
    }
}
}