#include "kernel/types.h"
#include "user/user.h"
const char PING[] = "toast";
const char PONG[] = "boast";
int main(int argc, char *argv[]) {
    int p1[2];
    int p2[2];
    pipe(p1); // p contains fds
    pipe(p2);
    int pid = fork();
    if(pid == 0){
        for(;;) {
            char* y[5];
            write(p1[1], &PING, 5);
            read(p2[0], &y, 5);
            printf("child: recieved byte %s\n", y);
            sleep(5);
        }
    }
    for(;;){
        char* x[5];
        read(p1[0], &x, 5);
        printf("parent: recieved byte %s\n", x);
        sleep(5);
        write(p2[1], &PONG, 5);
    }
    exit(0);
};


